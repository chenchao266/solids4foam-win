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

\*----------------------------------------------------------------------------*/

#include "solidPointTemperature.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "pointFields.H"
#include "OSspecific.H"
#ifdef OPENFOAMESIORFOUNDATION
    #include "volPointInterpolation.H"
#else
    #include "newLeastSquaresVolPointInterpolation.H"
#endif
#ifdef OPENFOAMFOUNDATION
    #include "OSspecific.H"
#endif

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(solidPointTemperature, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        solidPointTemperature,
        dictionary
    );
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::solidPointTemperature::writeData()
{
    // Lookup the solid mesh
    const fvMesh* meshPtr = NULL;
    if (time_.foundObject<fvMesh>("solid"))
    {
        meshPtr = &(time_.lookupObject<fvMesh>("solid"));
    }
    else
    {
        meshPtr = &(time_.lookupObject<fvMesh>("region0"));
    }
    const fvMesh& mesh = *meshPtr;

    if (mesh.foundObject<volScalarField>("T"))
    {
        // Read the temperature field
        const volScalarField& T = mesh.lookupObject<volScalarField>("T");

        // Create a point mesh
        const pointMesh& pMesh = pointMesh::New(mesh);

        // Create a point temperature field
        pointScalarField pointT
        (
            IOobject
            (
                "pointT",
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            pMesh,
            dimensionedScalar("zero", T.dimensions(), 0.0)
        );

#ifdef OPENFOAMFOUNDATION
        volPointInterpolation::New(mesh).interpolate(T, pointT);
#elif OPENFOAMESI
        mesh.lookupObject<volPointInterpolation>
        (
            "volPointInterpolation"
        ).interpolate(T, pointT);
#else
        mesh.lookupObject<newLeastSquaresVolPointInterpolation>
        (
            "newLeastSquaresVolPointInterpolation"
        ).interpolate(T, pointT);
#endif

        scalar pointTValue = 0.0;
        if (pointID_ > -1)
        {
            pointTValue = pointT[pointID_];
        }
        reduce(pointTValue, sumOp<scalar>());

        if (Pstream::master())
        {
            historyFilePtr_()
                << time_.time().value()
                << " " << pointTValue << endl;
        }
    }
    else
    {
        InfoIn(this->name() + " function object constructor")
            << "volScalarField T not found" << endl;
    }

    return true;
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::solidPointTemperature::solidPointTemperature
(
    const word& name,
    const Time& t,
    const dictionary& dict
)
:
    functionObject(name),
    name_(name),
    time_(t),
    pointID_(-1),
    historyFilePtr_()
{
    Info<< "Creating " << this->name() << " function object" << endl;

    // Lookup the point
    const vector point(dict.lookup("point"));

    const fvMesh* meshPtr = NULL;
    if (time_.foundObject<fvMesh>("solid"))
    {
        meshPtr = &(time_.lookupObject<fvMesh>("solid"));
    }
    else
    {
        meshPtr = &(time_.lookupObject<fvMesh>("region0"));
    }
    const fvMesh& mesh = *meshPtr;

    // Create history file if not already created
    if (historyFilePtr_.empty())
    {
        // Find the closest point
        scalar minDist = GREAT;

        forAll(mesh.points(), pI)
        {
            scalar dist = mag(mesh.points()[pI] - point);

            if (dist < minDist)
            {
                minDist = dist;
                pointID_ = pI;
            }
        }

        // Find global closest point
        const scalar globalMinDist = returnReduce(minDist, minOp<scalar>());
        int procNo = -1;
        if (mag(globalMinDist - minDist) < SMALL)
        {
            procNo = Pstream::myProcNo();
        }
        else
        {
            pointID_ = -1;
        }

        // More than one processor can have the point so we will take the proc
        // with the lowest processor number
        const int globalMinProc = returnReduce(procNo, minOp<int>());
        if (mag(globalMinProc - procNo) > SMALL)
        {
            pointID_ = -1;
        }

        if (pointID_ > -1)
        {
            Pout<< this->name()
                << ": distance from specified point is " << minDist
                << endl;
        }

        // File update
        if (Pstream::master())
        {
            fileName historyDir;

            const word startTimeName =
                time_.timeName(mesh.time().startTime().value());

            if (Pstream::parRun())
            {
                // Put in undecomposed case (Note: gives problems for
                // distributed data running)
                historyDir = time_.path()/".."/"postProcessing"/startTimeName;
            }
            else
            {
                historyDir = time_.path()/"postProcessing"/startTimeName;
            }

            // Create directory if does not exist.
            mkDir(historyDir);

            // Open new file at start up
            historyFilePtr_.reset
            (
                new OFstream
                (
                    historyDir/"solidPointTemperature_" + name + ".dat"
                )
            );

            // Add headers to output data
            if (historyFilePtr_.valid())
            {
                historyFilePtr_()
                    << "# Time value" << endl;
            }
        }
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::solidPointTemperature::start()
{
    return false;
}

#if FOAMEXTEND
bool Foam::solidPointTemperature::execute(const bool forceWrite)
#else
bool Foam::solidPointTemperature::execute()
#endif
{
    return writeData();
}

bool Foam::solidPointTemperature::read(const dictionary& dict)
{
    return true;
}


#ifdef OPENFOAMESIORFOUNDATION
bool Foam::solidPointTemperature::write()
{
    return false;
}
#endif

// ************************************************************************* //
