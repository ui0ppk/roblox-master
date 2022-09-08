/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/PartDropTool.h"
#include "Tool/MegaDragger.h"
#include "Tool/RunDragger.h"
#include "Tool/ToolsArrow.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/InputObject.h"
#include "v8datamodel/changehistory.h"
#include "SelectState.h"


namespace RBX {

const char* const sPartDropTool = "PartDropTool";

// ToDo - should the parts all be shared pointers as part of a multiplayer dragging redo?

PartDropTool::PartDropTool(	PartInstance* mousePart,
							const Vector3& hitLocal,
							Workspace* workspace,
							shared_ptr<Instance> selectWhenDone)
	: Named<PartDragTool, sPartDropTool>(mousePart,Vector3(0,0,0),workspace,selectWhenDone)
	, hitLocal(hitLocal)
{
	 capture();
	 dragging = true;
	 megaDragger->startDragging();
	 runDragger->initLocal(	workspace,
							megaDragger->getMousePart(), 
							hitLocal );
}

/*override*/ shared_ptr<MouseCommand> PartDropTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(0);
	return shared_from(this);
}
//MouseCommand* PartDropTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
//{
//	Super::onMouseUp(inputObject);
//	releaseCapture();
//	return NULL;
//}

void PartDropTool::onMouseDelta(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(this->captured());

	if (	(!dragging)
		&&	megaDragger->mousePartAlive()
		)
	{
		dragging = true;
		megaDragger->startDragging();
		runDragger->initLocal(	workspace,
								megaDragger->getMousePart(), 
								hitLocal );
	}

	onMouseIdle(inputObject);
}


shared_ptr<MouseCommand> PartDropTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	switch (inputObject->getKeyCode())
	{
	case SDLK_BACKSPACE:
	case SDLK_DELETE:
	case SDLK_ESCAPE:
		{
			return onCancelOperation();
		}
    default:
        break;
	}
	return Super::onKeyDown(inputObject);
}


shared_ptr<MouseCommand> PartDropTool::onCancelOperation()
{
	megaDragger->removeParts();
	//Cancel the operation by removing the part from the workspace
	releaseCapture();
	return shared_ptr<MouseCommand>();
}

PartDropTool::~PartDropTool()
{
}


} // namespace
