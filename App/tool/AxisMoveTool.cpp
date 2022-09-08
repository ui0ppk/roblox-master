/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/AxisMoveTool.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "Tool/Dragger.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "Util/Math.h"
#include "Util/HitTest.h"
#include "Util/Sound.h"
#include "AppDraw/DrawAdorn.h"
#include "v8datamodel/changehistory.h"
#include "RbxG3D/RbxRay.h"

namespace RBX {

///////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////

AxisToolBase::AxisToolBase(Workspace* workspace) 
	: ArrowToolBase(workspace)
	, dragging(false)
	, cursor("ArrowCursor")
{}


void AxisToolBase::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	onMouseIdle(inputObject);
}


void AxisToolBase::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	Super::onMouseIdle(inputObject);

	if (this->captured()) {
		cursor = "DragCursor";
	}
	else if (getOverHandle(inputObject)) {
		cursor = "ResizeCursor";
	}
	else {
		cursor = Super::getCursorName();
	}
}


shared_ptr<MouseCommand> AxisToolBase::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	NormalId normalId;
	Vector3 tempPt;
	if (getOverHandle(inputObject, tempPt, normalId)) {
		downPoint2d = inputObject->get2DPosition();
		dragAxis = (int)(normalId) % 3;
		lastPoint3d = DragUtilities::toGrid(tempPt);
		dragRay = RbxRay::fromOriginAndDirection(lastPoint3d, normalIdToVector3(normalId));
		capture();
		return shared_from(this);
	}
	else {
		return Super::onMouseDown(inputObject);		
	}
}


void AxisToolBase::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(captured());

	if ( !dragging 
		&&	(inputObject->get2DPosition() - downPoint2d).length() > 4)
	{
		ServiceClient< FilteredSelection<PVInstance> > pvSelection(this->workspace);
		megaDragger.reset(new MegaDragger(	NULL, 
											pvSelection->items(), 
											workspace));

		megaDragger->startDragging();
		megaDragger->alignAndCleanParts();
		dragging = true;
	}

	if (	dragging
		&&	megaDragger->anyDragPartAlive()) 
	{
		Vector3 closePoint = Math::closestPointOnRay(dragRay, getUnitMouseRay(inputObject));
		closePoint = DragUtilities::toGrid(closePoint);				// snapped

		Vector3 delta = DragUtilities::toGrid(closePoint - lastPoint3d);
		RBXASSERT(delta[(dragAxis + 1) % 3] == 0.0f);
		RBXASSERT(delta[(dragAxis + 2) % 3] == 0.0f);

		if (delta != Vector3::zero()) {
			megaDragger->continueDragging();			// do this every time - multiplayer
			Vector3 move = (getDragType() == HANDLE_MOVE) 
									? megaDragger->safeMoveAlongLine(delta)
									: megaDragger->safeRotateAlongLine(delta);
			if (move != Vector3::zero()) {
				lastPoint3d = DragUtilities::toGrid(lastPoint3d + move);
			}
		}
	}
}

shared_ptr<MouseCommand> AxisToolBase::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	if (dragging) 
	{
		megaDragger->finishDragging();
		megaDragger.release();
		dragging = false;
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
	}
	releaseCapture();
	return shared_ptr<MouseCommand>();
}

void AxisToolBase::render2d(Adorn* adorn)
{
	Extents worldExtents;

	if (getExtents(worldExtents)) {

		RBX::DrawAdorn::handles2d(
			worldExtents.size(),
			CoordinateFrame(worldExtents.center()),
			*(workspace->getConstCamera()),
			adorn,
			getDragType(),
			getHandleColor()); 
	}
}

void AxisToolBase::render3dAdorn(Adorn* adorn)
{
    renderHoverOver(adorn);

	Extents worldExtents;
	
	if (getExtents(worldExtents)) {

		RBX::DrawAdorn::handles3d(
			worldExtents.size(),
			CoordinateFrame(worldExtents.center()),
			adorn, 
			getDragType(),
			workspace->getConstCamera()->coordinateFrame().translation,
			Color3::orange());
	}
}

bool AxisToolBase::getExtents(Extents& extents) const
{
	ServiceClient<Selection> selection(workspace);

	std::vector<Primitive*> primitives;

	std::for_each(
			selection->begin(), 
			selection->end(), 
			boost::bind(	&DragUtilities::getPrimitives2, 
							_1, 
							boost::ref(primitives))
							);

	if (primitives.size() > 0) 
	{
		extents = Dragger::computeExtents(primitives);
		return true;
	}
	else {
		return false;
	}
}

bool AxisToolBase::getOverHandle(const shared_ptr<InputObject>& inputObject) const
{
	Vector3 tempV;
	NormalId tempN;
	return getOverHandle(inputObject, tempV, tempN);
}

bool AxisToolBase::getOverHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& normalId) const
{
	RBXASSERT(!captured());

	Extents extents;
    bool result = false;

	if (getExtents(extents)) 
    {
        if ( getDragType() == HANDLE_MOVE )
        {
            result = HandleHitTest::hitTestMoveHandleWorld(	
                extents,			                                  
				getUnitMouseRay(inputObject),
				hitPointWorld, 
				normalId);
        }
        else
        {
            result = HandleHitTest::hitTestHandleWorld(	
                extents,
			    getDragType(),
			    getUnitMouseRay(inputObject),
			    hitPointWorld, 
			    normalId );
        }
	}

	return result;
}

} // namespace
