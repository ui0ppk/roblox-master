/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/ResizeTool.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ChangeHistory.h"
#include "Tool/Dragger.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "Util/Math.h"
#include "Util/HitTest.h"
#include "Util/SoundService.h"
#include "AppDraw/DrawAdorn.h"

namespace RBX {

const char* const sResizeTool = "Resize";

/////////////////////////////////////////////////////////////////////
//
// RESIZE TOOL
//


void ResizeTool::findTargetPV(const shared_ptr<InputObject>& inputObject)
{
	ServiceClient< Selection > selection(this->workspace);

	Instances::const_iterator it;

	for (it = selection->begin(); it != selection->end(); ++it) {
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get())) {

			RbxRay gridRay(getUnitMouseRay(inputObject));
			if (HandleHitTest::hitTestHandleLocal(	part->getPartPrimitive()->getExtentsLocal(),
													part->getLocation(),
													HANDLE_RESIZE,
													gridRay, 
													hitPointGrid, 
													localNormalId,
													part->getResizeHandleMask().normalIdMask))
			{
				targetPV = shared_from(part);
				return;
			}
		}
	}
	targetPV.reset();
};

void ResizeTool::render3dAdorn(Adorn* adorn)
{
    renderHoverOver(adorn);

	ServiceClient< Selection > selection(this->workspace);

	Instances::const_iterator it;

	for (it = selection->begin(); it != selection->end(); ++it) {
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get())) {

			RBX::DrawAdorn::handles3d(
				part->getPartSizeXml(),
				part->getCoordinateFrame(),
				adorn, 
				HANDLE_RESIZE,
				workspace->getConstCamera()->coordinateFrame().translation,
				DrawAdorn::resizeColor(),
                true,
				part->getResizeHandleMask().normalIdMask);
		}
	}
}

void ResizeTool::render2d(Adorn* adorn)
{
	ServiceClient< Selection > selection(this->workspace);

	Instances::const_iterator it;

	for (it = selection->begin(); it != selection->end(); ++it) {
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get())) {

			DrawAdorn::handles2d(
				part->getPartSizeXml(),
				part->getCoordinateFrame(),
				*(workspace->getConstCamera()),
				adorn, 
				HANDLE_RESIZE,
				DrawAdorn::resizeColor(),
                true,
				part->getResizeHandleMask().normalIdMask);
		}
	}
}


const std::string ResizeTool::getCursorName() const
{
	return overHandle ? "ResizeCursor" : Super::getCursorName();
}

void ResizeTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(!captured());

	findTargetPV(inputObject);
	overHandle = (!targetPV.expired());

	Super::onMouseHover(inputObject);
}

shared_ptr<MouseCommand> ResizeTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	findTargetPV(inputObject);

	if (targetPV.expired()) {
		return Super::onMouseDown(inputObject);		
	}
	else {
		down = inputObject->get2DPosition();
		moveAxis = 0;
		movePerp = 0;
		overHandle = true;
		shared_ptr<PartInstance> part = shared_polymorphic_downcast<PartInstance>(targetPV.lock());
		moveIncrement = part->getResizeIncrement();
		capture();
		return shared_from(this);	// captured is true....
	}
}


void ResizeTool::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	if (!targetPV.expired()) {
		Vector3 worldNormal = Math::getWorldNormal(localNormalId, targetPV.lock()->getLocation());
		RbxRay axisRay = RbxRay::fromOriginAndDirection(hitPointGrid, worldNormal);
		RbxRay gridRay = getUnitMouseRay(inputObject);
		Vector3 closePoint = RBX::Math::closestPointOnRay(axisRay, gridRay);
		Vector3 handleToCurrent = closePoint - hitPointGrid;
		float distanceAlongAxis = axisRay.direction().dot(handleToCurrent);

		float delta = distanceAlongAxis - moveAxis;
		if (delta > moveIncrement) {
			int move = (Math::iFloor(delta)/moveIncrement)*moveIncrement;
			moveAxis += move;
			capturedDrag(move);
		}
		if (delta < -moveIncrement) {
			int move = -((Math::iFloor(-delta)/moveIncrement)*moveIncrement);
			moveAxis += move;
			capturedDrag(move);
		}
	}
}

shared_ptr<MouseCommand> ResizeTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	if (captured()) {

		releaseCapture();


		// Save the undo/redo state - if deleted in the process, then don't
		if (!targetPV.expired()) {
			ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);

			workspace->getDataState().setDirty(true);
		}
	}

	return shared_ptr<MouseCommand>();
}


void ResizeTool::capturedDrag(int axisDelta)
{
	if (targetPV.expired()) {
		RBXASSERT(0);	// should be caught higher up
	}
	else {
		// Debug version confirms the dynamic_cast of targetInstance
		shared_ptr<PartInstance> part = shared_polymorphic_downcast<PartInstance>(targetPV.lock());
        part->resize(localNormalId, axisDelta);
	}
}

} // namespace
