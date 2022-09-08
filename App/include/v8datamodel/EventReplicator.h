#pragma once

#include "rbx/signal.h"
#include "reflection/type.h"
#include "Reflection/reflection.h"
#include "Reflection/Event.h"

/*

This code lets you specify a remote event that is only fired remotely if somebody listens to it.

There is one problem with this: Count properties aren't reliable in our replication scheme.
It might work if only the server increments and decrements the counter. Also, the counter 
probably won't be decremented if a client incremented it and then disconnects.

*/

namespace RBX
{
	template <typename Parent, typename Signature>
	class EventReplicatorBase
	{
	protected:
		Reflection::BoundProp<int>& connectionCount;
		
		Parent* instance;
		Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent;
		rbx::signal<void()>& connectionSignal;
		
		rbx::signals::connection listenerConnection;
		void listenerConnectionAdded()
		{ 
			//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
			//					"   Connection incremented to %s", 
			//					connectionCount.name.c_str()
			//				);
			connectionCount.setValue(instance, std::max(connectionCount.getValue(instance) + 1, 1));
		}


		rbx::signals::connection signalConnection;
		virtual void connectSignalListener(){}
	public:
		EventReplicatorBase(rbx::signal<void()>& connectionSignal, 
						Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent,
						Reflection::BoundProp<int>& connectionCount)
			: connectionSignal(connectionSignal)
			, remoteEvent(remoteEvent)
			, connectionCount(connectionCount)
			, instance(NULL)
		{ 
			
		}

		~EventReplicatorBase()
		{
			if(listenerConnection.connected()){
				listenerConnection.disconnect();
			}
			if(signalConnection.connected()){
				signalConnection.disconnect();
			}
		}
		void setInstance(Parent* instance)
		{
			this->instance = instance;
		}
		void setListenerMode(bool alreadyConnected)
		{
			if(!listenerConnection.connected()){
				//Only one EventReplicator should be working on each signal, if not we'll run into trouble
				RBXASSERT(connectionSignal.empty());
				listenerConnection = connectionSignal.connect(boost::bind(&EventReplicatorBase::listenerConnectionAdded, this));
				if(alreadyConnected){
					listenerConnectionAdded();
				}
			}
		}
		void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
		{
			if(!listenerConnection.connected()){
				if(descriptor.name == connectionCount.name){
					//Someone on the other side is listening now, so we need to set up a connection
					if(connectionCount.getValue(instance) > 0){
						if(!signalConnection.connected()){
							//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
							//	"   Connection received to %s", 
							//	connectionCount.name.c_str()
							//);
							connectSignalListener();
						}
					}
					else{
						if(signalConnection.connected()){
							//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
							//	"   Disconnection received to %s", 
							//	connectionCount.name.c_str()
							//);
							signalConnection.disconnect();
						}
					}
				}
			}
		}
	};

	template<int arity, class Parent, typename Signature>
	class EventReplicatorImpl;

	template<class Parent, typename Signature>
	class EventReplicatorImpl<0, Parent, Signature> : public EventReplicatorBase<Parent, Signature>
	{
	public:
		EventReplicatorImpl(rbx::signal<void()>& connectionSignal, 
							Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent,
							Reflection::BoundProp<int>& connectionCount)
						: EventReplicatorBase<Parent, Signature>(connectionSignal,remoteEvent,connectionCount)
		{
			BOOST_STATIC_ASSERT(boost::function_traits<Signature>::arity == 0);
		}

	protected:
		void signalProducedIncremented()
		{
			this->remoteEvent.replicateEvent(this->instance);
		}

		/*implement*/ void connectSignalListener()
		{
			this->signalConnection = this->remoteEvent.getSignalPtr(this->instance)->connect(boost::bind(&EventReplicatorImpl::signalProducedIncremented, this));
		}
	};

	template<class Parent, typename Signature>
	class EventReplicatorImpl<1, Parent, Signature> : public EventReplicatorBase<Parent, Signature>
	{
		
	public:
		
		EventReplicatorImpl(rbx::signal<void()>& connectionSignal, 
							Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent,
							Reflection::BoundProp<int>& connectionCount)
						: EventReplicatorBase<Parent, Signature>(connectionSignal,remoteEvent,connectionCount)
		{
			BOOST_STATIC_ASSERT(boost::function_traits<Signature>::arity == 1);
		}

	protected:
		
