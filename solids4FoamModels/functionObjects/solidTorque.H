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
    solidTorque

Description
    FunctionObject reports torque on the specified patch about the given axis.

    The user must specify the patch to be integrated over, and optionally the
    name of the stress tensor.

    To specify the axis, a point on the axis must be specified and the axis
    direction.

Author
    Philip Cardiff, UCD. All rights reserved.

SourceFiles
    solidTorque.C

\*---------------------------------------------------------------------------*/

#ifndef solidTorque_H
#define solidTorque_H

#include "functionObject.H"
#include "dictionary.H"
#include "fvMesh.H"
#include "OFstream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class solidTorque Declaration
\*---------------------------------------------------------------------------*/

class solidTorque
:
    public functionObject
{
    // Private data

        //- Name
        const word name_;

        //- Reference to main object registry
        const Time& time_;

        //- History patch ID
        label historyPatchID_;

        //- If the history patch is found
        bool patchFound_;

        //- Point on the axis
        const vector pointOnAxis_;

        //- Axis direction
        vector axis_;

        //- Stress/strain file ptr
        autoPtr<OFstream> historyFilePtr_;


    // Private Member Functions

        //- Write data
        bool writeData();

        //- Disallow default bitwise copy construct
        solidTorque(const solidTorque&);

        //- Disallow default bitwise assignment
        void operator=(const solidTorque&);


public:

    //- Runtime type information
    TypeName("solidTorque");


    // Constructors

        //- Construct from components
        solidTorque
        (
            const word& name,
            const Time&,
            const dictionary&
        );


    // Member Functions

        //- start is called at the start of the time-loop
        virtual bool start();

        //- execute is called at each ++ or += of the time-loop
#if FOAMEXTEND
        virtual bool execute(const bool forceWrite);
#else
        virtual bool execute();
#endif

        //- Read and set the function object if its data has changed
        virtual bool read(const dictionary& dict);

#ifdef OPENFOAMESIORFOUNDATION
        //- Write
        virtual bool write();
#else
        //- Update for changes of mesh
        virtual void updateMesh(const mapPolyMesh&)
        {}

        //- Update for changes of mesh
        virtual void movePoints(const pointField&)
        {}
#endif
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
