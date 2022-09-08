/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/ModelInstance.h"

namespace RBX {

	extern const char* const sStatusInstance;
	class StatusInstance 
		: public DescribedCreatable<StatusInstance, ModelInstance, sStatusInstance, Reflection::ClassDescriptor::INTERNAL>
	{
	private:
		typedef DescribedCreatable<StatusInstance, ModelInstance, sStatusInstance, Reflection::ClassDescriptor::INTERNAL> Super;

	public:
		StatusInstance();

	protected:
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ bool askForbidParent(const Instance* instance) const { return !askSetParent(instance); }
	};

} // namespace
