#include "stdafx.h"

#include "V8DataModel/StudioToolVerb.h"
#include "V8DataModel/StudioTool.h"
#include "V8DataModel/StudioToolMouseCommand.h"
#include "V8DataModel/Workspace.h"

namespace RBX
{
	StudioToolVerb::StudioToolVerb(const char* name, StudioTool* studioTool,  Workspace* workspace, bool toggle) 
		: Verb(workspace, name)
		, workspace(workspace)
		, studioTool(studioTool)
		, toggle(toggle)
	{}
	bool StudioToolVerb::isEnabled() const
	{
		return studioTool && studioTool->getEnabled();
	}
	bool StudioToolVerb::isChecked() const
	{
		if(StudioToolMouseCommand* mouseCommand = dynamic_cast<StudioToolMouseCommand*>(workspace->getCurrentMouseCommand())){
			return mouseCommand->getStudioTool() == studioTool;
		}
		return false;
	}

	void StudioToolVerb::doIt(IDataState* dataState)
	{
		if (isChecked() && toggle) {
			workspace->setNullMouseCommand();			// Toggle on / off if already on
		}
		else {
			studioTool->equip(workspace);
		}
	}
}