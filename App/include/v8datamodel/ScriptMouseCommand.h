/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX {

class Mouse;

// A command to interface the generic, scriptable HopperBin to the RBX command architecture
class ScriptMouseCommand : public MouseCommand
{
private:
	shared_ptr<Mouse> mouse;		// A scriptable representation of the Mouse

public:
	ScriptMouseCommand(Workspace* workspace);
	~ScriptMouseCommand();

	shared_ptr<Mouse> getMouse() {return mouse;}

	virtual TextureId getCursorId() const;

	/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);

	/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);

	/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		
	/*override*/ shared_ptr<MouseCommand> onMouseWheelForward(const shared_ptr<InputObject>& inputObject);
	/*override*/ shared_ptr<MouseCommand> onMouseWheelBackward(const shared_ptr<InputObject>& inputObject);

	/*override*/ shared_ptr<MouseCommand> onRightMouseDown(const shared_ptr<InputObject>& inputObject);
	/*override*/ shared_ptr<MouseCommand> onRightMouseUp(const shared_ptr<InputObject>& inputObject);

	/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);

	/*override*/ shared_ptr<MouseCommand> onPeekKeyDown(const shared_ptr<InputObject>& inputObject);
	/*override*/ shared_ptr<MouseCommand> onPeekKeyUp(const shared_ptr<InputObject>& inputObject);

	/*override*/ const Name& getName() const;
};

} // namespace