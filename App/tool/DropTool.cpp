/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/DropTool.h"
#include "V8DataModel/PartInstance.h"
#include "Tool/PartDropTool.h"
#include "Tool/GroupDropTool.h"

namespace RBX {


shared_ptr<MouseCommand> DropTool::createDropTool(const Vector3& hitLocal,
									   const std::vector<Instance*>& dragInstances,
									   Workspace* workspace,
									   shared_ptr<Instance> selectIfNoDrag,
									   bool suppressPartsAlign)
{
	PartArray partArray;
	DragUtilities::instancesToParts(dragInstances, partArray);

	if(partArray.size() == 0){
		return shared_ptr<MouseCommand>();
	}
	PartInstance* hitPart = partArray[0].lock().get();
											
	if (partArray.size() == 1) {
		return Creatable<MouseCommand>::create<PartDropTool>(	hitPart,
									hitLocal,
									workspace,
									selectIfNoDrag);
	}
	else {
		return Creatable<MouseCommand>::create<GroupDropTool>(hitPart, 
								 partArray, 
								 workspace,
								 suppressPartsAlign);
	}
}


} // namespace
