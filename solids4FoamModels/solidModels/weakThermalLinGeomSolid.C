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

#include "weakThermalLinGeomSolid.H"
#include "volFields.H"
#include "fvm.H"
#include "fvc.H"
#include "fvMatrices.H"
#include "addToRunTimeSelectionTable.H"
#include "solidTractionFvPatchVectorField.H"
#include "fvcGradf.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace solidModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(weakThermalLinGeomSolid, 0);
addToRunTimeSelectionTable(solidModel, weakThermalLinGeomSolid, dictionary);


// * * * * * * * * * * *  Private Member Functions * * * * * * * * * * * * * //

bool weakThermalLinGeomSolid::converged
(
    const int iCorr,
#ifdef OPENFOAMESIORFOUNDATION
    const SolverPerformance<scalar>& solverPerfT,
#else
    const lduSolverPerformance& solverPerfT,
#endif
    const volScalarField& T
)
{
    // We will check a number of different residuals for convergence
    bool converged = false;

    // Calculate relative residuals
    const scalar absResidualT =
        gMax
        (
#ifdef OPENFOAMESIORFOUNDATION
            DimensionedField<double, volMesh>
#endif
            (
                mag(T.internalField() - T.prevIter().internalField())
            )
        );
    const scalar residualT =
        absResidualT
       /max
        (
            gMax
            (
#ifdef OPENFOAMESIORFOUNDATION
                DimensionedField<double, volMesh>
#endif
                (
                    mag(T.internalField() - T.oldTime().internalField())
                )
            ),
            SMALL
        );

    // If one of the residuals has converged to an order of magnitude
    // less than the tolerance then consider the solution converged
    // force at leaast 1 outer iteration and the material law must be converged
    if (iCorr > 1)
    {
        if (absResidualT < absTTol_)
        {
            Info<< "    T has converged to within the " << absTTol_
                << " degrees" << endl;
            converged = true;
        }
        else if
        (
            solverPerfT.initialResidual() < solutionTol()
         && residualT < solutionTol()
        )
        {
            Info<< "    Both residuals have converged" << endl;
            converged = true;
        }
        else if
        (
            residualT < alternativeTol()
        )
        {
            Info<< "    The relative residual has converged" << endl;
            converged = true;
        }
        else if
        (
            solverPerfT.initialResidual() < alternativeTol()
        )
        {
            Info<< "    The solver residual has converged" << endl;
            converged = true;
        }
        else
        {
            converged = false;
        }
    }

    // Print residual information
    if (iCorr == 0)
    {
        Info<< "    Corr, res, relRes, iters" << endl;
    }
    else if (iCorr % infoFrequency() == 0 || converged)
    {
        Info<< "    " << iCorr
            << ", " << solverPerfT.initialResidual()
            << ", " << residualT
            << ", " << solverPerfT.nIterations() << endl;

        if (converged)
        {
            Info<< endl;
        }
    }
    else if (iCorr == nCorr() - 1)
    {
        maxIterReached()++;
        Warning
            << "Max iterations reached within temperature loop" << endl;
    }

    return converged;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

weakThermalLinGeomSolid::weakThermalLinGeomSolid
(
    Time& runTime,
    const word& region
)
:
    linGeomTotalDispSolid(runTime, region),
    thermal_(mesh()),
    rhoC_
    (
        IOobject
        (
            "rhoC",
            runTime.timeName(),
            mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        thermal_.C()*mechanical().rho()
    ),
    k_(thermal_.k()),
   T_
    (
        IOobject
        (
            "T",
            runTime.timeName(),
            mesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh()
    ),
    gradT_
    (
        IOobject
        (
            "grad(T)",
            runTime.timeName(),
            mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh(),
        dimensionedVector("0", dimTemperature/dimLength, vector::zero_)
    ),
    absTTol_
    (
        solidModelDict().lookupOrDefault<scalar>
        (
            "absoluteTemperatureTolerance",
            1e-06
        )
    )
{
    T_.oldTime();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


bool weakThermalLinGeomSolid::evolve()
{
    Info << "Evolving thermal solid solver" << endl;

    int iCorr = 0;
#ifdef OPENFOAMESIORFOUNDATION
    SolverPerformance<scalar> solverPerfT;
    SolverPerformance<vector>::debug = 0;
#else
    lduSolverPerformance solverPerfT;
    blockLduMatrix::debug = 0;
#endif

    Info<< "Solving the energy equation for T" << endl;

    // Energy equation non-orthogonality corrector loop
    do
    {
        // Store fields for under-relaxation and residual calculation
        T_.storePrevIter();

        // Heat equation
        fvScalarMatrix TEqn
        (
            rhoC_*fvm::ddt(T_)
         == fvm::laplacian(k_, T_, "laplacian(k,T)")
        );

        // Under-relaxation the linear system
        TEqn.relax();

        // Solve the linear system
        solverPerfT = TEqn.solve();

        // Under-relax the field
        T_.relax();

        // Update gradient of temperature
        gradT_ = fvc::grad(T_);
    }
    while (!converged(iCorr, solverPerfT, T_) && ++iCorr < nCorr());

    // Now solve the momentum equation
    // Note: a thermal elastic mechanical law should be chosen if the effect of
    // thermal stress is to be included in the momentum equation.
    linGeomTotalDispSolid::evolve();

#ifdef OPENFOAMESIORFOUNDATION
    SolverPerformance<scalar>::debug = 1;
#else
    blockLduMatrix::debug = 1;
#endif

    return true;
}


void weakThermalLinGeomSolid::writeFields(const Time& runTime)
{
    Info<< "Max T = " << max(T_).value() << nl
        << "Min T = " << min(T_).value() << endl;

    // Heat flux
    volVectorField heatFlux
    (
        IOobject
        (
            "heatFlux",
            runTime.timeName(),
            mesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
       -k_*gradT_
    );

    Info<< "Max magnitude of heat flux = " << max(mag(heatFlux)).value()
        << endl;

    linGeomTotalDispSolid::writeFields(runTime);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace solidModels

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
