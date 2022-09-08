/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"

namespace RBX {

	// Summary: A simple class that exposes a configurable function. 
	// Usage: It allows scripts to expose their functions
	extern const char* const sBindableFunction;
	class BindableFunction : public DescribedCreatable<BindableFunction, Instance, sBindableFunction>
	{
		struct Invocation
		{
			shared_ptr<const Reflection::Tuple> arguments;
			boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction;
			boost::function<void(std::string)> errorFunction;

		};
		typedef std::queue<Invocation> Queue;
		Queue queue;
	public:
		BindableFunction():DescribedCreatable<BindableFunction, Instance, sBindableFunction>("Function") {}
			
		void invoke(shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		typedef boost::function<void (shared_ptr<const Reflection::Tuple>, boost::function<void(shared_ptr<const Reflection::Tuple>)>, boost::function<void(std::string)>)> OnInvokeCallback;
		OnInvokeCallback onInvoke;
		void processQueue(const OnInvokeCallback& oldValue);
		/*override*/ bool askSetParent(const Instance* instance) const;
	};

#if 0
	// Summary: A simple class that exposes a property 
	// Usage: It allows scripts to expose their properties
	extern const char* const sPropertyInstance;
	class PropertyInstance : public DescribedCreatable<PropertyInstance, Instance, sPropertyInstance>
	{
	public:
		PropertyInstance():DescribedCreatable<PropertyInstance, Instance, sPropertyInstance>("Property") {}

		rbx::signal<void(Reflection::Variant)> valueChanged;
		boost::function<Reflection::Variant()> getCallback;
		boost::function<void(Reflection::Variant)> setCallback;

		Reflection::Variant getValue();
		void setValue(const Reflection::Variant& value);

		void fireValueChanged();
	};
#endif

	// Summary: A simple class that exposes a fire-able event. 
	// Usage: It allows scripts to expose their events
	extern const char* const sBindableEvent;
	class BindableEvent : public DescribedCreatable<BindableEvent, Instance, sBindableEvent>
	{
	public:
		BindableEvent():DescribedCreatable<BindableEvent, Instance, sBindableEvent>("Event") {}
		rbx::signal<void(shared_ptr<const Reflection::Tuple>)> event;
		void fire(shared_ptr<const Reflection::Tuple> arguments);
		/*override*/ bool askSetParent(const Instance* instance) const;
	};


}	// namespace RBX
