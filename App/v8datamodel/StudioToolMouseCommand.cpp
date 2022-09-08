/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/StudioToolMouseCommand.h"
#include "V8DataModel/StudioTool.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/PartInstance.h"
#include "Humanoid/Humanoid.h"

#include "util/standardout.h"

namespace RBX {

const char *const sStudioToolMouseCommand = "StudioToolMouseCommand";

StudioToolMouseCommand::StudioToolMouseCommand(Workspace* workspace, shared_ptr<StudioTool> studioTool)
	: Named<ScriptMouseCommand, sStudioToolMouseCommand>(workspace)
	,tool(studioTool)
{
	toolUnequipped = tool->unequippedSignal.connect(boost::bind(&StudioToolMouseCommand::onEvent_ToolUnequipped, this));
}
StudioToolMouseCommand::~StudioToolMouseCommand()
{
	if(toolUnequipped.connected()){
		tool->unequip();
		RBXASSERT(!toolUnequipped.connected());
	}
}

void StudioToolMouseCommand::onEvent_ToolUnequipped()
{
	toolUnequipped.disconnect();
}


shared_ptr<MouseCommand> StudioToolMouseCommand::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	tool->activate();

	if (toolUnequipped.connected()) {
		return Super::onMouseDown(inputObject);
	}
	else {
		return shared_ptr<MouseCommand>();		// will reset to default
	}
}

shared_ptr<MouseCommand> StudioToolMouseCommand::onMouseUp(const shared_ptr<InputObject>& inputObject) 
{
	tool->deactivate();

	return Super::onMouseUp(inputObject);
}

} // namespace
