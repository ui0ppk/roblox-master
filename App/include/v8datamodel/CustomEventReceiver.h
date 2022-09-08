#pragma once

#include "rbx/signal.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8Tree/Instance.h"

namespace RBX {

class CustomEvent;

extern const char* const sCustomEventReceiver;
class CustomEventReceiver
	: public DescribedCreatable<CustomEventReceiver, Instance, sCustomEventReceiver> {
private:
	typedef DescribedCreatable<CustomEventReceiver, Instance, sCustomEventReceiver> Super;
	
	// prevent copy and assign: this class does sensitive pointer
	// management, which would be complicated by allowing copies/assigns.
	CustomEventReceiver(const CustomEventReceiver& other);
	CustomEventReceiver& operator=(const CustomEventReceiver& other);

	weak_ptr<CustomEvent> sourceEvent;
	rbx::signals::scoped_connection sourceValueChangedConnection;
	float lastReceivedValue;

public:
	// public for interoperation with CustomEvent
	rbx::signal<void(float)> sourceValueChanged;
	// connection signals public for testing
	rbx::signal<void(shared_ptr<Instance>)> eventConnected;
	rbx::signal<void(shared_ptr<Instance>)> eventDisconnected;

	CustomEventReceiver()
		:Super(sCustomEventReceiver),
		lastReceivedValue(0)
	{
		sourceValueChangedConnection = sourceValueChanged.connect(
				boost::bind(&CustomEventReceiver::setCurrentValue, this, _1));
	}

	static Reflection::RefPropDescriptor<CustomEventReceiver, Instance> prop_Source;
	static Reflection::EventDesc<CustomEventReceiver, void(float)> event_SourceValueChanged;
	static Reflection::BoundFuncDesc<CustomEventReceiver, float()> func_GetCurrentValue;
	static Reflection::EventDesc<CustomEventReceiver, void(shared_ptr<Instance>)> event_EventConnected;
	static Reflection::EventDesc<CustomEventReceiver, void(shared_ptr<Instance>)> event_EventDisconnected;

	/*override*/ bool askSetParent(const Instance* instance) const {
		return Instance::fastDynamicCast<PartInstance>(instance) != NULL;
	}

	/*override*/ bool askForbidChild(const Instance* instance) const {
		return true;
	}

	// needs to be forward declared because it depends on CustomEvent
	/*override*/// virtual void onAncestorChanged(const AncestorChanged& event);

	// should only be used for serialization
	Instance* const getSource() const {
		return (Instance*) sourceEvent.lock().get();
	}
	// should only be used for serialization
	void setSource(Instance* sourceEvent);

	void setCurrentValue(float newValue) {
		RBXASSERT(newValue != lastReceivedValue);
		lastReceivedValue = newValue;
	}

	float getCurrentValue() {
		return lastReceivedValue;
	}

protected:
	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
};

}