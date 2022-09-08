/* Copyright 2011 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Bindable.h"
#include "FastLog.h"

namespace RBX {

    REFLECTION_BEGIN();
	const char* const sBindableFunction = "BindableFunction";
	static Reflection::BoundYieldFuncDesc<BindableFunction, shared_ptr<const Reflection::Tuple>(shared_ptr<const Reflection::Tuple>)> func_Invoke(&BindableFunction::invoke, "Invoke", "arguments", Security::None);
	static Reflection::BoundAsyncCallbackDesc<BindableFunction, shared_ptr<const Reflection::Tuple>(shared_ptr<const Reflection::Tuple>)> callback_OnInvoke("OnInvoke", &BindableFunction::onInvoke, "arguments", &BindableFunction::processQueue);

#if 0
	const char* const sPropertyInstance = "Property";
	static Reflection::PropDescriptor<PropertyInstance, Reflection::Variant> prop_Value("Value", category_Data, &PropertyInstance::getValue, &PropertyInstance::setValue, Reflection::PropertyDescriptor::SCRIPTING);
	static Reflection::BoundCallbackDesc<Reflection::Variant()> callback_GetValue("OnGetValue", &PropertyInstance::getCallback);
	static Reflection::BoundCallbackDesc<void(Reflection::Variant)> callback_SetValue("OnSetValue", &PropertyInstance::setCallback, "value");
	static Reflection::EventDesc<PropertyInstance, void(Reflection::Variant)> event_ValueChanged(&PropertyInstance::valueChanged, "ValueChanged", "value");
	static Reflection::BoundFuncDesc<PropertyInstance, void()> func_FireValueChanged(&PropertyInstance::fireValueChanged, "FireValueChanged", Security::None);
#endif

	const char* const sBindableEvent = "BindableEvent";
	static Reflection::BoundFuncDesc<BindableEvent, void(shared_ptr<const Reflection::Tuple>)> func_Fire(&BindableEvent::fire, "Fire", "arguments", Security::None);
	static Reflection::EventDesc<BindableEvent, void(shared_ptr<const Reflection::Tuple>)> event_Event(&BindableEvent::event, "Event", "arguments");
    REFLECTION_END();

	void BindableFunction::processQueue(const OnInvokeCallback& oldValue)
	{
		if (onInvoke.empty())
			return;

		while (!queue.empty())
		{
			// Copy it to the stack to avoid re-entrancy bugs
			const Invocation invocation = queue.front();
			queue.pop();

            onInvoke(invocation.arguments, invocation.resumeFunction, invocation.errorFunction);
		}
	}

	void BindableFunction::invoke(shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
	{
		if (onInvoke.empty())
		{
			Invocation invocation = {arguments, resumeFunction, errorFunction};
			queue.push(invocation);
			return;
		}

        onInvoke(arguments, resumeFunction, errorFunction);
	}

	bool BindableFunction::askSetParent( const Instance* instance ) const
	{
		return true;
	}

	void BindableEvent::fire( shared_ptr<const Reflection::Tuple> arguments )
	{
		event(arguments);
	}

	bool BindableEvent::askSetParent( const Instance* instance ) const
	{
		return true;
	}

#if 0
	void PropertyInstance::fireValueChanged()
	{
		valueChanged(getCallback());
		raisePropertyChanged(prop_Value);
	}
	Reflection::Variant PropertyInstance::getValue()
	{
		return getCallback();
	}
	void PropertyInstance::setValue(const Reflection::Variant& value)
	{
		setCallback(value);
	}
#endif

} // namespace
