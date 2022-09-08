#include "stdafx.h"

#include "V8Datamodel/DebrisService.h"
#include "V8Datamodel/TimerService.h"
#include "util/standardout.h"

const char* const RBX::sDebrisService = "Debris";

using namespace RBX;

REFLECTION_BEGIN();
static Reflection::PropDescriptor<DebrisService, int> prop_MaxItems("MaxItems", category_Data, &DebrisService::getMaxItems, &DebrisService::setMaxItems);
static Reflection::BoundFuncDesc<DebrisService, void(shared_ptr<Instance>, double)> func_AddItem(&DebrisService::addItem, "AddItem", "item", "lifetime", 10, Security::None);
static Reflection::BoundFuncDesc<DebrisService, void(shared_ptr<Instance>, double)> func_AddItem_deprecated(&DebrisService::addItem, "addItem", "item", "lifetime", 10, Security::None);

static Reflection::BoundFuncDesc<DebrisService, void(bool)> func_LegacyMaxItems(&DebrisService::setLegacyMaxItems, "SetLegacyMaxItems", "enabled", Security::LocalUser);
REFLECTION_END();

DebrisService::DebrisService(void)
	:DescribedNonCreatable<DebrisService, Instance, sDebrisService>(sDebrisService)
	,maxItems(1000)
	,legacyMaxItems(false)
{
}

static void cleanup(weak_ptr<Instance> item)
{
	if (shared_ptr<Instance> i = item.lock())
		try
		{
			i->destroy();
		}
		catch (RBX::base_exception& e)
		{
			StandardOut::singleton()->print(MESSAGE_WARNING, e);
		}
}

void DebrisService::cleanup()
{
	while ((int)queue.size() > maxItems)
	{
		::cleanup(queue.front());
		queue.pop();
	}
}


void DebrisService::addItem(shared_ptr<Instance> item, double lifetime)
{
	if (!item)
		return;

	// Make sure the current context has the security credentials high
	// enough to act on the item.
	item->securityCheck();

	if (timer)
		timer->delay(boost::bind(&::cleanup, weak_ptr<Instance>(item)), lifetime);

	queue.push(item);
	cleanup();
}

void DebrisService::setLegacyMaxItems(bool value)
{
	legacyMaxItems = value;
}
void DebrisService::setMaxItems(int value)
{
	if(!legacyMaxItems){
		RBX::Security::Context::current().requirePermission(RBX::Security::LocalUser, "DebrisService MaxItems is restricted");
	}

	if (value!=maxItems)
	{
		if (value<0)
			throw std::runtime_error("DebrisService MaxItems must be greater than 0");
		maxItems = value;
		raiseChanged(prop_MaxItems);
		cleanup();
	}
}


void DebrisService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	while ((int)queue.size()>0)
	{
		::cleanup(queue.front());
		queue.pop();
	}

	Super::onServiceProvider(oldProvider, newProvider);

	timer = shared_from(ServiceProvider::create<TimerService>(newProvider));
}
