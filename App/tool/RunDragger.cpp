/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/RunDragger.h"
#include "Tool/Dragger.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/MouseCommand.h"
#include "V8DataModel/Filters.h"
#include "V8World/Primitive.h"
#include "V8World/Contact.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "V8World/WeldJoint.h"
#include "V8World/MultiJoint.h"
#include "V8World/Tolerance.h"
#include "Util/Units.h"
#include "Util/Math.h"
#include "V8kernel/Body.h"
#include "V8World/Joint.h"
#include "V8DataModel/JointInstance.h"
#include "V8World/Geometry.h"

namespace RBX {

void RunDragger::SnapInfo::updateSurfaceFromHit()
{
	const CoordinateFrame& snapCoord = snap->getCoordinateFrame();
	Vector3 hitInSnap = snapCoord.pointToObjectSpace(hitWorld);
	//Extents snapExtents(snap->getExtentsLocal());

    if( !snap->getGeometry()->isTerrain() )
	{
        mySurfaceId = snap->getGeometry()->closestSurfaceToPoint(hitInSnap);
		if (mySurfaceId == (size_t)-1)
			snap = NULL;
	}
}


void RunDragger::SnapInfo::updateHitFromSurface(const RbxRay& mouseRay)
{
	//Extents snapExtents(snap->getExtentsLocal());
	const CoordinateFrame& snapCoord = snap->getCoordinateFrame();
	RbxRay rayInSnap = snapCoord.toObjectSpace(mouseRay);

    if( snap->getGeometry()->isTerrain() )
    {
        Vector3 snapHitPoint;
        int surfId;
		CoordinateFrame surfaceCoordInTerrain;
		if( snap->getGeometry()->hitTestTerrain(mouseRay, snapHitPoint, surfId, surfaceCoordInTerrain) )
        {
            mySurfaceId = (size_t)surfId;
            hitWorld = snapHitPoint;
        }
    }
    else
    {
	    Plane plane = snap->getGeometry()->getPlaneFromSurface(mySurfaceId);

	    Vector3 hitInSnap;
	    bool ok = Math::intersectRayPlane(rayInSnap, plane, hitInSnap);
	    if (ok) {
		    lastHitWorld = hitWorld;
		    hitWorld = snapCoord.pointToWorldSpace(hitInSnap);
	    }
	    else {
		    RBXASSERT(0);
		    ok = Math::intersectRayPlane(rayInSnap, plane, hitInSnap);
	    }
        // Debugging - record last good one
        static RbxRay lastRay = rayInSnap;
        static Plane lastPlane = plane;
   }

}

float RunDragger::SnapInfo::hitOutsideExtents()
{
	Extents snapExtents(snap->getExtentsLocal());
	const CoordinateFrame& snapCoord = snap->getCoordinateFrame();
	Vector3 hitInSnap = snapCoord.pointToObjectSpace(hitWorld);
	Vector3 clipInSnap = snapExtents.clip(hitInSnap);
	return (hitInSnap - clipInSnap).magnitude();
}

RunDragger::RunDragger()
	: workspace(NULL)
	, drag(NULL)
{}

RunDragger::~RunDragger()
{}

void RunDragger::snapInfoFromSnapPart()
{
	RBXASSERT(!dragPart.expired());
	RBXASSERT(Workspace::getWorldIfInWorkspace(dragPart.lock().get()) == workspace->getWorld());
	RBXASSERT(dragPart.lock()->getPartPrimitive() == drag);
	RBXASSERT(PartInstance::nonNullInWorkspace(dragPart.lock()));

	shared_ptr<PartInstance> p(snapPart.lock());
	if (PartInstance::nonNullInWorkspace(p)) {
		RBXASSERT(Workspace::getWorldIfInWorkspace(p.get()) == workspace->getWorld());
		snapInfo.snap = p->getPartPrimitive();
	}
	else {
		// snap part deleted while dragging probably multiplayer...
		snapInfo.snap = NULL;
	}
}

void RunDragger::snapPartFromSnapInfo()
{
	if (snapInfo.snap) {
		snapPart = shared_from(PartInstance::fromPrimitive(snapInfo.snap));
		RBXASSERT(Workspace::getWorldIfInWorkspace(snapPart.lock().get()) == workspace->getWorld());
	}
	else {
		snapPart.reset();
	}
}

void RunDragger::initLocal(	Workspace*				_workspace,
						    weak_ptr<PartInstance>	_dragPart,	
						    const Vector3&			_dragPointLocal)
{
	RBXASSERT(!_dragPart.expired());
	RBXASSERT(PartInstance::nonNullInWorkspace(_dragPart.lock()));
	workspace = _workspace;
	dragPart = _dragPart;

	shared_ptr<PartInstance> dp(dragPart.lock());

	drag = dp->getPartPrimitive();
	dragPointLocal = _dragPointLocal;
    dragOriginalRotation = drag->getCoordinateFrame().rotation;
	drag->setVelocity(Velocity::zero());
	snapInfo = SnapInfo();

	turnUpright(dp.get());

	RBXASSERT(Workspace::getWorldIfInWorkspace(dragPart.lock().get()) == workspace->getWorld());
}

void RunDragger::init(	Workspace*				_workspace,
						weak_ptr<PartInstance>	_dragPart,	
						const Vector3&			_dragPointWorld)
{
	RBXASSERT(!_dragPart.expired());
	RBXASSERT(PartInstance::nonNullInWorkspace(_dragPart.lock()));
	workspace = _workspace;
	dragPart = _dragPart;

	shared_ptr<PartInstance> dp(dragPart.lock());

	drag = dp->getPartPrimitive();
	dragPointLocal = drag->getCoordinateFrame().pointToObjectSpace(_dragPointWorld);
    dragOriginalRotation = drag->getCoordinateFrame().rotation;
	drag->setVelocity(Velocity::zero());
	snapInfo = SnapInfo();

	turnUpright(dp.get());

	RBXASSERT(Workspace::getWorldIfInWorkspace(dragPart.lock().get()) == workspace->getWorld());
}

/*
	Jumping to a new part because of a collision - 
	Go through all surfaces pointing towards the user, pick the one
	with a snapHitWorld farthest from the user
*/

RunDragger::SnapInfo RunDragger::createSnapSurface(Primitive* snap, G3D::Array<size_t> *ignore)
{
	RBXASSERT(snap);

	SnapInfo answer;
	answer.snap = snap;

	//Extents snapExtents(snap->getExtentsLocal());
	const CoordinateFrame& snapCoord = snap->getCoordinateFrame();	
	RbxRay rayInSnap = snapCoord.toObjectSpace(mouseRay);

    bool found = false;
	float bestDistance = 0.0;
    if( snap->getGeometry()->isTerrain() )
    {
        Vector3 snapHitPoint;
        int surfId;
		CoordinateFrame surfaceCoordInTerrain;

		if( snap->getGeometry()->hitTestTerrain(mouseRay, snapHitPoint, surfId, surfaceCoordInTerrain) )
        {
            answer.mySurfaceId = (size_t)surfId;
            answer.hitWorld = snapHitPoint;
            found = true;
        }
    }
    else
    {
	    // Angled surface snapping test...
	    int numSurf = snap->getGeometry()->getNumSurfaces();
	    for (int i = 0; i < numSurf; ++i) {
		    //NormalId n = static_cast<NormalId>(i);
		    size_t nS = static_cast<size_t>(i);
		    if (!ignore || (ignore->find(nS) == ignore->end())) {
			    Plane plane = snap->getGeometry()->getPlaneFromSurface(nS);
			    if (plane.halfSpaceContainsFinite(rayInSnap.origin())) {
				    Vector3 hitInSnap;
				    if (Math::intersectRayPlane(rayInSnap, plane, hitInSnap)) {
					    Vector3 hitInWorld = snapCoord.pointToWorldSpace(hitInSnap);
					    float distanceFromMouse = (mouseRay.origin() - hitInWorld).magnitude();
					    if (distanceFromMouse > bestDistance) {
						    bestDistance = distanceFromMouse;
						    answer.mySurfaceId = nS;
						    answer.hitWorld = hitInWorld;
						    found = true;
					    }
				    }
			    }
		    }
	    }
    }

	return found ? answer : SnapInfo();
}
	
bool RunDragger::moveDragPart()
{
    if( !snapInfo.snap || !drag || (!snapInfo.snap->getConstGeometry()->isTerrain() && (snapInfo.mySurfaceId == (size_t)-1)))
	    return false;

	// Snap object rotation matrix in world
#ifdef RBXASSERTENABLED
	const Matrix3& snapRotation = snapInfo.snap->getCoordinateFrame().rotation;
#endif

	// Face normal for the target face we are attempting to snap to
	CoordinateFrame snapSurfaceCoords = getSnapSurfaceCoord();
	Vector3 snapNormalWorld = snapSurfaceCoords.vectorToWorldSpace(Vector3::unitZ());

    CoordinateFrame originalDragCoord(dragOriginalRotation,drag->getCoordinateFrame().translation);
    drag->setCoordinateFrame(originalDragCoord);

	// Drag object rotation matrix in world
	Matrix3 dragInWorld = drag->getCoordinateFrame().rotation;
	RBXASSERT(Math::isOrthonormal(dragInWorld));
	RBXASSERT(Math::isOrthonormal(snapRotation));
	
	// The face from the drag object that we want to align with the snap face and its normal
	myDragSurfaceId = drag->getGeometry()->getMostAlignedSurface(-snapNormalWorld, dragInWorld);
	CoordinateFrame dragSurfaceCoords = drag->getCoordinateFrame() * drag->getGeometry()->getSurfaceCoordInBody(myDragSurfaceId);
	Vector3 dragNormalWorld = dragSurfaceCoords.vectorToWorldSpace(Vector3::unitZ());

	// Compute the rotation to apply to the drag object so that dragNormal = -snapNormal
	// First transform both normals to the drag body's coordinate system
	Vector3 dragNormalInDrag = drag->getCoordinateFrame().vectorToObjectSpace(dragNormalWorld);
	Vector3 snapNormalInDrag = drag->getCoordinateFrame().vectorToObjectSpace(snapNormalWorld);
	Matrix3 dragRotationToGoal = Math::fromVectorToVectorRotation(dragNormalInDrag, -snapNormalInDrag);

	// Update the drag surface rotation
	CoordinateFrame dragCoord(dragInWorld * dragRotationToGoal, drag->getCoordinateFrame().translation);
	drag->setCoordinateFrame(dragCoord);
	dragSurfaceCoords = dragCoord * drag->getGeometry()->getSurfaceCoordInBody(myDragSurfaceId);

	// dragRotationToGoal gets the surfaces of interest to have their normals aligned.  
	// Now find dragRotationToGoal2 that is the minimal rotation about the common normals that aligns any of their 
	// x or y axes.
	Vector3 snapFaceInWorldY = snapSurfaceCoords.vectorToWorldSpace(Vector3::unitY());
	Vector3 dragFaceInWorldY = dragSurfaceCoords.vectorToWorldSpace(Vector3::unitY());
	Vector3 rotAxisInDrag = dragCoord.vectorToObjectSpace(dragFaceInWorldY.cross(snapFaceInWorldY));

	Matrix3 dragRotationToGoal2 = Matrix3::identity();
	if( rotAxisInDrag.magnitude() > 1e-3 )
	{
		rotAxisInDrag /= rotAxisInDrag.magnitude();
		float rotAngle = acos(dragFaceInWorldY.dot(snapFaceInWorldY)/(dragFaceInWorldY.magnitude() * snapFaceInWorldY.magnitude()));
		const float quarterPi = 0.78539816;
		if( rotAngle > quarterPi && rotAngle < 3.0 * quarterPi )
			rotAngle -= 2.0f * quarterPi;
		else if( rotAngle >= 3.0f * quarterPi )
			rotAngle = rotAngle - 4.0f * quarterPi;
		if( fabs(rotAngle) < 1e-3 ) 
			rotAngle = 0.0f;

		dragRotationToGoal2 = Math::fromRotationAxisAndAngle(rotAxisInDrag, rotAngle);
	}
	

	Math::orthonormalizeIfNecessary(dragRotationToGoal2);
	dragCoord.rotation *= dragRotationToGoal2;
	drag->setCoordinateFrame(dragCoord);

	// Now for the translation part
	Vector3 dragHitWorld = dragCoord.pointToWorldSpace(dragPointLocal);
	RbxRay throughDragHit = RbxRay::fromOriginAndDirection(dragHitWorld, mouseRay.direction());
	RbxRay rayInDrag = dragCoord.toObjectSpace(throughDragHit);
	Extents dragExtentsLocal(drag->getExtentsLocal());

	Plane plane = drag->getGeometry()->getPlaneFromSurface(myDragSurfaceId);
#if _DEBUG
	bool hitPlane =
#endif
    Math::intersectLinePlane(	Line::fromPointAndDirection(rayInDrag.origin(), rayInDrag.direction()),
                             plane,
                             dragHitLocal);
    
	RBXASSERT_IF_VALIDATING(hitPlane);

	// clamp the place where we think the ray hits the drag part - for 
	// steep obtuse angles, keep this local hit point within 100x the size of the drag part
	//
	Vector3 clampedHitLocal = dragHitLocal.clamp(100*dragExtentsLocal.min(), 100*dragExtentsLocal.max());
	Vector3 oppositeHitWorld = dragCoord.pointToWorldSpace(clampedHitLocal);
	Vector3 hitToCenter = dragCoord.translation - oppositeHitWorld;
	dragCoord.translation = snapInfo.hitWorld + hitToCenter;

	// Apply rotation and translation to snap part
	dragPart.lock()->setCoordinateFrame(dragCoord);

	return snapDragPart();
}

bool RunDragger::snapDragPart()
{
	// These are coordinate systems for the specified surfaces.  The CSs are expressed in terms of the bodies, not world.
	// The surface centroid is the origin and the frame is aligned with the surface normal (for z).  The y axis of
	// this frame is the projection of either the body's y or z axis, depending on which one has a more predominant projection.

	CoordinateFrame snapSurfaceCoords = getSnapSurfaceCoord();
	CoordinateFrame dragSurfaceCoords = drag->getCoordinateFrame() * drag->getGeometry()->getSurfaceCoordInBody(myDragSurfaceId);

	//Vector3 snapOriginWorld = snapSurfaceCoords.translation;
	Vector3 dragOriginWorld = dragSurfaceCoords.translation;
	Vector3 dragOriginInSnapFaceCoord = snapSurfaceCoords.pointToObjectSpace(dragOriginWorld);

	Vector3 snapDragOriginInSnapFaceCoord = Math::iRoundVector3(dragOriginInSnapFaceCoord);
	Vector3 snapDragOriginInWorld = snapSurfaceCoords.pointToWorldSpace(snapDragOriginInSnapFaceCoord);
	Vector3 moveDrag = snapDragOriginInWorld - dragOriginWorld;
	CoordinateFrame newDragCoord = drag->getCoordinateFrame();
	newDragCoord.translation += moveDrag;

	dragPart.lock()->setCoordinateFrame(newDragCoord);

	if (snapDragOriginInSnapFaceCoord != snapInfo.lastDragSnap)
	{
		snapInfo.lastDragSnap = snapDragOriginInSnapFaceCoord;
		return true;
	}
	else 
	{
		return false;
	}
}

bool RunDragger::notTried(Primitive* check, const G3D::Array<Primitive*>& tried)
{

	RBXASSERT(check != drag);
	return (check && (tried.find(check) == tried.end()));
}

bool RunDragger::adjacent(Primitive* p0, Primitive* p1)
{
	Contact* c = p0->getFirstContact();

	while (c) {
		if (c->otherPrimitive(p0) == p1) {
			return c->computeIsAdjacentUi(Tolerance::maxOverlapOrGap());

		}
		c = p0->getNextContact(c);
	}
	return false;
}



RunDragger::SnapInfo RunDragger::rayHitsPart(const G3D::Array<Primitive*>& triedSnap, bool forceAdjacent)
{
	SnapInfo answer;

	PartByLocalCharacter filter(workspace);

	PartInstance* foundPart = MouseCommand::getMousePart(	mouseRay,
															*workspace->getWorld()->getContactManager(),
															drag,
															&filter,
															answer.hitWorld);
	if (foundPart) {
		answer.snap = foundPart->getPartPrimitive();
		if (notTried(answer.snap, triedSnap)) {
			if (!forceAdjacent || adjacent(answer.snap, drag)) {
				answer.updateSurfaceFromHit();
				if (answer.snap)
					return answer;
			}
		}
	}
	return SnapInfo();
}




RunDragger::SnapInfo RunDragger::bestProximatePart(
										const G3D::Array<Primitive*>& triedSnap, 
										Contact::ProximityTest proximityTest)
{
	SnapInfo answer;

	Contact* c = drag->getFirstContact();

	float bestDistance = FLT_MAX;
	while (c) {
		if ( (c->*proximityTest)(Tolerance::maxOverlapOrGap()) ) {
			Primitive* test = c->otherPrimitive(drag);
			if (notTried(test, triedSnap)) {
				// Angled surface testing...
				SnapInfo testInfo;
				testInfo = createSnapSurface(test);
				if (testInfo.snap) {
					float distance = testInfo.hitOutsideExtents();
					if (distance < bestDistance) {
						distance = bestDistance;
						answer = testInfo;
					}
				}
			}
		}
		c = drag->getNextContact(c);
	}
	return answer;
}

bool RunDragger::fallOffEdge()
{
    if( !snapInfo.snap->getGeometry()->isTerrain() )
	    return !Joint::FacesOverlapped(snapInfo.snap, snapInfo.mySurfaceId, drag, myDragSurfaceId);
    else
        return false;
}

bool RunDragger::fallOffPart(bool& snapped)
{
    if( snapInfo.snap->getGeometry()->isTerrain() )
        return false;

    G3D::Array<size_t> triedSurfaces;

	while ((snapInfo.snap != NULL) && fallOffEdge()) 
	{
		triedSurfaces.append(snapInfo.mySurfaceId);
		snapInfo = createSnapSurface(snapInfo.snap, &triedSurfaces);	// ignore this surface
		if (snapInfo.snap) {
			snapped = moveDragPart();
		}
	}
	return (snapInfo.snap == NULL);
}

bool RunDragger::colliding()
{
    RBXASSERT_VERY_FAST(!drag->hasAutoJoints());
    return workspace->getWorld()->getContactManager()->intersectingOthers(drag,Tolerance::maxOverlapOrGap());
}

bool RunDragger::rayHitsCloserPart()
{
	G3D::Array<Primitive*> ignoreNone;
	SnapInfo answer = rayHitsPart(ignoreNone, false);			// don't force adjacent
	if (answer.snap && (answer.snap != snapInfo.snap)) {
		float answerDistance = (answer.hitWorld - mouseRay.origin()).magnitude();
		float snapDistance = (snapInfo.hitWorld - mouseRay.origin()).magnitude();
		if (answerDistance < snapDistance) {
			return true;
		}
	}
	return false;
}	

bool RunDragger::tooCloseToCamera()
{
	Vector3 dragHitWorld = drag->getCoordinateFrame().pointToWorldSpace(dragPointLocal);
	
	float currentDistance = (dragHitWorld - mouseRay.origin()).magnitude();
	return (currentDistance < PartInstance::cameraTranslucentDistance());
}

RunDragger::SnapInfo RunDragger::findSnap(const G3D::Array<Primitive*>& triedSnap)
{
	SnapInfo answer;

	// 1. Ray hits an adjancent part
	answer = rayHitsPart(triedSnap, true);
	if (answer.snap) {
		return answer;
	}

	// 2. Get the best adjacent part, if any
	answer = bestProximatePart(triedSnap, &Contact::computeIsAdjacentUi);	
	if (answer.snap) {
		return answer;
	}

	// 3. Get the best colliding part, if any
	answer = bestProximatePart(triedSnap, &Contact::computeIsCollidingUi);		
	if (answer.snap) {
		return answer;
	}

	// 4. Ray hits any part
	answer = rayHitsPart(triedSnap, false);	
	if (answer.snap) {
		return answer;
	}

	return SnapInfo();
}

void RunDragger::findNoSnapPosition(const CoordinateFrame& original)
{
    dragPart.lock()->setCoordinateFrame(original);
    snapInfo.mySurfaceId = (size_t)-1;

    // If we haven't hit anything but the ground plane then raycast
    // on the plane and try to move the part to it using safeMovYDrop
    // which will search up if the part collides with another part.
    
    PartByLocalCharacter filter(workspace);
    Vector3 hitWorld;

    PartInstance* foundPart = MouseCommand::getMousePart(mouseRay,
                                                         *workspace->getWorld()->getContactManager(),
                                                         drag,
                                                         &filter,
                                                         hitWorld);
    if (!foundPart)
    {
        G3D::Array<Primitive*> primitives;
        primitives.push_back(drag);
        Vector3 currentLoc = PartInstance::fromPrimitive(drag)->getCoordinateFrame().translation;
        Vector3 hitPoint;
        if (Math::intersectRayPlane(MouseCommand::getSearchRay(mouseRay),Plane(Vector3::unitY(), Vector3::zero()), hitPoint))
        {
            PartArray empty;
            hitPoint = DragUtilities::hitObjectOrPlane(empty, mouseRay, *workspace->getWorld()->getContactManager());
            hitPoint = DragUtilities::toGrid(hitPoint);
            Vector3 delta = DragUtilities::toGrid(hitPoint - currentLoc);
            delta = delta - delta.dot(Vector3::unitY()) * Vector3::unitY();
            Dragger::safeMoveYDrop(primitives, delta, *workspace->getWorld()->getContactManager());
        }
    }
}

// Note: will NOT update dragOriginalRotation
void RunDragger::turnUpright(PartInstance* part)
{
	RBXASSERT(part);
	if (part->isStandardPart()) {
		CoordinateFrame dragCoord = part->getCoordinateFrame();

		// Angled surface ... (nothing done here yet)
		NormalId upInWorld = Math::getClosestObjectNormalId(Vector3::unitY(), dragCoord.rotation);

		if (upInWorld != NORM_Y) {
			dragCoord.rotation = Matrix3::identity();
			part->setCoordinateFrame(dragCoord);
		}
	}
}

// Note: will NOT update dragOriginalRotation
void RunDragger::rotatePart(PartInstance* part)
{
	RBXASSERT(part);

	CoordinateFrame rotated = part->getCoordinateFrame();
	Math::rotateMatrixAboutY90(rotated.rotation);
	part->setCoordinateFrame(rotated);
}

void RunDragger::rotatePart90DegAboutSnapFaceAxis( Vector3::Axis axis )
{
	rotatePartAboutSnapFaceAxis(axis, Math::piHalff());
}

void RunDragger::rotatePartAboutSnapFaceAxis( Vector3::Axis axis, const float& angleInRads )
{
	if( !snapInfo.snap || !drag )
		return;
	
	Vector3 rotAxis = Vector3::unitX();
	if( axis == Vector3::Y_AXIS )
		rotAxis = Vector3::unitY();
	else if( axis == Vector3::Z_AXIS )
		rotAxis = Vector3::unitZ();

	CoordinateFrame snapSurfaceCoordInWorld = getSnapSurfaceCoord();
	Vector3 snapFaceAxisInWorld = snapSurfaceCoordInWorld.vectorToWorldSpace(rotAxis);
	CoordinateFrame dragCoord = drag->getCoordinateFrame();
	Vector3 snapFaceAxisInDrag = dragCoord.vectorToObjectSpace(snapFaceAxisInWorld);

	Matrix3 dragRotation = Math::fromRotationAxisAndAngle(snapFaceAxisInDrag, angleInRads);
	Math::orthonormalizeIfNecessary(dragRotation);
	dragCoord.rotation *= dragRotation;
	drag->setCoordinateFrame(dragCoord);
    dragOriginalRotation = dragCoord.rotation;
}

// Note: will NOT update dragOriginalRotation
void RunDragger::tiltPart(PartInstance* part, const CoordinateFrame& camera)
{
	RBXASSERT(part);

	CoordinateFrame dragCoord = part->getCoordinateFrame();

	// Angled surface testing...
	NormalId normId = Math::getClosestObjectNormalId(camera.rightVector(), dragCoord.rotation);
	Vector3 axis = Math::getWorldNormal(normId, dragCoord);

	size_t surfId = part->getPartPrimitive()->getGeometry()->getMostAlignedSurface(camera.rightVector(), dragCoord.rotation);
	axis = dragCoord.vectorToWorldSpace(part->getPartPrimitive()->getGeometry()->getSurfaceNormalInBody(surfId));

	Matrix3 tiltM = Matrix3::fromAxisAngleFast(axis, Math::piHalff());
	dragCoord.rotation = tiltM * dragCoord.rotation;
	part->setCoordinateFrame(dragCoord);
}

// safely move the drag primitive up until it's not colliding with anything
void RunDragger::findSafeY()
{
	if (colliding()) {
		PartInstance* part = PartInstance::fromPrimitive(drag);
		G3D::Array<Primitive*> temp;
		temp.append(part->getPartPrimitive());

//		std::vector<PVInstance*> temp;
//		temp.push_back(part);

		Vector3 moved = Dragger::safeMoveNoDrop(temp, Vector3::zero(), *workspace->getWorld()->getContactManager());
		RBXASSERT(moved.x == 0.0);
		RBXASSERT(moved.y != 0.0);
		CoordinateFrame newC = drag->getCoordinateFrame();
		newC.translation += moved;
		dragPart.lock()->setCoordinateFrame(newC);
	}
}

bool RunDragger::snap(const RbxRay& _mouseRay)
{
	snapInfoFromSnapPart();

	mouseRay = _mouseRay;
	CoordinateFrame originalDrag = drag->getCoordinateFrame();

	G3D::Array<Primitive*> triedSnap;

	if (snapInfo.snap) {										// if last snap part is now too far, zero it
		snapInfo.updateHitFromSurface(mouseRay);	// update the hit point in world
	}
	else {
		snapInfo = findSnap(triedSnap);
	}


	int failure = 0;

	while (snapInfo.snap && (triedSnap.find(snapInfo.snap) == triedSnap.end())) 
	{
		triedSnap.append(snapInfo.snap);
		// Angled surface testing...
		bool snapped;
        snapped = moveDragPart();

		failure = 0;
		if (fallOffPart(snapped))		{failure = 1;}
		else if (colliding())			{failure = 2;}
		else if (rayHitsCloserPart())	{failure = 3;}
		else if (tooCloseToCamera())	{failure = 4;}

        if (failure > 0) {	
			snapInfo = findSnap(triedSnap);
		}
		else {									// ok - we're done
			snapPartFromSnapInfo();
			return snapped;
		}
	}

	findNoSnapPosition(originalDrag);		// nothing found - float the drag part
	snapPartFromSnapInfo();
	return false;
}

CoordinateFrame RunDragger::getSnapSurfaceCoord()
{
    if( !snapInfo.snap->getGeometry()->isTerrain() )
    {
	    return snapInfo.snap->getCoordinateFrame() * snapInfo.snap->getGeometry()->getSurfaceCoordInBody(snapInfo.mySurfaceId);
    }
    else
    {
        Vector3 snapHitPoint;
        int surfId;
		CoordinateFrame surfaceCoordInTerrain;
        RbxRay rayInSnap = mouseRay;
		if( snapInfo.snap->getGeometry()->hitTestTerrain(rayInSnap, snapHitPoint, surfId, surfaceCoordInTerrain) )
        {
            snapInfo.mySurfaceId = (size_t)surfId;
            snapInfo.hitWorld = snapHitPoint;
        }
        return surfaceCoordInTerrain;
    }
}

/*
	Mouse Down:  Get drag part details
		dragPart			// set by the mouse down
		dragPointLocal		// set by the down point
		headPart			// if exists, set a mouse down

	Mouse Move:
		
		Array< tried snap parts this step >
			append current part if any

		If snap part too far – set to NULL

		Existing Snap Part?
			NO - If no snap part, try to find one + snap surface
				snap
				snapSurface
				
				1.	Shoot a ray - find part.  If drag part adjacent, use it.

				2.	Check other adjacent parts - use point that is closest to the 
					surface on the other adjacent parts
					dragSurface
					dragInSnap

				3.  Check other colliding parts

				4.	If not adjacent, shoot a ray (if head – make sure close enough)

				5.	NULL – no snap part found

				Confirm the part is not too far away if head


		Found a Snap Part?

			Move dragPart() along snap part / snap surface

			Check Edges
				if (off the edge) {
					try other surfaces (this edge first)

				if (still off the edge) {
					find another snap (fell off this part)
					try again;

			Check Collisions
				if (colliding with a different part) {
					if not used,
						make this the snap part
						find snap surface on this part
					try again
				}
			


	If no snap part
		1. old position relative to old snap part? - use it ==  old position global? 

		3. float? - use it

		4. find safe position
*/

} // namespace
