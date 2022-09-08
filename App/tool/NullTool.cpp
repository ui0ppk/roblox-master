/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/NullTool.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/UserController.h"
#include "Util/UserInputBase.h"
#include "Util/NavKeys.h"
#include "V8DataModel/ClickDetector.h"
#include "v8datamodel/UserInputService.h"
#include "Network/Players.h"

FASTFLAGVARIABLE(UseFixedRightMouseClickBehaviour, true)

namespace RBX {

const char* const sNewNullTool = "NewNullTool";

NewNullTool::NewNullTool(Workspace* workspace) 
	: Named<MouseCommand, sNewNullTool>(workspace)
	, cursor("ArrowCursor")
	, hasWaypoint(false)
{
	if (RBX::ServiceProvider::find<UserInputService>(workspace))
	{
		shared_ptr<InputObject> mousePos = Creatable<Instance>::create<InputObject>(InputObject::TYPE_MOUSEIDLE, InputObject::INPUT_STATE_CHANGE, Vector3::zero(), Vector3::zero(), DataModel::get(workspace));
		onMouseIdle(mousePos);
	}
}



NewNullTool::~NewNullTool()
{}

bool NewNullTool::isInFirstPerson()
{
	// check to see if we're in first person or not
	if (ModelInstance* character = RBX::Network::Players::findLocalCharacter(workspace))
	{
		if (Humanoid* humanoid = Humanoid::modelIsCharacter(character))
		{
			if (PartInstance *head = humanoid->getHeadSlow())
			{
				if(head->getLocalTransparencyModifier() >= 0.99f)
				{
					// when the character's head is transparency = 1, means he's transparent due to close camera
					// = in fps
					return true;
				}
			}
		}
	}
	return false;
}

void NewNullTool::getIndicatedPart(const shared_ptr<InputObject>& inputObject,
		const bool& raiseClickEvent, PartInstance** instance,
		bool* clickable, Vector3* waypoint)
{
    FilterInvisibleNonColliding invisibleNonColliding;
    RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(DataModel::get(workspace));
    *instance = getPartByLocalCharacter(workspace, inputObject, &invisibleNonColliding, *waypoint);
    *clickable = ClickDetector::isClickable(shared_from(*instance), distanceToCharacter(*waypoint), raiseClickEvent,localPlayer);
}

void NewNullTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(inputObject->getUserInputType() == InputObject::TYPE_MOUSEIDLE);

	PartInstance* foundPart;
	bool clickable;
	getIndicatedPart(inputObject, false /*click event*/,
			&foundPart, &clickable, &waypoint);

	UserInputService* userInputService = RBX::ServiceProvider::find<UserInputService>(workspace);

	if(foundPart && clickable)
	{
		cursor = "DragCursor";
	}
	else if (!MouseCommand::isAdvArrowToolEnabled())
	{
		if (userInputService)
		{
			cursor = userInputService->getDefaultMouseCursor(false, true).toString();
		}
	}
	else 
	{
		shared_ptr<Network::Player> player = Network::Players::findAncestorPlayer(foundPart);
		RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(DataModel::get(workspace));

		if (localPlayer || player || RBX::Network::Players::serverIsPresent(DataModel::get(workspace)))
		{
			if (userInputService)
			{
				cursor = userInputService->getDefaultMouseCursor(false, true).toString();
			}
		}
		else
		{
			if (userInputService)
			{
				cursor = userInputService->getDefaultMouseCursor(true, true).toString();
			}
		}
	}
}

