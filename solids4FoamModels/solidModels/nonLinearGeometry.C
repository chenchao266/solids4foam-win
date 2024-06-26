/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     | Version:     3.2
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

\*---------------------------------------------------------------------------*/

#include "nonLinearGeometry.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef OPENFOAMESI
const Foam::Enum
<
    Foam::nonLinearGeometry::nonLinearType
>
Foam::nonLinearGeometry::nonLinearNames_
({
    {Foam::nonLinearGeometry::nonLinearType::LINEAR_GEOMETRY, "linearGeometry"},
    {Foam::nonLinearGeometry::nonLinearType::UPDATED_LAGRANGIAN, "updatedLagrangian"},
    {Foam::nonLinearGeometry::nonLinearType::TOTAL_LAGRANGIAN, "totalLagrangian"},
});
#else
template<>
const char*
Foam::NamedEnum<Foam::nonLinearGeometry::nonLinearType, 3>::names[] =
{
    "linearGeometry",
    "updatedLagrangian",
    "totalLagrangian"
};

const Foam::NamedEnum<Foam::nonLinearGeometry::nonLinearType, 3>
Foam::nonLinearGeometry::nonLinearNames_;
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
