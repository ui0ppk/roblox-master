/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/ScriptMouseCommand.h"

namespace RBX {

// A generic mouse command associated with a Tool - fires events to Tool for activation, etc.

	class StudioTool;
	class PMouse;

	extern const char *const sStudioToolMouseCommand;
	class StudioToolMouseCommand : public Named<ScriptMouseCommand, sStudioToolMouseCommand>
	{
	private:
		typedef Named<ScriptMouseCommand, sStudioToolMouseCommand> Super;
		shared_ptr<StudioTool> tool;
		rbx::signals::scoped_connection toolUnequipped;	

		void onEvent_ToolUnequipped();
	public:
		StudioToolMouseCommand(Workspace* workspace, shared_ptr<StudioTool> tool);
		~StudioToolMouseCommand();

		const StudioTool* getStudioTool() const { return tool.get(); }
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject); 
	};

} // namespace