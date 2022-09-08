/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Hopper.h"
#include "Script/IScriptFilter.h"

namespace RBX {

	extern const char *const sBackpack;
	class Backpack 
		: public DescribedCreatable<Backpack, Hopper, sBackpack>
		, public IScriptFilter
	{
	private:
		// IScriptOwner
		/*override*/ bool scriptShouldRun(BaseScript* script);

	public:
		Backpack();
	};


}	// namespace 