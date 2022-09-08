#include "stdafx.h"

#include "V8DataModel/Configuration.h"
#include "V8DataModel/CollectionService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/Value.h"

namespace RBX {

const char* const sConfiguration = "Configuration";

Configuration::Configuration()
	:DescribedCreatable<Configuration, Instance, sConfiguration>()
{
	setName("Configuration");
}


bool Configuration::askForbidChild(const Instance* instance) const
{
	return (dynamic_cast<const IValue*>(instance) == NULL);
}

bool Configuration::askSetParent(const Instance* instance) const
{
	if (!fastDynamicCast<PartInstance>(instance))
		if (instance->getDescriptor() != ModelInstance::classDescriptor())	// ModelInstance, but not Workspace
			return false;
		
	if (instance->getChildren())
	{
		Instances::const_iterator end = instance->getChildren()->end();
		for (Instances::const_iterator iter = instance->getChildren()->begin(); iter != end; ++iter)
		{
			if (fastDynamicCast<Configuration>(iter->get()))
				//There can be only one Configuration object per part
				return false;
		}
	}
	return true;
}


void Configuration::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(oldProvider){
		oldProvider->create<CollectionService>()->removeInstance(shared_from(this));
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if(newProvider){
		newProvider->create<CollectionService>()->addInstance(shared_from(this));
	}
}
	
}
