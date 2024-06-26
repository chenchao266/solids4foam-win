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
    dimensionedVectorTensorN

Description
    Dimensioned VectorN and TensorN obtained from generic dimensioned type.

Author
    Ivor Clifford

SourceFiles
    dimensionedVectorTensorN.C

\*---------------------------------------------------------------------------*/

#ifndef dimensionedVectorTensorN_H
#define dimensionedVectorTensorN_H

#include "dimensionedType.H"
#include "VectorNFieldTypes.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

#define doMakeTypedef(type, Type, ...)      \
    typedef dimensioned<type> dimensioned##Type;

forAllVectorNTypes(doMakeTypedef)

forAllTensorNTypes(doMakeTypedef)

forAllDiagTensorNTypes(doMakeTypedef)

forAllSphericalTensorNTypes(doMakeTypedef)

#undef doMakeTypedef


// friend functions

dimensionSet diag(const dimensionSet& ds);
dimensionSet cmptSum(const dimensionSet& ds);
dimensionSet contractLinear(const dimensionSet& ds);
dimensionSet contractScalar(const dimensionSet& ds);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define UNARY_FUNCTION(returnType, type, fun, text)                                 \
inline dimensioned< returnType > fun(const dimensioned< type >& t);


#define BINARY_OPERATOR(returnType, type1, type2, op, text)                         \
dimensioned< returnType > op(const dimensioned< type1 >&,                           \
    const dimensioned< type2 >&);


#define dimensionedType_Funs(tensorType, diagTensorType,                            \
    sphericalTensorType, vectorType, cmptType, ...)                             \
                                                                                    \
UNARY_FUNCTION(tensorType, tensorType, inv, inv)                                    \
UNARY_FUNCTION(diagTensorType, diagTensorType, inv, inv)                            \
UNARY_FUNCTION(sphericalTensorType, sphericalTensorType, inv, inv)                  \
                                                                                    \
UNARY_FUNCTION(diagTensorType, tensorType, diag, diag)                              \
UNARY_FUNCTION(diagTensorType, diagTensorType, diag, diag)                          \
UNARY_FUNCTION(sphericalTensorType, sphericalTensorType, diag, diag)                \
                                                                                    \
UNARY_FUNCTION(vectorType, vectorType, cmptSum, cmptSum)                            \
                                                                                    \
BINARY_OPERATOR(tensorType, tensorType, diagTensorType, operator+, +)               \
BINARY_OPERATOR(tensorType, diagTensorType, tensorType, operator+, +)               \
BINARY_OPERATOR(tensorType, tensorType, sphericalTensorType, operator+, +)          \
BINARY_OPERATOR(tensorType, sphericalTensorType, tensorType, operator+, +)          \
BINARY_OPERATOR(diagTensorType, diagTensorType, sphericalTensorType, operator+, +)  \
BINARY_OPERATOR(diagTensorType, sphericalTensorType, diagTensorType, operator+, +)  \
                                                                                    \
BINARY_OPERATOR(tensorType, tensorType, diagTensorType, operator-, -)               \
BINARY_OPERATOR(tensorType, diagTensorType, tensorType, operator-, -)               \
BINARY_OPERATOR(tensorType, tensorType, sphericalTensorType, operator-, -)          \
BINARY_OPERATOR(tensorType, sphericalTensorType, tensorType, operator-, -)          \
BINARY_OPERATOR(diagTensorType, diagTensorType, sphericalTensorType, operator-, -)  \
BINARY_OPERATOR(diagTensorType, sphericalTensorType, diagTensorType, operator-, -)


forAllVectorTensorNTypes(dimensionedType_Funs)

#undef dimensionedType_Funs
#undef UNARY_FUNCTION
#undef BINARY_OPERATOR

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "dimensionedVectorTensorNI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

