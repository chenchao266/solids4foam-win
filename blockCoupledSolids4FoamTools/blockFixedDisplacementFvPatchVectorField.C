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

#include "blockFixedDisplacementFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "fvcMeshPhi.H"
#include "fixedValuePointPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

blockFixedDisplacementFvPatchVectorField::
blockFixedDisplacementFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchVectorField(p, iF),
    blockFvPatchVectorField(),
    totalDisp_(vector::zero_),
    dispSeries_()
{}


blockFixedDisplacementFvPatchVectorField::
blockFixedDisplacementFvPatchVectorField
(
    const blockFixedDisplacementFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchVectorField(ptf, p, iF, mapper),
    blockFvPatchVectorField(),
    totalDisp_(ptf.totalDisp_),
    dispSeries_(ptf.dispSeries_)
{}


blockFixedDisplacementFvPatchVectorField::
blockFixedDisplacementFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchVectorField(p, iF, dict),
    blockFvPatchVectorField(),
    totalDisp_(vector::zero_),
    dispSeries_()
{
    // Check if displacement is time-varying
    if (dict.found("displacementSeries"))
    {
        Info<< "    displacement is time-varying" << endl;
        dispSeries_ =
            interpolationTable<vector>(dict.subDict("displacementSeries"));

        fvPatchField<vector>::operator==
        (
            dispSeries_(this->db().time().timeOutputValue())
        );
    }
    else
    {
        if (dict.found("value"))
        {
            vectorField val = vectorField("value", dict, p.size());

            totalDisp_ = gAverage(val);
        }
    }
}

blockFixedDisplacementFvPatchVectorField::
blockFixedDisplacementFvPatchVectorField
(
    const blockFixedDisplacementFvPatchVectorField& ptf
)
:
    fixedValueFvPatchVectorField(ptf),
    blockFvPatchVectorField(),
    totalDisp_(ptf.totalDisp_),
    dispSeries_(ptf.dispSeries_)
{}


blockFixedDisplacementFvPatchVectorField::
blockFixedDisplacementFvPatchVectorField
(
    const blockFixedDisplacementFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchVectorField(ptf, iF),
    blockFvPatchVectorField(),
    totalDisp_(ptf.totalDisp_),
    dispSeries_(ptf.dispSeries_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void blockFixedDisplacementFvPatchVectorField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    vectorField disp(patch().size(), totalDisp_);
    vectorField pointDisp(patch().patch().nPoints(), totalDisp_);

    if (dispSeries_.size())
    {
        disp = dispSeries_(this->db().time().timeOutputValue());
        pointDisp = dispSeries_(this->db().time().timeOutputValue());
    }

    if (dimensionedInternalField().name() == "DD")
    {
        // Incremental approach, so we wil set the increment of displacement
        // Lookup the old displacement field and subtract it from the total
        // displacement
        const volVectorField& Dold =
            db().lookupObject<volVectorField>("D").oldTime();

        disp -= Dold.boundaryField()[patch().index()];
    }

    fvPatchField<vector>::operator==(disp);


    // Update point displacement field

    const fvMesh& mesh = patch().boundaryMesh().mesh();

    if
    (
        mesh.foundObject<pointVectorField>
        (
            "point" + dimensionedInternalField().name()
        )
    )
    {
        if
        (
            mesh.lookupObject<pointVectorField>
            (
                "point" + dimensionedInternalField().name()
            ).boundaryField()[patch().index()].type() == "fixedValue"
        )
        {
            if (dimensionedInternalField().name() == "DD")
            {
                // Old time field
                const pointVectorField& pointD =
                    mesh.objectRegistry::lookupObject<pointVectorField>
                    (
                        "pointD"
                    );

                const pointVectorField& pointDOld = pointD.oldTime();

                const labelList& meshPoints = patch().patch().meshPoints();

                forAll(meshPoints, pI)
                {
                    const label pointID = meshPoints[pI];

                    pointDisp[pI] -= pointDOld[pointID];
                }
            }

            fixedValuePointPatchVectorField& patchPointDD =
                refCast<fixedValuePointPatchVectorField>
                (
                    const_cast<pointVectorField&>
                    (
                        mesh.objectRegistry::lookupObject<pointVectorField>
                        (
                            "point" + dimensionedInternalField().name())
                        ).boundaryField()[patch().index()]
                );

            patchPointDD == pointDisp;
        }
    }

    fixedValueFvPatchVectorField::updateCoeffs();
}


Foam::tmp<Foam::Field<vector> >
blockFixedDisplacementFvPatchVectorField::snGrad() const
{
    // snGrad without non-orthogonal correction
    // return (*this - patchInternalField())*this->patch().deltaCoeffs();

    // Lookup previous boundary gradient
    const fvPatchField<tensor>& gradField =
        patch().lookupPatchField<volTensorField, tensor>
        (
            "grad(" + dimensionedInternalField().name() + ")"
        );

    // Calculate correction vector
    vectorField n = this->patch().nf();
    vectorField delta = this->patch().delta();
    vectorField k = ((I - sqr(n)) & delta);

    return
    (
        //*this - patchInternalField()
        //*this - (patchInternalField() + (k & gradField.patchInternalField()))
        //*this - (patchInternalField() + (k & gradField))
        (*this - (k & gradField)) - patchInternalField()
    )*this->patch().deltaCoeffs();
}


tmp<Field<vector> > blockFixedDisplacementFvPatchVectorField::
gradientBoundaryCoeffs() const
{
    FatalErrorIn("gradientBoundaryCoeffs()")
        << "This function should not be called!" << nl
        << "This boundary condition is only for use with the block coupled"
        << " solid solver"
        << abort(FatalError);

    // Keep the compiler happy
    return *this;
}


void blockFixedDisplacementFvPatchVectorField::insertBlockCoeffs
(
    const solidPolyMesh& solidMesh,
    const surfaceScalarField& muf,
    const surfaceScalarField& lambdaf,
    const GeometricField<vector, fvPatchField, volMesh>& U,
    Field<vector>& blockB,
    BlockLduMatrix<vector>& blockM
) const
{
    // Const reference to polyPatch and the fvMesh
    const polyPatch& ppatch = patch().patch();

    // Update the displacement
    // We shouldn't have to use const_cast ...
    const_cast<blockFixedDisplacementFvPatchVectorField&>(*this).updateCoeffs();

    // Const reference to the patch field
    const vectorField& pU = *this;

    // Grab block diagonal
    Field<tensor>& d = blockM.diag().asSquare();

    // Index offset for addressing the diagonal of the boundary faces
    const label start = ppatch.start();

    // We will calculate the average of the current diagonal to scale the
    // coefficients for the fixedValue boundary conditions
    // If we don't do this then the convergence can be worse and also the
    // results can be strange
    // Note that this scale factor is just approximate and its exact value
    // does not matter
    const tensor averageDiag = gAverage(d);
    scalar diagSign =
        (averageDiag.xx() + averageDiag.yy() + averageDiag.zz());
    diagSign /= mag(diagSign);
    const scalar scaleFac = diagSign*(1.0/sqrt(3.0))*mag(averageDiag);

    if (mag(scaleFac) < SMALL)
    {
        FatalErrorIn
        (
            "void blockFixedDisplacementFvPatchVectorField::insertBlockCoeffs\n"
            "(\n"
            "    const solidPolyMesh& solidMesh,\n"
            "    const surfaceScalarField& muf,\n"
            "    const surfaceScalarField& lambdaf,\n"
            "    const GeometricField<vector, fvPatchField, volMesh>& U,\n"
            "    Field<vector>& blockB,\n"
            "    BlockLduMatrix<vector>& blockM\n"
            ") const"
        )   << "The average diagonal coefficient is zero! The internal faces "
            << "should be discretised before inserting the boundary condition "
            << "equations" << abort(FatalError);
    }

    // We currently assume that the tangential gradient along fixedValue
    // boundaries is zero i.e. the values are uniform... do we?
    // This BC just forces the value so we don't make any simplifying
    // assumptions.
    forAll(ppatch, faceI)
    {
        // Find the face index in the linear system
        const label varI = solidMesh.findOldVariableID(start + faceI);

        // For displacement, the value is fixed so we will set the
        // diag and source so as to force this fixed value

        // Diagonal contribution for the boundary face
        d[varI] += tensor(scaleFac*I);

        // Source contribution
        blockB[varI] += scaleFac*pU[faceI];
    }
}

void blockFixedDisplacementFvPatchVectorField::write(Ostream& os) const
{
    if (dispSeries_.size())
    {
        os.writeKeyword("displacementSeries") << nl;
        os << token::BEGIN_BLOCK << nl;
        dispSeries_.write(os);
        os << token::END_BLOCK << nl;
    }

    fixedValueFvPatchVectorField::write(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField
(
    fvPatchVectorField,
    blockFixedDisplacementFvPatchVectorField
);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
