/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"

namespace RBX {

	extern const char* const sConfiguration;
	class Configuration
		: public DescribedCreatable<Configuration, Instance, sConfiguration>
	{
	private:
		typedef DescribedCreatable<Configuration, Instance, sConfiguration> Super;
	public:
		Configuration();

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askForbidChild(const Instance* instance) const;
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	};
}