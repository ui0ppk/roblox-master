#pragma once

#include "rbx/signal.h"
#include "V8DataModel/CustomEventReceiver.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/CollectionService.h"
#include "V8Tree/Instance.h"

namespace RBX {

extern const char* const sCustomEvent;
class CustomEvent : public DescribedCreatable<CustomEvent, Instance, sCustomEvent> {
private:
	typedef DescribedCreatable<CustomEvent, Instance, sCustomEvent>  Super;
	typedef std::list<weak_ptr<CustomEventReceiver> > ReceiverList;

	// prevent copy and assign: this class does sensitive pointer
	// management, which would be complicated by allowing copies/assigns.
	CustomEvent(const CustomEvent& other);
	CustomEvent& operator=(const CustomEvent& other);

	ReceiverList receivers;
	float currentValue;

	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		if(oldProvider)
			oldProvider->create<CollectionService>()->removeInstance(shared_from(this));

		Super::onServiceProvider(oldProvider, newProvider);

		if(newProvider) {
			newProvider->create<CollectionService>()->addInstance(shared_from(this));
		} else {
			ReceiverList copy = receivers;
			for (ReceiverList::iterator itr = copy.begin();
					itr != copy.end();
					++itr) {
				shared_ptr<CustomEventReceiver> receiver = itr->lock();
				if (receiver) {
					receiver->setSource(NULL);
				}
			}
		}
	}

public:
	CustomEvent()
		:Super(sCustomEvent),
		currentValue(0)
	{}

	// The persisted current value is just for storage -- it is set from the
	// SetValue callback, and setPersistedCurrentValue _WILL_NOT_ cause
	// receivers to get a value update.
	static Reflection::PropDescriptor<CustomEvent, float> prop_PersistedCurrentValue;
	static Reflection::BoundFuncDesc<CustomEvent, void(float)> func_SetValue;
	static Reflection::BoundFuncDesc<CustomEvent, shared_ptr<const Instances>() > func_GetAttachedReceivers;
	static Reflection::EventDesc<CustomEvent, void(shared_ptr<Instance>)> event_ReceiverConnected;
	static Reflection::EventDesc<CustomEvent, void(shared_ptr<Instance>)> event_ReceiverDisconnected;

	// signals are public for testing only
	rbx::signal<void(shared_ptr<Instance>)> receiverConnected;
	rbx::signal<void(shared_ptr<Instance>)> receiverDisconnected;

	/*override*/ virtual bool askSetParent(const Instance* instance) const {
		return Instance::fastDynamicCast<PartInstance>(instance) != NULL;
	}

	/*override*/ virtual bool askForbidChild(const Instance* instance) const {
		return true;
	}

	float getPersistedCurrentValue() const {
		return currentValue;
	}

	// This method is intended for serialization only, it should not be involved
	// with triggering valueChanged events on receivers.
	void setPersistedCurrentValue(float newValue) {
		currentValue = G3D::clamp(newValue, 0 , 1);
	}

	void setCurrentValue(float newValue) {
		currentValue = G3D::clamp(newValue, 0, 1);
		raiseChanged(prop_PersistedCurrentValue);
		for (ReceiverList::iterator itr = receivers.begin();
				itr != receivers.end();
				++itr) {
			shared_ptr<CustomEventReceiver> receiver = itr->lock();
			if (receiver && receiver->getCurrentValue() != currentValue) {
				receiver->sourceValueChanged(currentValue);
			}
		}
	}

	shared_ptr<const Instances> getAttachedReceivers() {
		shared_ptr<Instances> result(new Instances);

		for (ReceiverList::iterator itr = receivers.begin();
				itr != receivers.end();
				++itr) {
			shared_ptr<CustomEventReceiver> receiver = itr->lock();
			if (receiver) {
				result->push_back(receiver);
			}
		}
		return result;
	}

	void addReceiver(CustomEventReceiver* receiver) {
		bool found = false;
		for (ReceiverList::iterator itr = receivers.begin();
				itr != receivers.end() && !found;
				++itr) {
			shared_ptr<CustomEventReceiver> list_receiver = itr->lock();
			if (list_receiver.get() == receiver) {
				found = true;
			}
		}
		if (!found) {
			receivers.push_back(weak_ptr<CustomEventReceiver>(shared_from(receiver)));
			// send event only after internal state has been updated
			receiverConnected(shared_from(receiver));
		}
	}

	void removeReceiver(CustomEventReceiver* receiver) {
		ReceiverList::iterator foundIterator;
		bool found = false;
		for (ReceiverList::iterator itr = receivers.begin();
				itr != receivers.end() && !found;
				++itr) {
			shared_ptr<CustomEventReceiver> list_receiver = itr->lock();
			if (list_receiver.get() == receiver) {
				foundIterator = itr;
				found = true;
			}
		}
		if (found) {
			receivers.erase(foundIterator);
			// send event only after internal state has been updated
			receiverDisconnected(shared_from(receiver));
		}
	}
};

} // namespace

