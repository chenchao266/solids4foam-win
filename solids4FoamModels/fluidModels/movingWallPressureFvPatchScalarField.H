/*---------------------------------------------------------------------------* \
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
    movingWallPressureFvPatchScalarField

Description

SourceFiles
    movingWallPressureFvPatchScalarField.C

\*---------------------------------------------------------------------------*/

#ifndef movingWallPressureFvPatchScalarFields_H
#define movingWallPressureFvPatchScalarFields_H

#include "fvPatchFields.H"
#include "fixedGradientFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                  Class movingWallPressureFvPatch Declaration
\*---------------------------------------------------------------------------*/

class movingWallPressureFvPatchScalarField
:
    public fixedGradientFvPatchScalarField
{
    // Private data

        //- Acceleration of the solid side of the interface
        //  in the previous fsi iteration
        vectorField prevAcceleration_;

public:

    //- Runtime type information
    TypeName("movingWallPressure");


    // Constructors

        //- Construct from patch and internal field
        movingWallPressureFvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        movingWallPressureFvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given
        //  movingWallPressureFvPatchScalarField onto a new patch
        movingWallPressureFvPatchScalarField
        (
            const movingWallPressureFvPatchScalarField&,
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        movingWallPressureFvPatchScalarField
        (
            const movingWallPressureFvPatchScalarField&
#ifdef OPENFOAMFOUNDATION
        ) = delete;
#else
        );

        //- Construct and return a clone
        virtual tmp<fvPatchScalarField> clone() const
        {
            return tmp<fvPatchScalarField>
            (
                new movingWallPressureFvPatchScalarField(*this)
            );
        }
#endif

        //- Construct as copy setting internal field reference
        movingWallPressureFvPatchScalarField
        (
            const movingWallPressureFvPatchScalarField&,
            const DimensionedField<scalar, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchScalarField> clone
        (
            const DimensionedField<scalar, volMesh>& iF
        ) const
        {
            return tmp<fvPatchScalarField>
            (
                new movingWallPressureFvPatchScalarField(*this, iF)
            );
        }


    // Member functions

        //- Update the coefficients associated with the patch field
        virtual void updateCoeffs();

        //- Get previous pressure patch field
        const vectorField& prevAcceleration() const
        {
            return prevAcceleration_;
        }

        //- Get previous pressure patch field
        vectorField& prevAcceleration()
        {
            return prevAcceleration_;
        }

//         //- Evaluate the patch field
//         virtual void evaluate
//         (
// #ifdef OPENFOAMESIORFOUNDATION
//             const Pstream::commsTypes commsType = Pstream::commsTypes::blocking
// #else
//             const Pstream::commsTypes commsType = Pstream::blocking
// #endif
//         );

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
