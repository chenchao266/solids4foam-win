/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     | Version:     4.0
    \\  /    A nd           | Web:         http://www.foam-extend.org
     \\/     M anipulation  | For copyright notice see file Copyright
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

Class
    Foam::newMovingWallVelocityFvPatchVectorField

Description
    Foam::newMovingWallVelocityFvPatchVectorField

SourceFiles
    newMovingWallVelocityFvPatchVectorField.C

\*---------------------------------------------------------------------------*/

#ifndef newMovingWallVelocityFvPatchVectorField_H
#define newMovingWallVelocityFvPatchVectorField_H

#include "fvPatchFields.H"
#include "fixedValueFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                 Class newMovingWallVelocityFvPatch Declaration
\*---------------------------------------------------------------------------*/

class newMovingWallVelocityFvPatchVectorField
:
    public fixedValueFvPatchVectorField
{

    // To keep track when to copy patch face centres
    label myTimeIndex_;

    // Old face centres of patch. Needed for (second order) time integration
    vectorField Fc_;
    vectorField oldFc_;
    vectorField oldoldFc_;

    // Acceleration
    vectorField acceleration_;

public:

    //- Runtime type information
    TypeName("newMovingWallVelocity");


    // Constructors

        //- Construct from patch and internal field
        newMovingWallVelocityFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        newMovingWallVelocityFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given newMovingWallVelocityFvPatchVectorField
        //  onto a new patch
        newMovingWallVelocityFvPatchVectorField
        (
            const newMovingWallVelocityFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        newMovingWallVelocityFvPatchVectorField
        (
            const newMovingWallVelocityFvPatchVectorField&
#ifdef OPENFOAMFOUNDATION
        ) = delete;
#else
        );

        //- Construct and return a clone
        virtual tmp<fvPatchVectorField> clone() const
        {
            return tmp<fvPatchVectorField>
            (
                new newMovingWallVelocityFvPatchVectorField(*this)
            );
        }
#endif

        //- Construct as copy setting internal field reference
        newMovingWallVelocityFvPatchVectorField
        (
            const newMovingWallVelocityFvPatchVectorField&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchVectorField> clone
        (
            const DimensionedField<vector, volMesh>& iF
        ) const
        {
            return tmp<fvPatchVectorField>
            (
                new newMovingWallVelocityFvPatchVectorField(*this, iF)
            );
        }



    // Member functions

        // Access functions

            //- Get patch (moving wall) acceleration
            const vectorField& acceleration() const
            {
                return acceleration_;
            }

    // Evaluation functions

        //- Update the coefficients associated with the patch field
        virtual void updateCoeffs();

        //- Return patch-normal gradient
        virtual tmp<Field<vector> > snGrad() const;

        //- Return the matrix source coefficients corresponding to the
        //  evaluation of the gradient of this patchField
        virtual tmp<Field<vector> > gradientBoundaryCoeffs() const;

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
