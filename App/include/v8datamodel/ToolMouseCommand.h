/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/ScriptMouseCommand.h"

namespace RBX {

// A generic mouse command associated with a Tool - fires events to Tool for activation, etc.

	class Tool;
	class Mouse;

	extern const char* const sToolMouseCommand;
	class ToolMouseCommand : public Named<ScriptMouseCommand, sToolMouseCommand>
	{
	private:
		typedef Named<ScriptMouseCommand, sToolMouseCommand> Super;
		shared_ptr<Tool> tool;
		rbx::signals::scoped_connection toolUnequipped;	

		bool mouseIsDown;

		void onEvent_ToolUnequipped();

		void updateTargetPoint(const shared_ptr<InputObject>& inputObject, bool replicate);
		
		bool tryClickable(const shared_ptr<InputObject>& inputObject, shared_ptr<PartInstance> part);

	public:
		ToolMouseCommand(Workspace* workspace, Tool* tool);

		~ToolMouseCommand()
		{
		}

		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onRightMouseDown(const shared_ptr<InputObject>& inputObject);

		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject); 

		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject); 

		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject); 
		/*override*/ shared_ptr<MouseCommand> onRightMouseUp(const shared_ptr<InputObject>& inputObject); 

	};

} // namespace