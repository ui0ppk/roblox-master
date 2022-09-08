/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/PartDragTool.h"
#include "Tool/MegaDragger.h"
#include "Tool/RunDragger.h"
#include "Tool/ToolsArrow.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/InputObject.h"
#include "V8Datamodel/ChangeHistory.h"
#include "SelectState.h"


namespace RBX {

const char* const sPartDragTool = "PartDragTool";

// ToDo - should the parts all be shared pointers as part of a multiplayer dragging redo?

PartDragTool::PartDragTool(	PartInstance* mousePart,
							const Vector3& hitPointWorld,
							Workspace* workspace,
							shared_ptr<Instance> selectIfNoDrag)
	: Named<MouseCommand, sPartDragTool>(workspace)
	, hitWorld(hitPointWorld)
	, dragging(false)
	, selectIfNoDrag(selectIfNoDrag)
{
	FASTLOG1(FLog::MouseCommandLifetime, "PartDragTool created: %p", this);
	RBXASSERT(mousePart);
//	RBXASSERT(mousePart->isTopLevelPVInstance());

	runDragger.reset(new RunDragger());

	std::vector<PVInstance*> pvInstances;
	pvInstances.push_back(mousePart);
	megaDragger.reset(new MegaDragger(	mousePart, 
										pvInstances, 
										workspace));
}

shared_ptr<MouseCommand> PartDragTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	downPoint = inputObject->get2DPosition();
	capture();
	return shared_from(this);
}


void PartDragTool::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());

	if ((inputObject->get2DPosition() - downPoint).length() > 4) {
		onMouseDelta(inputObject);
	}
	else {
		onMouseIdle(inputObject);
	}
}

void PartDragTool::onMouseDelta(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());

	if (	(!dragging)
		&&	megaDragger->mousePartAlive()
		)
	{
		dragging = true;
		megaDragger->startDragging();
		runDragger->init(	workspace,
							megaDragger->getMousePart(), 
							hitWorld	);
	}

	onMouseIdle(inputObject);
}

void PartDragTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(captured());

	if (	captured() 
		&&	dragging 
		&&	megaDragger->mousePartAlive())
	{
		megaDragger->continueDragging();			// do this every time - multiplayer

		runDragger->snap(getUnitMouseRay(inputObject));
	}
}


shared_ptr<MouseCommand> PartDragTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());

	if (dragging) {
		if (megaDragger->mousePartAlive()) {
			runDragger->snap(getUnitMouseRay(inputObject));		// final snap
		}
		megaDragger->finishDragging();
		dragging = false;
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
	}
	else {
		if (selectIfNoDrag) {
			ServiceClient< Selection > selection(workspace);
			selection->clearSelection();
			selection->addToSelection(selectIfNoDrag);
		}
	}
	releaseCapture();
	return shared_ptr<MouseCommand>();
}



shared_ptr<MouseCommand> PartDragTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	if (megaDragger->mousePartAlive()) {
		if (dragging) {
			shared_ptr<PartInstance> mousePart(megaDragger->getMousePart().lock());
			switch (inputObject->getKeyCode())
			{
			case SDLK_r:	
				{
					runDragger->rotatePart90DegAboutSnapFaceAxis(Vector3::Z_AXIS);
					break;
				}
			case SDLK_t:
				{
					runDragger->rotatePart90DegAboutSnapFaceAxis(Vector3::X_AXIS);
					break;
				}
            default:
                break;
			}
		}
		return shared_from(this);
	}
	else {
		return shared_ptr<MouseCommand>();
	}
}


void PartDragTool::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

	if (megaDragger->mousePartAlive()) 
	{
		megaDragger->getMousePart().lock()->render3dSelect(adorn, SELECT_NORMAL);
	}
}



PartDragTool::~PartDragTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "PartDragTool destroyed: %p", this);
}


} // namespace
