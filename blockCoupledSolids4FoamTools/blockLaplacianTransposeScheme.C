/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
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

#include "fv.H"
#include "HashTable.H"
#include "linear.H"
#include "fvMatrices.H"

#include "blockLaplacianTransposeScheme.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

// defineTypeNameAndDebug(blockLaplacianTranspose, 0);
defineRunTimeSelectionTable(blockLaplacianTranspose, Istream);

// * * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * //

tmp<blockLaplacianTranspose> blockLaplacianTranspose::New
(
    const fvMesh& mesh,
    Istream& schemeData
)
{
    if (fv::debug)
    {
        Info<< "blockLaplacianTranspose::New(const fvMesh&, Istream&) : "
               "constructing blockLaplacianTranspose"
            << endl;
    }

    if (schemeData.eof())
    {
        FatalIOErrorIn
        (
            "blockLaplacianTranspose::New(const fvMesh&, Istream&)",
            schemeData
        )   << "fvmBlockLaplacianTranspose scheme not specified" << endl << endl
            << "Valid fvmBlockLaplacianTranspose schemes are :" << endl
            << IstreamConstructorTablePtr_->sortedToc()
            << exit(FatalIOError);
    }

    const word schemeName(schemeData);

#if (OPENFOAM >= 2112)
    auto* ctorPtr = IstreamConstructorTable(schemeName);

    if (!ctorPtr)
    {
        FatalIOErrorInLookup
        (
            schemeData,
            "fvmBlockLaplacianTranspose",
            schemeName,
            *IstreamConstructorTablePtr_
        ) << exit(FatalIOError);
    }

#else
    IstreamConstructorTableType::iterator cstrIter =
        IstreamConstructorTablePtr_->find(schemeName);

    if (cstrIter == IstreamConstructorTablePtr_->end())
    {
        FatalIOErrorIn
        (
            "blockLaplacianTranspose::New(const fvMesh&, Istream&)",
            schemeData
        )   << "unknown fvmBlockLaplacianTranspose scheme " << schemeName
            << endl << endl
            << "Valid fvmBlockLaplacianTranspose schemes are :" << endl
            << IstreamConstructorTablePtr_->sortedToc()
            << exit(FatalIOError);
    }

    auto* ctorPtr = cstrIter();
#endif

    return ctorPtr(mesh, schemeData);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

blockLaplacianTranspose::~blockLaplacianTranspose()
{}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