void NewNullTool::updateClickDetectorHover(const shared_ptr<InputObject>& inputObject)
{
	if( RBX::Network::Player* player = RBX::Network::Players::findLocalPlayer(DataModel::get(workspace)) )
	{
		if(PartInstance* newHoverPart = getPartByLocalCharacter(workspace, inputObject))
		{
			if(newHoverPart != lastHoverPart.get()) // we were hovering on another part, tell that part we stopped hovering
			{
				ClickDetector::stopHover(lastHoverPart, player);
				ClickDetector::isHovered(newHoverPart, distanceToCharacter(newHoverPart->getTranslationUi()), true, player);

				lastHoverPart = shared_from(newHoverPart);
			}
		}
		else if (lastHoverPart) // we were hovering, but now we can't find a part, stop hovering
		{
			ClickDetector::stopHover(lastHoverPart, player);
			lastHoverPart = shared_ptr<PartInstance>();
		}
	}
}

// i.e. - mouse move
void NewNullTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(inputObject->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT);

    if (!FFlag::UseFixedRightMouseClickBehaviour)
    {
	if(rightMouseClickPart)
	{
		PartInstance* potentialNewRightMousePart = getPartByLocalCharacter(workspace, inputObject);
		if(potentialNewRightMousePart != rightMouseClickPart.get()) // if we move the right mouse over another object in the middle of click, we reset rightmouse object
			rightMouseClickPart = shared_ptr<PartInstance>();
	}
    }

	updateClickDetectorHover(inputObject);
	
	PartInstance* foundPart;
	bool clickable;
	getIndicatedPart(inputObject, false /*click event*/,
			&foundPart, &clickable, &waypoint);

	UserInputService* userInputService = RBX::ServiceProvider::find<UserInputService>(workspace);

	if(foundPart && clickable)
	{
		cursor = "DragCursor";
	}
	else if (!MouseCommand::isAdvArrowToolEnabled())
	{
		if (userInputService)
		{
			cursor = userInputService->getDefaultMouseCursor(false, true).toString();
		}
	}
	else
	{
		RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(DataModel::get(workspace));
		shared_ptr<Network::Player> player = Network::Players::findAncestorPlayer(foundPart);

		if (localPlayer || player || RBX::Network::Players::serverIsPresent(DataModel::get(workspace)))
		{
			if (userInputService)
			{
				cursor = userInputService->getDefaultMouseCursor(false, true).toString();;
			}
		}
		else if (userInputService)
		{
			cursor = userInputService->getDefaultMouseCursor(true, true).toString();;
		}
	}
		
	hasWaypoint = false;
}

shared_ptr<MouseCommand> NewNullTool::onRightMouseDown(const shared_ptr<InputObject>& inputObject)
{
    if (!FFlag::UseFixedRightMouseClickBehaviour)
    {
	if(PartInstance* part = getPartByLocalCharacter(workspace, inputObject))
		rightMouseClickPart = shared_from(part);
    }
	return Super::onRightMouseDown(inputObject);
}

shared_ptr<MouseCommand> NewNullTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	PartInstance* foundPart;
	bool clickable;
	Vector3 hitPoint;
	getIndicatedPart(inputObject, true /*click event*/,
			&foundPart, &clickable, &hitPoint);
	if (clickable) 
		return shared_from(this); // no click to move on a clickable brick

	return shared_from(this);
}

shared_ptr<MouseCommand> NewNullTool::onRightMouseUp(const shared_ptr<InputObject>& inputObject)
{
    if (!FFlag::UseFixedRightMouseClickBehaviour)
    {
	if(rightMouseClickPart)
	{
		RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(workspace);
		bool clickable = ClickDetector::isClickable(rightMouseClickPart, distanceToCharacter(rightMouseClickPart->getTranslationUi()), true, localPlayer);
		rightMouseClickPart = shared_ptr<PartInstance>();
		if(clickable)
			return shared_from(this);
	}
	return Super::onRightMouseUp(inputObject);
}
    else
    {
        PartInstance* foundPart;
        bool clickable;
        Vector3 hitPoint;
        getIndicatedPart(inputObject, true /*click event*/,
                         &foundPart, &clickable, &hitPoint);
        if (clickable)
            return shared_from(this); // no click to move on a clickable brick
        return Super::onRightMouseUp(inputObject);
    }
}

void NewNullTool::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);
}

} // namespace
