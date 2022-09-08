#include "stdafx.h"

#include "V8DataModel/StudioTool.h"

#include "V8DataModel/StudioToolMouseCommand.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "Network/Players.h"

namespace RBX
{

const char *const sStudioTool = "StudioTool";

REFLECTION_BEGIN();
static Reflection::EventDesc<StudioTool, void(shared_ptr<Instance>)> event_Equipped(&StudioTool::equippedSignal, "Equipped", "mouse");
static Reflection::EventDesc<StudioTool, void()> event_Unequipped(&StudioTool::unequippedSignal, "Unequipped");
static Reflection::EventDesc<StudioTool, void()> event_Activated(&StudioTool::activatedSignal, "Activated");
static Reflection::EventDesc<StudioTool, void()> event_Deactivated(&StudioTool::deactivatedSignal, "Deactivated");
static Reflection::PropDescriptor<StudioTool, bool> prop_Enabled("Enabled", "State", &StudioTool::getEnabled, &StudioTool::setEnabled);
REFLECTION_END();

StudioTool::StudioTool()
	:DescribedNonCreatable<StudioTool, Instance, sStudioTool>(sStudioTool)
	,enabled(false)
{}

void StudioTool::setEnabled(bool value)
{
	if(enabled != value){
		enabled = value;
		raisePropertyChanged(prop_Enabled);
	}
}

void StudioTool::activate()
{
	activatedSignal();
}
void StudioTool::deactivate()
{
	deactivatedSignal();
}

shared_ptr<Mouse> StudioTool::onEquipping(Workspace* workspace)
{
	shared_ptr<StudioToolMouseCommand> studioToolMouseCommand = Creatable<MouseCommand>::create<StudioToolMouseCommand>(workspace, shared_from(this));
	workspace->setMouseCommand(studioToolMouseCommand);
	return studioToolMouseCommand->getMouse();
}
void StudioTool::equip(Workspace* workspace)
{
	equippedSignal(onEquipping(workspace));
}

void StudioTool::unequip()
{
	unequippedSignal();
}

}
