/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ToolMouseCommand.h"
#include "V8DataModel/Tool.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ClickDetector.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"


#include "util/standardout.h"

FASTFLAG(UseFixedRightMouseClickBehaviour)

namespace RBX {

const char* const sToolMouseCommand = "ToolMouseCommand";

ToolMouseCommand::ToolMouseCommand(Workspace* workspace, Tool* tool)
	: Named<ScriptMouseCommand, sToolMouseCommand>(workspace)
	,tool(shared_from<Tool>(tool))
	,mouseIsDown(false)
{
	if (tool) {
		toolUnequipped = tool->unequippedSignal.connect(boost::bind(&ToolMouseCommand::onEvent_ToolUnequipped, this));
	}

}

void ToolMouseCommand::onEvent_ToolUnequipped()
{
	toolUnequipped.disconnect();
}

void ToolMouseCommand::updateTargetPoint(const shared_ptr<InputObject>& inputObject, bool replicate)
{
	Vector3 hitWorld;
	if (Humanoid* h = Humanoid::getLocalHumanoidFromContext(tool.get()))
	{
		if (!getPartByLocalCharacter(workspace, inputObject, NULL, hitWorld)) 
		{
			RBX::RbxRay ray = getSearchRay(inputObject);
			PartInstance* head = h->getHeadSlow();
			hitWorld = head ? head->getCoordinateFrame().translation : ray.origin();
			hitWorld += ray.direction();
		}
		if (replicate)
			h->setTargetPoint(hitWorld);
		else
			h->setTargetPointLocal(hitWorld);
	}
}

shared_ptr<MouseCommand> ToolMouseCommand::onRightMouseDown(const shared_ptr<InputObject>& inputObject)
{
	return Super::onRightMouseDown(inputObject);
}

shared_ptr<MouseCommand> ToolMouseCommand::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if(inputObject->isCtrlEvent())
	{
		if(PartInstance* part = getPartByLocalCharacter(workspace, inputObject))
			tryClickable(inputObject,shared_from(part));
		return Super::onMouseUp(inputObject);
	}

	mouseIsDown = true;
    
    // make sure the mouse has been updated when we fire this event (touch devices may not of updated recently)
    getMouse()->cacheInputObject(inputObject);

	// These 2 lines needs to be in this order for proper replication,
	// propertyChanged item are added to the front of pending items list if the property belongs to player
	// items are sent from front to back
    //  we need to also update the target point before activate, but only locally (touch platforms need to ensure we have right target point before update)
    updateTargetPoint(inputObject, false);
    
	tool->activate();
	updateTargetPoint(inputObject, mouseIsDown);

	if (toolUnequipped.connected())
		return Super::onMouseDown(inputObject);
	else
		return shared_ptr<MouseCommand>();		// will reset to default
}

void ToolMouseCommand::onMouseHover(const shared_ptr<InputObject>& inputObject) 
{
    if (!FFlag::UseFixedRightMouseClickBehaviour)
    {
        if(rightMouseClickPart)
        {
            PartInstance* potentialNewRightMousePart = getPartByLocalCharacter(workspace, inputObject);
            if(rightMouseClickPart && (potentialNewRightMousePart != rightMouseClickPart.get())) // if we move the right mouse over another object in the middle of click, we reset rightmouse object
                rightMouseClickPart = shared_ptr<PartInstance>();
        }
    }

	updateTargetPoint(inputObject, mouseIsDown);

	return Super::onMouseHover(inputObject);
}

void ToolMouseCommand::onMouseIdle(const shared_ptr<InputObject>& inputObject) 
{
	updateTargetPoint(inputObject, mouseIsDown);

	return Super::onMouseIdle(inputObject);
}

bool ToolMouseCommand::tryClickable(const shared_ptr<InputObject>& inputObject, shared_ptr<PartInstance> part)
{
	bool clickable = false;
	if(part)
	{
		RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(tool.get());
		clickable = ClickDetector::isClickable(part, distanceToCharacter(part->getTranslationUi()), true, localPlayer);
	}

	return clickable;
}

shared_ptr<MouseCommand> ToolMouseCommand::onRightMouseUp(const shared_ptr<InputObject>& inputObject)
{
	return Super::onRightMouseUp(inputObject);
}

shared_ptr<MouseCommand> ToolMouseCommand::onMouseUp(const shared_ptr<InputObject>& inputObject) 
{
	mouseIsDown = false;
	// These 2 lines needs to be in this order for proper replication,
	// propertyChanged item are added to the front of pending items list if the property belongs to player
	// items are sent from front to back 
	tool->deactivate();
	updateTargetPoint(inputObject, true);

	return Super::onMouseUp(inputObject);
}

} // namespace
