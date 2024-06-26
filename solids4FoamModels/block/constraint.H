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
    constraint

Description
    A storage mechanism which allows setting of the fixed value and
    consequently recovering the equation for a single row of the matrix as
    well as the source. The equation is taken out of the matrix using a
    variant of compact matrix storage mechanism.

SourceFiles
    constraint.C

\*---------------------------------------------------------------------------*/

#ifndef constraint_H
#define constraint_H

#include "labelList.H"
#include "scalarField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class Type>
class constraint;

template<class Type>
Ostream& operator<<(Ostream&, const constraint<Type>&);


/*---------------------------------------------------------------------------*\
                        Class constraint Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class constraint
{
    // Private data

        //- Matrix row ID
        label rowID_;

        //- Fixed value
        Type value_;

        //- Fixed components (0-1) 1 = fixed, 0 = free
        Type fixedComponents_;

        //- Are matrix coefficients set?
        bool matrixCoeffsSet_;

        //- Diagonal coefficient
        scalar diagCoeff_;

        //- Source
        Type source_;

        //- Upper coefficients
        scalarField* upperCoeffsOwnerPtr_;

        scalarField* upperCoeffsNeighbourPtr_;

        //- Lower coefficients
        scalarField* lowerCoeffsOwnerPtr_;

        scalarField* lowerCoeffsNeighbourPtr_;

    // Private Member Functions

        //- Return scalar component of value. Used to simplify templating
        scalar componentOfValue(const Type&, const direction) const;


public:


    // Constructors

        //- Construct from components
        constraint
        (
            const label row,
            const Type value,
            const Type& fixedCmpts = pTraits<Type>::one_
        );

        //- Construct from matrix and other components
        template<template<class> class Matrix>
        constraint
        (
            const Matrix<Type>&,
            const label row,
            const Type value,
            const Type& fixedCmpts = pTraits<Type>::one_
        );

        //- Construct as copy
        constraint(const constraint&);

        //- Construct from Istream
        constraint(Istream&);


    // Destructor

        ~constraint();


    // Member Functions

        //- Return matrix row ID
        label rowID() const
        {
            return rowID_;
        }

        //- Return fixed value
        Type value() const
        {
            return value_;
        }

        //- Return map of fixed components
        const Type& fixedComponents() const
        {
            return fixedComponents_;
        }

        //- Return diagonal coefficient
        scalar diagCoeff() const;

        //- Return source
        Type source() const;

        //- Return off-diagonal coefficients
        const scalarField& upperCoeffsOwner() const;

        const scalarField& upperCoeffsNeighbour() const;

        const scalarField& lowerCoeffsOwner() const;

        const scalarField& lowerCoeffsNeighbour() const;

        //- Combine with existing equation
        void combine(const constraint&);

        //- Set matrix coefficients
        template<template<class> class Matrix>
        void setMatrix(const Matrix<Type>&);


        //- Eliminate equation
        template<template<class> class Matrix>
        static void eliminateEquation(Matrix<Type>&, const label, const Type&);

        //- Eliminate equation
        template<template<class> class Matrix>
        void eliminateEquation(Matrix<Type>&) const;

        //- Eliminate component equation with given direction and
        //  component source
        template<template<class> class Matrix>
        void eliminateEquation
        (
            Matrix<Type>&,
            const direction,
            scalarField&
        ) const;


        //- Set source in eliminated equation
        template<template<class> class Matrix>
        static void setSource(Matrix<Type>&, const label, const Type&);

        //- Set source in eliminated equation
        template<template<class> class Matrix>
        void setSource(Matrix<Type>&) const;

        //- Set source and diagonal in eliminated equation
        template<template<class> class Matrix>
        void setSourceDiag
        (
            Matrix<Type>&,
            const direction,
            scalarField& psiCmpt,
            scalarField& sourceCmpt
        ) const;


        //- Reconstruct matrix coefficients
        template<template<class> class Matrix>
        void reconstructMatrix(Matrix<Type>&) const;

        //- Clear matrix coefficients
        void clearMatrix();


    // Member Operators

        void operator=(const constraint<Type>&);


    // Ostream Operator

        friend Ostream& operator<< <Type>
        (
            Ostream&,
            const constraint<Type>&
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#    include "constraint.C"
#    include "constraintTools.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "scalarConstraint.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
