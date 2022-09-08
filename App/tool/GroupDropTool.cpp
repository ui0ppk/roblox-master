/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/GroupDropTool.h"
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

const char* const sGroupDropTool = "GroupDropTool";

// ToDo - should the parts all be shared pointers as part of a multiplayer dragging redo?

GroupDropTool::GroupDropTool(	PartInstance* mousePart,
								const PartArray& partArray,
								Workspace* workspace,
								bool suppressAlign)
	: Named<GroupDragTool, sGroupDropTool>(mousePart,Vector3(),partArray,workspace)
{
	capture();
		
	dragging = true;
	megaDragger->startDragging();
	if (!suppressAlign)
		megaDragger->alignAndCleanParts();
	else
		megaDragger->cleanParts();
	lastHit = mousePart->getLocation().translation;

}

/*override*/ shared_ptr<MouseCommand> GroupDropTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(0);
	return shared_from(this);
}

shared_ptr<MouseCommand> GroupDropTool::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	Super::onMouseUp(inputObject);
	releaseCapture();
	return shared_ptr<MouseCommand>();
}



shared_ptr<MouseCommand> GroupDropTool::onKeyDown(const shared_ptr<InputObject>& inputObject)
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


shared_ptr<MouseCommand> GroupDropTool::onCancelOperation()
{
	megaDragger->removeParts();
	dragging = false;
	//Cancel the operation by removing the part from the workspace
	releaseCapture();
	return shared_ptr<MouseCommand>();
}

GroupDropTool::~GroupDropTool()
{
}


} // namespace
