/*---------------------------------------------------------------------------*\
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
    Foam::tableReader

Description
    Base class to read table data for the interpolationTable

SourceFiles
    tableReader.C

\*---------------------------------------------------------------------------*/

#ifndef tableReader_H
#define tableReader_H

#include "fileName.H"
#include "wordList.H"
#include "vector2.H"
#include "tensor2.H"
#include "typeInfo.H"
#include "runTimeSelectionTables.H"
#include "autoPtr.H"
#include "dictionary.H"
#include "Tuple2.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class tableReader Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class tableReader
{

public:

    //- Runtime type information
    TypeName("tableReader");

    // Declare run-time constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            tableReader,
            dictionary,
            (const dictionary& dict),
            (dict)
        );


    // Constructors

        //- Construct from dictionary
        tableReader(const dictionary& dict);

        //- Construct and return a clone
        virtual autoPtr<tableReader<Type>> clone() const = 0;


    // Selectors

        //- Return a reference to the selected tableReader
        static autoPtr<tableReader> New(const dictionary& spec);


    //- Destructor
    virtual ~tableReader();


    // Member Functions

        //- Read the table
        virtual void operator()
        (
            const fileName&,
            List<Tuple2<scalar, Type>>&
        ) = 0;

        //- Read the 2D table
        virtual void operator()
        (
            const fileName&,
            List<Tuple2<scalar, List<Tuple2<scalar, Type>>>>&
        ) = 0;

        //- Write additional information
        virtual void write(Ostream& os) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "tableReader.C"
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
