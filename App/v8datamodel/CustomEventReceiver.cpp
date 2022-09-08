#include "stdafx.h"

#include "V8DataModel/CustomEventReceiver.h"
#include "V8DataModel/CustomEvent.h"
#include "V8DataModel/CollectionService.h"
#include "reflection/reflection.h"

namespace RBX {

const char* const sCustomEventReceiver = "CustomEventReceiver";
REFLECTION_BEGIN();
Reflection::RefPropDescriptor<CustomEventReceiver, Instance>
CustomEventReceiver::prop_Source("Source", category_Data,
		&CustomEventReceiver::getSource, &CustomEventReceiver::setSource,
		Reflection::PropertyDescriptor::STANDARD, Security::None);

Reflection::EventDesc<CustomEventReceiver, void(float)>
CustomEventReceiver::event_SourceValueChanged(
		&CustomEventReceiver::sourceValueChanged, "SourceValueChanged", "newValue",
		Security::None);

Reflection::BoundFuncDesc<CustomEventReceiver, float()>
CustomEventReceiver::func_GetCurrentValue(
		&CustomEventReceiver::getCurrentValue, "GetCurrentValue", Security::None);

Reflection::EventDesc<CustomEventReceiver, void(shared_ptr<Instance>)>
CustomEventReceiver::event_EventConnected(
		&CustomEventReceiver::eventConnected, "EventConnected", "event",
		Security::None);

Reflection::EventDesc<CustomEventReceiver, void(shared_ptr<Instance>)>
CustomEventReceiver::event_EventDisconnected(
		&CustomEventReceiver::eventDisconnected, "EventDisconnected", "event",
		Security::None);
REFLECTION_END();

void CustomEventReceiver::setSource(Instance* sourceEvent) {
	shared_ptr<CustomEvent> currentSourceEvent = this->sourceEvent.lock();
	if (sourceEvent == currentSourceEvent.get()) {
		return;
	}

	// disconnect from old sourceEvent if this was connected before
	if (currentSourceEvent.get() != NULL) {
		currentSourceEvent->removeReceiver(this);
		this->sourceEvent.reset();
		// send event only after internal state has been updated
		eventDisconnected(currentSourceEvent);
	}

	// connect to the new sourceEvent if it is not NULL
	if (sourceEvent != NULL) {
		CustomEvent* castSource = Instance::fastDynamicCast<CustomEvent>(sourceEvent);
		debugAssert(castSource != NULL);
		if (castSource != NULL) {
			castSource->addReceiver(this);
			this->sourceEvent = shared_from(castSource);
			if (getCurrentValue() != castSource->getPersistedCurrentValue()) {
				sourceValueChanged(castSource->getPersistedCurrentValue());
			}
			// send event only after internal state is updated
			eventConnected(shared_from(castSource));
		}
	}

	if (sourceEvent == NULL && getCurrentValue() != 0) {
		sourceValueChanged(0);
	}

	raisePropertyChanged(prop_Source);
}

void CustomEventReceiver::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(oldProvider)
		oldProvider->create<CollectionService>()->removeInstance(shared_from(this));

	Super::onServiceProvider(oldProvider, newProvider);

	if(newProvider) {
		newProvider->create<CollectionService>()->addInstance(shared_from(this));
	} else {
		setSource(NULL);
	}
}


} // namespace RBX


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag4 = 0;
    };
};
