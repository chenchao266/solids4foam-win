/*---------------------------------------------------------------------------*\
License
    This file is part of solids4foam.

    solids4foam is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    solids4foam is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with solids4foam.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "coupledUnsLinGeomLinearElasticSolid.H"
#include "volFields.H"
#include "fvm.H"
#include "fvc.H"
#include "fvMatrices.H"
#include "addToRunTimeSelectionTable.H"
#include "linearElastic.H"
#include "processorPolyPatch.H"
#ifdef FOAMEXTEND
    #include "blockSolidTractionFvPatchVectorField.H"
    #include "fvcGradf.H"
    #include "BlockFvmDivSigma.H"
    #include "blockFixedDisplacementZeroShearFvPatchVectorField.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace solidModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(coupledUnsLinGeomLinearElasticSolid, 0);
addToRunTimeSelectionTable
(
    solidModel, coupledUnsLinGeomLinearElasticSolid, dictionary
);


// * * * * * * * * * * * *  Private Member Functions * * * * * * * * * * * * //

void coupledUnsLinGeomLinearElasticSolid::writeBanner(messageStream& os) const
{
    os  << nl
        << "/*---------------------------------------------------------------------------*\\\n"
        << "|    For further information on this class, please see the following          |\n"
        << "|    publication:                                                             |\n"
        << "|                                                                             |\n"
        << "|    P. Cardiff, Z. Tukovic, H, Jasak, A. Ivankovic: A block-coupled finite   |\n"
        << "|    volume methodology for linear elasticity and unstructured meshes.        |\n"
        << "|    Computers and Structures, 2016, 175 100-122,                             |\n"
        << "|    DOI: 10.1016/-j.compstruc.2016.07.004.                                   |\n"
        << "\\*---------------------------------------------------------------------------*/"
        << nl << endl;
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

coupledUnsLinGeomLinearElasticSolid::coupledUnsLinGeomLinearElasticSolid
(
    Time& runTime,
    const word& region
)
:
    solidModel(typeName, runTime, region),
