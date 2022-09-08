/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/AxisMoveTool.h"

namespace RBX {

	extern const char* const sAxisRotateTool;
	class AxisRotateTool : public Named<AxisToolBase, sAxisRotateTool>
	{
	private:
		/*override*/ Color3 getHandleColor() const {return Color3::green();}
		/*override*/ HandleType getDragType() const {return HANDLE_ROTATE;}

	public:
		AxisRotateTool(Workspace* workspace) : Named<AxisToolBase, sAxisRotateTool>(workspace)
		{}

		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<AxisRotateTool>(workspace);}
	};



} // namespace RBX