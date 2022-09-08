/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/GrabTool.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/UserInputService.h"
#include "Tool/DragTool.h"

namespace RBX {

const char* const sGrabTool = "Grab";

GrabTool::GrabTool(Workspace* workspace) 
	: Named<MouseCommand, sGrabTool>(workspace)
	, cursor("ArrowCursor")
{
	FASTLOG1(FLog::MouseCommandLifetime, "GrabTool created: %p", this);
}

void GrabTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	onMouseHover(inputObject);
}

void GrabTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	PartInstance* foundPart = getUnlockedPartByLocalCharacter(inputObject);

	UserInputService* userInputService = RBX::ServiceProvider::find<UserInputService>(workspace);

	if (!foundPart)
	{
		if (userInputService)
		{
			cursor = userInputService->getDefaultMouseCursor(false).toString();
		}
	}
	else if (foundPart->getPartLocked()) 
	{
		if (userInputService)
		{
			cursor = userInputService->getDefaultMouseCursor(true).toString();
		}
	}
	else 
	{
		cursor = "DragCursor";
	}
}

shared_ptr<MouseCommand> GrabTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	Vector3 hitPointWorld;
	if (PartInstance* dragPart = getUnlockedPartByLocalCharacter(inputObject, hitPointWorld)) 
	{
		std::vector<Instance*> dragInstances;

		Instance* topSelectable = getTopSelectable3d(dragPart);
		if( !topSelectable ) return shared_ptr<MouseCommand>();

		dragInstances.push_back(topSelectable);

		return DragTool::onMouseDown(	dragPart, 
										hitPointWorld,
										dragInstances,				// will either be the part, or the model the part is in
										inputObject,
										workspace,
										shared_ptr<Instance>());
	}

	return shared_ptr<MouseCommand>();
}

GrabTool::~GrabTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "GrabTool destroyed: %p", this);
}

} // namespace
