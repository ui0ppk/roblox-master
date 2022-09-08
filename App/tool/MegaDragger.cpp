/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/MegaDragger.h"

#include "Tool/ToolsArrow.h"
#include "Tool/AdvMoveTool.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/MouseCommand.h"
#include "Tool/Dragger.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "V8World/Tolerance.h"

namespace RBX {


MegaDragger::MegaDragger(PartInstance* mousePartPtr,
						const std::vector<PVInstance*>& pvInstances,
						RootInstance* rootInstance,
						DRAG::JoinType joinType)
	: mousePart(shared_from<PartInstance>(mousePartPtr))
	, rootInstance(rootInstance)
	, contactManager(*rootInstance->getWorld()->getContactManager())
	, joined(true)
	, joinType(joinType)
{
	DragUtilities::pvsToParts(pvInstances, dragParts);
}


MegaDragger::MegaDragger(PartInstance* mousePartPtr,
						const PartArray& partArray,
						RootInstance* rootInstance,
						DRAG::JoinType joinType)
	: mousePart(shared_from<PartInstance>(mousePartPtr))
	, rootInstance(rootInstance)
	, contactManager(*rootInstance->getWorld()->getContactManager())
	, joined(true)
	, dragParts(partArray)
	, joinType(joinType)
{
}

MegaDragger::~MegaDragger()
{
	if (!joined) {
		RBXASSERT(0);
	}
}


void MegaDragger::startDragging()
{
	// destroy joints outside the parts....
	if (joinType != DRAG::NO_UNJOIN_NO_JOIN) {
		DragUtilities::unJoinFromOutsiders(dragParts);
	}

	DragUtilities::setDragging(dragParts);

//	if (mousePartAlive()) {
//		shared_ptr<PartInstance> mp(mousePart.lock());
//		rootInstance->setCameraIgnoreParts(mp.get());
//	}

	joined = false;
}

void MegaDragger::continueDragging()
{
	RBXASSERT(!joined);
	DragUtilities::unJoinFromOutsiders(dragParts);
	DragUtilities::setDragging(dragParts);		// do this every time - multiplayer
}


void MegaDragger::finishDragging()
{
	RBXASSERT(!joined);

//	rootInstance->clearCameraIgnoreParts();

	if (joinType == DRAG::UNJOIN_JOIN) {
		DragUtilities::joinToOutsiders(dragParts);
	}
	
	DragUtilities::stopDragging(dragParts);

	if (DragUtilities::anyPartAlive(dragParts)) 
	{
		rootInstance->setInsertPoint(DragUtilities::computeExtents(dragParts).topCenter());
	}

	joined = true;
}


Vector3 MegaDragger::hitObjectOrPlane(const shared_ptr<InputObject>& inputObject)
{
	return DragUtilities::hitObjectOrPlane(dragParts, MouseCommand::getUnitMouseRay(inputObject, rootInstance), contactManager);
}


void MegaDragger::alignAndCleanParts()
{
	RBXASSERT(!joined);

	if (mousePartAlive()) {
		shared_ptr<PartInstance> mp = mousePart.lock();
		if (mp && !mp->aligned()) {
			CoordinateFrame orgCoord = mp->worldSnapLocation();
			CoordinateFrame alignedCoord = Math::snapToGrid(orgCoord, Dragger::dragSnap());
			DragUtilities::move(dragParts, orgCoord, alignedCoord);
		}
	}
	DragUtilities::clean(dragParts);
}

void MegaDragger::cleanParts()
{
	RBXASSERT(!joined);
	DragUtilities::clean(dragParts);
}

Vector3 MegaDragger::safeMoveYDrop(const Vector3& tryDrag)
{
	RBXASSERT(!joined);	
	return DragUtilities::safeMoveYDrop(dragParts, tryDrag, contactManager);
}


void MegaDragger::getPartsForDrag(G3D::Array<Primitive*>& primitives)
{
	RBXASSERT(!joined);
	DragUtilities::partsToPrimitives(dragParts, primitives);
	RBXASSERT(primitives.size() > 0);
}


Vector3 MegaDragger::safeMoveNoDrop(const Vector3& tryDrag)				// Moves down until collision - if necessary, moves up
{
	G3D::Array<Primitive*> primitives;
	getPartsForDrag(primitives);
	Vector3 answer = Dragger::safeMoveNoDrop(primitives, tryDrag, contactManager);
	RBXASSERT(!contactManager.intersectingOthers(primitives, Tolerance::maxOverlapOrGap()));
	return answer;
}

Vector3 MegaDragger::safeMoveAlongLine(const Vector3& tryDrag,
                                       bool snapToWorld)
{
	G3D::Array<Primitive*> primitives;
	getPartsForDrag(primitives);
	if (primitives.size() > 0) {
		Vector3 answer = Dragger::safeMoveAlongLine(primitives, tryDrag, contactManager, Dragger::groundPlaneDepth(), snapToWorld);
		RBXASSERT(		(answer == Vector3::zero()) 
					|| !contactManager.intersectingOthers(primitives, Tolerance::maxOverlapOrGap()));
		return answer;
	}
	else {
		return Vector3::zero();
	}
}

Vector3 MegaDragger::safeMoveAlongLine2(const Vector3& tryDrag, bool& out_isCollided)
{		
	// apply drag vector, return directly if there's no intersection
	if (moveSafePlaceAlongLine(tryDrag))
		return tryDrag;
	
	// we are having an intersection, move back
	Vector3 drag(tryDrag);
	drag *= -1.0f;
	out_isCollided = true;
	moveSafePlaceAlongLine(drag);

	return Vector3::zero();
}

bool MegaDragger::moveAlongLine(const Vector3& tryDrag)
{	return moveSafePlaceAlongLine(tryDrag); }

Vector3 MegaDragger::safeRotateAlongLine(const Vector3& tryDrag)
{
	G3D::Array<Primitive*> primitives;
	getPartsForDrag(primitives);
	if (primitives.size() > 0) {
		Vector3 axis = Math::safeDirection(tryDrag);
		float amount = tryDrag.magnitude();

		Matrix3 rotation = Matrix3::fromAxisAngleFast(axis, amount);
		Dragger::safeRotate(primitives, rotation, contactManager);

//		RBXASSERT(!contactManager.intersectingOthers(primitives, Tolerance::maxOverlapOrGap()));
		return tryDrag;
	}
	else {
		return Vector3::zero();
	}
}

Vector3 MegaDragger::safeRotateAlongLine2(const Vector3& tryDrag, const float &angle)
{
	if (angle > 0.0)
	{
		G3D::Array<Primitive*> primitives;	
		DragUtilities::partsToPrimitives(dragParts, primitives);

		if (primitives.size() > 0) 
		{
			Vector3 axis = Math::safeDirection(tryDrag);

            Matrix3 rotMatrix = Matrix3::fromAxisAngleFast(axis, angle);
            
            if (primitives.size() == 1 && AdvMoveTool::advLocalRotationMode)
            {
                PartInstance* part = PartInstance::fromPrimitive(primitives[0]);
                if (part)
                {			
                    CoordinateFrame temp = part->getCoordinateFrame();
                    temp.rotation = temp.rotation * rotMatrix;
                    part->setCoordinateFrame(temp);
                }
            }
            else
            {
                Dragger::safeRotate2(primitives, rotMatrix, contactManager);
            }

			return tryDrag;
		}
	}
	
	return Vector3::zero();	
}


Vector3 MegaDragger::safeMoveToMinimumHeight(float height)
{
	Vector3 center = DragUtilities::computeExtents(dragParts).center();

	if (height > center.y)
	{
		return safeMoveAlongLine(Vector3(0, height - center.y, 0));
	}
	return Vector3::zero();
}



void MegaDragger::removeParts()
{
	for(size_t i = 0; i<dragParts.size(); i++){
		if(shared_ptr<PartInstance> part = dragParts[i].lock()){
			part->setParent(NULL);	
		}
	}
	joined = true;
}

bool MegaDragger::safeRotate(const Matrix3& rotMatrix)
{
	RBXASSERT(!joined);
	G3D::Array<Primitive*> primitives;
	DragUtilities::partsToPrimitives(dragParts, primitives);
	RBXASSERT(primitives.size() > 0);

    if (primitives.size() == 1 && AdvMoveTool::advLocalRotationMode)
    {
        PartInstance* part = PartInstance::fromPrimitive(primitives[0]);
        if (part)
        {			
            CoordinateFrame temp = part->getCoordinateFrame();
            temp.rotation = temp.rotation * rotMatrix;
            
            temp.rotation.orthonormalize();
            
            part->setCoordinateFrame(temp);
        }
    }
	else if (primitives.size() >= 1)
    {
			Dragger::safeRotate2(primitives, rotMatrix, contactManager);
    }

	//return intersection status
	return (!contactManager.intersectingOthers(primitives, Tolerance::maxOverlapOrGap()));
}

Matrix3 MegaDragger::rotateDragParts(const Matrix3& rotMatrix, bool respectCollisions)
{
	bool hasNoCollision = safeRotate(rotMatrix);
	if (!respectCollisions || hasNoCollision)
		return rotMatrix;
	
	//we are having an intersection rotate back
	safeRotate(rotMatrix.inverse());
	return Matrix3::identity();
}

bool MegaDragger::mousePartAlive()
{
	return (PartInstance::nonNullInWorkspace(mousePart.lock()));
}

bool MegaDragger::anyDragPartAlive()
{
	return DragUtilities::anyPartAlive(dragParts);
}

bool MegaDragger::moveSafePlaceAlongLine(const Vector3& tryDrag)
{
	G3D::Array<Primitive*> primitives;	
	DragUtilities::partsToPrimitives(dragParts, primitives);

	if (primitives.size() <= 0)
		return true;

		for (int i = 0; i < primitives.size(); ++i) {		
			PartInstance* part = PartInstance::fromPrimitive(primitives[i]);
			if (!part)
				continue;
			CoordinateFrame coord = part->getCoordinateFrame();
			coord.translation += tryDrag;
			part->setCoordinateFrame(coord);
		}

	return !Dragger::intersectingWorldOrOthers(primitives, contactManager, Tolerance::maxOverlapOrGap(), Dragger::maxDragDepth());
}

void MegaDragger::setToSelection(const Workspace* workspace)
{
	RBX::Selection* selection = ServiceProvider::find<Selection>(workspace);
	if (!selection)
		return;

	std::vector<RBX::PVInstance*> pvInstances;

	for (RBX::Instances::const_iterator iter = selection->begin(); iter != selection->end(); ++iter)
	{
		if ((*iter)->isDescendantOf(workspace))
			if (shared_ptr<RBX::PVInstance> pvInstance = RBX::Instance::fastSharedDynamicCast<RBX::PVInstance>(*iter))
				pvInstances.push_back(pvInstance.get());
	}

	if (pvInstances.size() > 0)
	{
		dragParts.clear();
		DragUtilities::pvsToParts(pvInstances, dragParts);
	}
}

} // namespace
