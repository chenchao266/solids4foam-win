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
    Foam::directionInfo

Description
    Holds direction in which to split cell (in fact a local coordinate axes).
    Information is a label and a direction.

    The direction is the normal
    direction to cut in. The label's meaning depends on whether the info
    is on a cell or on a face:
        - in cell: edge that is being cut. (determines for hex how cut is)
        - in face: local face point that is being cut or -1.
            -# (-1)  : cut is tangential to plane
            -# (>= 0): edge fp..fp+1 is cut

            (has to be facepoint, not vertex since vertex not valid across
             processors whereas f[0] should correspond to f[0] on other side)

    The rule is that if the label is set (-1 or higher) it is used
    (topological information only), otherwise the vector is used. This makes
    sure that we use topological information as much as possible and so a
    hex mesh is cut purely topologically. All other shapes are cut
    geometrically.

SourceFiles
    directionInfoI.H
    directionInfo.C

\*---------------------------------------------------------------------------*/

#ifndef directionInfo_H
#define directionInfo_H

#include "point.H"
#include "labelList.H"
#include "tensor2.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class polyPatch;
class polyMesh;
class primitiveMesh;
class edge;
class face;
class polyMesh;

// Forward declaration of friend functions and operators
class directionInfo;

Ostream& operator<<(Ostream&, const directionInfo&);
Istream& operator>>(Istream&, directionInfo&);


/*---------------------------------------------------------------------------*\
                           Class directionInfo Declaration
\*---------------------------------------------------------------------------*/

class directionInfo
{
    // Private data

        // Either mesh edge or face point
        label index_;

        // Local n axis
        vector n_;


    // Private Member Functions

        //- edge uses two labels
        static bool equal(const edge& e, const label, const label);

        //- Calculate mid point of edge.
        static point eMid(const primitiveMesh& mesh, const label edgeI);

        //- Find edge among edgeLabels that uses v0 and v1
        static label findEdge
        (
            const primitiveMesh& mesh,
            const labelList& edgeLabels,
            const label v1,
            const label v0
        );

        //- Return 'lowest' of a,b in face of size.
        static label lowest
        (
            const label size,
            const label a,
            const label b
        );

public:

    // Static Functions

        //- Given edge on hex cell find corresponding edge on face. Is either
        //  index in face or -1 (cut tangential to face). Public since is
        //  needed to fill in seed faces in meshWave.
        static label edgeToFaceIndex
        (
            const primitiveMesh& mesh,
            const label cellI,
            const label faceI,
            const label edgeI
        );

    // Constructors

        //- Construct null
        inline directionInfo();

        //- Construct from components
        inline directionInfo
        (
            const label,
            const vector& n
        );


    // Member Functions

        // Access

            inline label index() const
            {
                return index_;
            }

            inline const vector& n() const
            {
                return n_;
            }

        // Needed by FaceCellWave

            //- Check whether origin has been changed at all or
            //  still contains original (invalid) value.
            inline bool valid() const;

            //- Check for identical geometrical data. Used for cyclics checking.
            inline bool sameGeometry
            (
                const polyMesh&,
                const directionInfo&,
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
            bool updateCell
            (
                const polyMesh&,
                const label thisCellI,
                const label neighbourFaceI,
                const directionInfo& neighbourInfo,
                const scalar tol
            );

            //- Influence of neighbouring cell.
            bool updateFace
            (
                const polyMesh&,
                const label thisFaceI,
                const label neighbourCellI,
                const directionInfo& neighbourInfo,
                const scalar tol
            );

            //- Influence of different value on same face.
            bool updateFace
            (
                const polyMesh&,
                const label thisFaceI,
                const directionInfo& neighbourInfo,
                const scalar tol
            );

    // Member Operators

        // Needed for List IO
        inline bool operator==(const directionInfo&) const;

        inline bool operator!=(const directionInfo&) const;


    // IOstream Operators

        friend Ostream& operator<<(Ostream&, const directionInfo&);
        friend Istream& operator>>(Istream&, directionInfo&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "directionInfoI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
