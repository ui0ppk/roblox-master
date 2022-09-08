/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Mouse.h"
#include "V8DataModel/DataModel.h"
#include "V8Tree/Instance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/InputObject.h"
#include "Util/TextureId.h"
#include "rbx/signal.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/MouseCommand.h"


namespace RBX {

class PartInstance;
class PVInstance;

extern const char *const sPluginMouse;

class PluginMouse : public DescribedNonCreatable<PluginMouse, Mouse, sPluginMouse>
{
private:

public:
	PluginMouse();
	~PluginMouse();

	void fireDragEnterEvent(shared_ptr<const RBX::Instances> instances, shared_ptr<InputObject> input);

	rbx::signal<void(shared_ptr<const Instances>)> dragEnterEventSignal;
};


} // namespace
