/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/GameTool.h"

#include "Tool/RunDragger.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "Tool/PartDragTool.h"
#include "v8datamodel/UserInputService.h"
#include "Util/Sound.h"

namespace RBX {

const char* const sGameTool = "GameTool";

GameTool::GameTool(Workspace* workspace) 
	: Named<MouseCommand, sGameTool>(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "GameTool created: %p", this);	
}


void GameTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	onMouseHover(inputObject);
}

bool GameTool::draggablePart(const PartInstance* part, const Vector3& hitPoint) const
{
	return 	(	!part->getPartLocked() 
			&&	!part->lockedInPlace()
			&&	characterCanReach(hitPoint));
}

void GameTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	if (inputObject->getWindowSize().x==0 || inputObject->getWindowSize().y==0)
		return;

	Vector3 hitPoint;
	PartInstance* foundPart = getUnlockedPartByLocalCharacter(inputObject, hitPoint);

	UserInputService* userInputService = RBX::ServiceProvider::find<UserInputService>(workspace);
		
	if (!foundPart) 
	{
		if (userInputService)
		{
			cursor = userInputService->getDefaultMouseCursor(false).toString();
		}
	}
	else if (!draggablePart(foundPart, hitPoint)) 
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


shared_ptr<MouseCommand> GameTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	Vector3 hitPointWorld;
	PartInstance* dragPart = getUnlockedPartByLocalCharacter(inputObject, hitPointWorld);

	if (dragPart && draggablePart(dragPart, hitPointWorld)) 
	{
		shared_ptr<PartDragTool> partTool = Creatable<MouseCommand>::create<PartDragTool>(	dragPart,
													hitPointWorld,
													workspace,
													shared_ptr<Instance>());
		return partTool->onMouseDown(inputObject);
	}

	return shared_ptr<MouseCommand>();
}

GameTool::~GameTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "GameTool destroyed: %p", this);	
}

} // namespace
