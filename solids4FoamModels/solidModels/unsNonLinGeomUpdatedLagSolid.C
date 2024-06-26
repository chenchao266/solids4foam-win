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

#include "unsNonLinGeomUpdatedLagSolid.H"
#include "fvm.H"
#include "fvc.H"
#include "fvMatrices.H"
#include "addToRunTimeSelectionTable.H"
#include "bound.H"
#include "symmetryPolyPatch.H"
#include "twoDPointCorrector.H"
#include "solidTractionFvPatchVectorField.H"
#include "fvcGradf.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace solidModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(unsNonLinGeomUpdatedLagSolid, 0);
addToRunTimeSelectionTable
(
    solidModel, unsNonLinGeomUpdatedLagSolid, dictionary
);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

unsNonLinGeomUpdatedLagSolid::unsNonLinGeomUpdatedLagSolid
(
    Time& runTime,
    const word& region
)
:
    solidModel(typeName, runTime, region),
    sigmaf_
    (
        IOobject
        (
            "sigmaf",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh(),
        dimensionedSymmTensor("zero", dimForce/dimArea, symmTensor::zero_)
    ),
    gradDDf_
    (
        IOobject
        (
            "grad(" + DD().name() + ")f",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        mesh(),
        dimensionedTensor("0", dimless, tensor::zero_)
    ),
    F_
    (
        IOobject
        (
            "F",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh(),
        dimensionedTensor("I", dimless, I)
    ),
    Ff_
    (
        IOobject
        (
            "Ff",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh(),
        dimensionedTensor("I", dimless, I)
    ),
    J_
    (
        IOobject
        (
            "J",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        det(F_)
    ),
    Jf_
    (
        IOobject
        (
            "Jf",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        det(Ff_)
    ),
    relF_
    (
        IOobject
        (
            "relF",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        I + gradDD().T()
    ),
    relFf_
    (
        IOobject
        (
            "relFf",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        I + gradDDf_.T()
    ),
    relFinv_
    (
        IOobject
        (
            "relFinv",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        inv(relF_)
    ),
    relFinvf_
    (
        IOobject
        (
            "relFinvf",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        inv(relFf_)
    ),
    relJ_
    (
        IOobject
        (
            "relJ",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        det(relF_)
    ),
    relJf_
    (
        IOobject
        (
            "relJf",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        det(relFf_)
    ),
    rho_
    (
        IOobject
        (
            "rho",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        mechanical().rho()
    ),
    impK_(mechanical().impK()),
    impKf_(mechanical().impKf()),
    rImpK_(1.0/impK_)
{
    DDisRequired();

    // For consistent restarts, we will update the relative kinematic fields
    DD().correctBoundaryConditions();
    if (restart())
    {
        mechanical().interpolate(DD(), pointDD(), false);
        mechanical().grad(DD(), pointDD(), gradDD());
        mechanical().grad(DD(), pointDD(), gradDDf_);
        relFf_ = I + gradDDf_.T();
        relFinvf_ = inv(relFf_);
        Ff_ = relFf_ & Ff_.oldTime();
        relJf_ = det(relFf_);
        Jf_ = relJf_*Jf_.oldTime();

        Ff_.storeOldTime();
        Jf_.storeOldTime();

        // Let the mechanical law know
        mechanical().setRestart();
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


bool unsNonLinGeomUpdatedLagSolid::evolve()
{
    Info<< "Evolving solid solver" << endl;

    int iCorr = 0;
#ifdef OPENFOAMESIORFOUNDATION
    SolverPerformance<vector> solverPerfDD;
    SolverPerformance<vector>::debug = 0;
#else
    lduSolverPerformance solverPerfDD;
    blockLduMatrix::debug = 0;
#endif

    Info<< "Solving the momentum equation for DD" << endl;

    // Momentum equation loop
    do
    {
        // Store fields for under-relaxation and residual calculation
        DD().storePrevIter();

        // Momentum equation incremental updated Lagrangian form
        fvVectorMatrix DDEqn
        (
            fvm::d2dt2(rho_, DD())
          + fvc::d2dt2(rho_, D().oldTime())
         == fvm::laplacian(impKf_, DD(), "laplacian(DDD,DD)")
          - fvc::laplacian(impKf_, DD(), "laplacian(DDD,DD)")
          + fvc::div((relJf_*relFinvf_.T() & mesh().Sf()) & sigmaf_)
          + rho()*g()
        );

        // Under-relax the linear system
        DDEqn.relax();

        // Enforce any cell displacements
        solidModel::setCellDisps(DDEqn);

        // Solve the linear system
        solverPerfDD = DDEqn.solve();

        // Under-relax the DD field
        relaxField(DD(), iCorr);

        // Interpolate DD to pointDD
        mechanical().interpolate(DD(), pointDD(), false);

        // Update gradient of displacement increment
        mechanical().grad(DD(), pointDD(), gradDD());
        mechanical().grad(DD(), pointDD(), gradDDf_);

        // Relative deformation gradient
        relFf_ = I + gradDDf_.T();

        // Inverse relative deformation gradient
        relFinvf_ = inv(relFf_);

        // Total deformation gradient
        Ff_ = relFf_ & Ff_.oldTime();

        // Relative Jacobian
        relJf_ = det(relFf_);

        // Jacobian of deformation gradient
        Jf_ = relJf_*Jf_.oldTime();

        // Calculate the stress using run-time selectable mechanical law
        mechanical().correct(sigmaf_);
    }
    while
    (
       !converged
        (
            iCorr,
#ifdef OPENFOAMESIORFOUNDATION
            mag(solverPerfDD.initialResidual()),
            cmptMax(solverPerfDD.nIterations()),
#else
            solverPerfDD.initialResidual(),
            solverPerfDD.nIterations(),
#endif
            DD()
        ) && ++iCorr < nCorr()
    );

    // Relative deformation gradient
    relF_ = I + gradDD().T();

    // Inverse relative deformation gradient
    relFinv_ = inv(relF_);

    // Total deformation gradient
    F_ = relF_ & F_.oldTime();

    // Relative Jacobian
    relJ_ = det(relF_);

    // Jacobian of deformation gradient
    J_ = relJ_*J_.oldTime();

    // Calculate the stress using run-time selectable mechanical law
    mechanical().correct(sigma());

    // Update gradient of total displacement
    gradD() = fvc::grad(D().oldTime() + DD());

    // Total displacement
    D() = D().oldTime() + DD();

    // Total displacement at points
    pointD() = pointD().oldTime() + pointDD();

    // Velocity
    U() = fvc::ddt(D());

#ifdef OPENFOAMESIORFOUNDATION
    SolverPerformance<vector>::debug = 1;
#else
    blockLduMatrix::debug = 1;
#endif

    return true;
}


tmp<vectorField> unsNonLinGeomUpdatedLagSolid::tractionBoundarySnGrad
(
    const vectorField& traction,
    const scalarField& pressure,
    const fvPatch& patch
) const
{
    // Patch index
    const label patchID = patch.index();

    // Patch implicit stiffness field
    const scalarField& impK = impKf_.boundaryField()[patchID];

    // Patch reciprocal implicit stiffness field
    const scalarField& rImpK = rImpK_.boundaryField()[patchID];

    // Patch gradient
    const tensorField& gradDD = gradDDf_.boundaryField()[patchID];

    // Patch stress
    const symmTensorField& sigma = sigmaf_.boundaryField()[patchID];

    // Patch relative deformation gradient inverse
    const tensorField& relFinv = relFinvf_.boundaryField()[patchID];

    // Patch relative Jacobian
    const scalarField& relJ = relJf_.boundaryField()[patchID];

    // Patch unit normals (updated configuration)
    const vectorField n(patch.nf());

    // Patch unit normals (deformed configuration)
    const vectorField nCurrent(relJ*relFinv.T() & n);

    // Return patch snGrad
    return tmp<vectorField>
    (
        new vectorField
        (
            (
                (traction - n*pressure)
              - (nCurrent & sigma)
              + (n & (impK*gradDD))
            )*rImpK
        )
    );
}


void unsNonLinGeomUpdatedLagSolid::updateTotalFields()
{
    // Density
    rho_ = rho_.oldTime()/relJ_;

    // Move the mesh to the deformed configuration
#ifdef OPENFOAMESIORFOUNDATION
    const vectorField oldPoints = mesh().points();
#else
    const vectorField oldPoints = mesh().allPoints();
#endif
    moveMesh(oldPoints, DD(), pointDD());

    solidModel::updateTotalFields();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace solidModels

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
