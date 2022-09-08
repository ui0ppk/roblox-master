/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/DragUtilities.h"
#include "Tool/Dragger.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/MouseCommand.h"
#include "V8World/ContactManager.h"
#include "V8World/Primitive.h"
#include "V8World/World.h"
#include "V8World/Tolerance.h"
#include "Tool/ToolsArrow.h"

namespace RBX {


Vector3 DragUtilities::safeMoveYDrop(const PartArray& parts, const Vector3& tryDrag, ContactManager& contactManager, const float customPlaneHeight)
{
	G3D::Array<Primitive*> primitives;
	DragUtilities::partsToPrimitives(parts, primitives);
	RBXASSERT(primitives.size() > 0);

	Vector3 answer = Dragger::safeMoveYDrop(primitives, tryDrag, contactManager, customPlaneHeight);
	RBXASSERT(!contactManager.intersectingOthers(primitives, Tolerance::maxOverlapOrGap()));

	return answer;
}



Vector3 DragUtilities::hitObjectOrPlane(const PartArray& parts, const RbxRay& unitMouseRay, const ContactManager& contactManager, bool snapToGrid)
{
	Vector3 hit;
	std::vector<const Primitive*> primitives;
	partsToPrimitives(parts, primitives);
	hitObjectOrPlane(	contactManager,
						unitMouseRay,
						primitives,
						hit,
                        snapToGrid);
	return hit;
}

bool DragUtilities::hitObject(const PartArray& parts, const RbxRay& unitMouseRay, const ContactManager& contactManager, Vector3& hit, bool snapToGrid)
{
	std::vector<const Primitive*> primitives;
	partsToPrimitives(parts, primitives);
	return hitObject(contactManager, unitMouseRay, primitives, hit, snapToGrid);
}


bool DragUtilities::anyPartAlive(const PartArray& parts)
{
	for (size_t i = 0; i < parts.size(); ++i) {
		if (PartInstance::nonNullInWorkspace(parts[i].lock())) {
			return true;
		}
	}
	return false;
}


World* DragUtilities::partsToPrimitives(const PartArray& parts, G3D::Array<Primitive*>& primitives)
{
	RBXASSERT(primitives.size() == 0);
	World* answer = NULL;

	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			primitives.append(part->getPartPrimitive());
			answer = answer ? answer : Workspace::getWorldIfInWorkspace(part.get());
			RBXASSERT(answer);
		}
	}
	return answer;
}

void DragUtilities::partsToPrimitives(const PartArray& parts, std::vector<const Primitive*>& primitives)
{
	RBXASSERT(primitives.size() == 0);

	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			primitives.push_back(part->getPartPrimitive());
		}
	}
}

void DragUtilities::partsToPrimitives(const PartArray& parts, std::vector<Primitive*>& primitives)
{
	RBXASSERT(primitives.size() == 0);

	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			primitives.push_back(part->getPartPrimitive());
		}
	}
}

void DragUtilities::pvsToParts(const std::vector<PVInstance*>& pvInstances, PartArray& parts)
{
	for (size_t i = 0; i < pvInstances.size(); ++i) {
		PartInstance::findParts(pvInstances[i], parts);
	}

	removeDuplicateParts(parts);
}

void DragUtilities::instancesToParts(const std::vector<Instance*>& instances, PartArray& parts)
{
	for (size_t i = 0; i < instances.size(); ++i) {
		PartInstance::findParts(instances[i], parts);
	}

	removeDuplicateParts(parts);
}

void DragUtilities::instancesToParts(const Instances& instances, PartArray& parts)
{
	for (size_t i = 0; i < instances.size(); ++i)
		PartInstance::findParts(instances[i].get(), parts);

	removeDuplicateParts(parts);
}

void DragUtilities::removeDuplicateParts(PartArray& parts)
{
	if (parts.size() > 1)
	{
		// create a set, as per different results on web inserting manually seems to be more efficient as compared to creating via constructor
		std::set<weak_ptr<PartInstance> > tempParts;
		for (size_t i = 0; i < parts.size(); ++i)
			tempParts.insert(parts[i]);
		parts.assign(tempParts.begin(), tempParts.end());
	}
}

bool DragUtilities::notJoined(const PartArray& parts)
{
	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			if (part->getPartPrimitive()->hasAutoJoints()) {
				return false;
			}
		}
	}
	return true;
}

