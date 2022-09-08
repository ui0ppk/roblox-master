/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/AdvDragTool.h"
#include "V8DataModel/PartInstance.h"
#include "Tool/PartDragTool.h"
#include "Tool/GroupDragTool.h"
#include "Tool/AdvLuaDragTool.h"

namespace RBX {


shared_ptr<MouseCommand> AdvDragTool::onMouseDown(PartInstance* hitPart,
									const Vector3& hitWorld,
									const std::vector<Instance*>& dragInstances,
									const shared_ptr<InputObject>& inputObject, 
									Workspace* workspace,
									shared_ptr<Instance> selectIfNoDrag)
{
	RBXASSERT(hitPart);

	PartArray partArray;
	DragUtilities::instancesToParts(dragInstances, partArray);

	shared_ptr<MouseCommand> advLuaDragTool = Creatable<MouseCommand>::create<AdvLuaDragTool>( hitPart, 
												hitWorld, 
												partArray, 
												workspace,
												selectIfNoDrag);
	return advLuaDragTool->onMouseDown(inputObject);
}


} // namespace
