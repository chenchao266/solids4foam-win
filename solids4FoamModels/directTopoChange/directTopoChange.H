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
    Foam::directTopoChange

Description
    Direct mesh changes based on v1.3 polyTopoChange syntax.

    Instead of recording changes and executing them all in one go (as did
    v1.3 polyTopoChange) this class actually holds the current
    points/faces/cells and does the change immediately.
    It can be asked to compress out all unused points/faces/cells and
    renumber everything to be consistent.

    Note:
    - directTopoChange can be copied.
    - adding a face using non-existing cells causes all intermediate cells
    to be added. So always first add cells/points and then faces.
    (or set strict checking)
    - strict checking:
        - any added/modified face can only use already existing vertices
        - any added face can only use already existing cells
        - no item can be removed more than once.
    - removed cell: cell set to 0 faces.
    - removed face: face set to 0 vertices.
    - removed point: coordinate set to greatPoint (GREAT,GREAT,GREAT).
    Note that this might give problems if this value is used already.
    To see if point is equal to above value we don't use == (which might give
    problems with roundoff error) but instead compare the individual component
    with >.
    - coupled patches: the reorderCoupledFaces routine (borrowed from
    the couplePatches utility) reorders coupled patch faces and
    uses the cyclicPolyPatch,processorPolyPatch functionality.

SourceFiles
    directTopoChange.C
    directTopoChangeI.H
    directTopoChangeTemplates.C

\*---------------------------------------------------------------------------*/

#ifndef directTopoChange_H
#define directTopoChange_H

#include "autoPtr.H"
#include "DynamicList.H"
#include "labelList.H"
#include "IOobject.H"
#include "typeInfo.H"
#include "pointField.H"
#include "PtrList.H"
#include "cellList.H"
#include "Map.H"
#include "HashSet.H"
#include "mapPolyMesh.H"
#include "CompactListList.H"
#include "PackedList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class face;
class primitiveMesh;
class polyMesh;
class Time;
class fileName;
class polyBoundaryMesh;
class polyPatch;
class dictionary;
class topoAction;
class objectMap;

/*---------------------------------------------------------------------------*\
                           Class directTopoChange Declaration
\*---------------------------------------------------------------------------*/

class directTopoChange
{
    // Static data members

        //- Value of deleted point
        static const point greatPoint;


    // Private data

        //- Whether to allow referencing illegal points/cells/faces
        //  when adding/removing data.
        bool strict_;


        // Patches

            //- Number of patches
            label nPatches_;


        // Points

            //- Current point set
            DynamicList<point> points_;

            //- Original point label (or masterpoint for added points)
            DynamicList<label> pointMap_;

            //- For all original and added points contains new point label.
            //  (used to map return value of addPoint to new mesh point)
            DynamicList<label> reversePointMap_;

            //- Zone of point
            Map<label> pointZone_;

            //- Retired points
            labelHashSet retiredPoints_;


        // Faces

            //- Current faceList
            DynamicList<face> faces_;

            //- Patch for every external face (-1 for internal faces)
            DynamicList<label> region_;

            //- Owner for all faces
            DynamicList<label> faceOwner_;

            //- Neighbour for internal faces (-1 for external faces)
            DynamicList<label> faceNeighbour_;

            //- Original face label. Or master face for added-from-faces;
            //  -1 for faces added-from-edge or added-from-point)
            DynamicList<label> faceMap_;

            //- For all original and added faces contains new face label
            //  (used to map return value of addFace to new mesh face)
            DynamicList<label> reverseFaceMap_;

            //- Faces added from point (corresponding faceMap_ will
            //  be -1)
            Map<label> faceFromPoint_;

            //- Faces added from edge (corresponding faceMap_ will
            //  be -1)
            Map<label> faceFromEdge_;

            //- In mapping whether to reverse the flux.
            labelHashSet flipFaceFlux_;

            //- Zone of face
            Map<label> faceZone_;

            //- Orientation of face in zone
            Map<bool> faceZoneFlip_;

            //- Active faces
            label nActiveFaces_;


        // Cells

            //- Original cell label or master cell for added-from-cell;
            //  -1 for cells added from face or edge.
            DynamicList<label> cellMap_;

            //- For all original and added cells contains new cell label
            //  (used to map return value of addCell to new mesh cell)
            DynamicList<label> reverseCellMap_;

            //- Cells added from point
            Map<label> cellFromPoint_;

            //- Cells added from edge
            Map<label> cellFromEdge_;

            //- Cells added from face
            Map<label> cellFromFace_;

            //- Zone of cell
            DynamicList<label> cellZone_;


    // Private Member Functions

        //- Reorder contents of container according to map
        template<class T>
        static void reorder(const labelList& map, DynamicList<T>&);
        template<class T>
        static void reorder(const labelList& map, List<DynamicList<T> >&);
        template<class T>
        static void renumberKey(const labelList& map, Map<T>&);

