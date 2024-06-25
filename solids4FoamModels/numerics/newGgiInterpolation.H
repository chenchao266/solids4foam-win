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
    newGgiInterpolation

Author
    Hrvoje Jasak, Wikki Ltd.  All rights reserved

Description
    Mass-conservative face interpolation: typedefs for polyPatch and faceZone

\*---------------------------------------------------------------------------*/

#ifndef newGgiInterpolation_H
#define newGgiInterpolation_H

#include "newGGIInterpolationTemplate.H"
#include "primitivePatch.H"
#include "face.H"
#include "SubList.H"
#include "pointField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    typedef newGGIInterpolation
    <
        PrimitivePatch<SubList<face>, const pointField&>,
        PrimitivePatch<SubList<face>, const pointField&>
    >   newGgiInterpolation;

    typedef newGGIInterpolation
    <
        PrimitivePatch<List<face>, const pointField&>,
        PrimitivePatch<List<face>, const pointField&>
    >   newGgiZoneInterpolation;

    typedef newGGIInterpolation
    <
        PrimitivePatch<List<face>, pointField>,
        PrimitivePatch<List<face>, pointField>
    >   newGgiStandAlonePatchInterpolation;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
