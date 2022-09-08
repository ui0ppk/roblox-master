/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/AdvRunDragger.h"
#include "Tool/Dragger.h"
#include "Tool/ToolsArrow.h"
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
#include "V8World/Geometry.h"
#include "V8World/MegaClusterPoly.h"
#include "V8DataModel/JointInstance.h"
#include "GfxBase/IAdornableCollector.h"
#include "AppDraw/Draw.h"

namespace {

const float mouseRayMinDistance = 0.5f;

} // namespace

namespace RBX {

bool AdvRunDragger::dragMultiPartsAsSinglePart = false;

void AdvRunDragger::SnapInfo::updateSurfaceFromHit()
{
    if( snap )
    {
	    const CoordinateFrame& snapCoord = snap->getCoordinateFrame();
	    Vector3 hitInSnap = snapCoord.pointToObjectSpace(hitWorld);
	    Extents snapExtents(snap->getExtentsLocal());

	    surface = snapExtents.closestFace(hitInSnap); 

        if( !snap->getGeometry()->isTerrain() )
		{
            mySurfaceId = snap->getGeometry()->closestSurfaceToPoint(hitInSnap);
			if (mySurfaceId == (size_t)-1)
				snap = NULL;
		}
    }
}


void AdvRunDragger::SnapInfo::updateHitFromSurface(const RbxRay& mouseRay)
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
	    if (ok) 
        {
		    hitWorld = snapCoord.pointToWorldSpace(hitInSnap);
	    }
	    else 
        {
		    RBXASSERT(0);
		    ok = Math::intersectRayPlane(rayInSnap, plane, hitInSnap);
	    }

	    // Debugging - record last good one
	    static RbxRay lastRay = rayInSnap;
	    static Plane lastPlane = plane;
    }
}

float AdvRunDragger::SnapInfo::hitOutsideExtents()
{
	Extents snapExtents(snap->getExtentsLocal());
	const CoordinateFrame& snapCoord = snap->getCoordinateFrame();
	Vector3 hitInSnap = snapCoord.pointToObjectSpace(hitWorld);
	Vector3 clipInSnap = snapExtents.clip(hitInSnap);
	return (hitInSnap - clipInSnap).magnitude();
}

AdvRunDragger::AdvRunDragger()
	: isAdornable(false)
	, workspace(NULL)
	, drag(NULL)
	, gridMode(DRAG::ONE_STUD)
	, jointCreateMode(true)
	, snapGridOrigin(0.0f, 0.0f, 0.0f)
	, snapGridOriginNeedsUpdating(true)
	, primaryPart(NULL)
	, handleMultipleParts(false)
{}

AdvRunDragger::~AdvRunDragger()
{
	if( workspace && isAdornable)
		workspace->getAdornableCollector()->onRenderableDescendantRemoving(this);
}

void AdvRunDragger::snapInfoFromSnapPart()
{
#ifdef _DEBUG
	World* world = workspace->getWorld();
#endif
	RBXASSERT(!dragPart.expired());
#ifdef _DEBUG
	RBXASSERT(Workspace::getWorldIfInWorkspace(dragPart.lock().get()) == world);
#endif
	RBXASSERT(dragPart.lock()->getPartPrimitive() == drag);
	RBXASSERT(handleMultipleParts || PartInstance::nonNullInWorkspace(dragPart.lock()));

	shared_ptr<PartInstance> p(snapPart.lock());
	if (PartInstance::nonNullInWorkspace(p)) {
#ifdef _DEBUG
		RBXASSERT(Workspace::getWorldIfInWorkspace(p.get()) == world);
#endif
		snapInfo.snap = p->getPartPrimitive();
	}
	else {
		// snap part deleted while dragging probably multiplayer...
		snapInfo.snap = NULL;
	}
}

void AdvRunDragger::snapPartFromSnapInfo()
{
	if (snapInfo.snap) {
		snapPart = shared_from(PartInstance::fromPrimitive(snapInfo.snap));
		RBXASSERT(Workspace::getWorldIfInWorkspace(snapPart.lock().get()) == workspace->getWorld());
	}
	else {
		snapPart.reset();
	}
}

