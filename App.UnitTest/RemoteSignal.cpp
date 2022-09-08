/*
	This unit tests the two flavors of RemoteEventDesc. One with fancy macros and one without.
	Erik is working on a replacement for RemoteEventDesc, so eventually this should be deprecated
	and then factored out.
*/
#include <boost/test/unit_test.hpp>

#include "rbx/test/test_tools.h"

#include "rbx/test/DataModelFixture.h"
#include "v8dataModel/Workspace.h"
#include "v8dataModel/EventReplicator.h"
#include "Network/Players.h"

using namespace RBX;

BOOST_GLOBAL_FIXTURE(NetworkFixture)

extern const char* const sRemoteEventClass;
class RemoteEventClass : public RBX::DescribedCreatable<RemoteEventClass, RBX::Instance, sRemoteEventClass>
{
public:
	rbx::remote_signal<void(int)> signal;

	CEvent evt;
	int lastValue;

	RemoteEventClass():evt(false),lastValue(-1)
	{
	}

	rbx::signals::connection selfConnect()
	{
		return signal.connect(boost::bind(&RemoteEventClass::onEvent, this, _1));
	}

	void onEvent(int value)
	{
		lastValue = value;
		evt.Set();
	}
};

static RBX::Reflection::RemoteEventDesc<RemoteEventClass, void(int)> test_BroadcastEvent(&RemoteEventClass::signal, "BroadcastEvent", "value", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);

const char* const sRemoteEventClass = "RemoteEventClass";
RBX_REGISTER_CLASS(RemoteEventClass);


extern const char* const sEventReplicatorClass;
class EventReplicatorClass : public RBX::DescribedCreatable<EventReplicatorClass, RBX::Instance, sEventReplicatorClass>
{
	bool listenerModeSet;
public:
	rbx::remote_signal<void(int)> signal;
	DECLARE_EVENT_REPLICATOR_SIG(EventReplicatorClass,Signal, void(int));

	CEvent evt;
	int lastValue;

	EventReplicatorClass();

	rbx::signals::connection selfConnect()
	{
		return signal.connect(boost::bind(&EventReplicatorClass::onEvent, this, _1));
	}

	void onEvent(int value)
	{
		lastValue = value;
		evt.Set();
	}

	void onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
	{
		RBX::Instance::onPropertyChanged(descriptor);
		eventReplicatorSignal.onPropertyChanged(descriptor);
	}

	void onAncestorChanged(const AncestorChanged& event)
	{
		RBX::Instance::onAncestorChanged(event);

		if(!listenerModeSet && Network::Players::serverIsPresent(this,false))
		{
			listenerModeSet = true;
			eventReplicatorSignal.setListenerMode(!signal.empty());
		}
	}
};

static RBX::Reflection::RemoteEventDesc<EventReplicatorClass, void(int)> test_Event(&EventReplicatorClass::signal, "Event", "value", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
IMPLEMENT_EVENT_REPLICATOR(EventReplicatorClass, test_Event, "Event", Signal);

EventReplicatorClass::EventReplicatorClass():evt(false),lastValue(-1),listenerModeSet(false)
	, CONSTRUCT_EVENT_REPLICATOR(EventReplicatorClass,EventReplicatorClass::signal, test_Event, Signal)
	{
		CONNECT_EVENT_REPLICATOR(Signal);
	}

const char* const sEventReplicatorClass = "EventReplicatorClass";
RBX_REGISTER_CLASS(EventReplicatorClass);



BOOST_AUTO_TEST_SUITE(RemoteSignals)

BOOST_AUTO_TEST_CASE(RemoteEvent)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	shared_ptr<RemoteEventClass> serverTest;

	// Create a RemoteEventClass on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverTest = Creatable<Instance>::create<RemoteEventClass>();
		serverTest->setName("Test");
		serverTest->setParent(serverDm->getWorkspace());
	}

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Wait for it to replicate
	G3D::System::sleep(2);

	shared_ptr<RemoteEventClass> clientTest;

	// Confirm the client got it
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientTest = shared_from_static_cast<RemoteEventClass>(clientDm->getWorkspace()->findFirstChildByName("Test"));
		BOOST_CHECK(clientTest != 0);
	}

	{
		BOOST_MESSAGE("Test Client to Server");

#if !RBX_PLATFORM_IOS
		rbx::signals::scoped_connection connection(serverTest->selfConnect());

		{
			RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
			test_BroadcastEvent.fireAndReplicateEvent(clientTest.get(), 12);
		}

		BOOST_CHECK(serverTest->evt.Wait(RBX::Time::Interval::from_seconds(10)));	
		BOOST_CHECK_EQUAL(serverTest->lastValue, 12);
#endif
	}

	{
		BOOST_MESSAGE("Test Server to Client");

#if !RBX_PLATFORM_IOS
		rbx::signals::scoped_connection connection(clientTest->selfConnect());

		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
			test_BroadcastEvent.fireAndReplicateEvent(serverTest.get(), 13);
		}

		BOOST_CHECK(clientTest->evt.Wait(RBX::Time::Interval::from_seconds(10)));	
		BOOST_CHECK_EQUAL(clientTest->lastValue, 13);
