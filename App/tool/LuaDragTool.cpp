/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/LuaDragTool.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/ChangeHistory.h"

namespace RBX {

const char* const sLuaDragTool = "LuaDragTool";


const std::string LuaDragTool::getCursorName() const 
{
	return (luaDragger->isDragging()) ? "GrabRotateCursor" : "DragCursor";
}


LuaDragTool::LuaDragTool(	PartInstance* mousePart,
							const Vector3& hitPointWorld,
							const std::vector<weak_ptr<PartInstance> >& partArray,
							Workspace* workspace,
							shared_ptr<Instance> selectIfNoDrag
							)
	: Named<MouseCommand, sLuaDragTool>(workspace)
	, selectIfNoDrag(selectIfNoDrag)

{
	FASTLOG1(FLog::MouseCommandLifetime, "LuaDragTool created: %p", this);
	luaDragger = Creatable<Instance>::create<LuaDragger>();

	luaDragger->mouseDown(	shared_from<PartInstance>(mousePart),
							mousePart->getCoordinateFrame().pointToObjectSpace(hitPointWorld),
							partArray	);

}

LuaDragTool::~LuaDragTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "LuaDragTool destroyed: %p", this);
}


shared_ptr<MouseCommand> LuaDragTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	capture();

	return shared_from(this);
}

void LuaDragTool::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());
	luaDragger->mouseMove(MouseCommand::getUnitMouseRay(inputObject, workspace));
}


void LuaDragTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());
	luaDragger->mouseMove(MouseCommand::getUnitMouseRay(inputObject, workspace));
}


shared_ptr<MouseCommand> LuaDragTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());
	luaDragger->mouseUp();
	if (!luaDragger->didDrag() && selectIfNoDrag.lock()) {
		ServiceClient< Selection > selection(workspace);
		selection->clearSelection();
		selection->addToSelection(selectIfNoDrag.lock().get());
	}

	releaseCapture();
	
	if (DragUtilities::anyPartAlive(luaDragger->getParts())) {
		workspace->setInsertPoint(DragUtilities::computeExtents(luaDragger->getParts()).topCenter());
	}

	if (luaDragger->didDrag())
		ChangeHistoryService::requestWaypoint("Drag", workspace);

	return shared_ptr<MouseCommand>();
}

shared_ptr<MouseCommand> LuaDragTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	if (luaDragger->isDragging()) {
		switch (inputObject->getKeyCode())
		{
		case SDLK_r: // _R_otate 90 about snap face z axis
			{
				luaDragger->rotateOnSnapFace(Vector3::Z_AXIS, Math::matrixRotateY());
				break;
			}
		case SDLK_t: // _T_ilt 90 about snap face x axis
			{
				luaDragger->rotateOnSnapFace(Vector3::X_AXIS, Math::matrixTiltZ());
				break;
			}
        default:
            break;
		}
		return shared_from(this);
	}
	else {
		RBXASSERT(0);		// when did this happen
		return shared_ptr<MouseCommand>();
	}
}


} // namespace
