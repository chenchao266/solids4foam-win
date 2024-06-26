/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "fixedMixedModeCohesiveZoneModel.H"
#include "addToRunTimeSelectionTable.H"
#include "solidCohesiveFvPatchVectorField.H"
#include "cohesiveZoneInitiation.H"
#include "crackerFvMesh.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(fixedMixedModeCohesiveZoneModel, 0)
    addToRunTimeSelectionTable
    (
        cohesiveZoneModel, fixedMixedModeCohesiveZoneModel, dictionary
    );
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


Foam::vector Foam::fixedMixedModeCohesiveZoneModel::damageTraction
(
    const scalar faceDeltaEff,
    const scalar faceSigmaMax,
    const scalar faceDeltaC,
    const vector& faceInitTrac
) const
{
    if (faceDeltaEff > faceDeltaC)
    {
        return vector::zero_;
    }

    scalar scaleFac = 1.0;

    if (!dugdale_)
    {
        // Linear shape
        scaleFac = 1.0 - (faceDeltaEff/faceDeltaC);
    }

    return scaleFac*faceInitTrac;
}


Foam::vector Foam::fixedMixedModeCohesiveZoneModel::unloadingDamageTraction
(
    const scalar faceDeltaEff,
    const scalar faceSigmaMax,
    const scalar faceDeltaC,
    const vector& faceInitTrac,
    const scalar faceMagUnloadingDeltaEff
) const
{
    // First calculate traction at unloadingDeltaEff

    scalar scaleFac = 1.0;

    if (!dugdale_)
    {
        // Linear shape
        scaleFac = 1.0 - (faceMagUnloadingDeltaEff/faceDeltaC);
    }

    vector faceTrac = scaleFac*faceInitTrac;

    // Now scale traction back to origin

    return faceTrac*faceDeltaEff/faceMagUnloadingDeltaEff;
}


