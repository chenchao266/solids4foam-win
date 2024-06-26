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

\*---------------------------------------------------------------------------*/

#include "amiZoneInterpolation.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::amiZoneInterpolation::interpolateToTargetPointsTwoStep
(
    const Field<Type>& fld,
    const UList<Type>& defaultValues
) const
{
    // Check size of the field
    if (fld.size() != sourcePatch_.nPoints())
    {
        FatalErrorIn(type() + "::interpolateToTargetPoints(...)")
            << "The field size is wrong!" << nl
            << "nPoints on patch = " << sourcePatch_.nPoints() << nl
            << "nPoints in field = " << fld.size() << abort(FatalError);
    }

    tmp<Field<Type>> tresult
    (
        new Field<Type>(sourcePatch_.nPoints(), pTraits<Type>::zero_)
    );

    // Interpolate point field to face field
    const Field<Type> sourceFaceFld =
        sourcePatchInterp_.pointToFaceInterpolate<Type>(fld);

    // Interpolate source faces to target faces
    const Field<Type> targetFaceFld = interpolateToTarget(sourceFaceFld);

    // Interpolate face field to point field
    tresult.ref() =
        targetPatchInterp_.faceToPointInterpolate<Type>(targetFaceFld);

    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::amiZoneInterpolation::interpolateToSourcePointsTwoStep
(
    const Field<Type>& fld,
    const UList<Type>& defaultValues
) const
{
    // Check size of the field
    if (fld.size() != targetPatch_.nPoints())
    {
        FatalErrorIn(type() + "::interpolateToSourcePoints(...)")
            << "The field size is wrong!" << nl
            << "nPoints on patch = " << targetPatch_.nPoints() << nl
            << "nPoints in field = " << fld.size() << abort(FatalError);
    }

    tmp<Field<Type>> tresult
    (
        new Field<Type>(targetPatch_.nPoints(), pTraits<Type>::zero_)
    );

    // Interpolate point field to face field
    const Field<Type> targetFaceFld =
        targetPatchInterp_.pointToFaceInterpolate<Type>(fld);

    // Interpolate target faces to source faces
    const Field<Type> sourceFaceFld = interpolateToSource(targetFaceFld);

    // Interpolate face field to point field
    tresult.ref() =
        sourcePatchInterp_.faceToPointInterpolate<Type>(sourceFaceFld);

    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::amiZoneInterpolation::interpolateToSourcePoints
(
    const Field<Type>& pf
) const
{
    if (pf.size() != targetPatch_.nPoints())
    {
        FatalErrorIn
        (
            "amiZoneInterpolation::interpolateToSourcePoints"
            "(const Field<Type> pf)"
        )   << "given field does not correspond to patch. Patch size: "
            << targetPatch_.nPoints() << " field size: " << pf.size()
            << abort(FatalError);
    }

    tmp<Field<Type> > tresult
    (
        new Field<Type>
        (
            sourcePatch_.nPoints(),
            pTraits<Type>::zero_
        )
    );

    // Escape the interpolation if there are no faces in the target patch
    if (sourcePatch_.nPoints() == 0)
    {
        return tresult;
    }

    Field<Type>& result = tresult.ref();

    //const List<typename TargetPatch::FaceType>& targetFaces =
    const List<face>& targetFaces =
        targetPatch_.localFaces();

    const List<labelPair>& addr = sourcePointAddr();

    const List< List<scalar> >& weights = sourcePointWeights();

    forAll (result, pointI)
    {
        if (addr[pointI].first() > -1)
        {
            const face& hitFace =
                targetFaces[addr[pointI].first()];

            label pI = addr[pointI].second();

            Type ctrF = average(Field<Type>(pf, hitFace));

            result[pointI] =
                weights[pointI][0]*pf[hitFace[pI]]
              + weights[pointI][1]*pf[hitFace.nextLabel(pI)]
              + weights[pointI][2]*ctrF;
        }
    }

    return tresult;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::amiZoneInterpolation::interpolateToTargetPoints
(
    const Field<Type>& pf
) const
{
    if (pf.size() != sourcePatch_.nPoints())
    {
        FatalErrorIn
        (
            "amiZoneInterpolation::interpolateToTargetPoints"
            "(const Field<Type> pf)"
        )   << "given field does not correspond to patch. Patch size: "
            << sourcePatch_.nPoints() << " field size: " << pf.size()
            << abort(FatalError);
    }

    tmp<Field<Type> > tresult
    (
        new Field<Type>
        (
            targetPatch_.nPoints(),
            pTraits<Type>::zero_
        )
    );

    // Escape the interpolation if there are no faces in the target patch
    if (targetPatch_.nPoints() == 0)
    {
        return tresult;
    }

    Field<Type>& result = tresult.ref();

    //const List<typename TargetPatch::FaceType>& sourceFaces =
    const List<face>& sourceFaces =
        sourcePatch_.localFaces();

    const List<labelPair>& addr = targetPointAddr();

    const List< List<scalar> >& weights = targetPointWeights();

    forAll (result, pointI)
    {
        if (addr[pointI].first() > -1)
        {
            const face& hitFace =
                sourceFaces[addr[pointI].first()];

            label pI = addr[pointI].second();

            Type ctrF = average(Field<Type>(pf, hitFace));

            result[pointI] =
                weights[pointI][0]*pf[hitFace[pI]]
              + weights[pointI][1]*pf[hitFace.nextLabel(pI)]
              + weights[pointI][2]*ctrF;
        }
    }

    return tresult;
}


// ************************************************************************* //