#ifdef FOAMEXTEND
    extendedMesh_(mesh()),
    solutionVec_
    (
        IOobject
        (
            "solutionVec",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        vectorField(extendedMesh_.nVariables(), vector::zero_)
    ),
#endif
    muf_
    (
        IOobject
        (
            "interpolate(mu)",
            runTime.timeName(),
            mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh(),
        dimensionedScalar("0", dimPressure, 0.0)
    ),
    lambdaf_
    (
        IOobject
        (
            "interpolate(lambda)",
            runTime.timeName(),
            mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh(),
        dimensionedScalar("0", dimPressure, 0.0)
    )
{
    writeBanner(Info);

    DisRequired();

    // We will directly read the linearElastic mechanicalLaw
    const PtrList<mechanicalLaw>& mechLaws = mechanical();
    if (mechLaws.size() != 1)
    {
        FatalErrorIn
        (
            "coupledUnsLinGeomLinearElasticSolid::"
            "coupledUnsLinGeomLinearElasticSolid"
        )   << type() << " can currently only be used with a single material"
            << "\nConsider using one of the other solidModels."
            << abort(FatalError);
    }
    else if (!isA<linearElastic>(mechLaws[0]))
    {
        FatalErrorIn
        (
            "coupledUnsLinGeomLinearElasticSolid::"
            "coupledUnsLinGeomLinearElasticSolid"
        )   << type() << " can only be used with the linearElastic "
            << "mechanicalLaw" << nl
            << "Consider using one of the other linearGeometry solidModels."
            << abort(FatalError);
    }

    // Cast the mechanical law to a linearElastic mechanicalLaw
    const linearElastic& mech = refCast<const linearElastic>(mechLaws[0]);

    // Set mu and lambda fields
    muf_ = mech.mu();
    lambdaf_ = mech.lambda();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


bool coupledUnsLinGeomLinearElasticSolid::evolve()
{
#ifdef FOAMEXTEND
    Info << "Evolving solid solver" << endl;

    BlockSolverPerformance<vector> solverPerfD("undefined", "blockD");
    BlockLduMatrix<vector>::debug = 5;

    Info<< "Solving the momentum equation for D" << endl;

    // Note: there is no momentum loop as everything is linear and we have
    // hard-coded the linear definition of stress

    // Global coefficients are currently stored in the extendedMesh so we
    // must clear them out each time before constructing a new equation
    // Using a global patch may be a nicer solution
    //extendedMesh_.clearOut();
    extendedMesh_.clearOutGlobalCoeffs();

    // Store fields for under-relaxation and residual calculation
    D().storePrevIter();

    // Create source vector for block matrix
    vectorField blockB(solutionVec_.size(), vector::zero_);

    // Create block system
    BlockLduMatrix<vector> blockM(extendedMesh_);

    // Grab block diagonal and set it to zero
    Field<tensor>& d = blockM.diag().asSquare();
    d = tensor::zero_;

    // Grab linear off-diagonal and set it to zero
    Field<tensor>& l = blockM.lower().asSquare();
    Field<tensor>& u = blockM.upper().asSquare();
    u = tensor::zero_;
    l = tensor::zero_;

    // Insert coefficients

    // For now we create separate matrices for each term of the three
    // diffusion terms in the momentum equation and add the contributions
    // together manually; a BlockFvMatrix class would help make this look
    // nicer

    // Laplacian
    // non-orthogonal correction is treated implicitly
    BlockLduMatrix<vector> blockMatLap =
        BlockFvm::laplacian(extendedMesh_, muf_, D(), blockB);

    // Laplacian transpose == div(mu*gradU.T())
    BlockLduMatrix<vector> blockMatLapTran =
        BlockFvm::laplacianTranspose(extendedMesh_, muf_, D(), blockB);

    // Laplacian trace == div(lambda*I*tr(gradU))
    BlockLduMatrix<vector> blockMatLapTrac =
        BlockFvm::laplacianTrace(extendedMesh_, lambdaf_, D(), blockB);

    // Add diagonal contributions
    d += blockMatLap.diag().asSquare();
    d += blockMatLapTran.diag().asSquare();
    d += blockMatLapTrac.diag().asSquare();

    // Add off-diagonal contributions
    u += blockMatLap.upper().asSquare();
    u += blockMatLapTran.upper().asSquare();
    u += blockMatLapTrac.upper().asSquare();
    l += blockMatLap.lower().asSquare();
    l += blockMatLapTran.lower().asSquare();
    l += blockMatLapTrac.lower().asSquare();

    // Add contribution for processor boundaries
    blockM.interfaces() = blockMatLap.interfaces();
    forAll(mesh().boundaryMesh(), patchI)
    {
        const word& patchType = mesh().boundaryMesh()[patchI].type();
        if (patchType == processorPolyPatch::typeName)
        {
            Field<tensor>& coupleUpper =
                blockM.coupleUpper()[patchI].asSquare();

            coupleUpper = blockMatLap.coupleUpper()[patchI].asSquare();
            coupleUpper +=
                blockMatLapTran.coupleUpper()[patchI].asSquare();
            coupleUpper +=
                blockMatLapTrac.coupleUpper()[patchI].asSquare();
        }
    }

    // We manually add the boundary conditions equations
    // More thinking is required to get it to fit cleanly with the rest of
    // the block coupled machinery
    extendedMesh_.insertBoundaryConditions
    (
        blockM, blockB, muf_, lambdaf_, D()
    );

    // Add terms temporal and gravity terms to the block matrix and source
    extendedMesh_.addFvMatrix
    (
        blockM,
        blockB,
        rho()*fvm::d2dt2(D()) - rho()*g(),
        true
    );

    // Under-relax the linear system
    if (mesh().solutionDict().relaxEquation("DEqn"))
    {
        FatalError
            << "Equation under-relaxation disabled!"
            << abort(FatalError);
        //Info<< "Under-relaxing the equation" << endl;
        //blockM.relax
        //(
        //    solutionVec_,
        //    blockB,
        //    mesh().solutionDict().relaxationFactor("DEqn")
        //);
    }

    // Block coupled solver call
    solverPerfD =
        BlockLduSolver<vector>::New
        (
            D().name(),
            blockM,
            mesh().solutionDict().solver("blockD")
        )->solve(solutionVec_, blockB);

    // Transfer solution vector to D field
    extendedMesh_.copySolutionVector(solutionVec_, D());

    // Under-relax the field
    D().relax();

    // Update gradient of displacement
    mechanical().interpolate(D(), pointD());

    // Enforce zero normal displacement on symmetry
    // Todo: we should create a blockSymmetry boundary condition
    // pointField& pointDI = pointD().internalField();
    // forAll(mesh().boundaryMesh(), patchI)
    // {
    //     if
    //     (
    //         D().boundaryField()[patchI].type()
    //      == blockFixedDisplacementZeroShearFvPatchVectorField::typeName
    //     )
    //     {
    //         WarningIn("coupledUnsLinGeomLinearElasticSolid::evolve()")
    //             << "Setting the normal displacement on patch "
    //             << mesh().boundaryMesh()[patchI].name() << " to zero"
    //             << endl;

    //         const labelList& meshPoints =
    //             mesh().boundaryMesh()[patchI].meshPoints();
    //         const vectorField& pointNormals =
    //             mesh().boundaryMesh()[patchI].pointNormals();

    //         // Remove normal component
    //         forAll(meshPoints, pI)
    //         {
    //             const label pointID = meshPoints[pI];
    //             pointDI[pointID] =
    //                 ((I - sqr(pointNormals[pI])) & pointDI[pointID]);
    //         }
    //     }
    // }
    gradD() = fvc::grad(D(), pointD());

    // We will call fvc::grad a second time as fixed displacement boundaries
    // need the patch internal field values to calculate snGrad, which
    // is then used to set snGrad on the gradD boundary
    D().correctBoundaryConditions();
    gradD() = fvc::grad(D(), pointD());

    // Increment of displacement
    DD() = D() - D().oldTime();

    // Update gradient of displacement increment
    gradDD() = gradD() - gradD().oldTime();

    // Calculate the stress using run-time selectable mechanical law
    mechanical().correct(sigma());

    // Increment of point displacement
    pointDD() = pointD() - pointD().oldTime();

    // Velocity
    U() = fvc::ddt(D());

    BlockLduMatrix<vector>::debug = 1;
#else
    notImplemented("Not implemented for this version of OpenFOAM/FOAM");
#endif

    return true;
}

tmp<vectorField> coupledUnsLinGeomLinearElasticSolid::tractionBoundarySnGrad
(
    const vectorField& traction,
    const scalarField& pressure,
    const fvPatch& patch
) const
{
    // Patch index
    const label patchID = patch.index();

    // Patch mechanical property
    const scalarField& mu = muf_.boundaryField()[patchID];
    const scalarField& lambda = lambdaf_.boundaryField()[patchID];

    // Patch gradient
    const tensorField& pGradD = gradD().boundaryField()[patchID];

    // Patch stress
    const symmTensorField& pSigma = sigma().boundaryField()[patchID];

    // Patch unit normals
    const vectorField n(patch.nf());

    // Return patch snGrad
    return tmp<vectorField>
    (
        new vectorField
        (
            (
                (traction - n*pressure)
              - (n & (pSigma - (2.0*mu + lambda)*pGradD))
            )/(2.0*mu + lambda)
        )
    );
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace solidModels

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