void AdvRunDragger::initLocal(	Workspace*				_workspace,
						    weak_ptr<PartInstance>	_dragPart,	
						    const Vector3&			_dragPointLocal,
							WeakParts				_dragParts)
{
	RBXASSERT(!_dragPart.expired());
	RBXASSERT(PartInstance::nonNullInWorkspace(_dragPart.lock()));
	workspace = _workspace;
	dragPart = _dragPart;

	handleMultipleParts = dragMultiPartsAsSinglePart && (_dragParts.size() > 1);

	// set up the drag parts container
#ifdef _DEBUG
	World* world = workspace->getWorld();
#endif
	weakDragParts = _dragParts;
	for( unsigned int i = 0; i < _dragParts.size(); i++ )
	{
		weak_ptr<PartInstance> partFromGroup = _dragParts[i];
		Primitive* dragPrim = NULL;

		RBXASSERT(!partFromGroup.expired());
#ifdef _DEBUG
		RBXASSERT(Workspace::getWorldIfInWorkspace(partFromGroup.lock().get()) == world);
#endif
		RBXASSERT(PartInstance::nonNullInWorkspace(partFromGroup.lock()));

		shared_ptr<PartInstance> p(partFromGroup.lock());
		if (PartInstance::nonNullInWorkspace(p))
		{
#ifdef _DEBUG
			RBXASSERT(Workspace::getWorldIfInWorkspace(p.get()) == world);
#endif
			dragPrim = p->getPartPrimitive();
			dragParts.push_back(dragPrim);
		}
	}

	primaryPart = NULL;
	if (handleMultipleParts)
	{
		Vector3 hitPointWorld(dragPart.lock()->getCoordinateFrame().pointToWorldSpace(_dragPointLocal));

		savePrimsForMultiDrag();

		ServiceClient< FilteredSelection<Instance> > instanceSelection(workspace);
		std::vector<Instance*> selection = instanceSelection->items();
		if (selection.size() == 1)
		{
			RBX::ModelInstance* mInstance = RBX::Instance::fastDynamicCast<RBX::ModelInstance>(selection[0]);
			if (mInstance)
				primaryPart = mInstance->getPrimaryPartSetByUser();
		}

		if (!primaryPart)
			primaryPart = dragPart.lock().get();

		CoordinateFrame primaryPartLocation = primaryPart->getCoordinateFrame();
		Extents ext = Dragger::computeExtentsRelative(savedPrimsForMultiDrag, primaryPartLocation);
		
		Vector3 center = Math::toGrid(primaryPartLocation.pointToWorldSpace(ext.center()), 0.001f);
		Vector3 size = Math::toGrid(ext.size(), 0.001f);
		// create temporary part
		tempPart = Creatable<Instance>::create<PartInstance>();
		// set location
		CoordinateFrame frame(primaryPartLocation.rotation, center);
		tempPart->setCoordinateFrame(frame);
		// set size
		tempPart->setPartSizeXml(size);
		// set temporary part as drag part
		dragPart = tempPart;

		// get drag point in temp part cordinate frame
		dragPointLocal = dragPart.lock()->getCoordinateFrame().pointToObjectSpace(hitPointWorld);
	}

	shared_ptr<PartInstance> dp(dragPart.lock());

	drag = dp->getPartPrimitive();
	if (!handleMultipleParts)
		dragPointLocal = _dragPointLocal;
	dragOriginalRotation = drag->getCoordinateFrame().rotation;
	drag->setVelocity(Velocity::zero());
	snapInfo = SnapInfo();

	//turnUpright(dp.get());

	RBXASSERT(handleMultipleParts || (Workspace::getWorldIfInWorkspace(dragPart.lock().get()) == workspace->getWorld()));

	// Add the run dragger instance to the collection of adornable objects.
	workspace->getAdornableCollector()->onRenderableDescendantAdded(this);
	isAdornable = true;
}

void AdvRunDragger::init(	Workspace*				_workspace,
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
	drag->setVelocity(Velocity::zero());
	snapInfo = SnapInfo();

	//turnUpright(dp.get());

	RBXASSERT(Workspace::getWorldIfInWorkspace(dragPart.lock().get()) == workspace->getWorld());
}

