/* Copyright 2011 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Remote.h"

#include "Network/Players.h"
#include "FastLog.h"

#include "util/RobloxGoogleAnalytics.h"

DYNAMIC_FASTINTVARIABLE(RemoteDelayedQueueLimit, 256)

namespace {
	static void sendRemoteFunctionStatsOnServer(const char* function)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "RemoteFunction", function);
	}

	static void sendRemoteEventStatsOnServer(const char* function)
	{
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "RemoteEvent", function);
	}

} // namespace

namespace RBX {

    REFLECTION_BEGIN();
	const char* const sRemoteFunction = "RemoteFunction";
	static Reflection::BoundYieldFuncDesc<RemoteFunction, shared_ptr<const Reflection::Tuple>(shared_ptr<const Reflection::Tuple>)> func_InvokeServer(&RemoteFunction::invokeServer, "InvokeServer", "arguments", Security::None);
	static Reflection::BoundYieldFuncDesc<RemoteFunction, shared_ptr<const Reflection::Tuple>(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)> func_InvokeClient(&RemoteFunction::invokeClient, "InvokeClient", "player", "arguments", Security::None);

	static Reflection::BoundAsyncCallbackDesc<RemoteFunction, shared_ptr<const Reflection::Tuple>(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)> callback_OnServerInvoke("OnServerInvoke", &RemoteFunction::onServerInvoke, "player", "arguments", &RemoteFunction::onServerInvokeChanged);
	static Reflection::BoundAsyncCallbackDesc<RemoteFunction, shared_ptr<const Reflection::Tuple>(shared_ptr<const Reflection::Tuple>)> callback_OnClientInvoke("OnClientInvoke", &RemoteFunction::onClientInvoke, "arguments", &RemoteFunction::onClientInvokeChanged);

	static Reflection::RemoteEventDesc<RemoteFunction, void(int, shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)> event_RemoteOnInvokeServer(&RemoteFunction::remoteOnInvokeServer, "RemoteOnInvokeServer", "id", "player", "arguments", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::RemoteEventDesc<RemoteFunction, void(int, shared_ptr<const Reflection::Tuple>)> event_RemoteOnInvokeClient(&RemoteFunction::remoteOnInvokeClient, "RemoteOnInvokeClient", "id", "arguments", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::RemoteEventDesc<RemoteFunction, void(int, shared_ptr<const Reflection::Tuple>)> event_RemoteOnInvokeSuccess(&RemoteFunction::remoteOnInvokeSuccess, "RemoteOnInvokeSuccess", "id", "arguments", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
	static Reflection::RemoteEventDesc<RemoteFunction, void(int, std::string)> event_RemoteOnInvokeError(&RemoteFunction::remoteOnInvokeError, "RemoteOnInvokeError", "id", "arguments", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

	const char* const sRemoteEvent = "RemoteEvent";
	static Reflection::BoundFuncDesc<RemoteEvent, void(shared_ptr<const Reflection::Tuple>)> func_FireServer(&RemoteEvent::fireServer, "FireServer", "arguments", Security::None);
	static Reflection::BoundFuncDesc<RemoteEvent, void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)> func_FireClient(&RemoteEvent::fireClient, "FireClient", "player", "arguments", Security::None);
	static Reflection::BoundFuncDesc<RemoteEvent, void(shared_ptr<const Reflection::Tuple>)> func_FireAllClients(&RemoteEvent::fireAllClients, "FireAllClients", "arguments", Security::None);

	static Reflection::RemoteEventDesc<RemoteEvent,
        void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>),
        LatchedSignal<rbx::remote_signal, void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)>,
        LatchedSignal<rbx::remote_signal, void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)>* (RemoteEvent::*)(bool)
        > event_OnServerEvent(&RemoteEvent::getOrCreateOnServerEvent, "OnServerEvent", "player", "arguments", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
    
	static Reflection::RemoteEventDesc<RemoteEvent,
        void(shared_ptr<const Reflection::Tuple>),
        LatchedSignal<rbx::remote_signal, void(shared_ptr<const Reflection::Tuple>)>,
        LatchedSignal<rbx::remote_signal, void(shared_ptr<const Reflection::Tuple>)>* (RemoteEvent::*)(bool)
        > event_OnClientEvent(&RemoteEvent::getOrCreateOnClientEvent, "OnClientEvent", "arguments", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
    REFLECTION_END();

    bool isPlayerValid(Instance* instance, const SystemAddress& address)
    {
        Network::Player* player = Instance::fastDynamicCast<Network::Player>(instance);

        return player && player->getRemoteAddressAsRbxAddress() == address;
    }

    DelayedInvocationQueue::DelayedInvocationQueue(size_t limit)
        : limit(limit)
    {
    }

    bool DelayedInvocationQueue::push(const boost::function<void ()>& item)
    {
        if (items.size() >= limit)
            return false;

        items.push_back(item);

        return true;
    }

    void DelayedInvocationQueue::process()
    {
        std::vector<boost::function<void()> > temp;
        items.swap(temp);

        for (size_t i = 0; i < temp.size(); ++i)
        {
            try
            {
                temp[i]();
            }
            catch (const RBX::base_exception& e)
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "DelayedInvocationQueue::process encountered an error %s", e.what());
                RBXCRASH();
            }
        }
    }

    RemoteFunction::RemoteFunction()
        : DescribedCreatable<RemoteFunction, Instance, sRemoteFunction>("RemoteFunction")
        , delayedInvocations(DFInt::RemoteDelayedQueueLimit)
        , lastRemoteInvocationId(0)
    {
        remoteOnInvokeSuccess.connect(boost::bind(&RemoteFunction::localSuccess, this, _1, _2));
        remoteOnInvokeError.connect(boost::bind(&RemoteFunction::localError, this, _1, _2));
    }

    void RemoteFunction::invokeServer(shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
        if (Network::Players::serverIsPresent(this))
        {
            throw std::runtime_error("InvokeServer can only be called from the client");
        }
        else if (Network::Players::clientIsPresent(this))
        {
            Instance* player = Network::Players::findLocalPlayer(this);

            int id = createRemoteInvocation(weak_ptr<Instance>(), resumeFunction, errorFunction);

            // send a remote event
            Reflection::EventArguments args(3);
            args[0] = id;
            args[1] = shared_from(player);
            args[2] = arguments;

            raiseEventInvocation(event_RemoteOnInvokeServer, args, NULL);
        }
        else
        {
            // play solo; fire the event locally to simulate client-server replication
            Instance* player = Network::Players::findLocalPlayer(this);

            localInvokeServer(shared_from(player), arguments, resumeFunction, errorFunction);
        }
    }

    void RemoteFunction::invokeClient(shared_ptr<Instance> player, shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
        if (Network::Players::clientIsPresent(this))
        {
            throw std::runtime_error("InvokeClient can only be called from the server");
        }
        else if (Network::Players::serverIsPresent(this))
        {
            Network::Player* p = Instance::fastDynamicCast<Network::Player>(player.get());

            if (!p)
                throw std::runtime_error("InvokeClient: player argument must be a Player object");

            const RBX::SystemAddress& target = p->getRemoteAddressAsRbxAddress();

            int id = createRemoteInvocation(player, resumeFunction, errorFunction);

            Reflection::EventArguments args(2);
            args[0] = id;
            args[1] = arguments;

            raiseEventInvocation(event_RemoteOnInvokeClient, args, &target);

			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&sendRemoteFunctionStatsOnServer, "invokeClient"));
			}
        }
        else
        {
            // play solo; fire the event locally to simulate client-server replication
            localInvokeClient(arguments, resumeFunction, errorFunction);
        }
    }

    void RemoteFunction::onServerInvokeChanged(const ServerInvokeCallback& oldValue)
    {
        if (Network::Players::clientIsPresent(this))
        {
            onServerInvoke.clear();

            throw std::runtime_error("OnServerInvoke can only be implemented on the server");
        }

        processDelayedInvocations();
    }

    void RemoteFunction::onClientInvokeChanged(const ClientInvokeCallback& oldValue)
    {
        if (Network::Players::serverIsPresent(this))
        {
            onClientInvoke.clear();

            throw std::runtime_error("OnClientInvoke can only be implemented on the client");
        }

        processDelayedInvocations();
    }

    void RemoteFunction::processDelayedInvocations()
	{
        delayedInvocations.process();
	}

	bool RemoteFunction::askSetParent( const Instance* instance ) const
	{
		return true;
	}

    void RemoteFunction::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
    {
        Instance::onServiceProvider(oldProvider, newProvider);

        playerRemovingConnection.disconnect();

        if (Network::Players* players = ServiceProvider::find<Network::Players>(newProvider))
        {
            playerRemovingConnection = players->playerRemovingSignal.connect(boost::bind(&RemoteFunction::onPlayerRemoving, this, _1));
        }
    }

    bool RemoteFunction::consumeRemoteInvocation(int id, RemoteInvocation& result)
    {
        std::map<int, RemoteInvocation>::iterator it = remoteInvocations.find(id);

        if (it == remoteInvocations.end())
            return false;

        result = it->second;

        remoteInvocations.erase(it);

        return true;
    }

    int RemoteFunction::createRemoteInvocation(const weak_ptr<Instance>& player, const boost::function<void(shared_ptr<const Reflection::Tuple>)>& resumeFunction, const boost::function<void(std::string)>& errorFunction)
    {
        // Generate (hopefully, unique) invocation id in [1..INT_MAX]
        if (lastRemoteInvocationId == INT_MAX)
            lastRemoteInvocationId = 0;

        int id = ++lastRemoteInvocationId;

        // Insert an invocation continuation
        RBXASSERT(remoteInvocations.count(id) == 0);
        RemoteInvocation invocation = {player, resumeFunction, errorFunction};
        remoteInvocations[id] = invocation;

        return id;
    }

    void RemoteFunction::processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source)
    {
        if (&descriptor == &event_RemoteOnInvokeServer)
        {
            RBXASSERT(args.size() == 3);

            int id = args[0].cast<int>();
            shared_ptr<Instance> player = args[1].cast<shared_ptr<Instance> >();
            shared_ptr<const Reflection::Tuple> arguments = args[2].cast<shared_ptr<const Reflection::Tuple> >();

            // Validate player object; ignore invalid calls for security reasons
            if (isPlayerValid(player.get(), source))
            {
                localInvokeServer(player, arguments, boost::bind(&RemoteFunction::remoteSuccess, this, source, id, _1), boost::bind(&RemoteFunction::remoteError, this, source, id, _1));
            }
            else
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "RemoteFunction::processRemoteEvent: ignore a remote call from %x:%d", source.getAddress(), source.getPort());
            }
        }
        else if (&descriptor == &event_RemoteOnInvokeClient)
        {
            RBXASSERT(args.size() == 2);

            int id = args[0].cast<int>();
            shared_ptr<const Reflection::Tuple> arguments = args[1].cast<shared_ptr<const Reflection::Tuple> >();

            localInvokeClient(arguments, boost::bind(&RemoteFunction::remoteSuccess, this, source, id, _1), boost::bind(&RemoteFunction::remoteError, this, source, id, _1));
        }
        else
        {
            EventSource::processRemoteEvent(descriptor, args, source);
        }
    }

    void RemoteFunction::localInvokeServer(shared_ptr<Instance> player, shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
		if (onServerInvoke.empty())
		{
			if (!delayedInvocations.push(boost::bind(&RemoteFunction::localInvokeServer, this, player, arguments, resumeFunction, errorFunction)))
                errorFunction("Remote function invocation queue exhausted for " + getFullName() + "; did you forget to implement OnServerInvoke?");

			return;
		}

		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(flag, boost::bind(&sendRemoteFunctionStatsOnServer, "invokeServer"));
		}

        onServerInvoke(player, arguments, resumeFunction, errorFunction);
    }

    void RemoteFunction::localInvokeClient(shared_ptr<const Reflection::Tuple> arguments, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
    {
		if (onClientInvoke.empty())
		{
			if (!delayedInvocations.push(boost::bind(&RemoteFunction::localInvokeClient, this, arguments, resumeFunction, errorFunction)))
                errorFunction("Remote function invocation queue exhausted for " + getFullName() + "; did you forget to implement OnClientInvoke?");

			return;
		}

        onClientInvoke(arguments, resumeFunction, errorFunction);
    }

    void RemoteFunction::remoteSuccess(SystemAddress address, int id, shared_ptr<const Reflection::Tuple> result)
    {
        Reflection::EventArguments args(2);
        args[0] = id;
        args[1] = result;

        raiseEventInvocation(event_RemoteOnInvokeSuccess, args, &address);
    }

    void RemoteFunction::remoteError(SystemAddress address, int id, std::string error)
    {
        Reflection::EventArguments args(2);
        args[0] = id;
        args[1] = error;

        raiseEventInvocation(event_RemoteOnInvokeError, args, &address);
    }

    void RemoteFunction::localSuccess(int id, shared_ptr<const Reflection::Tuple> arguments)
    {
        RemoteInvocation invocation;

        if (consumeRemoteInvocation(id, invocation))
        {
            invocation.resumeFunction(arguments);
        }
        else
        {
            RBXASSERT(false);
        }
    }

    void RemoteFunction::localError(int id, std::string error)
    {
        RemoteInvocation invocation;

        if (consumeRemoteInvocation(id, invocation))
        {
            invocation.errorFunction(error);
        }
        else
        {
            RBXASSERT(false);
        }
    }

    void RemoteFunction::onPlayerRemoving(shared_ptr<Instance> player)
    {
        weak_ptr<Instance> weakPlayer = player;

        std::vector<int> invocations;

        for (std::map<int, RemoteInvocation>::iterator it = remoteInvocations.begin(); it != remoteInvocations.end(); ++it)
            if (weak_equal(it->second.player, weakPlayer))
                invocations.push_back(it->first);

        std::string message = "Player " + player->getName() + " disconnected during remote call to " + getFullName();

        for (size_t i = 0; i < invocations.size(); ++i)
        {
            RemoteInvocation invocation;
            consumeRemoteInvocation(invocations[i], invocation);

            invocation.errorFunction(message);
        }
    }

    RemoteEvent::RemoteEvent()
        : DescribedCreatable<RemoteEvent, Instance, sRemoteEvent>("RemoteEvent")
        , onServerEvent(this, "OnServerEvent", DFInt::RemoteDelayedQueueLimit)
        , onClientEvent(this, "OnClientEvent", DFInt::RemoteDelayedQueueLimit)
    {
    }

    void RemoteEvent::fireServer(shared_ptr<const Reflection::Tuple> arguments)
    {
        if (Network::Players::serverIsPresent(this))
        {
            throw std::runtime_error("FireServer can only be called from the client");
        }
        else if (Network::Players::clientIsPresent(this))
        {
            Instance* player = Network::Players::findLocalPlayer(this);

            // remote call
            Reflection::EventArguments args(2);
            args[0] = shared_from(player);
            args[1] = arguments;

            raiseEventInvocation(event_OnServerEvent, args, NULL);
        }
        else
        {
            // play solo; fire the event locally to simulate client-server replication
            Instance* player = Network::Players::findLocalPlayer(this);

            onServerEvent(shared_from(player), arguments);
        }
    }

    void RemoteEvent::fireClient(shared_ptr<Instance> player, shared_ptr<const Reflection::Tuple> arguments)
    {
        if (Network::Players::clientIsPresent(this))
        {
            throw std::runtime_error("FireClient can only be called from the server");
        }
        else if (Network::Players::serverIsPresent(this))
        {
            Network::Player* p = Instance::fastDynamicCast<Network::Player>(player.get());

            if (!p)
                throw std::runtime_error("FireClient: player argument must be a Player object");

            const RBX::SystemAddress& target = p->getRemoteAddressAsRbxAddress();

            // remote call
            Reflection::EventArguments args(1);
            args[0] = arguments;

            raiseEventInvocation(event_OnClientEvent, args, &target);

			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&sendRemoteEventStatsOnServer, "fireClient"));
			}
        }
        else
        {
            // play solo; fire the event locally to simulate client-server replication
            onClientEvent(arguments);
        }
    }

    void RemoteEvent::fireAllClients(shared_ptr<const Reflection::Tuple> arguments)
    {
        if (Network::Players::clientIsPresent(this))
        {
            throw std::runtime_error("FireAllClients can only be called from the server");
        }
        else if (Network::Players::serverIsPresent(this))
        {
            // remote call
            Reflection::EventArguments args(1);
            args[0] = arguments;

            raiseEventInvocation(event_OnClientEvent, args, NULL);

			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&sendRemoteEventStatsOnServer, "fireAllClients"));
			}
        }
        else
        {
            // play solo; fire the event locally to simulate client-server replication
            onClientEvent(arguments);
        }
    }

    LatchedSignal<rbx::remote_signal, void(shared_ptr<Instance>, shared_ptr<const Reflection::Tuple>)>* RemoteEvent::getOrCreateOnServerEvent(bool create)
    {
        if (create && Network::Players::clientIsPresent(this))
            throw std::runtime_error("OnServerEvent can only be used on the server");

        return &onServerEvent;
    }
    
    LatchedSignal<rbx::remote_signal, void(shared_ptr<const Reflection::Tuple>)>* RemoteEvent::getOrCreateOnClientEvent(bool create)
    {
        if (create && Network::Players::serverIsPresent(this))
            throw std::runtime_error("OnClientEvent can only be used on the client");
        
        return &onClientEvent;
    }

	bool RemoteEvent::askSetParent( const Instance* instance ) const
	{
		return true;
	}

    void RemoteEvent::processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source)
    {
        if (&descriptor == &event_OnServerEvent)
        {
            RBXASSERT(args.size() == 2);

            shared_ptr<Instance> player = args[0].cast<shared_ptr<Instance> >();

            // Validate player object; ignore invalid calls for security reasons
            if (isPlayerValid(player.get(), source))
            {
                EventSource::processRemoteEvent(descriptor, args, source);

                {
                    static boost::once_flag flag = BOOST_ONCE_INIT;
                    boost::call_once(flag, boost::bind(&sendRemoteEventStatsOnServer, "fireServer"));
                }
            }
            else
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "RemoteEvent::processRemoteEvent: ignore a remote call from %x:%d", source.getAddress(), source.getPort());
            }
        }
        else
        {
            EventSource::processRemoteEvent(descriptor, args, source);
        }
    }

} // namespace
