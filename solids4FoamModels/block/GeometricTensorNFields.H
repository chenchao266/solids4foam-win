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
    GeometricTensorNFields

Description
    Specialisation of GeometricField<T> for TensorN.

SourceFiles
    GeometricTensorNFields.C

\*---------------------------------------------------------------------------*/

#ifndef GeometricTensorNFields_H
#define GeometricTensorNFields_H

#include "GeometricField.H"
#include "DimensionedTensorNFields.H"
#include "TensorNFieldFields.H"

#define TEMPLATE template<template<class> class PatchField, class GeoMesh>
#include "GeometricFieldFunctionsM.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define TensorN_FieldFunctions(tensorType, diagTensorType, sphericalTensorType,     \
    vectorType, CmptType, ...)                                                  \
                                                                                    \
UNARY_FUNCTION(tensorType, tensorType, T, transform)                                \
UNARY_FUNCTION(tensorType, tensorType, inv, inv)                                    \
UNARY_FUNCTION(diagTensorType, tensorType, diag, diag)                              \
UNARY_FUNCTION(diagTensorType, tensorType, T, transform)                            \
UNARY_FUNCTION(tensorType, tensorType, negSumDiag, negSumDiag)                      \
UNARY_FUNCTION(CmptType, tensorType, contractScalar, contractScalar)                \
UNARY_FUNCTION(vectorType, tensorType, contractLinear, contractLinear)              \
                                                                                    \
BINARY_OPERATOR(tensorType, CmptType, tensorType, *,'*',multiply)                   \
BINARY_TYPE_OPERATOR(tensorType, CmptType, tensorType, *,'*',multiply)              \
                                                                                    \
BINARY_OPERATOR(tensorType, CmptType, tensorType, /,'|',divide)                     \
BINARY_TYPE_OPERATOR(tensorType, CmptType, tensorType, /,'|',divide)                \
                                                                                    \
BINARY_OPERATOR(vectorType, vectorType, tensorType, /,'|',divide)                   \
BINARY_TYPE_OPERATOR(vectorType, vectorType, tensorType, /,'|',divide)              \
                                                                                    \
BINARY_OPERATOR(tensorType, tensorType, tensorType, /,'|',divide)                   \
BINARY_TYPE_OPERATOR(tensorType, tensorType, tensorType, /,'|',divide)              \
                                                                                    \
BINARY_OPERATOR(tensorType, tensorType, diagTensorType, /,'|',divide)               \
BINARY_TYPE_OPERATOR(tensorType, tensorType, diagTensorType, /,'|',divide)          \
                                                                                    \
BINARY_OPERATOR(tensorType, diagTensorType, tensorType, /,'|',divide)               \
BINARY_TYPE_OPERATOR(tensorType, diagTensorType, tensorType, /,'|',divide)          \
                                                                                    \
BINARY_OPERATOR(tensorType, sphericalTensorType, tensorType, /,'|',divide)          \
BINARY_TYPE_OPERATOR(tensorType, sphericalTensorType, tensorType, /,'|',divide)     \
                                                                                    \
BINARY_OPERATOR(tensorType, tensorType, sphericalTensorType, /,'|',divide)          \
BINARY_TYPE_OPERATOR(tensorType, tensorType, sphericalTensorType, /,'|',divide)     \
                                                                                    \
BINARY_OPERATOR(tensorType, tensorType, tensorType, +,'+',add)                      \
BINARY_OPERATOR(tensorType, tensorType, tensorType, -,'-',subtract)                 \
                                                                                    \
BINARY_TYPE_OPERATOR(tensorType, tensorType, tensorType, +,'+',add)                 \
BINARY_TYPE_OPERATOR(tensorType, tensorType, tensorType, -,'-',subtract)            \
                                                                                    \
BINARY_OPERATOR(tensorType, diagTensorType, tensorType, +,'+',add)                  \
BINARY_OPERATOR(tensorType, diagTensorType, tensorType, -,'-',subtract)             \
                                                                                    \
BINARY_TYPE_OPERATOR(tensorType, diagTensorType, tensorType, +,'+',add)             \
BINARY_TYPE_OPERATOR(tensorType, diagTensorType, tensorType, -,'-',subtract)        \
                                                                                    \
BINARY_OPERATOR(tensorType, sphericalTensorType, tensorType, +,'+',add)             \
BINARY_OPERATOR(tensorType, sphericalTensorType, tensorType, -,'-',subtract)        \
                                                                                    \
BINARY_TYPE_OPERATOR(tensorType, sphericalTensorType, tensorType, +,'+',add)        \
BINARY_TYPE_OPERATOR(tensorType, sphericalTensorType, tensorType, -,'-',subtract)

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

forAllVectorTensorNTypes(TensorN_FieldFunctions)

}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#undef TensorN_FieldFunctions

#include "undefFieldFunctionsM.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#   include "GeometricTensorNFields.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
