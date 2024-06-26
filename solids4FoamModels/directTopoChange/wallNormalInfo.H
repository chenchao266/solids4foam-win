/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     | Version:     4.1
    \\  /    A nd           | Web:         http://www.foam-extend.org
     \\/     M anipulation  | For copyright notice see file Copyright
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::wallNormalInfo

Description
    Holds information regarding nearest wall point.
    Used in wall refinement.

SourceFiles
    wallNormalInfoI.H
    wallNormalInfo.C

\*---------------------------------------------------------------------------*/

#ifndef wallNormalInfo_H
#define wallNormalInfo_H

#include "point.H"
#include "label.H"
#include "scalar.H"
#include "tensor2.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class polyPatch;
class polyMesh;

// Forward declaration of friend functions and operators
class wallNormalInfo;

Ostream& operator<<(Ostream&, const wallNormalInfo&);
Istream& operator>>(Istream&, wallNormalInfo&);


/*---------------------------------------------------------------------------*\
                           Class wallNormalInfo Declaration
\*---------------------------------------------------------------------------*/

class wallNormalInfo
{
    // Private data

        //- Normal at nearest wall point
        vector normal_;

    // Private Member Functions

        //- Evaluate distance to point. Update normal_
        inline bool update(const wallNormalInfo& w2);

public:

    // Static data members

        //- initial point far away.
        static point greatVector;

    // Constructors

        //- Construct null
        inline wallNormalInfo();

        //- Construct from normal
        inline wallNormalInfo(const vector& normal);


    // Member Functions

        // Access

            inline const vector& normal() const;


        // Needed by FaceCellWave

            //- Check whether origin has been changed at all or
            //  still contains original (invalid) value.
            inline bool valid() const;

            //- Check for identical geometrical data. Used for cyclics checking.
            inline bool sameGeometry
            (
                const polyMesh&,
                const wallNormalInfo&,
                const scalar
            ) const;

            //- Convert any absolute coordinates into relative to (patch)face
            //  centre
            inline void leaveDomain
            (
                const polyMesh&,
                const polyPatch&,
                const label patchFaceI,
                const point& faceCentre
            );

            //- Reverse of leaveDomain
            inline void enterDomain
            (
                const polyMesh&,
                const polyPatch&,
                const label patchFaceI,
                const point& faceCentre
            );

            //- Apply rotation matrix to any coordinates
            inline void transform
            (
                const polyMesh&,
                const tensor&
            );

            //- Influence of neighbouring face.
            inline bool updateCell
            (
                const polyMesh&,
                const label thisCellI,
                const label neighbourFaceI,
                const wallNormalInfo& neighbourInfo,
                const scalar tol
            );

            //- Influence of neighbouring cell.
            inline bool updateFace
            (
                const polyMesh&,
                const label thisFaceI,
                const label neighbourCellI,
                const wallNormalInfo& neighbourInfo,
                const scalar tol
            );

            //- Influence of different value on same face.
            inline bool updateFace
            (
                const polyMesh&,
                const label thisFaceI,
                const wallNormalInfo& neighbourInfo,
                const scalar tol
            );

    // Member Operators

        // Needed for List IO
        inline bool operator==(const wallNormalInfo&) const;

        inline bool operator!=(const wallNormalInfo&) const;


    // IOstream Operators

        friend Ostream& operator<<(Ostream&, const wallNormalInfo&);
        friend Istream& operator>>(Istream&, wallNormalInfo&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "wallNormalInfoI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