        //- Renumber elements of list according to map
        static void renumber(const labelList&, DynamicList<label>&);
        static void renumber(const labelList&, labelHashSet&);
        //- Special handling of reverse maps which have <-1 in them
        static void renumberReverseMap(const labelList&, DynamicList<label>&);

        //- Renumber & compact elements of list according to map
        static void renumberCompact(const labelList&, labelList&);

        //- Count number of added and removed quantities from maps.
        static void countMap
        (
            const labelList& map,
            const labelList& reverseMap,
            label& nAdd,
            label& nInflate,
            label& nMerge,
            label& nRemove
        );

        //- Print some stats about mesh
        static void writeMeshStats(const polyMesh& mesh, Ostream&);

        //- Calculate object maps. Requires reverseMap to have destination
        //  to be marked with <-1.
        static void getMergeSets
        (
            const labelList& reverseCellMap,
            const labelList& cellMap,
            List<objectMap>& cellsFromCells
        );

        //- Check inputs to modFace or addFace
        void checkFace
        (
            const face&,
            const label faceI,
            const label own,
            const label nei,
            const label patchI,
            const label zoneI
        ) const;

        //- Construct cells (in packed storage)
        void makeCells
        (
            const label nActiveFaces,
            labelList& cellFaces,
            labelList& cellFaceOffsets
        ) const;

        //- Construct cellCells (in packed storage)
        void makeCellCells
        (
            const label nActiveFaces,
            CompactListList<label>& cellCells
        ) const;

        //- Cell ordering (bandCompression). Returns number of remaining cells.
        label getCellOrder(const CompactListList<label>&, labelList&) const;

        //- Do upper-triangular ordering and patch ordering.
        void getFaceOrder
        (
            const label nActiveFaces,
            const labelList& cellFaces,
            const labelList& cellFaceOffsets,

            labelList& oldToNew,
            labelList& patchSizes,
            labelList& patchStarts
        ) const;

        //- Compact and reorder faces according to map
        void reorderCompactFaces
        (
            const label newSize,
            const labelList& oldToNew
        );

        //- Remove all unused/removed points/faces/cells and update
        //  face ordering (always), cell ordering (bandcompression,
        //  orderCells=true),
        //  point ordering (sorted into internal and boundary points,
        //  orderPoints=true)
        void compact
        (
            const bool orderCells,
            const bool orderPoints,
            label& nInternalPoints,
            labelList& patchSizes,
            labelList& patchStarts
        );

        //- Select either internal or external faces out of faceLabels
        static labelList selectFaces
        (
            const primitiveMesh&,
            const labelList& faceLabels,
            const bool internalFacesOnly
        );

        //- Calculate mapping for patchpoints only
        void calcPatchPointMap
        (
            const List<Map<label> >&,
            const polyBoundaryMesh&,
            labelListList&
        ) const;

        void calcFaceInflationMaps
        (
            const polyMesh&,
            List<objectMap>&,
            List<objectMap>&,
            List<objectMap>&
        ) const;

        void calcCellInflationMaps
        (
            const polyMesh&,
            List<objectMap>&,
            List<objectMap>&,
            List<objectMap>&,
            List<objectMap>&
        ) const;

        void resetZones
        (
            const polyMesh&,        // mesh to get existing info from
            polyMesh&,              // mesh to change zones on
            labelListList&,
            labelListList&,
            labelListList&
        ) const;

        void calcFaceZonePointMap
        (
            const polyMesh&,
            const List<Map<label> >&,
            labelListList&
        ) const;


        // Coupling

            //- Rotate face by number of positions
            static face rotateFace(const face& f, const label nPos);

            //- Do all coupled patch face reordering
            void reorderCoupledFaces
            (
                const bool syncParallel,
                const polyBoundaryMesh&,
                const labelList& patchStarts,
                const labelList& patchSizes,
                const pointField& points
            );

        void compactAndReorder
        (
            const polyMesh&,
            const bool syncParallel,
            const bool orderCells,
            const bool orderPoints,
            label& nInternalPoints,
            pointField& newPoints,
            labelList& patchSizes,
            labelList& patchStarts,
            List<objectMap>& pointsFromPoints,
            List<objectMap>& facesFromPoints,
            List<objectMap>& facesFromEdges,
            List<objectMap>& facesFromFaces,
            List<objectMap>& cellsFromPoints,
            List<objectMap>& cellsFromEdges,
            List<objectMap>& cellsFromFaces,
            List<objectMap>& cellsFromCells,
            List<Map<label> >& oldPatchMeshPointMaps,
            labelList& oldPatchNMeshPoints,
            labelList& oldPatchStarts,
            List<Map<label> >& oldFaceZoneMeshPointMaps
        );

public:

    //- Runtime type information
    ClassName("directTopoChange");



    // Constructors

        //- Construct without mesh. Either specify nPatches or use
        //  setNumPatches before trying to make a mesh (makeMesh, changeMesh)
        directTopoChange(const label nPatches, const bool strict = true);

        //- Construct from mesh. Adds all points/face/cells from mesh.
        directTopoChange(const polyMesh& mesh, const bool strict = true);


    // Member Functions

        // Access

            //- Points. Shrunk after constructing mesh or calling of compact()
            const DynamicList<point>& points() const
            {
                return points_;
            }

            //- Faces
            const DynamicList<face>& faces() const
            {
                return faces_;
            }

            const DynamicList<label>& region() const
            {
                return region_;
            }

            //- Owner
            const DynamicList<label>& faceOwner() const
            {
                return faceOwner_;
            }

            //- Neighbour
            const DynamicList<label>& faceNeighbour()const
            {
                return faceNeighbour_;
            }

            //- Is point removed?
            inline bool pointRemoved(const label pointI) const;

            //- Is face removed?
            inline bool faceRemoved(const label faceI) const;

            //- Is cell removed?
            inline bool cellRemoved(const label cellI) const;


        // Edit

            //- Clear all storage
            void clear();

            //- Add all points/faces/cells of mesh. Additional offset for patch
            //  or zone ids.
            void addMesh
            (
                const polyMesh&,
                const labelList& patchMap,
                const labelList& pointZoneMap,
                const labelList& faceZoneMap,
                const labelList& cellZoneMap
            );

            //- Move all points. Incompatible with other topology changes.
            void movePoints(const pointField& newPoints);

            //- For compatibility with directTopoChange: set topological action
            label setAction(const topoAction& action);

            //- Add point. Return new point label.
            //  Notes:
            //  - masterPointID can be < 0 (appended points)
            //  - inCell = false: add retired point (to end of point list)
            label addPoint
            (
                const point&,
                const label masterPointID,
                const label zoneID,
                const bool inCell
            );

            //- Modify coordinate.
            //  Notes:
            //  - inCell = false: add retired point (to end of point list)
            void modifyPoint
            (
                const label,
                const point&,
                const label newZoneID,
                const bool inCell
            );

            //- Remove/merge point.
            void removePoint(const label, const label);

            //- Add face to cells. Return new face label.
            //  own,nei<0, zoneID>=0 : add inactive face (to end of face list)
            label addFace
            (
                const face& f,
                const label own,
                const label nei,
                const label masterPointID,
                const label masterEdgeID,
                const label masterFaceID,
                const bool flipFaceFlux,
                const label patchID,
                const label zoneID,
                const bool zoneFlip
            );

            //- Modify vertices or cell of face.
            void modifyFace
            (
                const face& f,
                const label faceI,
                const label own,
                const label nei,
                const bool flipFaceFlux,
                const label patchID,
                const label zoneID,
                const bool zoneFlip
            );

            //- Remove/merge face.
            void removeFace(const label, const label);

            //- Add cell. Return new cell label.
            label addCell
            (
                const label masterPointID,
                const label masterEdgeID,
                const label masterFaceID,
                const label masterCellID,
                const label zoneID
            );

            //- Modify zone of cell
            void modifyCell(const label, const label zoneID);

            //- Remove/merge cell.
            void removeCell(const label, const label);

            //- Explicitly set the number of patches if construct-without-mesh
            //  used
            inline void setNumPatches(const label nPatches);


        // Other

            //- Inplace changes mesh without change of patches.
            //  Adapts patch start/end and by default does parallel matching.
            //  Clears all data. Returns map.
            //  inflate = true : keep old mesh points. Put new points into the
            //  returned map (preMotionPoints) so we can use inflation. Any
            //  points out of nothing (appended points) are vector::zero.
            //  inflate = false: set mesh points directly.
            //  Empty preMotionPoints in the map.
            //  orderCells :  whether to order the cells
            // (see bandCompression.H)
            //  orderPoints : whether to order the points into internal first
            //  followed by boundary points. This is not fully consistent
            //  with upper-triangular ordering of points and edges so
            //  is only done when explicitly asked for.
            autoPtr<mapPolyMesh> changeMesh
            (
                polyMesh& mesh,
                const bool inflate,
                const bool syncParallel = true,
                const bool orderCells = false,
                const bool orderPoints = false
            );

            //- Create new mesh with old mesh patches
            autoPtr<mapPolyMesh> makeMesh
            (
                autoPtr<polyMesh>& newMesh,
                const IOobject& io,
                const polyMesh& mesh,
                const bool syncParallel = true,
                const bool orderCells = false,
                const bool orderPoints = false
            );

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "directTopoChangeI.H"

#ifdef NoRepository
#   include "directTopoChangeTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
