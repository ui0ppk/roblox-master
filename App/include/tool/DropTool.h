/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

/*
	// End user tools
	
	// Drop tool is like a drag tool, except it knows how to cancel itself, and has a different behavior on mouseUp
	
*/

namespace RBX {

	class Workspace;
	class PartInstance;
	class PVInstance;

	class DropTool
	{
	public:
		static shared_ptr<MouseCommand> createDropTool(const Vector3& hitWorld,
											const std::vector<Instance*>& dragInstances,
											Workspace* workspace,
											shared_ptr<Instance> selectIfNoDrag,
											bool suppressPartsAlign = false);

	};

} // namespace RBX