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

#include "AitkenCouplingInterface.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

namespace fluidSolidInterfaces
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(AitkenCouplingInterface, 0);
addToRunTimeSelectionTable
(
    fluidSolidInterface, AitkenCouplingInterface, dictionary
);


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

AitkenCouplingInterface::AitkenCouplingInterface
(
    Time& runTime,
    const word& region
)
:
    fluidSolidInterface(typeName, runTime, region),
    relaxationFactor_
    (
        fsiProperties().lookupOrAddDefault<scalar>("relaxationFactor", 0.01)
    ),
    relaxationFactorMax_
    (
        fsiProperties().lookupOrAddDefault<scalar>
        (
            "relaxationFactorMax", 1.0
        )
    ),
    predictSolid_(fsiProperties().lookupOrAddDefault<Switch>("predictSolid", true)),
    aitkenRelaxationFactors_(nGlobalPatches(), relaxationFactor_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool AitkenCouplingInterface::evolve()
{
    initializeFields();

    updateInterpolatorAndGlobalPatches();

    scalar residualNorm = 0;

    // Check if coupling switch needs to be updated
    if (!coupled())
    {
        updateCoupled();
    }

    if (predictSolid_ && coupled())
    {
        updateForce();

        solid().evolve();

        residualNorm =
            updateResidual();
    }

    do
    {
        outerCorr()++;

        // Transfer the displacement from the solid to the fluid
        updateDisplacement();

        // Move the fluid mesh
        moveFluidMesh();

        // Solve fluid
        fluid().evolve();

        if (coupled())
        {
            // Transfer the force from the fluid to the solid
            updateForce();

            // Solve solid
            solid().evolve();

            // Calculate the FSI residual
            residualNorm = updateResidual();
        }
        else
        {
            residualNorm = 0.0;
        }

        // Optional: write residuals to file
        if (writeResidualsToFile() && Pstream::master())
        {
            residualFile()
                << runTime().value() << " "
                << outerCorr() << " "
                << residualNorm << endl;
        }
    }
    while (residualNorm > outerCorrTolerance() && outerCorr() < nOuterCorr());

    solid().updateTotalFields();

    // Optional: correct fluid mesh to avoid build-up of interface position
    // errors
    if (additionalMeshCorrection())
    {
        // Transfer the displacement from the solid to the fluid, where we will
        // use no relaxation; in that way, we can force the solid and fluid
        // interfaces to stay aligned
        forAll(fluid().globalPatches(), interfaceI)
        {
            fluidZonesPointsDisplsPrev()[interfaceI] =
                fluidZonesPointsDispls()[interfaceI];

            fluidZonesPointsDispls()[interfaceI] += residuals()[interfaceI];
        }

        // Move the fluid mesh
        moveFluidMesh();
    }

    return 0;
}


void AitkenCouplingInterface::updateDisplacement()
{
    Info<< nl << "Time = " << fluid().runTime().timeName()
        << ", iteration: " << outerCorr() << endl;

    if (outerCorr() < 3)
    {
        Info<< "Current fsi under-relaxation factor (fixed): "
            << relaxationFactor_ << endl;

        forAll(fluid().globalPatches(), interfaceI)
        {
            fluidZonesPointsDisplsPrev()[interfaceI] =
                fluidZonesPointsDispls()[interfaceI];

            fluidZonesPointsDispls()[interfaceI] +=
                relaxationFactor_*residuals()[interfaceI];
        }
    }
    else
    {
        forAll(fluid().globalPatches(), interfaceI)
        {
            aitkenRelaxationFactors_[interfaceI] =
                -aitkenRelaxationFactors_[interfaceI]
               *(
                    sum
                    (
                        residualsPrev()[interfaceI]
                      & (residuals()[interfaceI] - residualsPrev()[interfaceI])
                    )
                   /(
                        sum
                        (
                            (
                                residuals()[interfaceI]
                              - residualsPrev()[interfaceI]
                            )
                          & (
                                residuals()[interfaceI]
                              - residualsPrev()[interfaceI]
                            )
                        )
                    )
                );

            if (Pstream::parRun())
            {
                if (!Pstream::master())
                {
                    aitkenRelaxationFactors_[interfaceI] = 0.0;
                }

                // Pass to all procs
                reduce(aitkenRelaxationFactors_[interfaceI], sumOp<scalar>());
            }

            aitkenRelaxationFactors_[interfaceI] =
                mag(aitkenRelaxationFactors_[interfaceI]);

            if (aitkenRelaxationFactors_[interfaceI] > relaxationFactorMax_)
            {
                // PC: in this case, would 1.0 be a better option?
                // Of course, the current option is more more stable
                // aitkenRelaxationFactors_[interfaceI] = relaxationFactor_;
                // aitkenRelaxationFactors_[interfaceI] = 1.0;
                // MT: switched from 1.0 or relaxationFactor_ to a
                // relaxationFactorMax, as discussed at
                // https://bitbucket.org/philip_cardiff/solids4foam-release/issues/30/aitken-coupling-under-relaxation-factor
                aitkenRelaxationFactors_[interfaceI] = relaxationFactorMax_;
            }

            Info<< "Current fsi under-relaxation factor (Aitken) of "
                << fluidMesh().boundary()
                   [
                       fluid().globalPatches()[interfaceI].patch().index()
                   ].name()
                << ": " << aitkenRelaxationFactors_[interfaceI] << endl;

            fluidZonesPointsDisplsPrev()[interfaceI] =
                fluidZonesPointsDispls()[interfaceI];

            fluidZonesPointsDispls()[interfaceI] +=
                aitkenRelaxationFactors_[interfaceI]*residuals()[interfaceI];
        }
    }

    // Update movingWallPressure boundary conditions, if found
    fluidSolidInterface::updateMovingWallPressureAcceleration();

    // Update elasticWallPressure boundary conditions, if found
    fluidSolidInterface::updateElasticWallPressureAcceleration();

    // Make sure that displacement on all processors is equal to one
    // calculated on master processor
    fluidSolidInterface::syncFluidZonePointsDispl(fluidZonesPointsDispls());
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fluidSolidInterfaces

} // End namespace Foam

// ************************************************************************* //
