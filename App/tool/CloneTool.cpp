/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/CloneTool.h"
#include "Tool/PartDragTool.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "Util/SoundService.h"

namespace RBX {

const char* const sCloneTool = "Clone";

CloneTool::CloneTool(Workspace* workspace)
	: Named<MouseCommand, sCloneTool>(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "CloneTool created: %p", this);	
}

CloneTool::~CloneTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "CloneTool destroyed: %p", this);	
}

void CloneTool::onMouseIdle(const shared_ptr<InputObject>& inputObject)


{
	clonePart = shared_from(getUnlockedPartByLocalCharacter(inputObject));
}


shared_ptr<MouseCommand> CloneTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if (clonePart.get()) {
		//Vector3 oldPosition = clonePart->getCoordinateFrame().translation;
		
		//Shouldn't be able to clone something you can't create in a script
		if(shared_ptr<PartInstance> newPart = shared_polymorphic_downcast<PartInstance>(clonePart->clone(ScriptingCreator))){

			Instances instances;
			instances.push_back(newPart);
			workspace->insertInstances(instances, workspace, INSERT_TO_TREE, SUPPRESS_PROMPTS);	// only move up for safety

			ServiceProvider::create<Soundscape::SoundService>(workspace)->playSound(PING_SOUND);

			shared_ptr<PartDragTool> partTool = Creatable<MouseCommand>::create<PartDragTool>(	newPart.get(),
														newPart->getCoordinateFrame().translation,
														workspace,
														shared_ptr<Instance>());
			return partTool->onMouseDown(inputObject);
		}
	}
	return shared_ptr<MouseCommand>();
}


const std::string CloneTool::getCursorName() const
{
	if (clonePart) {
		return "CloneOverCursor";
	}
	else {
		return "CloneCursor";
	}
}

} // namespace
