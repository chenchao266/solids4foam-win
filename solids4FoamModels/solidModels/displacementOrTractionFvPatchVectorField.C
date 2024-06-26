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

#include "displacementOrTractionFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "transformField.H"
#include "volFields.H"
#include "lookupSolidModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

displacementOrTractionFvPatchVectorField::
displacementOrTractionFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    solidDirectionMixedFvPatchVectorField(p, iF),
    constantDisplacement_(p.size(), vector::zero_),
    constantTraction_(p.size(), vector::zero_),
    displacementSeries_(),
    tractionSeries_(),
    specifyNormalDirection_(false)
{}


displacementOrTractionFvPatchVectorField::
displacementOrTractionFvPatchVectorField
(
    const displacementOrTractionFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    solidDirectionMixedFvPatchVectorField(ptf, p, iF, mapper),
#ifdef OPENFOAMFOUNDATION
    constantDisplacement_(mapper(ptf.constantDisplacement_)),
    constantTraction_(mapper(ptf.constantTraction_)),
#else
    constantDisplacement_(ptf.constantDisplacement_, mapper),
    constantTraction_(ptf.constantTraction_, mapper),
#endif
    displacementSeries_(ptf.displacementSeries_),
    tractionSeries_(ptf.tractionSeries_),
    specifyNormalDirection_(ptf.specifyNormalDirection_)
{}


displacementOrTractionFvPatchVectorField::
displacementOrTractionFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    solidDirectionMixedFvPatchVectorField(p, iF),
    constantDisplacement_(p.size(), vector::zero_),
    constantTraction_(p.size(), vector::zero_),
    displacementSeries_(),
    tractionSeries_(),
    specifyNormalDirection_(p.size(), 0)
{
    // Check if displacement is time-varying
    if (dict.found("displacementSeries") && dict.found("constantDisplacement"))
    {
        FatalErrorIn
        (
            "displacementOrTractionFvPatchVectorField::"
            "displacementOrTractionFvPatchVectorField"
        )   << "constantDisplacement or displacementSeries can be specified, "
            << "not both!" << abort(FatalError);
    }
    else if (dict.found("displacementSeries"))
    {
        Info<< type() << ": " << patch().name()
            << " displacement is time-varying" << endl;
        displacementSeries_ =
            interpolationTable<vector>(dict.subDict("displacementSeries"));
    }
    else
    {
        constantDisplacement_ = vectorField("constantDisplacement", dict, p.size());
    }

    // Check if traction is time-varying
    if (dict.found("tractionSeries") && dict.found("constantTraction"))
    {
        FatalErrorIn
        (
            "displacementOrTractionFvPatchVectorField::"
            "displacementOrTractionFvPatchVectorField"
        )   << "constantTraction or tractionSeries can be specified, "
            << "not both!" << abort(FatalError);
    }
    else if (dict.found("tractionSeries"))
    {
        Info<< type() << ": " << patch().name()
            << " traction is time-varying" << endl;
    }
    else
    {
        constantTraction_ = vectorField("constantTraction", dict, p.size());
    }

    // Check if specifyNormalDirection is specified
    if (dict.found("specifyNormalDirection"))
    {
        specifyNormalDirection_ =
            Field<label>("specifyNormalDirection", dict, p.size());
    }

    // Lookup and set refValue, refGrad, valueFraction and value
    if (dict.found("refValue"))
    {
        refValue() = vectorField("refValue", dict, p.size());
    }
    else if (dict.found("value"))
    {
        refValue() = vectorField("value", dict, p.size());
    }
    else
    {
        FatalErrorIn
        (
            "displacementOrTractionFvPatchVectorField::"
            "displacementOrTractionFvPatchVectorField"
        )   << "value or refValue entry must be specified for patch "
            << patch().name() << abort(FatalError);
    }

    if (dict.found("refGradient"))
    {
        refGrad() = vectorField("refGradient", dict, p.size());
    }
    else
    {
        refGrad() = vector::zero_;
    }

    if (dict.found("valueFraction"))
    {
        valueFraction() = symmTensorField("valueFraction", dict, p.size());
    }
    else
    {
        valueFraction() = symmTensor::zero_;
    }

    if (dict.found("value"))
    {
        Field<vector>::operator=(vectorField("value", dict, p.size()));
    }
    else
    {
        const Field<vector> normalValue
        (
            transform(valueFraction(), refValue())
        );

        const Field<vector> gradValue
        (
            patchInternalField() + refGrad()/patch().deltaCoeffs()
        );

        const Field<vector> transformGradValue
        (
            transform(I - valueFraction(), gradValue)
        );

        Field<vector>::operator=(normalValue + transformGradValue);
    }

    // Unit normals
    const vectorField nf(patch().nf());

    // Set fixed displacement directions based on specifyNormalDirection and
    // valueFraction
    forAll(specifyNormalDirection_, faceI)
    {
        if (specifyNormalDirection_[faceI] == 1)
        {
            // Displacement is specified in normal direction and traction in
            // tangential directions
            valueFraction()[faceI] = sqr(nf[faceI]);
        }
        else if (specifyNormalDirection_[faceI] == -1)
        {
            // Displacement is specified in tangential directions and traction
            // in the normal direction
            valueFraction()[faceI] = (I - sqr(nf[faceI]));
        }
        else if (specifyNormalDirection_[faceI] != 0)
        {
            FatalErrorIn
            (
                "displacementOrTractionFvPatchVectorField::"
                "displacementOrTractionFvPatchVectorField() "
            )   << "specifyNormalDirection can only be -1, 0 or 1"
                << abort(FatalError);
        }
    }
}


displacementOrTractionFvPatchVectorField::
displacementOrTractionFvPatchVectorField
(
    const displacementOrTractionFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    solidDirectionMixedFvPatchVectorField(ptf, iF),
    constantDisplacement_(ptf.constantDisplacement_),
    constantTraction_(ptf.constantTraction_),
    displacementSeries_(ptf.displacementSeries_),
    tractionSeries_(ptf.tractionSeries_),
    specifyNormalDirection_(ptf.specifyNormalDirection_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

// Map from self
void displacementOrTractionFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    solidDirectionMixedFvPatchVectorField::autoMap(m);

#ifdef OPENFOAMFOUNDATION
    m(constantDisplacement_, constantDisplacement_);
    m(constantTraction_, constantTraction_);

    // mapper is not defined for an int so we will cast to
    // a scalar and back again
    //m(specifyNormalDirection_, specifyNormalDirection_);
    scalarField t(specifyNormalDirection_.size(), 0.0);
    forAll(t, i)
    {
        t[i] = specifyNormalDirection_[i];
    }
    m(t, t);
    forAll(t, i)
    {
        specifyNormalDirection_[i] = t[i];
    }
#else
    constantDisplacement_.autoMap(m);
    constantTraction_.autoMap(m);
    specifyNormalDirection_.autoMap(m);
#endif
}


// Reverse-map the given fvPatchField onto this fvPatchField
void displacementOrTractionFvPatchVectorField::rmap
(
    const fvPatchField<vector>& ptf,
    const labelList& addr
)
{
    solidDirectionMixedFvPatchVectorField::rmap(ptf, addr);

    const displacementOrTractionFvPatchVectorField& dmptf =
        refCast<const displacementOrTractionFvPatchVectorField>(ptf);

    constantDisplacement_.rmap(dmptf.constantDisplacement_, addr);
    constantTraction_.rmap(dmptf.constantTraction_, addr);
    specifyNormalDirection_.rmap(dmptf.specifyNormalDirection_, addr);
}


void displacementOrTractionFvPatchVectorField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    // Note: valueFraction is set in the constructor so no need to set it again

    // Get current displacement field to be specified
    vectorField disp = constantDisplacement_;

    if (displacementSeries_.size())
    {
        disp = displacementSeries_(db().time().timeOutputValue());
    }

    // Lookup the solidModel object
    const solidModel& solMod =
        lookupSolidModel(patch().boundaryMesh().mesh());

    if (solMod.incremental())
    {
        // Incremental approach, so we wil set the increment of displacement
        // Lookup the old displacement field and subtract it from the total
        // displacement
        const volVectorField& Dold =
            db().lookupObject<volVectorField>("D").oldTime();

        disp -= Dold.boundaryField()[patch().index()];
    }

    // Set displacement
    refValue() = disp;

    // Get current traction to be specified (defaults to zero)
    vectorField traction = constantTraction_;

    if (tractionSeries_.size())
    {
        traction = tractionSeries_(db().time().timeOutputValue());
    }

    // Iteratively set gradient to enforce the specified traction
    refGrad() =
        solMod.tractionBoundarySnGrad
        (
            traction,
            scalarField(patch().size(), 0.0),
            patch()
        );

    solidDirectionMixedFvPatchVectorField::updateCoeffs();
}


// Write
void displacementOrTractionFvPatchVectorField::write(Ostream& os) const
{
    if (displacementSeries_.size())
    {
        os.writeKeyword("displacementSeries") << nl;
        os << token::BEGIN_BLOCK << nl;
        displacementSeries_.write(os);
        os << token::END_BLOCK << nl;
    }
    else
    {
#ifdef OPENFOAMFOUNDATION
        writeEntry(os, "constantDisplacement", constantDisplacement_);
#else
        constantDisplacement_.writeEntry("constantDisplacement", os);
#endif
    }

    if (tractionSeries_.size())
    {
        os.writeKeyword("tractionSeries") << nl;
        os << token::BEGIN_BLOCK << nl;
        tractionSeries_.write(os);
        os << token::END_BLOCK << nl;
    }
    else
    {
#ifdef OPENFOAMFOUNDATION
        writeEntry(os, "constantTraction", constantTraction_);
#else
        constantTraction_.writeEntry("constantTraction", os);
#endif
    }

    os.writeKeyword("specifyNormalDirection")
        << specifyNormalDirection_ << token::END_STATEMENT << nl;

    solidDirectionMixedFvPatchVectorField::write(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField
(
    fvPatchVectorField,
    displacementOrTractionFvPatchVectorField
);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
