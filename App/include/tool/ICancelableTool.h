/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX
{
	class ICancelableTool
	{

	public:
		virtual shared_ptr<MouseCommand> onCancelOperation() = 0;

	};


} //namespace