#endif
	}

	DataModelFixture client2Dm;
	NetworkFixture::startClient(client2Dm);

	// Wait for it to replicate
	G3D::System::sleep(2);

	shared_ptr<RemoteEventClass> client2Test;

	// Confirm the client got it
	{
		RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
		client2Test = shared_from_static_cast<RemoteEventClass>(client2Dm->getWorkspace()->findFirstChildByName("Test"));
		BOOST_CHECK(client2Test != 0);
	}

	{
		BOOST_MESSAGE("Test Client to Client2");

#if !RBX_PLATFORM_IOS
		rbx::signals::scoped_connection connection(client2Test->selfConnect());

		{
			RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
			test_BroadcastEvent.fireAndReplicateEvent(clientTest.get(), 14);
		}

		BOOST_CHECK(client2Test->evt.Wait(RBX::Time::Interval::from_seconds(10)));	
		BOOST_CHECK_EQUAL(client2Test->lastValue, 14);
#endif
	}


}




BOOST_AUTO_TEST_CASE(EventReplicator)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	shared_ptr<EventReplicatorClass> serverTest;

	// Create a EventReplicatorClass on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverTest = Creatable<Instance>::create<EventReplicatorClass>();
		serverTest->setName("Test");
		serverTest->setParent(serverDm->getWorkspace());
	}

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Wait for it to replicate
	G3D::System::sleep(4);

	shared_ptr<EventReplicatorClass> clientTest;

	// Confirm the client got it
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientTest = shared_from_static_cast<EventReplicatorClass>(clientDm->getWorkspace()->findFirstChildByName("Test"));
		BOOST_CHECK(clientTest != 0);
	}

	{
		BOOST_MESSAGE("Test Client to Server");

		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
			connection = serverTest->selfConnect();
		}
		G3D::System::sleep(4);	// wait for connection to replicate

		{
			RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
			clientTest->signal(12);
		}

		BOOST_CHECK(serverTest->evt.Wait(RBX::Time::Interval::from_seconds(10)));	
		BOOST_CHECK_EQUAL(serverTest->lastValue, 12);
	}

	{
		BOOST_MESSAGE("Test Client not to Server");

		G3D::System::sleep(4);	// wait for connection to replicate

		{
			RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
			clientTest->signal(12);
		}

		BOOST_CHECK(!serverTest->evt.Wait(RBX::Time::Interval::from_seconds(4)));	
	}

	{
		BOOST_MESSAGE("Test Server to Client");

		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
			connection = clientTest->selfConnect();
		}
		G3D::System::sleep(4);	// wait for connection to replicate

		{
			RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
			serverTest->signal(13);
		}

		BOOST_WARN(clientTest->evt.Wait(RBX::Time::Interval::from_seconds(10)));	
		BOOST_WARN_EQUAL(clientTest->lastValue, 13);
	}

	DataModelFixture client2Dm;
	NetworkFixture::startClient(client2Dm);

	// Wait for it to replicate
	G3D::System::sleep(4);

	shared_ptr<EventReplicatorClass> client2Test;

	// Confirm the client got it
	{
		RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
		client2Test = shared_from_static_cast<EventReplicatorClass>(client2Dm->getWorkspace()->findFirstChildByName("Test"));
		BOOST_CHECK(client2Test != 0);
	}

	{
		BOOST_MESSAGE("Test Client to Client2");

		rbx::signals::scoped_connection connection;
		{
			RBX::DataModel::LegacyLock lock(&client2Dm, RBX::DataModelJob::Write);
			connection = client2Test->selfConnect();
		}
		G3D::System::sleep(4);	// wait for connection to replicate

		{
			RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
			clientTest->signal(14);
		}

		BOOST_CHECK(client2Test->evt.Wait(RBX::Time::Interval::from_seconds(10)));	
		BOOST_CHECK_EQUAL(client2Test->lastValue, 14);
	}


}




BOOST_AUTO_TEST_SUITE_END()