bool DragUtilities::notJoinedToOutsiders(const PartArray& parts)
{
	return true;
}

void DragUtilities::unJoinFromOutsiders(const PartArray& parts)
{
	G3D::Array<Primitive*> primitives;
	World* world = partsToPrimitives(parts, primitives);
	if (world) {
		world->destroyAutoJointsToWorld(primitives);
	}

	RBXASSERT(notJoinedToOutsiders(parts));
}


void DragUtilities::joinToOutsiders(const PartArray& parts)
{
	if (!AdvArrowTool::advCreateJointsMode)
		return;

	G3D::Array<Primitive*> primitives;
	World* world = partsToPrimitives(parts, primitives);
	if (world) {
		world->createAutoJointsToWorld(primitives);
	}

	RBXASSERT(notJoinedToOutsiders(parts));
}

void DragUtilities::unJoin(const PartArray& parts)
{
	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			part->destroyJoints();
		}
	}
	RBXASSERT(notJoined(parts));
}

void DragUtilities::join(const PartArray& parts)
{
	if (!AdvArrowTool::advCreateJointsMode)
		return;

	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			part->join();
		}
	}
}

void DragUtilities::joinWithInPartsOnly(const PartArray& parts)
{
	if (!AdvArrowTool::advCreateJointsMode)
		return;

	G3D::Array<Primitive*> primitives;
	World* world = partsToPrimitives(parts, primitives);
	if (world) {
		world->createAutoJointsToPrimitives(primitives);
	}
}

void DragUtilities::setDragging(const PartArray& parts)
{
	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			part->setDragging(true);
			part->setNetworkIsSleeping(false);	// twiddle this bit - I'm dragging client side
		}
	}
}

void DragUtilities::stopDragging(const PartArray& parts)
{

	for (size_t i = 0; i < parts.size(); ++i) {
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (PartInstance::nonNullInWorkspace(part)) {
			part->setDragging(false);
			if (parts.size() == 1) {
				part->setLinearVelocity(Vector3::zero());
				part->setRotationalVelocity(Vector3::zero());
			}
		}
	}
}



void DragUtilities::alignToGrid(PartInstance* part)
{
	const CoordinateFrame& partCoord = part->getCoordinateFrame();
    CoordinateFrame alignedCoord = Math::snapToGrid(partCoord, Tolerance::mainGrid());
    if (alignedCoord != partCoord)
    {
        part->setCoordinateFrame(alignedCoord);
    }
}

static float fRound(float a,int precision)
{ return (a > 0.f) ? ceil(a*precision - 0.5f)/precision : floor(a*precision + 0.5f)/precision; }

void DragUtilities::moveByDelta(PartInstance* part, const Vector3& delta, bool snapToWorld)
{
	CoordinateFrame coord = part->getCoordinateFrame();
	coord.translation += delta;
	// try correcting floating point issues
	coord.translation.x = fRound(coord.translation.x, 1e+6f);
	coord.translation.y = fRound(coord.translation.y, 1e+6f);
	coord.translation.z = fRound(coord.translation.z, 1e+6f);
	part->setCoordinateFrame(coord);
}


void DragUtilities::clean(PartInstance* part)
{
	if (part->aligned()) {
		alignToGrid(part);
	}
}

void DragUtilities::clean(const PartArray& parts)
{
    // Only clean single parts.  Multiple parts should
    // stick together when moving.
    if (parts.size() == 1)
    {
        const shared_ptr<PartInstance>& part = parts[0].lock();
        if (part.get())
        {
            clean(part.get());
        }
    }
}

void DragUtilities::move(const PartArray& parts,
						 CoordinateFrame from, 
						 CoordinateFrame to)
{
	for (size_t i = 0; i < parts.size(); ++i)
	{
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (part.get())
		{
			CoordinateFrame meInFrom = from.toObjectSpace(part->getCoordinateFrame());
			Math::orthonormalizeIfNecessary(meInFrom.rotation);
			part->setCoordinateFrame(to * meInFrom);
		}
	}
}

// same as ::move, but without orthonormalize
void DragUtilities::move2(const PartArray& parts,
						 CoordinateFrame from, 
						 CoordinateFrame to)
{
	for (size_t i = 0; i < parts.size(); ++i)
	{
		const shared_ptr<PartInstance>& part = parts[i].lock();
		if (part.get())
		{
			CoordinateFrame meInFrom = from.toObjectSpace(part->getCoordinateFrame());
			//Math::orthonormalizeIfNecessary(meInFrom.rotation);
			part->setCoordinateFrame(to * meInFrom);
		}
	}
}

