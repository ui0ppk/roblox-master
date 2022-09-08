/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"
#include "util/SystemAddress.h"

namespace RBX {

    bool isPlayerValid(Instance* instance, const SystemAddress& address);

    class DelayedInvocationQueue
    {
    public:
        DelayedInvocationQueue(size_t limit);

        bool push(const boost::function<void ()>& item);
        void process();

    private:
        std::vector<boost::function<void ()> > items;
        size_t limit;
    };

    template <template <typename> class BaseSignal, typename Signature> class LatchedSignal: public BaseSignal<Signature>
    {
    public:
        LatchedSignal(Instance* instance, const char* eventName, size_t queueLimit)
			: instance(instance)
			, eventName(eventName)
            , queue(queueLimit)
        {
        }

        template <typename A1> void operator()(A1 arg1)
        {
            fire1(arg1);
        }
        
        template <typename A1, typename A2> void operator()(A1 arg1, A2 arg2)
        {
            fire2(arg1, arg2);
        }

        template <typename F> rbx::signals::connection connect(const F& function)
        {
            rbx::signals::connection result = BaseSignal<Signature>::connect(function);
            queue.process();
            return result;
        }

    private:
		Instance* instance;
		const char* eventName;
        DelayedInvocationQueue queue;

        void push(const boost::function<void ()>& item)
		{
			if (!queue.push(item))
			{
				std::string name = instance ? instance->getFullName() : "unknown instance";

                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Remote event invocation queue exhausted for %s; did you forget to implement %s?", name.c_str(), eventName);
			}
		}

        template <typename A1> void fire1(A1 arg1)
        {
            if (this->empty())
                push(boost::bind(&LatchedSignal::template fire1<A1>, this, arg1));
            else
                BaseSignal<Signature>::operator()(arg1);
        }

        template <typename A1, typename A2> void fire2(A1 arg1, A2 arg2)
        {
            if (this->empty())
                push(boost::bind(&LatchedSignal::template fire2<A1, A2>, this, arg1, arg2));
            else
                BaseSignal<Signature>::operator()(arg1, arg2);
        }
    };

	extern const char* const sRemoteFunction;
	class RemoteFunction : public DescribedCreatable<RemoteFunction, Instance, sRemoteFunction>
	{
	public:
		RemoteFunction();
			
		typedef boost::function<void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>, boost::function<void(shared_ptr<const Reflection::Tuple>)>, boost::function<void(std::string)>)> ServerInvokeCallback;
		ServerInvokeCallback onServerInvoke;
		typedef boost::function<void(shared_ptr<const Reflection::Tuple>, boost::function<void(shared_ptr<const Reflection::Tuple>)>, boost::function<void(std::string)>)> ClientInvokeCallback;
		ClientInvokeCallback onClientInvoke;

		void invokeServer(shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void invokeClient(shared_ptr<Instance> player, shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);

		rbx::remote_signal<void(int, shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)> remoteOnInvokeServer;
		rbx::remote_signal<void(int, shared_ptr<const Reflection::Tuple>)> remoteOnInvokeClient;

		rbx::remote_signal<void(int, shared_ptr<const Reflection::Tuple>)> remoteOnInvokeSuccess;
		rbx::remote_signal<void(int, std::string)> remoteOnInvokeError;

		void onServerInvokeChanged(const ServerInvokeCallback& oldValue);
		void onClientInvokeChanged(const ClientInvokeCallback& oldValue);
		void processDelayedInvocations();

		/*override*/ bool askSetParent(const Instance* instance) const;
	    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    private:
		struct RemoteInvocation
		{
            weak_ptr<Instance> player;
			boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction;
			boost::function<void(std::string)> errorFunction;
		};

		DelayedInvocationQueue delayedInvocations;
        std::map<int, RemoteInvocation> remoteInvocations;

        rbx::signals::scoped_connection playerRemovingConnection;

        int lastRemoteInvocationId;

        bool consumeRemoteInvocation(int id, RemoteInvocation& result);
		int createRemoteInvocation(const weak_ptr<Instance>& player, const boost::function<void(shared_ptr<const Reflection::Tuple>)>& resumeFunction, const boost::function<void(std::string)>& errorFunction);

        virtual void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source);

		void localInvokeServer(shared_ptr<Instance> player, shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void localInvokeClient(shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);

        void remoteSuccess(SystemAddress address, int id, shared_ptr<const Reflection::Tuple> result);
        void remoteError(SystemAddress address, int id, std::string error);

        void localSuccess(int id, shared_ptr<const Reflection::Tuple> arguments);
        void localError(int id, std::string error);

        void onPlayerRemoving(shared_ptr<Instance> player);
	};

	extern const char* const sRemoteEvent;
	class RemoteEvent : public DescribedCreatable<RemoteEvent, Instance, sRemoteEvent>
	{
	public:
		RemoteEvent();

		void fireServer(shared_ptr<const Reflection::Tuple> arguments);
		void fireClient(shared_ptr<Instance> player, shared_ptr<const Reflection::Tuple> arguments);
		void fireAllClients(shared_ptr<const Reflection::Tuple> arguments);

		LatchedSignal<rbx::remote_signal, void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)>* getOrCreateOnServerEvent(bool create = true);
		LatchedSignal<rbx::remote_signal, void(shared_ptr<const Reflection::Tuple>)>* getOrCreateOnClientEvent(bool create = true);
        
		/*override*/ bool askSetParent(const Instance* instance) const;

    private:
        virtual void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source);
        
        LatchedSignal<rbx::remote_signal, void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)> onServerEvent;
		LatchedSignal<rbx::remote_signal, void(shared_ptr<const Reflection::Tuple>)> onClientEvent;
	};

}	// namespace RBX