		void signalProducedIncremented(typename boost::function_traits<Signature>::arg1_type arg1)
		{
			this->remoteEvent.replicateEvent(this->instance, arg1);
		}

		/*implement*/ void connectSignalListener()
		{
			this->signalConnection = this->remoteEvent.getSignalPtr(this->instance)->connect(boost::bind(
				&EventReplicatorImpl<1,Parent,Signature>::signalProducedIncremented, this, _1));
		}
	};

	template<class Parent, typename Signature>
	class EventReplicatorImpl<2, Parent, Signature> : public EventReplicatorBase<Parent, Signature>
	{
	public:
		EventReplicatorImpl(rbx::signal<void()>& connectionSignal, 
							Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent,
							Reflection::BoundProp<int>& connectionCount)
						: EventReplicatorBase<Parent, Signature>(connectionSignal,remoteEvent,connectionCount)
		{
			BOOST_STATIC_ASSERT(boost::function_traits<Signature>::arity == 2);
		}

	protected:
		
		void signalProducedIncremented(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2)
		{
			this->remoteEvent.replicateEvent(this->instance, arg1, arg2);
		}

		/*implement*/ void connectSignalListener()
		{
			this->signalConnection = this->remoteEvent.getSignalPtr(this->instance)->connect(boost::bind(
				&EventReplicatorImpl<2,Parent,Signature>::signalProducedIncremented, this, _1, _2));
		}
	};
	template<class Parent, typename Signature>
	class EventReplicatorImpl<3, Parent, Signature> : public EventReplicatorBase<Parent, Signature>
	{
	public:
		EventReplicatorImpl(rbx::signal<void()>& connectionSignal, 
							Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent,
							Reflection::BoundProp<int>& connectionCount)
						: EventReplicatorBase<Parent, Signature>(connectionSignal,remoteEvent,connectionCount)
		{
			BOOST_STATIC_ASSERT(boost::function_traits<Signature>::arity == 3);
		}

	protected:
		
		void signalProducedIncremented(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3)
		{
			this->remoteEvent.replicateEvent(this->instance, arg1, arg2, arg3);
		}

		/*implement*/ void connectSignalListener()
		{
			this->signalConnection = this->remoteEvent.getSignalPtr(this->instance)->connect(boost::bind(
				&EventReplicatorImpl<3,Parent,Signature>::signalProducedIncremented, this, _1, _2, _3));
		}
	};

	// Final entry class for EventReplicators
	template<class Parent, typename Signature>
	class EventReplicator : public EventReplicatorImpl<boost::function_traits<Signature>::arity, Parent, Signature>
	{
	public:
		EventReplicator(rbx::signal<void()>& connectionSignal, 
						Reflection::RemoteEventDesc<Parent,Signature>& remoteEvent, 
						Reflection::BoundProp<int>& connectionCount)
			: EventReplicatorImpl<boost::function_traits<Signature>::arity,Parent,Signature>(connectionSignal,remoteEvent,connectionCount)
		{}
	};

}

#define CHELPER2(a,b) a##b
#define CHELPER3(a,b,c) a##b##c

#define DECLARE_EVENT_REPLICATOR_SIG(Parent,eventName,signature)\
    int CHELPER3(var,eventName,ConnectionCount);\
	static Reflection::BoundProp<int> CHELPER3(prop_,eventName,ConnectionCount);\
	RBX::EventReplicator<Parent,signature> CHELPER2(eventReplicator,eventName);

#define category_EventReplicator "EventReplicator"
#define IMPLEMENT_EVENT_REPLICATOR(Parent,eventDesc,eventText,eventName)\
	Reflection::BoundProp<int> Parent::CHELPER3(prop_,eventName,ConnectionCount)(eventText "ConnectionCount", category_EventReplicator, &Parent::CHELPER3(var,eventName,ConnectionCount), RBX::Reflection::PropertyDescriptor::REPLICATE_ONLY);

#define CONSTRUCT_EVENT_REPLICATOR(Parent,remoteSignalName,eventDesc,eventName)\
  CHELPER2(eventReplicator,eventName)(remoteSignalName.connectionSignal,eventDesc,CHELPER3(prop_,eventName,ConnectionCount))\
  , CHELPER3(var,eventName,ConnectionCount)(0)


#define CONNECT_EVENT_REPLICATOR(eventName)\
	CHELPER2(eventReplicator,eventName).setInstance(this)
