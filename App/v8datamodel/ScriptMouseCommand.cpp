/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ScriptMouseCommand.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Mouse.h"
#include "Network/Players.h"
#include "Network/Player.h"


namespace RBX {

ScriptMouseCommand::ScriptMouseCommand(Workspace* workspace)
	:MouseCommand(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "ScriptMouseCommand created: %p", this);
	mouse = Creatable<Instance>::create<Mouse>();
	mouse->setWorkspace(workspace);
}


ScriptMouseCommand::~ScriptMouseCommand()
{
	FASTLOG1(FLog::MouseCommandLifetime, "ScriptMouseCommand destroyed: %p", this);
	mouse->setWorkspace(NULL);
}


TextureId ScriptMouseCommand::getCursorId() const 
{
	return mouse->getIcon();
}

shared_ptr<MouseCommand> ScriptMouseCommand::onMouseDown(const shared_ptr<InputObject>& inputObject) 
{
	mouse->update(inputObject);
	return shared_from(this);
}

void ScriptMouseCommand::onMouseHover(const shared_ptr<InputObject>& inputObject) 
{
	mouse->update(inputObject);
}

void ScriptMouseCommand::onMouseIdle(const shared_ptr<InputObject>& inputObject) 
{
	mouse->update(inputObject);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onMouseWheelForward(const shared_ptr<InputObject>& inputObject)
{
	mouse->update(inputObject);
	return shared_from(this);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onMouseWheelBackward(const shared_ptr<InputObject>& inputObject)
{
	mouse->update(inputObject);
	return shared_from(this);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onRightMouseDown(const shared_ptr<InputObject>& inputObject)
{
	mouse->update(inputObject);
	return shared_from(this);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onRightMouseUp(const shared_ptr<InputObject>& inputObject)
{
	mouse->update(inputObject);
	return shared_from(this);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onMouseUp(const shared_ptr<InputObject>& inputObject) 
{
	mouse->update(inputObject);
	return shared_from(this);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onPeekKeyDown(const shared_ptr<InputObject>& inputObject) 
{
	mouse->update(inputObject);
	return shared_from(this);
}

shared_ptr<MouseCommand> ScriptMouseCommand::onPeekKeyUp(const shared_ptr<InputObject>& inputObject) 
{
	mouse->update(inputObject);
	return shared_from(this);
}

const Name& ScriptMouseCommand::getName() const 
{
	// Just use the name of Mouse object
	return Mouse::className();
}

} // namespace