void Foam::fixedMixedModeCohesiveZoneModel::calcPenaltyFactor() const
{
    if (patch().size())
    {
        // Calculate penalty factor similar to standardPenalty contact model
        // approx penaltyFactor from mechanical properties
        // this can then be scaled using the penaltyScale

        const label patchID = patch().index();
        const fvMesh& mesh = this->mesh();

        // Lookup the implicit stiffness as a measure of the mechanical
        // stiffness
        const scalar impK =
            gAverage
            (
                mesh.lookupObject<volScalarField>
                (
                    "impK"
                ).boundaryField()[patchID]
            );

        // Average contact patch face area
        scalar faceArea = gAverage(patch().magSf());

        // average contact patch cell volume
        scalar cellVolume = 0.0;

        const volScalarField::DimensionedInternalField& V = mesh.V();
        const unallocLabelList& faceCells =
            mesh.boundary()[patchID].faceCells();

        forAll(mesh.boundary()[patchID], facei)
        {
            cellVolume += V[faceCells[facei]];
        }
        cellVolume /= patch().size();

        // Approximate penalty factor based on:
        // Hallquist, Goudreau, Benson - 1985 - Sliding interfaces with
        // contact-impact in large-scale Lagrangian computations, Computer
        // Methods in Applied Mechanics and Engineering, 51:107-137.
        // We approximate penalty factor for traction instead of force, so we
        // divide their proposed penalty factor by face-area
        penaltyFactor_ = penaltyScale_*impK*faceArea/cellVolume;
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //


// Construct from dictionary
Foam::fixedMixedModeCohesiveZoneModel::fixedMixedModeCohesiveZoneModel
(
    const word& name,
    const fvPatch& patch,
    const dictionary& dict
)
:
    cohesiveZoneModel(name, patch, dict),
    sigmaMax_(dict.lookup("sigmaMax")),
    GIc_(dict.lookup("GIc")),
    deltaC_(2.0*GIc_/sigmaMax_),
    cracked_(patch.size(), false),
    initTraction_(patch.size(), vector::zero_),
    deltaN_(patch.size(), 0.0),
    deltaS_(patch.size(), vector::zero_),
    unloadingDeltaEff_(patch.size(), vector::zero_),
    minUnloadingDelta_
    (
        dict.lookupOrDefault<scalar>("minUnloadingDelta", 0.001)
    ),
    beta_(dict.lookupOrDefault<scalar>("beta", 1.0)),
    dugdale_(dict.lookupOrDefault<Switch>("dugdale", false)),
    penaltyScale_(dict.lookupOrDefault<scalar>("penaltyScale", 1.0)),
    penaltyFactor_(dict.lookupOrDefault<scalar>("penaltyFactor", 0.0))
{
    // Scale deltaC if Dugdale law is specified
    if (dugdale_)
    {
        deltaC_ *= 0.5;
    }

    // Check limits of minUnloadingDelta
    if (minUnloadingDelta_ < 0.001)
    {
        WarningIn
        (
            "fixedMixedModeCohesiveZoneModel::fixedMixedModeCohesiveZoneModel"
        )   << "Limiting minUnloadingDelta to 0.001" << endl;

        minUnloadingDelta_ = 0.001;
    }
    else if (minUnloadingDelta_ > 1.0)
    {
        WarningIn
        (
            "fixedMixedModeCohesiveZoneModel::fixedMixedModeCohesiveZoneModel"
        )   << "Limiting minUnloadingDelta to 1.0" << endl;

        minUnloadingDelta_ = 1.0;
    }

    // Check beta limits
    if (beta_ < 0.0 || beta_ > 1.0)
    {
        FatalErrorIn
        (
            "fixedMixedModeCohesiveZoneModel::fixedMixedModeCohesiveZoneModel"
        )   << "beta outside limits: 0 <= beta <= 1" << abort(FatalError);
    }
    Info<< "    beta is " << beta_ << endl;

    if (dugdale_)
    {
        Info<< "    Dugdale shape" << endl;
    }
    else
    {
        Info<< "    Linear shape" << endl;
    }


    // Read optional fields
    if (dict.found("cracked"))
    {
        cracked_ = Field<bool>("cracked", dict, patch.size());
    }

    if (dict.found("initDirection"))
    {
        initTraction_ = vectorField("initDirection", dict, patch.size());
    }

    if (dict.found("deltaN"))
    {
        deltaN_ = scalarField("deltaN", dict, patch.size());
    }

    if (dict.found("deltaS"))
    {
        deltaS_ = vectorField("deltaS", dict, patch.size());
    }

    if (dict.found("unloadingDeltaEff"))
    {
        unloadingDeltaEff_ =
            vectorField("unloadingDeltaEff", dict, patch.size());
    }
}


// Construct as a copy
Foam::fixedMixedModeCohesiveZoneModel::fixedMixedModeCohesiveZoneModel
(
    const fixedMixedModeCohesiveZoneModel& czm
)
:
    cohesiveZoneModel(czm),
    sigmaMax_(czm.sigmaMax_),
    GIc_(czm.GIc_),
    deltaC_(czm.deltaC_),
    cracked_(czm.cracked_),
    initTraction_(czm.initTraction_),
    deltaN_(czm.deltaN_),
    deltaS_(czm.deltaS_),
    unloadingDeltaEff_(czm.unloadingDeltaEff_),
    minUnloadingDelta_(czm.minUnloadingDelta_),
    beta_(czm.beta_),
    dugdale_(czm.dugdale_),
    penaltyScale_(czm.penaltyScale_),
    penaltyFactor_(czm.penaltyFactor_)
{}



// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //


Foam::fixedMixedModeCohesiveZoneModel::~fixedMixedModeCohesiveZoneModel()
{}


// * * * * * * * * * * * * * Public Member Functions * * * * * * * * * * * * //

Foam::tmp<Foam::scalarField>
Foam::fixedMixedModeCohesiveZoneModel::crackingAndDamage() const
{
    tmp<scalarField> tcrackingAndDamage
    (
     new scalarField(patch().size(), 1.0)
    );
    scalarField& cad = tcrackingAndDamage();

    forAll(cad, faceI)
    {
        if (cracked_[faceI])
        {
            cad[faceI] = 2.0;
        }
    }

    return tcrackingAndDamage;
}


void Foam::fixedMixedModeCohesiveZoneModel::autoMap(const fvPatchFieldMapper& m)
{
    cracked_.autoMap(m);
    initTraction_.autoMap(m);
    deltaN_.autoMap(m);
    deltaS_.autoMap(m);
    unloadingDeltaEff_.autoMap(m);

    // Only perform mapping if the number of faces on the patch has changed

    const label nNewFaces = m.size() - m.sizeBeforeMapping();

    if (nNewFaces > 0)
    {
        // Lookup the faceBreaker law from the crackerFvMesh
        // Cast mesh to a crackerFvMesh
        const faceBreakerLaw& faceBreaker =
            refCast<const crackerFvMesh>(this->mesh()).faceBreaker();

        if (!isA<cohesiveZoneInitiation>(faceBreaker))
        {
            FatalErrorIn
            (
                "void Foam::fixedMixedModeCohesiveZoneModel::autoMap"
                "(const fvPatchFieldMapper& m)"
            )   << "The fixedMixedModel cohesiveZoneModel must be used with"
                << " the cohesiveZoneInitiation faceBreaker law in the "
                << "dynamicMeshDict" << abort(FatalError);
        }

        const cohesiveZoneInitiation& cohZoneInit =
            refCast<const cohesiveZoneInitiation>(faceBreaker);

        // Get the initiation tractions
        const List<vector>& initTracs = cohZoneInit.facesToBreakTractions();
        const List<vector>& initCoupledTracs =
            cohZoneInit.coupledFacesToBreakTractions();

        // Get normals of broken faces
        const List<vector>& initNormals = cohZoneInit.facesToBreakNormals();
        const List<vector>& initCoupledNormals =
            cohZoneInit.coupledFacesToBreakNormals();

        if (initTracs.size() > 1 || initCoupledTracs.size() > 1)
        {
            FatalErrorIn
            (
                "void Foam::fixedMixedModeCohesiveZoneModel::autoMap"
                "(const fvPatchFieldMapper& m)"
            )   << name() << " expects only 1 faces to be broken at a time"
                << abort(FatalError);
        }

        // Patch unit normals
        const vectorField& n = patch().patch().faceNormals();

        // Reset values on new faces to zero
        // Note: the method below is used to find which faces are new on the
        // patch

        const labelList& addressing = m.directAddressing();
        const label patchSize = patch().size();

        if (patchSize == 1 && nNewFaces == 1)
        {
            label i = 0;

            const vector& faceN = n[i];

            cracked_[i] = false;
            deltaN_[i] = 0.0;
            deltaS_[i] = vector::zero_;
            unloadingDeltaEff_[i] = vector::zero_;

            // If there is only one new face then it must be a coupled face
            if ((faceN & initCoupledNormals[0]) > 0.0)
            {
                initTraction_[i] = initCoupledTracs[0];
            }
            else
            {
                initTraction_[i] = -initCoupledTracs[0];
            }
        }
        else if (patchSize == 2 && nNewFaces == 1)
        {
            label i = 1;

            const vector& faceN = n[i];

            cracked_[i] = false;
            deltaN_[i] = 0.0;
            deltaS_[i] = vector::zero_;
            unloadingDeltaEff_[i] = vector::zero_;

            // If there is only one new face then it must be a coupled face
            if ((faceN & initCoupledNormals[0]) > 0.0)
            {
                initTraction_[i] = initCoupledTracs[0];
            }
            else
            {
                initTraction_[i] = -initCoupledTracs[0];
            }
        }
        else if (patchSize == 2 && nNewFaces == 2)
        {
            label i = 0;

            const vector& faceN0 = n[i];

            cracked_[i] = false;
            deltaN_[i] = 0.0;
            deltaS_[i] = vector::zero_;
            unloadingDeltaEff_[i] = vector::zero_;

            // If there are two new faces then they must be internal faces
            if ((faceN0 & initNormals[0]) > 0.0)
            {
                initTraction_[i] = initTracs[0];
            }
            else
            {
                initTraction_[i] = -initTracs[0];
            }

            i = 1;

            const vector& faceN1 = n[i];

            cracked_[i] = false;
            deltaN_[i] = 0.0;
            deltaS_[i] = vector::zero_;
            unloadingDeltaEff_[i] = vector::zero_;

            if ((faceN1 & initNormals[0]) > 0.0)
            {
                initTraction_[i] = initTracs[0];
            }
            else
            {
                initTraction_[i] = -initTracs[0];
            }
        }
        else
        {
            for (label i = 1; i < patchSize; i++)
            {
                if (addressing[i] == 0)
                {
                    const vector& faceN = n[i];

                    cracked_[i] = false;
                    deltaN_[i] = 0.0;
                    deltaS_[i] = vector::zero_;
                    unloadingDeltaEff_[i] = vector::zero_;

                    if (initTracs.size())
                    {
                        if ((faceN & initNormals[0]) > 0.0)
                        {
                            initTraction_[i] = initTracs[0];
                        }
                        else
                        {
                            initTraction_[i] = -initTracs[0];
                        }
                    }
                    else if (initCoupledTracs.size())
                    {
                        if ((faceN & initCoupledNormals[0]) > 0.0)
                        {
                            initTraction_[i] = initCoupledTracs[0];
                        }
                        else
                        {
                            initTraction_[i] = -initCoupledTracs[0];
                        }
                    }
                }
            }
        }
    }
}


void Foam::fixedMixedModeCohesiveZoneModel::rmap
(
    const solidCohesiveFvPatchVectorField& sc,
    const labelList& addr
)
{
    const fixedMixedModeCohesiveZoneModel& czm =
        refCast<const fixedMixedModeCohesiveZoneModel>(sc.cohesiveZone());

    cracked_.rmap(czm.cracked_, addr);
    initTraction_.rmap(czm.initTraction_, addr);
    deltaN_.rmap(czm.deltaN_, addr);
    deltaS_.rmap(czm.deltaS_, addr);
    unloadingDeltaEff_.rmap(czm.unloadingDeltaEff_, addr);
}


void Foam::fixedMixedModeCohesiveZoneModel::updateOldFields()
{
    // Update unloading delta

    const vectorField& n = patch().patch().faceNormals();

    const scalarField magDeltaEff = mag(deltaEff_);

    label nCracked = 0;
    label nDamaged = 0;

    const scalar faceDeltaC = deltaC_.value();

    forAll(deltaEff_, faceI)
    {
        vector& faceUnloadDelta = unloadingDeltaEff_[faceI];

        const scalar faceDeltaN = deltaN_[faceI];
        const vector& faceDeltaS = deltaS_[faceI];
        const scalar faceDeltaEff = deltaEff_[faceI];
        const scalar faceMagUnloadDelta = mag(faceUnloadDelta);
        const vector& faceN = n[faceI];

        if (faceDeltaEff > minUnloadingDelta_*faceDeltaC)
        {
            if (faceDeltaEff > faceMagUnloadDelta)
            {
                faceUnloadDelta = faceDeltaN*faceN + beta_*faceDeltaS;
            }
        }

        // Flag cracked faces
        bool& faceCracked = cracked_[faceI];

        const scalar faceMagDeltaEff = magDeltaEff[faceI];

        if (faceMagDeltaEff > faceDeltaC)
        {
            faceCracked = true;
            nCracked++;
        }
        else
        {
            nDamaged++;
        }
    }

    Info<< "Number of cracked faces: " << nCracked << nl
        << "Number of damaged faces: " << nDamaged << nl << endl;
}


void Foam::fixedMixedModeCohesiveZoneModel::updateTraction
(
    vectorField& traction,
    const vectorField& delta
)
{
    // Unit normal vectors
    const vectorField& n = patch().patch().faceNormals();

    // Check if the penaltyFactor needs to be calculated
    if (penaltyFactor_ < SMALL)
    {
        calcPenaltyFactor();
    }

    // Note: deltaN, deltaS, and deltaEff have been updated by
    // updateEnergy, which is called before updateTraction

    const scalar faceSigmaMax = sigmaMax_.value();
    const scalar faceDeltaC = deltaC_.value();

    // Set tractions for each face

    forAll(traction, faceI)
    {
        vector& faceTrac = traction[faceI];
        const bool& faceCracked = cracked_[faceI];

        const vector& faceInitTrac = initTraction_[faceI];
        const scalar faceDeltaN = deltaN_[faceI];
        const scalar faceDeltaEff = deltaEff_[faceI];
        const vector& faceUnloadingDeltaEff = unloadingDeltaEff_[faceI];

        const vector& faceN = n[faceI];

        // The phase can be in one of three phases:
        //    1) cracked
        //    2) loading
        //    3) unloading

        if (faceCracked)
        {
            // Cracked faces may have a contact traction
            if (faceDeltaN < 0.0)
            {
                faceTrac = faceDeltaN*penaltyFactor_*faceN;
            }
            else
            {
                faceTrac = vector::zero_;
            }
        }
        else if (faceDeltaEff > (mag(faceUnloadingDeltaEff) - SMALL))
        {
            // Loading

            if (faceDeltaEff > faceDeltaC)
            {
                // Face has cracked
                // Note: we only set the cracked flag during updateOldFields
                // as a face may jump in and out of being cracked during
                // iterations before it converges
                faceTrac = vector::zero_;
            }
            else
            {
                faceTrac =
                    damageTraction
                    (
                        faceDeltaEff,
                        faceSigmaMax,
                        faceDeltaC,
                        faceInitTrac
                    );
            }
        }
        else
        {
            // Unloading

            if (faceDeltaN < 0.0)
            {
                // Face is in contact
                faceTrac = faceDeltaN*penaltyFactor_*faceN;
            }
            else
            {
                faceTrac =
                    unloadingDamageTraction
                    (
                        faceDeltaEff,
                        faceSigmaMax,
                        faceDeltaC,
                        faceInitTrac,
                        mag(faceUnloadingDeltaEff)
                    );
            }
        }
    }

    if (debug)
    {
        Info<< "Max(mag(traction)) " << max(mag(traction)) << nl
            << "Min(mag(traction)) " << min(mag(traction)) << nl << endl;
    }

    // Note: we do not under-relax the traction field because instead the
    // faceDeltas are under-relaxed in solidCohesive
}

void Foam::fixedMixedModeCohesiveZoneModel::updateEnergy
(
    const vectorField& traction,
    const vectorField& delta
)
{
    // We do not integrate dissipated energies here as the current cohesive zone
    // model is a potential model i.e. the state of a face depends only on its
    // current delta. however, we do flag cracked faces in updateOldFields to
    // aviod faces being "healed".

    // Update delta vectors

    // Unit normal vectors
    const vectorField& n = patch().patch().faceNormals();

    // Update normal delta
    deltaN_ = n & delta;

    // Update shear delta
    deltaS_ = ((I - sqr(n)) & delta);

    // Update delta effective, where only positive deltaN is considered
    deltaEff_ = Foam::sqrt(pow(max(deltaN_, 0.0), 2) + magSqr(deltaS_));
}


Foam::tmp<Foam::surfaceScalarField>
Foam::fixedMixedModeCohesiveZoneModel::initiationTractionFraction() const
{
    // Reference to the mesh
    const fvMesh& mesh = this->mesh();

    // Face unit normals
    const surfaceVectorField n = mesh.Sf()/mesh.magSf();

    // Lookup the traction field from the solver: this should be up-to-date
    const surfaceVectorField& traction = meshTraction();

    // Calculate normal traction
    const surfaceScalarField normTraction =
        max(dimensionedScalar("zero", dimPressure, 0.0), n & traction);

    // Calculate shear traction
    const surfaceVectorField shearTraction = beta_*((I - sqr(n)) & traction);

    // Calculate effective traction
    const surfaceScalarField tractionEff =
        sqrt(sqr(normTraction) + magSqr(shearTraction));

    // Normal traction divided by sigmaMax, where only tensile normal tractions
    // are considered
    // The tractionFraction >= 1.0 for faces to be broken and added to the
    // cohesive zone
    return
        tmp<surfaceScalarField>
        (
            new surfaceScalarField
            (
                IOobject
                (
                    "tractionFraction",
                    mesh.time().timeName(),
                    mesh,
                    IOobject::NO_READ,
                    IOobject::NO_WRITE
                ),
                tractionEff/sigmaMax_
            )
        );
}


void Foam::fixedMixedModeCohesiveZoneModel::write(Ostream& os) const
{
    os.writeKeyword("type") << type()
        << token::END_STATEMENT << nl;

    os.writeKeyword("sigmaMax")
        << sigmaMax_ << token::END_STATEMENT << nl;
    os.writeKeyword("GIc")
        << GIc_ << token::END_STATEMENT << nl;

    cracked_.writeEntry("cracked", os);
    initTraction_.writeEntry("initTraction", os);
    deltaN_.writeEntry("deltaN", os);
    deltaS_.writeEntry("deltaS", os);
    deltaEff_.writeEntry("deltaEff", os);
    unloadingDeltaEff_.writeEntry("unloadingDeltaEff", os);

    os.writeKeyword("minUnloadingDelta")
        << minUnloadingDelta_ << token::END_STATEMENT << nl;
    os.writeKeyword("beta")
        << beta_ << token::END_STATEMENT << nl;
    os.writeKeyword("penaltyScale")
        << penaltyScale_ << token::END_STATEMENT << nl;
    os.writeKeyword("penaltyFactor")
        << penaltyFactor_ << token::END_STATEMENT << nl;
}


// ************************************************************************* //
