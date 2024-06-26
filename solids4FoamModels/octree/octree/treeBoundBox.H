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
    Foam::treeBoundBox

Description
    Standard boundBox + extra functionality for use in octree.

    Numbering of corner points is according to octant numbering.

    On the back plane (z=0):

    @verbatim
        Y
        ^
        |
        +--------+
        |2      3|
        |        |
        |        |
        |        |
        |0      1|
        +--------+->X
    @endverbatim

    For the front plane add 4 to the point labels.


SourceFiles
    treeBoundBoxI.H
    treeBoundBox.C

\*---------------------------------------------------------------------------*/

#ifndef treeBoundBox_H
#define treeBoundBox_H

#include "boundBox.H"
#include "direction.H"
#include "pointField.H"
#include "faceList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class Random;

// Forward declaration of friend functions and operators
class treeBoundBox;
bool operator==(const treeBoundBox&, const treeBoundBox&);
bool operator!=(const treeBoundBox&, const treeBoundBox&);

Istream& operator>>(Istream& is, treeBoundBox&);
Ostream& operator<<(Ostream& os, const treeBoundBox&);


/*---------------------------------------------------------------------------*\
                           Class treeBoundBox Declaration
\*---------------------------------------------------------------------------*/

class treeBoundBox
:
    public boundBox
{

private:

        //- To initialise faceNormals.
        static FixedList<vector, 6> calcFaceNormals();

public:

    // Static data members

        //- The great value used for greatBox and invertedBox
        static const scalar great;

        //- As per boundBox::greatBox, but with GREAT instead of VGREAT
        static const treeBoundBox greatBox;

        //- As per boundBox::invertedBox, but with GREAT instead of VGREAT
        static const treeBoundBox invertedBox;

        //- Bits used for octant/point coding.
        //  Every octant/corner point is the combination of three faces.
        enum octantBit
        {
            RIGHTHALF = 0x1 << 0,
            TOPHALF   = 0x1 << 1,
            FRONTHALF = 0x1 << 2
        };

        //- Face codes
        enum faceId
        {
            LEFT   = 0,
            RIGHT  = 1,
            BOTTOM = 2,
            TOP    = 3,
            BACK   = 4,
            FRONT  = 5
        };

        //- Bits used for face coding
        enum faceBit
        {
            NOFACE    = 0,
            LEFTBIT   = 0x1 << LEFT,    //1
            RIGHTBIT  = 0x1 << RIGHT,   //2
            BOTTOMBIT = 0x1 << BOTTOM,  //4
            TOPBIT    = 0x1 << TOP,     //8
            BACKBIT   = 0x1 << BACK,    //16
            FRONTBIT  = 0x1 << FRONT,   //32
        };

        //- Edges codes.
        //  E01 = edge between 0 and 1.
        enum edgeId
        {
            E01 = 0,
            E13 = 1,
            E23 = 2,
            E02 = 3,

            E45 = 4,
            E57 = 5,
            E67 = 6,
            E46 = 7,

            E04 = 8,
            E15 = 9,
            E37 = 10,
            E26 = 11
        };

        //- Face to point addressing
        static const faceList faces;

        //- Edge to point addressing
        static const edgeList edges;

        //- Per face the unit normal
        static const FixedList<vector, 6> faceNormals;


        //- Face on which neighbour is
        static direction neighbourFaceBits(const label&);


    // Constructors

        //- Construct null setting points to zero
        inline treeBoundBox();

        //- Construct from components
        inline treeBoundBox(const point& min, const point& max);

        //- Construct from components
        explicit inline treeBoundBox(const boundBox& bb);

        //- Construct as the bounding box of the given pointField.
        //  Local processor domain only (no reduce as in boundBox)
        explicit treeBoundBox(const UList<point>&);

        //- Construct as subset of points
        //  Local processor domain only (no reduce as in boundBox)
        treeBoundBox(const UList<point>&, const UList<label>& meshPoints);

        //- Construct from Istream
        treeBoundBox(Istream&);


    // Member functions

        // Access

            //- Typical dimension length,height,width
            inline scalar typDim() const;

            //- vertex coordinates. In octant coding.
            pointField points() const;


        // Check

            //- Corner point given octant
            inline point corner(const direction) const;

            //- Sub box given by octant number. Midpoint calculated.
            treeBoundBox subBbox(const direction) const;

            //- Sub box given by octant number. Midpoint provided.
            treeBoundBox subBbox(const point& mid, const direction) const;

            //- Returns octant number given point and the calculated midpoint.
            inline direction subOctant
            (
                const point& pt
            ) const;

            //- Returns octant number given point and midpoint.
            static inline direction subOctant
            (
                const point& mid,
                const point& pt
            );

            //- Returns octant number given point and the calculated midpoint.
            //  onEdge set if the point is on edge of subOctant
            inline direction subOctant
            (
                const point& pt,
                bool& onEdge
            ) const;

            //- Returns octant number given point and midpoint.
            //  onEdge set if the point is on edge of subOctant
            static inline direction subOctant
            (
                const point& mid,
                const point& pt,
                bool& onEdge
            );

            //- Returns octant number given intersection and midpoint.
            //  onEdge set if the point is on edge of subOctant
            //  If onEdge, the direction vector determines which octant to use
            //  (acc. to which octant the point would be if it were moved
            //  along dir)
            static inline direction subOctant
            (
                const point& mid,
                const vector& dir,
                const point& pt,
                bool& onEdge
            );

            //- Calculates optimal order to look for nearest to point.
            //  First will be the octant containing the point,
            //  second the octant with boundary nearest to the point etc.
            inline void searchOrder
            (
                const point& pt,
                 FixedList<direction, 8>& octantOrder
            ) const;

            //- Overlaps other boundingbox?
            inline bool overlaps(const boundBox&) const;

            //- Overlaps boundingSphere (centre + sqr(radius))?
            bool overlaps(const point&, const scalar radiusSqr) const;

            //- Intersects segment; set point to intersection position and face
            //  Will return true if intersection found.
            //  (pt argument used during calculation even if not intersecting).
            //  Calculates intersections from outside supplied vector
            //  (overallStart, overallVec). This is so when
            //  e.g. tracking through lots of consecutive boxes
            // (typical octree) we're not accumulating truncation errors. Set
            // to start, (end-start) if not used.
            bool intersects
            (
                const point& overallStart,
                const vector& overallVec,
                const point& start,
                const point& end,
                point& pt,
                direction& ptBits
            ) const;

            //- Like above but does not return faces point is on
            bool intersects
            (
                const point& start,
                const point& end,
                point& pt
            ) const;

            //- fully contains other boundingBox?
            inline bool contains(const treeBoundBox&) const;

            //- Contains point? (inside or on edge)
            inline bool contains(const point&) const;

            //- Contains point (inside or on edge) and moving in direction
            //  dir would cause it to go inside.
            bool contains(const vector& dir, const point&) const;

            //- Code position of pt on bounding box faces
            direction faceBits(const point& pt) const;

            //- Position of point relative to bounding box
            direction posBits(const point&) const;

            //- Calculate nearest and furthest (to point) vertex coords of
            //  bounding box
            void calcExtremities
            (
                const point& pt,
                point& nearest,
                point& furthest
            ) const;

            //- Returns distance point to furthest away corner.
            scalar maxDist(const point&) const;

            //- Compare distance to point with other bounding box
            //  return:
            //  -1 : all vertices of my bounding box are nearer than any of
            //       other
            //  +1 : all vertices of my bounding box are further away than
            //       any of other
            //   0 : none of the above.
            label distanceCmp(const point&, const treeBoundBox& other) const;

            //- Return slightly wider bounding box
            //  Extends all dimensions with s*span*Random::scalar01()
            //  and guarantees in any direction s*mag(span) minimum width
            inline treeBoundBox extend(Random&, const scalar s) const;

            //- Return slightly wider bounding box
            //  Extends all dimensions with s*span
            //  MB, 28/May/2011
            inline treeBoundBox extend(const scalar s) const;


    // Friend Operators

        friend bool operator==(const treeBoundBox&, const treeBoundBox&);
        friend bool operator!=(const treeBoundBox&, const treeBoundBox&);

    // IOstream operator

        friend Istream& operator>>(Istream& is, treeBoundBox&);
        friend Ostream& operator<<(Ostream& os, const treeBoundBox&);

};


//- Data associated with treeBoundBox type are contiguous
//template<>
//inline bool contiguous<treeBoundBox>() {return contiguous<boundBox>();}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

#include "treeBoundBoxI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
