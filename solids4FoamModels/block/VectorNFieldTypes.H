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

Description
    Include the header files for all the primitive types that Fields are
    instantiated for.

\*---------------------------------------------------------------------------*/

#ifndef VectorNFieldTypes_H
#define VectorNFieldTypes_H

#include "vectorx.H"
 
#include "tensorx.H"
 
#include "diagTensorx.H"
 

#define forAllVectorNTypes(m, ...)              \
    m(vector2, Vector2, ##__VA_ARGS__)                       \
    m(vector3, Vector3, ##__VA_ARGS__)                       \
    m(vector4, Vector4, ##__VA_ARGS__)                       \
    m(vector6, Vector6, ##__VA_ARGS__)                       \
    m(vector8, Vector8, ##__VA_ARGS__)

#define forAllTensorNTypes(m, ...)              \
    m(tensor2, Tensor2, ##__VA_ARGS__)                       \
    m(tensor3, Tensor3, ##__VA_ARGS__)                       \
    m(tensor4, Tensor4, ##__VA_ARGS__)                       \
    m(tensor6, Tensor6, ##__VA_ARGS__)                       \
    m(tensor8, Tensor8, ##__VA_ARGS__)

#define forAllDiagTensorNTypes(m, ...)          \
    m(diagTensor2, DiagTensor2, ##__VA_ARGS__)               \
    m(diagTensor3, DiagTensor3, ##__VA_ARGS__)               \
    m(diagTensor4, DiagTensor4, ##__VA_ARGS__)               \
    m(diagTensor6, DiagTensor6, ##__VA_ARGS__)               \
    m(diagTensor8, DiagTensor8, ##__VA_ARGS__)

#define forAllSphericalTensorNTypes(m, ...)     \
    m(sphericalTensor2, SphericalTensor2, ##__VA_ARGS__)     \
    m(sphericalTensor3, SphericalTensor3, ##__VA_ARGS__)     \
    m(sphericalTensor4, SphericalTensor4, ##__VA_ARGS__)     \
    m(sphericalTensor6, SphericalTensor6, ##__VA_ARGS__)     \
    m(sphericalTensor8, SphericalTensor8, ##__VA_ARGS__)

#define forAllVectorTensorNTypes(m, ...)                            \
    m(tensor2, diagTensor2, sphericalTensor2, vector2, scalar, ##__VA_ARGS__)    \
    m(tensor3, diagTensor3, sphericalTensor3, vector3, scalar, ##__VA_ARGS__)    \
    m(tensor4, diagTensor4, sphericalTensor4, vector4, scalar, ##__VA_ARGS__)    \
    m(tensor6, diagTensor6, sphericalTensor6, vector6, scalar, ##__VA_ARGS__)    \
    m(tensor8, diagTensor8, sphericalTensor8, vector8, scalar, ##__VA_ARGS__)

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