bool DragUtilities::hitObjectOrPlane(const ContactManager& contactManager,
									const RbxRay& unitSearchRay,
									const std::vector<const Primitive*>& ignorePrims,
									Vector3& hit,
                                    bool snapToGrid)
{
	RBXASSERT(unitSearchRay.direction().isUnit());
	RbxRay searchRay = MouseCommand::getSearchRay(unitSearchRay);	// length determines how far to search;

	if (!contactManager.getHit(	searchRay,
								&ignorePrims,
								NULL,
								hit))
	{
		if (!Math::intersectRayPlane(searchRay, Plane(Vector3::unitY(), Vector3::zero()), hit)) {
			hit = Vector3::zero();
			return false;
		}
	}
    if( snapToGrid )
	    hit = DragUtilities::toGrid(hit);
	return true;
}

bool DragUtilities::hitObject(	const ContactManager& contactManager,
								const RbxRay& unitSearchRay,
								const std::vector<const Primitive*>& ignorePrims,
								Vector3& hit,
                                bool snapToGrid)
{
	RBXASSERT(unitSearchRay.direction().isUnit());
	RbxRay searchRay = MouseCommand::getSearchRay(unitSearchRay);	// length determines how far to search;

	if (!contactManager.getHit(	searchRay,
								&ignorePrims,
								NULL,
								hit))
		return false;

    if( snapToGrid )
	    hit = DragUtilities::toGrid(hit);
	return true;
}


Extents DragUtilities::computeExtents(const PartArray& parts)
{
	G3D::Array<Primitive*> primitives;
	partsToPrimitives(parts, primitives);
	
	return Dragger::computeExtents(primitives);
}


void DragUtilities::getPrimitives(const Instance* instance, std::vector<Primitive*>& primitives)
{
	// TODO: This cast blows. We should refactor getPrimitives to use back_insert_iterator or a for_each pattern...
	std::vector<const Primitive*>& constPrimitives = reinterpret_cast<std::vector<const Primitive*>&>(primitives);
	getPrimitivesConst(instance, constPrimitives);
}

void DragUtilities::getPrimitivesConst(const Instance* instance, std::vector<const Primitive*>& primitives)
{
	const PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance);
	if ( part && part->getDragUtilitiesSupport() ) {
		primitives.push_back(part->getConstPartPrimitive());
	}

	for (size_t i = 0; i < instance->numChildren(); i++) {
		getPrimitivesConst(instance->getChild(i), primitives);
	}
}
Vector3 DragUtilities::toGrid(const Vector3 &point, const Vector3& grid)
{
	if (grid != Vector3::zero())
		return Math::toGrid(point, grid);

	return Math::toGrid(point, getGrid());
}

Vector3 DragUtilities::getGrid()
{
	static Vector3 v1(1.0f, 1.0f, 1.0f);
	static Vector3 v2(0.20f, 0.20f, 0.20f);
	static Vector3 v3(Tolerance::maxOverlapOrGap(), Tolerance::maxOverlapOrGap(), Tolerance::maxOverlapOrGap());

	if (AdvArrowToolBase::advGridMode == DRAG::ONE_STUD)
		return v1;

	if (AdvArrowToolBase::advGridMode == DRAG::QUARTER_STUD)
		return v2;

	if (AdvArrowToolBase::advGridMode == DRAG::OFF)
		return v3;

	return v3;
}

Vector3 DragUtilities::toLocalGrid(const Vector3& deltaIn)
{
    if (AdvArrowToolBase::advGridMode == DRAG::OFF)
        return deltaIn;

    Vector3 delta = deltaIn;
    float length = delta.length();
    
    delta.unitize();
    
    // We assume that grid is uniform for now.
    RBXASSERT(DragUtilities::getGrid().x == DragUtilities::getGrid().y &&
             DragUtilities::getGrid().y == DragUtilities::getGrid().z &&
             DragUtilities::getGrid().z == DragUtilities::getGrid().x);
    
    float gridSize = DragUtilities::getGrid().x;
    length = length / gridSize;
    length = ((float)Math::iRound(length)) * gridSize;
    delta = delta * length;
	return delta;
}


} // namespace