/*
	Jumping to a new part because of a collision - 
	Go through all surfaces pointing towards the user, pick the one
	with a snapHitWorld farthest from the user
*/
AdvRunDragger::SnapInfo AdvRunDragger::createSnapSurface(Primitive* snap, G3D::Array<size_t> *ignore)
{
	RBXASSERT(snap);

	SnapInfo answer;
	answer.snap = snap;

	//Extents snapExtents(snap->getExtentsLocal());
	const CoordinateFrame& snapCoord = snap->getCoordinateFrame();	
	RbxRay rayInSnap = snapCoord.toObjectSpace(mouseRay);

    bool found = false;

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
	    float bestDistance = 0.0;

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
	
bool AdvRunDragger::moveDragPart()
{
    if( !snapInfo.snap || !drag || (!snapInfo.snap->getConstGeometry()->isTerrain() && (snapInfo.mySurfaceId == (size_t)-1)))
		return false;

	// Snap object rotation matrix in world
	const Matrix3& snapRotation = snapInfo.snap->getCoordinateFrame().rotation;

	// Face normal for the target face we are attempting to snap to
    CoordinateFrame snapSurfaceCoords = getSnapSurfaceCoord();

	Vector3 snapNormalWorld = snapSurfaceCoords.vectorToWorldSpace(Vector3::unitZ());

	// Drag object rotation matrix in world

	//CoordinateFrame cf1(drag->getCoordinateFrame());
	tempOriginalCF = drag->getCoordinateFrame();

    // TODO - fix rotation to original when hitting "R" and "T"
	CoordinateFrame originalDragCoord(dragOriginalRotation,drag->getCoordinateFrame().translation);
	drag->setCoordinateFrame(originalDragCoord);

	Matrix3 dragInWorld = drag->getCoordinateFrame().rotation;
	RBXASSERT(Math::isOrthonormal(dragInWorld));
	RBXASSERT(Math::isOrthonormal(snapRotation));
	
	bool draggingOnSlope = false;
	// The face from the drag object that we want to align with the snap face and its normal
	if (snapInfo.snap && snapInfo.snap->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
	{
		// do not change surface if we've a minimal change in alignment
		float ang = fabs(Math::angle(-snapNormalWorld, originalDragCoord.vectorToWorldSpace(Vector3::unitZ())) - Math::piHalff());
		if (ang < 0.002 || (fabs(Math::piHalff() - ang) < 0.002))
		{
			myDragSurfaceId = drag->getGeometry()->getMostAlignedSurface(-snapNormalWorld, dragInWorld);
		}
		else
		{
			myDragSurfaceId = drag->getGeometry()->getMostAlignedSurface(-originalDragCoord.rotation.column(1), dragInWorld);
			draggingOnSlope = true;
		}
	}
	else
	{
		myDragSurfaceId = drag->getGeometry()->getMostAlignedSurface(-snapNormalWorld, dragInWorld);
	}

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

	if (!draggingOnSlope)
	{
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
	}

	// Now for the translation part
	Vector3 dragHitWorld = dragCoord.pointToWorldSpace(dragPointLocal);
	RbxRay throughDragHit = RbxRay::fromOriginAndDirection(dragHitWorld, mouseRay.direction());
	RbxRay rayInDrag = dragCoord.toObjectSpace(throughDragHit);

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
	Vector3 clampedHitLocal = dragHitLocal;
	Vector3 oppositeHitWorld = dragCoord.pointToWorldSpace(clampedHitLocal);
	Vector3 hitToCenter = dragCoord.translation - oppositeHitWorld;
    dragCoord.translation = snapInfo.hitWorld + hitToCenter;

	// Apply rotation and translation to snap part
	dragPart.lock()->setCoordinateFrame(dragCoord);

	// if we are dragging on slope then supress grid settings
	bool result = snapDragPart(draggingOnSlope);

	if (snapInfo.snap && snapInfo.snap->getGeometryType() == Geometry::GEOMETRY_SMOOTHCLUSTER)
	{	
		pushDragPart(snapNormalWorld);
	}

	return result;
}

bool AdvRunDragger::snapDragPart(bool supressGridSettings)
{
	// These are coordinate systems for the specified surfaces.  The CSs are expressed in terms of the bodies, not world.
	// The surface centroid is the origin and the frame is aligned with the surface normal (for z).  The y axis of
	// this frame is the projection of either the body's y or z axis, depending on which one has a more predominant projection.

	// Face normal for the target face we are attempting to snap to
    CoordinateFrame snapSurfaceCoords = getSnapSurfaceCoord();
	CoordinateFrame dragSurfaceCoords = drag->getCoordinateFrame() * drag->getGeometry()->getSurfaceCoordInBody(myDragSurfaceId);

	//Vector3 snapOriginWorld = snapSurfaceCoords.translation;
	Vector3 dragOriginWorld = dragSurfaceCoords.translation;
	Vector3 dragOriginInSnapFaceCoord = snapSurfaceCoords.pointToObjectSpace(dragOriginWorld);

	// set this vector based on the grid mode setting
	Vector3 snapDragOriginInSnapFaceCoord = Math::iRoundVector3(dragOriginInSnapFaceCoord);
	if( AdvArrowTool::advGridMode == DRAG::OFF || supressGridSettings)
		snapDragOriginInSnapFaceCoord = dragOriginInSnapFaceCoord;
	else if( AdvArrowTool::advGridMode == DRAG::QUARTER_STUD )
	{
		snapDragOriginInSnapFaceCoord = Math::iRoundVector3(dragOriginInSnapFaceCoord * 5.0f);
		snapDragOriginInSnapFaceCoord *= 0.2f;
	}

	Vector3 snapDragOriginInWorld = snapSurfaceCoords.pointToWorldSpace(snapDragOriginInSnapFaceCoord);
	Vector3 moveDrag = snapDragOriginInWorld - dragOriginWorld;
	CoordinateFrame newDragCoord = drag->getCoordinateFrame();
	newDragCoord.translation += moveDrag;

    dragPart.lock()->setCoordinateFrame(newDragCoord);
	if (handleMultipleParts)
		DragUtilities::move(weakDragParts, tempOriginalCF, newDragCoord);

	// update the snap surface origin if necessary
	dragSurfaceCoords = drag->getCoordinateFrame() * drag->getGeometry()->getSurfaceCoordInBody(myDragSurfaceId);
	if( (dragSurfaceCoords.translation - snapGridOrigin).magnitude() > 2.0f * drag->getGeometry()->getRadius() )
		snapGridOriginNeedsUpdating = true;
	if( snapGridOriginNeedsUpdating )
	{
		snapGridOrigin = dragSurfaceCoords.translation;
		snapGridOriginNeedsUpdating = false;
	}

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

bool AdvRunDragger::pushDragPart(const Vector3& snapNormal)
{
	if (!colliding())
		return true;
	
	CoordinateFrame original = drag->getCoordinateFrame();

	int steps = 10;
	float maxDistance = 0.5f;

	for (int i = 0; i < steps; ++i)
	{
		CoordinateFrame cf = original;
		cf.translation += ((i + 1) / float(steps) * maxDistance) * snapNormal;

		drag->setCoordinateFrame(cf);

		if (!colliding())
			return true;
	}

	drag->setCoordinateFrame(original);
	return false;
}

bool AdvRunDragger::notTried(Primitive* check, const G3D::Array<Primitive*>& tried)
{

	RBXASSERT(check != drag);
	return (check && (tried.find(check) == tried.end()));
}

bool AdvRunDragger::adjacent(Primitive* p0, Primitive* p1)
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



AdvRunDragger::SnapInfo AdvRunDragger::rayHitsPart(const G3D::Array<Primitive*>& triedSnap, bool forceAdjacent)
{
	SnapInfo answer;

	PartByLocalCharacter filter(workspace);
	PartInstance* foundPart = NULL;

	if (handleMultipleParts)
	{
		std::vector<const Primitive*> ignorePrims(savedPrimsForMultiDrag.begin(), savedPrimsForMultiDrag.end());
		foundPart = MouseCommand::getMousePart(	mouseRay,
												*workspace->getWorld()->getContactManager(),
												ignorePrims,
												&filter,
												answer.hitWorld);
	}
	else
	{
		foundPart = MouseCommand::getMousePart(	mouseRay,
												*workspace->getWorld()->getContactManager(),
												drag,
												&filter,
												answer.hitWorld);
	}

	if (foundPart) {
		answer.snap = foundPart->getPartPrimitive();
		if (notTried(answer.snap, triedSnap)) {
			Primitive* dragPrim = handleMultipleParts && primaryPart ? primaryPart->getPartPrimitive() : drag;
            if (!forceAdjacent || adjacent(answer.snap, dragPrim)) {
				answer.updateSurfaceFromHit();
				if (answer.snap)
					return answer;
			}
		}
	}
	return SnapInfo();
}




AdvRunDragger::SnapInfo AdvRunDragger::bestProximatePart(
										const G3D::Array<Primitive*>& triedSnap, 
										Contact::ProximityTest proximityTest)
{
	SnapInfo answer;

	Primitive* dragPrim = handleMultipleParts ? NULL : drag;
	Contact* c = handleMultipleParts ? getFirstContact(dragPrim) : dragPrim->getFirstContact();

	float bestDistance = FLT_MAX;
	while (c) {
		if ( (c->*proximityTest)(Tolerance::maxOverlapOrGap()) ) {
			Primitive* test = c->otherPrimitive(dragPrim);
			if (notTried(test, triedSnap)) {
				SnapInfo testInfo = createSnapSurface(test);
				if (testInfo.snap) {
					float distance = testInfo.hitOutsideExtents();
					if (distance < bestDistance) {
						distance = bestDistance;
						answer = testInfo;
					}
				}
			}
		}
		c = handleMultipleParts ? getNextContact(dragPrim, c) : drag->getNextContact(c);
	}
	return answer;
}

bool AdvRunDragger::fallOffEdge()
{
	// the dragger performs much more reliably if we provide a tolerance for face overlap.
	// passing 1.1 into the function below adds a 10% buffer to the face dimensions
    if( !snapInfo.snap->getGeometry()->isTerrain() )
	    return !Joint::FacesOverlapped(snapInfo.snap, snapInfo.mySurfaceId, drag, myDragSurfaceId, 1.1f);
    else
        return false;
}

bool AdvRunDragger::fallOffPart(bool& snapped)
{
    if( snapInfo.snap->getGeometry()->isTerrain() )
        return false;

	G3D::Array<size_t> triedSurfaces;

	while ((snapInfo.snap != NULL) && fallOffEdge()) 
	{
		triedSurfaces.append(snapInfo.mySurfaceId);
		snapInfo = createSnapSurface(snapInfo.snap, &triedSurfaces);	// ignore this surface
		if (snapInfo.snap) 
        {
			snapped = moveDragPart();
		}
	}
	return (snapInfo.snap == NULL);
}


bool AdvRunDragger::colliding()
{
    RBXASSERT_VERY_FAST(!drag->hasAutoJoints());
	if (handleMultipleParts)
	{
		// TODO: Fix this to get collision detection working using extents only		
		Extents boundingBoxExtents(drag->getExtentsWorld());
		boundingBoxExtents.expand(-2*Tolerance::maxOverlapOrGap());

		G3D::Array<Primitive*> foundPrims;
		boost::unordered_set<const Primitive*> ignorePrims(savedPrimsForMultiDrag.begin(), savedPrimsForMultiDrag.end());
		workspace->getWorld()->getContactManager()->getPrimitivesTouchingExtents(boundingBoxExtents, ignorePrims, 1, foundPrims);
		if (foundPrims.size() > 0)
		{
			if (workspace->getWorld()->getContactManager()->intersectingOthers(savedPrimsForMultiDrag, Tolerance::maxOverlapOrGap()))
				return true;
		}

		Region3 correctedRegion(boundingBoxExtents.min(), boundingBoxExtents.max());
		if (workspace->getWorld()->getContactManager()->terrainCellsInRegion3(correctedRegion))
		{
			if (workspace->getWorld()->getContactManager()->intersectingOthers(savedPrimsForMultiDrag, Tolerance::maxOverlapOrGap()))
				return true;
		}

		return false;
	}
	return workspace->getWorld()->getContactManager()->intersectingOthers(drag,Tolerance::maxOverlapOrGap());
}

bool AdvRunDragger::rayHitsCloserPart()
{
	G3D::Array<Primitive*> ignoreNone;
	SnapInfo answer = rayHitsPart(handleMultipleParts ? savedPrimsForMultiDrag : ignoreNone, false);			// don't force adjacent
	if (answer.snap && (answer.snap != snapInfo.snap)) {
		float answerDistance = (answer.hitWorld - mouseRay.origin()).magnitude();
		float snapDistance = (snapInfo.hitWorld - mouseRay.origin()).magnitude();
		if (answerDistance < snapDistance) {
			return true;
		}
	}
	return false;
}	


bool AdvRunDragger::tooCloseToCamera()
{
	Vector3 dragHitWorld = drag->getCoordinateFrame().pointToWorldSpace(dragPointLocal);
	
	float currentDistance = (dragHitWorld - mouseRay.origin()).magnitude();
    return (currentDistance < mouseRayMinDistance);
}

AdvRunDragger::SnapInfo AdvRunDragger::findSnap(const G3D::Array<Primitive*>& triedSnap)
{
	SnapInfo answer;

	// 1. Ray hits an adjancent part
	answer = rayHitsPart(triedSnap, true);
	if (answer.snap) {
		return answer;
	}

	// 2. Get the best adjacent part, if any
	answer = bestProximatePart(triedSnap, &Contact::computeIsAdjacentUi);	
	if (answer.snap)
    {
		return answer;
	}

	// 3. Get the best colliding part, if any
	answer = bestProximatePart(triedSnap, &Contact::computeIsCollidingUi);		
	if (answer.snap) 
    {
		return answer;
	}

	// 4. Ray hits any part
	answer = rayHitsPart(triedSnap, false);
	if (answer.snap) 
    {
		return answer;
	}

	return SnapInfo();
}

void AdvRunDragger::findNoSnapPosition(const CoordinateFrame& original)
{
    dragPart.lock()->setCoordinateFrame(original);
    snapInfo.mySurfaceId = (size_t)-1;

	if (handleMultipleParts)
	{
		//move parts to the original location
		for( unsigned int i = 0; i < weakDragParts.size(); i++ )
			weakDragParts[i].lock()->setCoordinateFrame(originalLocations[i]);
	}

    // If we haven't hit anything but the ground plane then raycast
    // on the plane and try to move the part to it using safeMovYDrop
    // which will search up if the part collides with another part.
    
    PartByLocalCharacter filter(workspace);
    Vector3 hitWorld;

    std::vector<const Primitive *> ignorePrims;
	handleMultipleParts ? ignorePrims.assign(savedPrimsForMultiDrag.begin(), savedPrimsForMultiDrag.end()) : ignorePrims.push_back(drag);

    PartInstance* foundPart = MouseCommand::getMousePart(mouseRay,
                                                            *workspace->getWorld()->getContactManager(),
                                                            ignorePrims,
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
            if (handleMultipleParts)
			{
				currentLoc = PartInstance::fromPrimitive(drag)->getCoordinateFrame().pointToWorldSpace(dragPointLocal);
				hitPoint = DragUtilities::hitObjectOrPlane(weakDragParts, mouseRay, *workspace->getWorld()->getContactManager());
			}
			else
			{
				PartArray ignoreParts;
				ignoreParts.push_back(shared_from(PartInstance::fromPrimitive(drag)));
				hitPoint = DragUtilities::hitObjectOrPlane(ignoreParts, mouseRay, *workspace->getWorld()->getContactManager());
			}

            hitPoint = DragUtilities::toGrid(hitPoint);
            Vector3 delta = DragUtilities::toGrid(hitPoint - currentLoc);
            delta = delta - delta.dot(Vector3::unitY()) * Vector3::unitY();

            if (handleMultipleParts)
			{
				RBXASSERT(primaryPart);
				RBX::CoordinateFrame cFrameBefore = primaryPart->getCoordinateFrame();
				G3D::Array<Primitive*> primsToMove(savedPrimsForMultiDrag);				
				Dragger::safeMoveYDrop(primsToMove, delta, *workspace->getWorld()->getContactManager());
				// update temporary part position
				PartArray tempPartArray;
				tempPartArray.push_back(dragPart);
				DragUtilities::move(tempPartArray, cFrameBefore, primaryPart->getCoordinateFrame());
			}
			else
			{
				Dragger::safeMoveYDrop(primitives, delta, *workspace->getWorld()->getContactManager());
			}
        }
    }
}

// Note: will NOT update dragOriginalRotation
void AdvRunDragger::turnUpright(PartInstance* part)
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
void AdvRunDragger::rotatePart(PartInstance* part)
{
	RBXASSERT(part);

	CoordinateFrame rotated = part->getCoordinateFrame();
	Math::rotateMatrixAboutY90(rotated.rotation);
	part->setCoordinateFrame(rotated);
}

void AdvRunDragger::rotatePart90DegAboutSnapFaceAxis( Vector3::Axis axis )
{
	rotatePartAboutSnapFaceAxis(axis, Math::piHalff());
}

void AdvRunDragger::rotatePartAboutSnapFaceAxis( Vector3::Axis axis, const float& angleInRads )
{
	if( !snapInfo.snap || !drag )
		return;
	
	Vector3 rotAxis = Vector3::unitX();
	if( axis == Vector3::Y_AXIS )
		rotAxis = Vector3::unitY();
	else if( axis == Vector3::Z_AXIS )
		rotAxis = Vector3::unitZ();

	CoordinateFrame snapSurfaceCoordInWorld = getSnapSurfaceCoord();
    //snapInfo.snap->getCoordinateFrame() * snapInfo.snap->getGeometry()->getSurfaceCoordInBody(snapInfo.mySurfaceId);
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
void AdvRunDragger::tiltPart(PartInstance* part, const CoordinateFrame& camera)
{
	RBXASSERT(part);

	CoordinateFrame dragCoord = part->getCoordinateFrame();

	// Angled surface testing...
	NormalId normId = Math::getClosestObjectNormalId(camera.rightVector(), dragCoord.rotation);
	Vector3 axis = Math::getWorldNormal(normId, dragCoord);

	RBXASSERT(0); // test: are we even hitting this?
	size_t surfId = part->getPartPrimitive()->getGeometry()->getMostAlignedSurface(camera.rightVector(), dragCoord.rotation);
	axis = dragCoord.vectorToWorldSpace(part->getPartPrimitive()->getGeometry()->getSurfaceNormalInBody(surfId));

	Matrix3 tiltM = Matrix3::fromAxisAngleFast(axis, Math::piHalff());
	dragCoord.rotation = tiltM * dragCoord.rotation;
	part->setCoordinateFrame(dragCoord);
}

// safely move the drag primitive up until it's not colliding with anything
void AdvRunDragger::findSafeY()
{
	if (colliding()) 
    {
		PartInstance* part = PartInstance::fromPrimitive(drag);
		G3D::Array<Primitive*> temp;
		temp.append(part->getPartPrimitive());

		Vector3 moved = Dragger::safeMoveNoDrop(temp, Vector3::zero(), *workspace->getWorld()->getContactManager());
		RBXASSERT(moved.x == 0.0);
		RBXASSERT(moved.y != 0.0);

		CoordinateFrame newC = drag->getCoordinateFrame();
		newC.translation += moved;
		dragPart.lock()->setCoordinateFrame(newC);
	}
	else
	{
		G3D::Array<Primitive*> primitives;
		primitives.push_back(drag);
		Vector3 currentLoc = PartInstance::fromPrimitive(drag)->getCoordinateFrame().translation;
		Vector3 hitPoint;
		if (Math::intersectRayPlane(MouseCommand::getSearchRay(mouseRay),Plane(Vector3::unitY(), Vector3::zero()), hitPoint))
		{
			PartArray empty;
			hitPoint = DragUtilities::hitObjectOrPlane(	empty, mouseRay, *workspace->getWorld()->getContactManager());
			hitPoint = DragUtilities::toGrid(hitPoint);
			Vector3 delta = DragUtilities::toGrid(hitPoint - currentLoc);
			delta = delta - delta.dot(Vector3::unitY()) * Vector3::unitY();
			Dragger::safeMoveYDrop(primitives, delta, *workspace->getWorld()->getContactManager());
		}
	}
}

bool AdvRunDragger::snap(const RbxRay& _mouseRay)
{
	snapInfoFromSnapPart();
	originalLocations.clear();

	mouseRay = _mouseRay;
	CoordinateFrame originalDrag = drag->getCoordinateFrame();
	// save original locations
	for( unsigned int i = 0; i < weakDragParts.size(); i++ )
		originalLocations.push_back(weakDragParts[i].lock()->getCoordinateFrame());

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
		
		bool snapped = moveDragPart();

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

bool AdvRunDragger::snapGroup(const RbxRay& _mouseRay)
{
	mouseRay = _mouseRay;

    SnapInfo answer;
    PartByLocalCharacter filter(workspace);
    std::vector<const Primitive*> excludeParts;
    for( unsigned int i = 0; i < dragParts.size(); i++ )
		excludeParts.push_back(dragParts[i]);

    PartInstance* foundPart = MouseCommand::getMousePart(mouseRay, *workspace->getWorld()->getContactManager(), excludeParts, &filter, answer.hitWorld);

    if (foundPart)
        answer.snap = foundPart->getPartPrimitive();

    answer.updateSurfaceFromHit();

    snapInfo = answer;

	Vector3 hitSnapped;
    float hitPointHeight = 0.0f;
	if (getSnapHitPoint(dragPart.lock().get(), _mouseRay, hitSnapped)) {
		Vector3 currentLoc = dragPart.lock()->getCoordinateFrame().translation;
		Vector3 currentBottom = currentLoc - Vector3(0.0f, hitPointHeight, 0.0f);
		Vector3 delta = hitSnapped - currentBottom;
		delta = DragUtilities::toGrid(delta);

		if (delta != Vector3::zero()) {
			DragUtilities::safeMoveYDrop(weakDragParts, delta, *workspace->getWorld()->getContactManager());
			Vector3 newLoc = dragPart.lock()->getCoordinateFrame().translation;
			float requestedUpMovement = delta.y;
			float actualUpMovement = newLoc.y - currentLoc.y;
			float error = actualUpMovement - requestedUpMovement;

            Vector3 dragPosInSnapSurface = getSnapSurfaceCoord().pointToObjectSpace(dragPart.lock()->getCoordinateFrame().translation);
            Vector3 snapGridOriginTemp = dragPart.lock()->getCoordinateFrame().translation
                - getSnapSurfaceCoord().vectorToWorldSpace(Vector3(0.0f, 0.0f, dragPosInSnapSurface.z));
            
            snapGridOrigin = snapGridOriginTemp;
            snapGridOriginTemp = DragUtilities::toGrid(snapGridOriginTemp, Vector3(5.0f, 5.0f, 5.0f));
            snapGridOrigin.x = snapGridOriginTemp.x;
            snapGridOrigin.z = snapGridOriginTemp.z;

			if (error != 0.0f) {
				hitPointHeight += error;
			}
		}
	}

    return true;
}

bool AdvRunDragger::getSnapHitPoint(PartInstance* part, const RbxRay& unitMouseRay, Vector3& hitPoint)
{
	if (Math::intersectRayPlane(MouseCommand::getSearchRay(unitMouseRay),
								Plane(Vector3::unitY(), Vector3::zero()), 
								hitPoint)) {
		hitPoint = DragUtilities::hitObjectOrPlane(	weakDragParts, 
													unitMouseRay, 
													*workspace->getWorld()->getContactManager(),
                                                    false);

		return true;
	}
	else {
		return false;
	}
}

/* Disable until we have a means in Studio to toggle the grid
void AdvRunDragger::render3dAdorn(Adorn* adorn)
{
	if( !snapInfo.snap || !drag )
		return;

    CoordinateFrame snapSurfaceCoords = getSnapSurfaceCoord();
    float snapSurfaceZDotWithWorldY = snapSurfaceCoords.vectorToWorldSpace(Vector3::unitZ()).dot(Vector3::unitY());
    bool snapSurfaceHorizontalMeasure = fabs(snapSurfaceZDotWithWorldY - 1.0f) > 0.001;
    if( dragParts.size() > 1 && snapSurfaceHorizontalMeasure )
        return;

	int boxesPerStud = 0;
	if( AdvArrowTool::advGridMode == DRAG::ONE_STUD )
		boxesPerStud = 1;
	else if( AdvArrowTool::advGridMode == DRAG::QUARTER_STUD )
		boxesPerStud = 5;

	G3D::Color3	color = Color3::gray();
    snapSurfaceCoords.translation = snapGridOrigin;

	float effectiveRadius = 2.0f * drag->getGeometry()->getRadius();
	Vector4 bounds(effectiveRadius, effectiveRadius, -effectiveRadius, -effectiveRadius);
	DrawAdorn::surfaceGridAtCoord(adorn, snapSurfaceCoords, bounds, Vector3::unitX(), Vector3::unitY(), color, boxesPerStud);
}
*/

CoordinateFrame AdvRunDragger::getSnapSurfaceCoord()
{
    if( !snapInfo.snap )
    {
        return CoordinateFrame();
    }
  
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

void AdvRunDragger::savePrimsForMultiDrag()
{
	// clear previous values if any
	savedPrimsForMultiDrag.clear();
	// get primitives
	DragUtilities::partsToPrimitives(weakDragParts, savedPrimsForMultiDrag);
	// sort primitives in Y order
	Primitive* tempPrim = NULL;
	for (int ii=0; ii < (int)savedPrimsForMultiDrag.size(); ++ii)
	{
		for (int jj=0; jj < ii; ++jj)
		{
			if (savedPrimsForMultiDrag[jj]->getFastFuzzyExtents().min().y > savedPrimsForMultiDrag[jj+1]->getFastFuzzyExtents().min().y)
			{
				tempPrim = savedPrimsForMultiDrag[jj];
				savedPrimsForMultiDrag[jj] = savedPrimsForMultiDrag[jj+1];
				savedPrimsForMultiDrag[jj+1] = tempPrim;
			}
		}
	}
}

Contact* AdvRunDragger::getFirstContact(Primitive*& prim)
{
	if (savedPrimsForMultiDrag.size() > 0)
	{
		for (int ii = 0; ii < savedPrimsForMultiDrag.size(); ++ii)
		{
			Contact* c = savedPrimsForMultiDrag[ii]->getFirstContact();
			if (c && (savedPrimsForMultiDrag.find(c->otherPrimitive(savedPrimsForMultiDrag[ii])) == savedPrimsForMultiDrag.end()))
			{
				prim = savedPrimsForMultiDrag[ii];
				return c;
			}
		}
	}

	return NULL;
}

Contact* AdvRunDragger::getNextContact(Primitive*& prim, Contact* c)
{
	if (savedPrimsForMultiDrag.size() > 0)
	{
		bool foundContact = false;
		G3D::Array<Primitive*>::iterator iter = std::find(savedPrimsForMultiDrag.begin(), savedPrimsForMultiDrag.end(), prim);
		while (iter != savedPrimsForMultiDrag.end())
		{
			Contact* localC = (*iter)->getFirstContact();
			while (localC)
			{
				if ((localC == c) || (prim != *iter))
					foundContact = true;

				if (!foundContact)
					localC = (*iter)->getNextContact(localC);
				else if (localC == c)
					localC = (*iter)->getNextContact(localC);

				if (foundContact && localC)
				{
					if (savedPrimsForMultiDrag.find(localC->otherPrimitive(*iter)) == savedPrimsForMultiDrag.end())
					{
						prim = *iter;
						return localC;
					}
					else
					{
						localC = (*iter)->getNextContact(localC);
					}
				}
			}
			++iter;
		}
	}
	return NULL;
}

#ifdef DEBUG_MULTIPLE_PARTS_DRAG
	void AdvRunDragger::render3dAdorn(Adorn* adorn)
	{
		if (dragPart.lock())
			RBX::Draw::selectionBox(dragPart.lock()->getPart(), adorn, Color4(Color3::orange()));
	}
#endif

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

