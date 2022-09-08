#include "stdafx.h"

#include "V8DataModel/CustomEvent.h"
#include "reflection/reflection.h"

namespace RBX {

const char* const sCustomEvent = "CustomEvent";

REFLECTION_BEGIN();
Reflection::PropDescriptor<CustomEvent, float>
CustomEvent::prop_PersistedCurrentValue("PersistedCurrentValue", category_Data,
		&CustomEvent::getPersistedCurrentValue,
		&CustomEvent::setPersistedCurrentValue,
		Reflection::PropertyDescriptor::STREAMING,
		Security::None);

Reflection::BoundFuncDesc<CustomEvent, void(float)>
CustomEvent::func_SetValue(&CustomEvent::setCurrentValue, "SetValue",
		"newValue", Security::None);

Reflection::BoundFuncDesc<CustomEvent, shared_ptr<const Instances>()>
CustomEvent::func_GetAttachedReceivers(&CustomEvent::getAttachedReceivers,
		"GetAttachedReceivers", Security::None);

Reflection::EventDesc<CustomEvent, void(shared_ptr<Instance>)>
CustomEvent::event_ReceiverConnected(
		&CustomEvent::receiverConnected, "ReceiverConnected", "receiver",
		Security::None);

Reflection::EventDesc<CustomEvent, void(shared_ptr<Instance>)>
CustomEvent::event_ReceiverDisconnected(
		&CustomEvent::receiverDisconnected, "ReceiverDisconnected", "receiver",
		Security::None);
REFLECTION_END();
}