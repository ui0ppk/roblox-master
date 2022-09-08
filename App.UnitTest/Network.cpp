#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "rbx/test/Base.UnitTest.Lib.h"
#include "v8dataModel/BasicPartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Lighting.h"
#include "v8dataModel/Workspace.h"
#include "v8tree/Instance.h"
#include "Network/Players.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "v8datamodel/VehicleSeat.h"
#include "reflection/reflection.h"
#include "script/script.h"

namespace RBX
{

extern const char* const sTheNewClass;

class TheNewClass : public DescribedCreatable<TheNewClass, PartInstance, sTheNewClass>
{
public:
    TheNewClass()
    :existingPropValue(0), theNewPropValue(0), existingEventValue(0), theNewEventValue(0), existingEvt(false), theNewEvt(false)
    {
    }
    ~TheNewClass(){}

    static Reflection::PropDescriptor<TheNewClass, int>* prop_ExistingProp;
    static Reflection::PropDescriptor<TheNewClass, int>* prop_TheNewProp;
    static Reflection::RemoteEventDesc<TheNewClass, void(int)>* event_ExistingEvent;
    static Reflection::RemoteEventDesc<TheNewClass, void(int)>* event_TheNewEvent;

    rbx::remote_signal<void(int)> onExistingEventSignal;
    void handleExistingEventSignal(int value)
    {
        existingEventValue = value;
        existingEvt.Set();
    }
    rbx::signals::connection connectExistingSignal()
    {
        return onExistingEventSignal.connect(boost::bind(&TheNewClass::handleExistingEventSignal, this, _1));
    }

    rbx::remote_signal<void(int)> onTheNewEventSignal;
    void handleNewEventSignal(int value)
    {
        theNewEventValue = value;
        theNewEvt.Set();
    }
    rbx::signals::connection connectTheNewSignal()
    {
        return onTheNewEventSignal.connect(boost::bind(&TheNewClass::handleNewEventSignal, this, _1));
    }

    int existingPropValue;
    int theNewPropValue;
    int existingEventValue;
    int theNewEventValue;
    CEvent existingEvt;
    CEvent theNewEvt;
    int getExistingProp() const {return existingPropValue;}
    void setExistingProp(int value)
    {
        if (value != existingPropValue)
        {
            existingPropValue = value;
            raiseChanged(*prop_ExistingProp);
        }
    }
    int getTheNewProp() const {return theNewPropValue;}
    void setTheNewProp(int value)
    {
        if (value != theNewPropValue)
        {
            theNewPropValue = value;
            raiseChanged(*prop_TheNewProp);
        }
    }
};

const char* const sTheNewClass = "TheNewClass";

RBX_REGISTER_CLASS(TheNewClass);

Reflection::PropDescriptor<TheNewClass, int>* TheNewClass::prop_ExistingProp = NULL;
Reflection::PropDescriptor<TheNewClass, int>* TheNewClass::prop_TheNewProp = NULL;

Reflection::RemoteEventDesc<TheNewClass, void(int)>* TheNewClass::event_ExistingEvent = NULL;
Reflection::RemoteEventDesc<TheNewClass, void(int)>* TheNewClass::event_TheNewEvent = NULL;

void registerProps(bool registerNewProp)
{
    RBXASSERT(Reflection::Descriptor::lockedDown);
    Reflection::Descriptor::lockedDown = false;

    static Reflection::PropDescriptor<TheNewClass, int> prop_ExistingProp("ExistingProp", category_Data, &TheNewClass::getExistingProp, &TheNewClass::setExistingProp, Reflection::PropertyDescriptor::STANDARD);
    TheNewClass::prop_ExistingProp = &prop_ExistingProp;

    if (registerNewProp)
    {
        static Reflection::PropDescriptor<TheNewClass, int> prop_TheNewProp("TheNewProp", category_Data, &TheNewClass::getTheNewProp, &TheNewClass::setTheNewProp, Reflection::PropertyDescriptor::STANDARD);
        TheNewClass::prop_TheNewProp = &prop_TheNewProp;
    }

    Reflection::Descriptor::lockedDown = true;
}

void registerEvents(bool registerNewEvent)
{
    RBXASSERT(Reflection::Descriptor::lockedDown);
    Reflection::Descriptor::lockedDown = false;

    static Reflection::RemoteEventDesc<TheNewClass, void(int)> event_ExistingEvent(&TheNewClass::onExistingEventSignal, "ExistingEvent", "value", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
    TheNewClass::event_ExistingEvent = &event_ExistingEvent;

    if (registerNewEvent)
    {
        static Reflection::RemoteEventDesc<TheNewClass, void(int)> event_TheNewEvent(&TheNewClass::onTheNewEventSignal, "TheNewEvent", "value", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
        TheNewClass::event_TheNewEvent = &event_TheNewEvent;
    }

    Reflection::Descriptor::lockedDown = true;
}

} // namespace RBX

using namespace RBX;
using namespace Network;

#ifdef RBX_PLATFORM_IOS
#define SLEEPTIME 5
#else
#define SLEEPTIME 2
#endif

BOOST_GLOBAL_FIXTURE(NetworkFixture)

BOOST_AUTO_TEST_SUITE(Network)

BOOST_AUTO_TEST_CASE(PartReplication)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		shared_ptr<PartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
		part->setAnchored(true);
		part->setName("TestPart");
		part->setParent(serverDm->getWorkspace());
	}

	// Wait for it to replicate
	G3D::System::sleep(SLEEPTIME);

	// Confirm the client got it
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("TestPart") != 0);
	}
}

BOOST_AUTO_TEST_CASE(FilterEnabled)
{
	DataModelFixture server;
	NetworkFixture::startServer(server);

	// Turn on filter enabled
	server->getWorkspace()->setNetworkFilteringEnabled(true);

	DataModelFixture client1;
	DataModelFixture client2;
	NetworkFixture::startClient(client1);
	NetworkFixture::startClient(client2);

	std::string BRICK_STRING = "Brick";
	std::string CHANGED_STRING = "Changed";

	// Create a brick on the server
	shared_ptr<PartInstance> serverPart;
	{
		RBX::DataModel::LegacyLock lock(&server, RBX::DataModelJob::Write);
		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName(BRICK_STRING);
		serverPart->setParent(server->getWorkspace());
	}

	BOOST_MESSAGE("Waiting for brick to replicate");
	G3D::System::sleep(SLEEPTIME);

	// Confirm both clients got it
	Instance* clientPart1;
	{
		RBX::DataModel::LegacyLock lock(&client1, RBX::DataModelJob::Write);
		clientPart1 = client1->getWorkspace()->findFirstChildByName(BRICK_STRING);
		BOOST_REQUIRE(clientPart1);
	}
	Instance* clientPart2;
	{
		RBX::DataModel::LegacyLock lock(&client2, RBX::DataModelJob::Write);
		clientPart2 = client2->getWorkspace()->findFirstChildByName(BRICK_STRING);
		BOOST_REQUIRE(clientPart2);
	}

	// Make client1 change the brick name
	{
		RBX::DataModel::LegacyLock lock(&client1, RBX::DataModelJob::Write);
		clientPart1->setName(CHANGED_STRING);
	}

	BOOST_MESSAGE("Giving client-1 a chance to replicate its name change");
	G3D::System::sleep(SLEEPTIME);

	// Confirm that client 1's change was rejected because the network filter is enabled
	{
		RBX::DataModel::LegacyLock lock(&server, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getName(), BRICK_STRING);
	}
	{
		RBX::DataModel::LegacyLock lock(&client2, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(clientPart2->getName(), BRICK_STRING);
	}
}

BOOST_AUTO_TEST_CASE(FilterDisabled)
{
	DataModelFixture server;
	NetworkFixture::startServer(server);

	// Turn off filter enabled
	server->getWorkspace()->setNetworkFilteringEnabled(false);

	DataModelFixture client1;
	DataModelFixture client2;
	NetworkFixture::startClient(client1);
	NetworkFixture::startClient(client2);

	std::string BRICK_STRING = "Brick";
	std::string CHANGED_STRING = "Changed";

	// Create a brick on the server
	shared_ptr<PartInstance> serverPart;
	{
		RBX::DataModel::LegacyLock lock(&server, RBX::DataModelJob::Write);
		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName(BRICK_STRING);
		serverPart->setParent(server->getWorkspace());
	}

	BOOST_MESSAGE("Waiting for brick to replicate");
	G3D::System::sleep(SLEEPTIME);

	// Confirm both clients got it
	Instance* clientPart1;
	{
		RBX::DataModel::LegacyLock lock(&client1, RBX::DataModelJob::Write);
		clientPart1 = client1->getWorkspace()->findFirstChildByName(BRICK_STRING);
		BOOST_REQUIRE(clientPart1);
	}
	Instance* clientPart2;
	{
		RBX::DataModel::LegacyLock lock(&client2, RBX::DataModelJob::Write);
		clientPart2 = client2->getWorkspace()->findFirstChildByName(BRICK_STRING);
		BOOST_REQUIRE(clientPart2);
	}

	// Make client1 change the brick name
	{
		RBX::DataModel::LegacyLock lock(&client1, RBX::DataModelJob::Write);
		clientPart1->setName(CHANGED_STRING);
	}

	BOOST_MESSAGE("Giving client-1 a chance to replicate its name change");
	G3D::System::sleep(SLEEPTIME);

	// Confirm that client 1's change was accepted because the network filter is disabled
	{
		RBX::DataModel::LegacyLock lock(&server, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getName(), CHANGED_STRING);
	}
	{
		RBX::DataModel::LegacyLock lock(&client2, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(clientPart2->getName(), CHANGED_STRING);
	}
}


BOOST_AUTO_TEST_CASE(ShipsInTheNight)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part on the server
	shared_ptr<PartInstance> serverPart;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName("TestPart");
		serverPart->setParent(serverDm->getWorkspace());
	}

	BOOST_MESSAGE("Waiting for TestPart to replicate");
	G3D::System::sleep(SLEEPTIME);

	// Confirm the client got it
	Instance* clientPart;
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart = clientDm->getWorkspace()->findFirstChildByName("TestPart");
		BOOST_REQUIRE(clientPart);
	}

	// simulate network lag from server to client
	clientDm.execute("r = game:GetService('NetworkClient'):GetChildren()[1] r:SetPropSyncExpiration(60) r:DisableProcessPackets()");
	serverDm.execute("r = game:GetService('NetworkServer'):GetChildren()[1] r:SetPropSyncExpiration(30)");

	// Now make both sides change the name
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		serverPart->setName("Red");
	}
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart->setName("Blue");
	}

	BOOST_MESSAGE("Waiting for client to replicate Blue to server");
	G3D::System::sleep(SLEEPTIME);

	// Confirm that client's value was rejected because Red hasn't been acknowledged by Client
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getName(), "Red");
	}
	// Confirm that client hasn't gotten the Red value yet
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(clientPart->getName(), "Blue");
	}

	// Re-open the pipe from server to client
	clientDm.execute("game:GetService('NetworkClient'):GetChildren()[1]:EnableProcessPackets()");

	BOOST_MESSAGE("Waiting for server to replicate Red to client");
	G3D::System::sleep(SLEEPTIME);

	// Confirm that the parts agree on Red
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getName(), "Red");
	}
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(clientPart->getName(), "Red");
	}

	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart->setName("Yellow");
	}

	BOOST_MESSAGE("Waiting for client to replicate Yellow to server");
	G3D::System::sleep(SLEEPTIME);

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getName(), "Yellow");
	}

}

// This test deals with the case where a script changes a property to a new value once it changes.
// The networking code should honor this case and make sure all peers get the right value.
BOOST_AUTO_TEST_CASE(PropertyBounceBack)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part on the server
	shared_ptr<PartInstance> serverPart;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName("TestPart");
		serverPart->setParent(serverDm->getWorkspace());
	}

	BOOST_MESSAGE("Waiting for TestPart to replicate");
	G3D::System::sleep(SLEEPTIME);

	// Confirm the client got it
	Instance* clientPart;
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart = clientDm->getWorkspace()->findFirstChildByName("TestPart");
		BOOST_REQUIRE(clientPart);
	}

	// set up a bounceback on the server
	serverDm.execute("p = workspace.TestPart p.Changed:connect(function() p.Name = 'Blue' end)");

	// Now make the client change a property
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart->setName("Red");
	}

	BOOST_MESSAGE("Waiting for client to replicate Red to server");
	G3D::System::sleep(SLEEPTIME);

	// Confirm that both client and server are Blue
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(serverPart->getName(), "Blue");
	}
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		BOOST_CHECK_EQUAL(clientPart->getName(), "Blue");
	}
}

class ParentChangeRecorder
{
public:
	std::vector<std::string> parentList;
	boost::shared_ptr<RBX::Instance> instance;

	rbx::signals::scoped_connection propChangedConnection;
	rbx::signals::scoped_connection ancestryChangedConnection;

	ParentChangeRecorder(boost::shared_ptr<RBX::Instance>_instance) : instance(_instance)
	{
		propChangedConnection = instance->propertyChangedSignal.connect(boost::bind(&ParentChangeRecorder::onParentChanged, this, _1));
		ancestryChangedConnection = instance->ancestryChangedSignal.connect(boost::bind(&ParentChangeRecorder::onAncestryChanged, this, _1, _2));
	}

	~ParentChangeRecorder() {}

	void disconnectListeners()
	{
		propChangedConnection.disconnect();
		ancestryChangedConnection.disconnect();
	}

	void onParentChanged(const RBX::Reflection::PropertyDescriptor* desc)
	{
		if (*desc == RBX::Instance::propParent)
		{
			if (instance->getParent())
				parentList.push_back(instance->getParent()->getName());
			else
				parentList.push_back("NULL");
		}
	}

	void onAncestryChanged(shared_ptr<Instance> child, shared_ptr<Instance> newParent)
	{
		BOOST_CHECK(Workspace::getWorkspaceIfInWorkspace(instance.get()) != NULL);
	}
};

BOOST_AUTO_TEST_CASE(ParentPropChangeArriveInOrder)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part with children on the server
	shared_ptr<PartInstance> serverPart;
	shared_ptr<PartInstance> childPart;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName("TestPart");
        serverPart->setParent(serverDm->getWorkspace());

		childPart = Creatable<Instance>::create<BasicPartInstance>();
		childPart->setAnchored(true);
		childPart->setName("child");
		childPart->setParent(serverPart.get());
	}

	BOOST_MESSAGE("Waiting for TestPart to replicate");
	G3D::System::sleep(SLEEPTIME);

	// Confirm the client got it
	Instance* clientPart;
	shared_ptr<ParentChangeRecorder> parentChangeRecorder;
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart = clientDm->getWorkspace()->findFirstChildByName("TestPart");
        BOOST_REQUIRE(clientPart);
		Instance* clientChildPart = clientPart->findFirstChildByName("child");
		BOOST_REQUIRE(clientChildPart);

		parentChangeRecorder.reset(new ParentChangeRecorder(shared_from(clientChildPart)));
	}

	BOOST_MESSAGE("Modifying parent on Server");
	// modify children's parent on server
	shared_ptr<PartInstance> serverPart2;
	shared_ptr<PartInstance> serverPart3;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

        BOOST_MESSAGE("Creating serverPart2");
		serverPart2 = Creatable<Instance>::create<BasicPartInstance>();
		serverPart2->setAnchored(true);
		serverPart2->setName("TestPart2");
		serverPart2->setParent(serverDm->getWorkspace());

        BOOST_MESSAGE("Setting parent to serverPart2");
		childPart->setParent(serverPart2.get());
		
		serverPart->setParent(NULL);

        BOOST_MESSAGE("Creating serverPart3");
		serverPart3 = Creatable<Instance>::create<BasicPartInstance>();
		serverPart3->setAnchored(true);
		serverPart3->setName("TestPart3");
		serverPart3->setParent(serverDm->getWorkspace());

        BOOST_MESSAGE("Setting parent to serverPart3");
		childPart->setParent(serverPart3.get());

	}

	G3D::System::sleep(SLEEPTIME);

	// Comfirm parent of all children is set correctly on client
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		
		parentChangeRecorder->disconnectListeners();
		
		BOOST_CHECK_EQUAL(parentChangeRecorder->parentList.size(), 2);

		// validate parents were set in correct order
		BOOST_CHECK_EQUAL(parentChangeRecorder->parentList[0], "TestPart2");
		BOOST_CHECK_EQUAL(parentChangeRecorder->parentList[1], "TestPart3");
	}

	
}

static void NewInstanceParentAlwaysValid_onChildAdded(shared_ptr<Instance> child, shared_ptr<RBX::CEvent> event)
{
	if (child->getName() == "TestPart2")
		event->Set();
}

BOOST_AUTO_TEST_CASE(NewInstanceParentAlwaysValid)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// Create a part with children on the server
	shared_ptr<PartInstance> serverPart;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverPart = Creatable<Instance>::create<BasicPartInstance>();
		serverPart->setAnchored(true);
		serverPart->setName("TestPart");
		serverPart->setParent(serverDm->getWorkspace());
	}

	BOOST_MESSAGE("Waiting for TestPart to replicate");
	G3D::System::sleep(SLEEPTIME);

	// Confirm the client got it
	Instance* clientPart;
	shared_ptr<RBX::CEvent> event(new RBX::CEvent(true));
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientPart = clientDm->getWorkspace()->findFirstChildByName("TestPart");
		BOOST_REQUIRE(clientPart);
		clientPart->onDemandWrite()->childAddedSignal.connect(boost::bind(&NewInstanceParentAlwaysValid_onChildAdded, _1, event));
	}

	// Create new parts on server
	shared_ptr<PartInstance> serverPart2;
	shared_ptr<PartInstance> serverPart3;	
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		serverPart2 = Creatable<Instance>::create<BasicPartInstance>();
		serverPart2->setAnchored(true);
		serverPart2->setName("TestPart2");
		serverPart2->setParent(serverPart.get()); // this should create a NewInstanceItem in replicator

		serverPart3 = Creatable<Instance>::create<BasicPartInstance>();
		serverPart3->setAnchored(true);
		serverPart3->setName("TestPart3");
		serverPart3->setParent(serverDm->getWorkspace());

		serverPart2->setParent(serverPart3.get());	// this should create a ParentPropChangeItem in replicator
	}

	// Confirm the client got it and set parent correctly
	BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(60)));
	G3D::System::sleep(SLEEPTIME);
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		Instance* clientTestPart3 = clientDm->getWorkspace()->findFirstChildByName("TestPart3");
		BOOST_REQUIRE(clientTestPart3);
		BOOST_CHECK(clientTestPart3->findFirstChildByName("TestPart2") != NULL);
	}
}

static void onChildAdded(shared_ptr<Instance> child, shared_ptr<RBX::CEvent> event)
{
	if (child->getName() == "LastPart")
		event->Set();
};

BOOST_AUTO_TEST_CASE(GuidsAreAssignedImmeidatelyOnCreation)
{
	shared_ptr<BasicPartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
	Guid::Data data;
	part->getGuid().extract(data);
	BOOST_CHECK(!data.scope.isNull());
}

BOOST_AUTO_TEST_CASE(ReplicationDoesNotAllowRedundantNewJointsToBeSeen)
{
	DataModelFixture serverDm;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		shared_ptr<BasicPartInstance> part0 = Creatable<Instance>::create<BasicPartInstance>();
		shared_ptr<BasicPartInstance> part1 = Creatable<Instance>::create<BasicPartInstance>();
		shared_ptr<Motor6D> motor = Creatable<Instance>::create<Motor6D>();

		motor->setName("Motor6D");
		motor->setPart0(part0.get());
		motor->setPart1(part1.get());
		motor->setParent(part0.get());

		part1->setName("Part1");
		part1->setParent(serverDm->getWorkspace());

		part0->setName("Part0");
		part0->setParent(serverDm->getWorkspace());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// wait for client to get initial parts
	G3D::System::sleep(SLEEPTIME);

	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);

		Workspace* workspace = clientDm->getWorkspace();

		BasicPartInstance* part0 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part0"));
		BOOST_REQUIRE(part0);
		BasicPartInstance* part1 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part1"));
		BOOST_REQUIRE(part1);
		Motor6D* motor = Instance::fastDynamicCast<Motor6D>(
			part0->findFirstChildByName("Motor6D"));
		BOOST_REQUIRE(motor);

		// This will temporarily unconnect the existing motor, attach the
		// parts with a manual weld, then destroy the manual weld and reattach
		// the motor.
		motor->setPart1(NULL);
		shared_ptr<ManualWeld> tempWeld = Creatable<Instance>::create<ManualWeld>();
		tempWeld->setPart0(part0);
		tempWeld->setPart1(part1);
		tempWeld->setName("ManualWeld");
		tempWeld->setParent(part0);
		tempWeld->setParent(NULL);
		motor->setPart1(part1);

		// sanity check that the engine has not kicked motor out
		BOOST_CHECK(motor->getParent());
	}

	// Wait of sequence to replicate to server
	G3D::System::sleep(SLEEPTIME);

	// make sure that the server joint state is valid
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		Workspace* workspace = serverDm->getWorkspace();
		BasicPartInstance* part0 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part0"));
		BasicPartInstance* part1 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part1"));
		Motor6D* motor = Instance::fastDynamicCast<Motor6D>(
			part0->findFirstChildByName("Motor6D"));
		Instance* tempWeld = part0->findFirstChildByName("ManualWeld");

		// verify everything is preserved in the correct state
		BOOST_CHECK(part0);
		BOOST_CHECK(part1);
		BOOST_CHECK(motor);
		BOOST_CHECK(tempWeld == NULL);
	}
}

BOOST_AUTO_TEST_CASE(ReplicationDoesNotAllowRedundantExistingJointsToBeSeen)
{
	DataModelFixture serverDm;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		shared_ptr<BasicPartInstance> part0 = Creatable<Instance>::create<BasicPartInstance>();
		shared_ptr<BasicPartInstance> part1 = Creatable<Instance>::create<BasicPartInstance>();
		shared_ptr<Motor6D> motor = Creatable<Instance>::create<Motor6D>();
		shared_ptr<ManualWeld> weld = Creatable<Instance>::create<ManualWeld>();
		Lighting* lighting = ServiceProvider::create<Lighting>(&serverDm);
		BOOST_REQUIRE(lighting);

		motor->setName("Motor6D");
		motor->setPart0(part0.get());
		motor->setPart1(part1.get());
		motor->setParent(serverDm->getWorkspace());

		part1->setName("Part1");
		part1->setParent(serverDm->getWorkspace());

		part0->setName("Part0");
		part0->setParent(serverDm->getWorkspace());

		weld->setName("ManualWeld");
		weld->setPart0(part0.get());
		weld->setPart1(part0.get());
		weld->setParent(lighting);
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	// wait for client to get initial parts
	G3D::System::sleep(SLEEPTIME);

	{
		DataModel::LegacyLock lock(&clientDm, DataModelJob::Write);

		Lighting* lighting = ServiceProvider::find<Lighting>(&clientDm);
		BOOST_REQUIRE(lighting);
		
		Motor6D* motor = Instance::fastDynamicCast<Motor6D>(
			clientDm->getWorkspace()->findFirstChildByName("Motor6D"));
		BOOST_REQUIRE(motor);
		ManualWeld* weld = Instance::fastDynamicCast<ManualWeld>(
			lighting->findFirstChildByName("ManualWeld"));
		BOOST_REQUIRE(weld);

		weld->setPart1(NULL);
		weld->setParent(clientDm->getWorkspace());
		PartInstance* part1 = motor->getPart1();
		BOOST_REQUIRE(part1);
		motor->setPart1(NULL);
		weld->setPart1(part1);
	}

	// let server catch up
	G3D::System::sleep(SLEEPTIME);

	// verify that everything is intact
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		Workspace* workspace = serverDm->getWorkspace();
		BasicPartInstance* part0 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part0"));
		BasicPartInstance* part1 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part1"));
		Motor6D* motor = Instance::fastDynamicCast<Motor6D>(
			workspace->findFirstChildByName("Motor6D"));
		ManualWeld* weld = Instance::fastDynamicCast<ManualWeld>(
			workspace->findFirstChildByName("ManualWeld"));

		// verify everything is preserved in the correct state
		BOOST_CHECK(part0);
		BOOST_CHECK(part1);
		BOOST_CHECK(motor);
		BOOST_CHECK(weld);
		BOOST_CHECK_EQUAL(motor->getPart0(), part0);
		BOOST_CHECK(motor->getPart1() == NULL);
		BOOST_CHECK_EQUAL(weld->getPart0(), part0);
		BOOST_CHECK_EQUAL(weld->getPart1(), part1);
	}
}

BOOST_AUTO_TEST_CASE(ReplicationDoesNotAllowRedundantExistingJointsWithCharacterToBeSeen)
{
	DataModelFixture serverDm;
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		shared_ptr<BasicPartInstance> part0 = Creatable<Instance>::create<BasicPartInstance>();
		shared_ptr<BasicPartInstance> part1 = Creatable<Instance>::create<BasicPartInstance>();
		shared_ptr<Motor6D> motor = Creatable<Instance>::create<Motor6D>();
		shared_ptr<ManualWeld> weld = Creatable<Instance>::create<ManualWeld>();
		Lighting* lighting = ServiceProvider::create<Lighting>(&serverDm);
		// Sanity check: actually have lighting created
		BOOST_REQUIRE(lighting);

		motor->setName("Motor6D");
		motor->setPart0(part0.get());
		motor->setPart1(part1.get());
		motor->setParent(serverDm->getWorkspace());

		part1->setName("Part1");
		part1->setParent(serverDm->getWorkspace());

		part0->setName("Part0");
		part0->setParent(serverDm->getWorkspace());

		weld->setName("ManualWeld");
		weld->setPart0(part0.get());
		weld->setPart1(part0.get());
		weld->setParent(lighting);
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm, true);

	// wait for client to get initial parts
	G3D::System::sleep(SLEEPTIME);

	ModelInstance* clientCharacterModel;
	{
		DataModel::LegacyLock lock(&clientDm, DataModelJob::Write);

		Lighting* lighting = ServiceProvider::find<Lighting>(&clientDm);
		BOOST_REQUIRE(lighting);
		Player* player = Players::findLocalPlayer(&clientDm);
		BOOST_REQUIRE(player);
		clientCharacterModel = player->getCharacter();
		BOOST_REQUIRE(clientCharacterModel);
		
		Motor6D* motor = Instance::fastDynamicCast<Motor6D>(
			clientDm->getWorkspace()->findFirstChildByName("Motor6D"));
		BOOST_REQUIRE(motor);
		ManualWeld* weld = Instance::fastDynamicCast<ManualWeld>(
			lighting->findFirstChildByName("ManualWeld"));
		BOOST_REQUIRE(weld);

		weld->setParent(clientCharacterModel);
		PartInstance* part1 = motor->getPart1();
		BOOST_REQUIRE(part1);
		motor->setPart1(NULL);
		weld->setPart1(part1);

		// Sanity check that the motor and weld haven't been kicked out
		// due to the duplicate-joint constraint
		BOOST_REQUIRE(motor->getParent());
		BOOST_REQUIRE(motor->getPart0() != NULL);
		BOOST_REQUIRE(motor->getPart1() == NULL);
		BOOST_REQUIRE(weld->getParent());
		BOOST_REQUIRE(weld->getPart0() == motor->getPart0());
		BOOST_REQUIRE(weld->getPart1() == part1);
	}

	// let server catch up
	G3D::System::sleep(SLEEPTIME);

	// verify that everything is intact on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		Workspace* workspace = serverDm->getWorkspace();
		BasicPartInstance* part0 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part0"));
		BasicPartInstance* part1 = Instance::fastDynamicCast<BasicPartInstance>(
			workspace->findFirstChildByName("Part1"));
		Motor6D* motor = Instance::fastDynamicCast<Motor6D>(
			workspace->findFirstChildByName("Motor6D"));
		ModelInstance* playerModel = Instance::fastDynamicCast<ModelInstance>(
			workspace->findFirstChildByName(clientCharacterModel->getName()));
		BOOST_REQUIRE(playerModel);
		ManualWeld* weld = Instance::fastDynamicCast<ManualWeld>(
			playerModel->findFirstChildByName("ManualWeld"));

		// verify everything is preserved in the correct state
		BOOST_CHECK(part0);
		BOOST_CHECK(part1);
		BOOST_CHECK(motor);
		BOOST_CHECK(weld);
		BOOST_CHECK_EQUAL(motor->getPart0(), part0);
		BOOST_CHECK(motor->getPart1() == NULL);
		BOOST_CHECK_EQUAL(weld->getPart0(), part0);
		BOOST_CHECK_EQUAL(weld->getPart1(), part1);
	}
}

static void onScriptLockGrantedOrNot(bool lockGranted, bool* state, RBX::CEvent* event)
{
	*state = lockGranted;
	event->Set();
}

BOOST_AUTO_TEST_CASE(ReplicationExplicitlyAssignDefaultPropertyValues)
{
	ScopedFastFlagSetting flag("ExplicitlyAssignDefaultPropVal", true);

	const PartMaterial kDefaultPartMaterial(PLASTIC_MATERIAL);
	const PartMaterial kPartMaterial(GRANITE_MATERIAL);

	const std::string kDefaultPartName("Part");	
	const std::string kDefaultScriptName("Script");
	const std::string kScriptName("Script_New");

	DataModelFixture serverDm;

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		shared_ptr<Script> script = Creatable<Instance>::create<Script>();
		script->setName(kScriptName);		
		script->setParent(serverDm->getWorkspace());

		shared_ptr<BasicPartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
		part->setRenderMaterial(kPartMaterial);
		part->setParent(serverDm->getWorkspace());
	}
	NetworkFixture::startServer(serverDm);

	// start clients
	DataModelFixture clientDm1;
	DataModelFixture clientDm2;
	NetworkFixture::startClient(clientDm1);
	NetworkFixture::startClient(clientDm2);

	// wait for clients to get script
	G3D::System::sleep(SLEEPTIME);

	// client1 instances
	BasicPartInstance* partClient1 = NULL;
	Script* scriptClient1 = NULL;
	Instance* localPlayerClient1 = NULL;

	bool scriptLockedStateClient1 = false;
	CEvent requestLockWaitEvent(true);

	{
		DataModel::LegacyLock lock(&clientDm1, DataModelJob::Write);

		localPlayerClient1 = Players::findLocalPlayer(&clientDm1);
		BOOST_REQUIRE(localPlayerClient1);

		// check if initial values are correct
		partClient1 = Instance::fastDynamicCast<BasicPartInstance>(clientDm1->getWorkspace()->findFirstChildByName(kDefaultPartName));
		BOOST_REQUIRE(partClient1);
		BOOST_REQUIRE(partClient1->getRenderMaterial() == kPartMaterial);
		scriptClient1 = Instance::fastDynamicCast<Script>(clientDm1->getWorkspace()->findFirstChildByName(kScriptName));
		BOOST_REQUIRE(scriptClient1);
		BOOST_REQUIRE(scriptClient1->getCurrentEditor() == NULL);
		
		// lock script
		scriptClient1->lockGrantedOrNot.connect(boost::bind(&onScriptLockGrantedOrNot, _1, &scriptLockedStateClient1, &requestLockWaitEvent));
		LuaSourceContainer::event_requestLock.replicateEvent(scriptClient1);
	}

	BOOST_CHECK(requestLockWaitEvent.Wait(RBX::Time::Interval::from_seconds(SLEEPTIME)));

	{
		DataModel::LegacyLock lock(&clientDm1, DataModelJob::Read);
		// make sure we get a lock on the script in client1
		BOOST_REQUIRE(scriptLockedStateClient1);
		// edit must not be null and it must be local player
		BOOST_REQUIRE(scriptClient1->getCurrentEditor());
		BOOST_REQUIRE(scriptClient1->getCurrentEditor() == localPlayerClient1);
	}

	G3D::System::sleep(1);

	shared_ptr<Script> scriptClient2;
	shared_ptr<BasicPartInstance> partClient2;
	{
		DataModel::LegacyLock lock(&clientDm2, DataModelJob::Write);
		// check initial values for client2, script must be locked
		Script* script = Instance::fastDynamicCast<Script>(clientDm2->getWorkspace()->findFirstChildByName(kScriptName));		
		BOOST_REQUIRE(script);
		BOOST_REQUIRE(script->getCurrentEditor());
		BasicPartInstance* part = Instance::fastDynamicCast<BasicPartInstance>(clientDm2->getWorkspace()->findFirstChildByName(kDefaultPartName));
		BOOST_REQUIRE(part);
		BOOST_REQUIRE(part->getRenderMaterial() == kPartMaterial);

		// hold on to pointers, so it doesn't get released
		scriptClient2 = shared_from(script);
		partClient2 = shared_from(part);
	}

	{
		DataModel::LegacyLock lock(&clientDm1, DataModelJob::Write);
		// change parent of script to DataModel
		scriptClient1->setParent(&clientDm1);
		partClient1->setParent(&clientDm1);
	}

	G3D::System::sleep(1);

	{
		DataModel::LegacyLock lock(&clientDm2, DataModelJob::Write);
		// since parents for script and part were changed to DataModel in client1 there must not be any script or part in client2
		Script* script = Instance::fastDynamicCast<Script>(clientDm2->getWorkspace()->findFirstChildByName(kScriptName));
		BOOST_REQUIRE(script == NULL);
		script = Instance::fastDynamicCast<Script>(clientDm2->getWorkspace()->findFirstChildByName(kDefaultScriptName));
		BOOST_REQUIRE(script == NULL);
		BasicPartInstance* part = Instance::fastDynamicCast<BasicPartInstance>(clientDm2->getWorkspace()->findFirstChildByName(kDefaultPartName));
		BOOST_REQUIRE(part == NULL);
	}

	{
		DataModel::LegacyLock lock(&clientDm1, DataModelJob::Write);
		// now remove the editor and change script name to default name
		scriptClient1->setCurrentEditor(NULL);
		scriptClient1->setName(kDefaultScriptName);
		scriptClient1->setParent(clientDm1->getWorkspace());
		// change material to default material
		partClient1->setRenderMaterial(kDefaultPartMaterial);
		partClient1->setParent(clientDm1->getWorkspace());
	}

	G3D::System::sleep(1);

	{
		DataModel::LegacyLock lock(&clientDm2, DataModelJob::Write);		
		
		// make sure script doesn't exist with original name
		Script* script = Instance::fastDynamicCast<Script>(clientDm2->getWorkspace()->findFirstChildByName(kScriptName));
		BOOST_REQUIRE(script == NULL);
		
		// script must be present with default name and editor must be NULL
		script = Instance::fastDynamicCast<Script>(clientDm2->getWorkspace()->findFirstChildByName(kDefaultScriptName));
		BOOST_REQUIRE(script);
		BOOST_REQUIRE(script->getCurrentEditor() == NULL);

		// part must be present with default material
		BasicPartInstance* part = Instance::fastDynamicCast<BasicPartInstance>(clientDm2->getWorkspace()->findFirstChildByName(kDefaultPartName));
		BOOST_REQUIRE(part);
		BOOST_REQUIRE(part->getRenderMaterial() == kDefaultPartMaterial);
	}
}

#if defined(WIN32) && 0
bool startStressTestClientProc(std::string& commandLine)
{
    if (RBX::Test::BaseGlobalFixture::fflagsAllOn)
    {
        commandLine.append(" --fflags=true");
    }
    else if (RBX::Test::BaseGlobalFixture::fflagsAllOff)
    {
        commandLine.append(" --fflags=false");
    }

    printf("Starting client proc with parameter: %s\n", commandLine.c_str());
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    TCHAR* tCharCmdline = new TCHAR[commandLine.size() + 1];
    tCharCmdline[commandLine.size()] = '\0';
    memcpy(tCharCmdline, commandLine.c_str(), sizeof(TCHAR)*commandLine.length());

    LPTSTR szCmdline = (LPTSTR)tCharCmdline;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    if (!CreateProcess(szPath,   //  module
        szCmdline,      // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        TRUE,          // Set handle inheritance to TRUE
        0,              // creation flag, 0 - reuse the current console
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf( "Child proc failed to start: (%d).\n", GetLastError() );
        return false;
    }
    return true;
}

// --log_level=test_suite --fflags=true --run_test=Network/ServerStressTest
// Uncomment #define TASKSCHEDULAR_PROFILING in TaskScheduler.cpp to see real-time profiling result
bool isServerStressTestServer = false;
BOOST_AUTO_TEST_CASE(ServerStressTest)
{
    ScopedFastFlagSetting flagSettingInitial("DebugTaskSchedulerProfiling", false);
    isServerStressTestServer = true;
    DataModelFixture serverDm;
    {
        NetworkFixture::startServer(serverDm);
    }
    serverDm->getWorkspace()->createTerrain();
    std::string commandLine = "--log_level=test_suite --run_test=Network/ServerStressTestClient";
    int clientCount = 50;
    for (int i=0; i<clientCount; i++)
    {
        BOOST_REQUIRE(startStressTestClientProc(commandLine));
    }
    G3D::System::sleep(clientCount/4); // wait for clientCount/4 seconds before all clients have connected
    ScopedFastFlagSetting flagSetting("DebugTaskSchedulerProfiling", true);
    G3D::System::sleep(1000);
}

BOOST_AUTO_TEST_CASE(ServerStressTestClient)
{
    if (isServerStressTestServer)
    {
        return;
    }
    ScopedFastFlagSetting flagSetting("DebugTaskSchedulerProfiling", false);
    DataModelFixture clientDm;
    {
        {
            DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

            // create base plate on client so player is guaranteed to land on it
            shared_ptr<BasicPartInstance> basePlate = Creatable<Instance>::create<BasicPartInstance>();
            basePlate->setTranslationUi(Vector3::zero());
            basePlate->setPartSizeUi(Vector3(500, 2.4f, 500));
            basePlate->setAnchored(true);
            basePlate->setName("BasePlate");
            basePlate->setParent(clientDm->getWorkspace());
        }

        NetworkFixture::startClient(clientDm);

        for (int i=0; i<10000; i++)
        {
            {
                DataModel::LegacyLock l(&clientDm, DataModelJob::Write);
                G3D::Random rng(1);
                RBX::Network::Players* players = RBX::ServiceProvider::find<RBX::Network::Players>(&clientDm);
                RBXASSERT(players);
                RBX::Network::Player* player = players->getLocalPlayer();
                RBXASSERT(player);
                ModelInstance* character = player->getCharacter();
                if (character)
                {
                    PartInstance* torso = Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("Torso"));
                    RBXASSERT(torso);
                    torso->setCoordinateFrameRoot(CoordinateFrame(torso->getTranslationUi() + Vector3((float)(rng.integer(0, 100)), i%100, 0)));
                }
            }
            G3D::System::sleep(0.1);
        }
    }
}

#endif

#ifndef _DEBUG
BOOST_FIXTURE_TEST_CASE(Perf, RBX::Test::PerformanceTestFixture)
{
	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	shared_ptr<RBX::CEvent> event(new RBX::CEvent(true));
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		clientDm->getWorkspace()->onDemandWrite()->childAddedSignal.connect(boost::bind(&onChildAdded, _1, event));
	}

	// Create a part on the server
	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

		for (int i = 0; i < 50; ++i)
			for (int j = 0; j < 50; ++j)
			{
				shared_ptr<PartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
				part->setAnchored(true);
				part->setName("TestPart");
				part->setCoordinateFrame(RBX::CoordinateFrame(G3D::Vector3(4*i, 4*j, 0)));
				part->setParent(serverDm->getWorkspace());
			}
		shared_ptr<PartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
		part->setAnchored(true);
		part->setName("LastPart");
		part->setCoordinateFrame(RBX::CoordinateFrame(G3D::Vector3(0, 0, 10)));
		part->setParent(serverDm->getWorkspace());
	}

	BOOST_MESSAGE("Waiting for LastPart to replicate");

	// Confirm the client got it
	BOOST_CHECK(event->Wait(RBX::Time::Interval::from_seconds(60)));	
}
#endif

static void on_dp_ChildAdded(shared_ptr<Instance> child, shared_ptr<RBX::CEvent> event)
{
	if (child->getName() == "LastPartDP")
		event->Set();
};

static void onGameLoaded( shared_ptr<RBX::CEvent> event)
{
	BOOST_MESSAGE("game loaded");
	event->Set();
};


BOOST_FIXTURE_TEST_CASE(DeserializeProcessTest, RBX::Test::PerformanceTestFixture)
{		
	RBX::Time now;
	RBX::Time was;

	DataModelFixture serverDm;
	NetworkFixture::startServer(serverDm);
		
	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm);

	serverDm->getWorkspace()->setNetworkFilteringEnabled(false);

	shared_ptr<RBX::CEvent> event1(new RBX::CEvent(true));
	shared_ptr<RBX::CEvent> event2(new RBX::CEvent(true));

	{
		RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);
		serverDm->getWorkspace()->onDemandWrite()->childAddedSignal.connect(boost::bind(&on_dp_ChildAdded, _1, event1));
	}

	clientDm->gameLoadedSignal.connect(boost::bind(&onGameLoaded, event2));
	event2->Wait(RBX::Time::Interval::from_seconds(2));	// if it takes this long it must be ready.

	// Create parts on the client
	{
		RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
		BOOST_MESSAGE("Waiting for LastPart to replicate");

		was = RBX::Time::now(RBX::Time::Benchmark);

		for (int i = 0; i < 50; ++i)
			for (int j = 0; j < 50; ++j)
			{
				shared_ptr<PartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
				part->setAnchored(true);
				part->setName("TestPart");
				part->setCoordinateFrame(RBX::CoordinateFrame(G3D::Vector3(4*i, 4*j, 0)));
				part->setParent(clientDm->getWorkspace());
			}
			shared_ptr<PartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
			part->setAnchored(true);
			part->setName("LastPartDP");
			part->setCoordinateFrame(RBX::CoordinateFrame(G3D::Vector3(0, 0, 10)));
			part->setParent(clientDm->getWorkspace());
	}
	// Confirm the client got it
	BOOST_CHECK(event1->Wait(RBX::Time::Interval::from_seconds(60)));
	now = RBX::Time::now(RBX::Time::Benchmark);
	double ptime = (now - was).seconds();

	BOOST_MESSAGE(RBX::format("time for parts to propagate to server %f seconds", ptime));

}

BOOST_AUTO_TEST_SUITE_END()

// TODO: Move to DataModelFixture.cpp
std::auto_ptr<RBX::Reflection::Tuple> DataModelFixture::execute(const char* script, const RBX::Reflection::Tuple& args, RBX::Security::Identities identity)
{
	DataModel::LegacyLock lock(dataModel.get(), DataModelJob::Write);

	ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(dataModel.get());

	return scriptContext->executeInNewThread(identity, ProtectedString::fromTestSource(script), "Script", args);
}

BOOST_AUTO_TEST_SUITE(ProtocolSchema)

BOOST_AUTO_TEST_CASE(NewClass)
{
    ScopedFastFlagSetting debugProtocolSynchronization("DebugProtocolSynchronization", true);
    ScopedFastFlagSetting debugLocalRccServerConnection("DebugLocalRccServerConnection", false);
    ScopedFastFlagSetting debugForceRegenerateSchemaBitStream("DebugForceRegenerateSchemaBitStream", true);
    DataModelFixture serverDm;
    {
        NetworkFixture::startServer(serverDm);
    }
    DataModelFixture clientDm;
    {
        NetworkFixture::startClient(clientDm);
        // Wait for connection and protocol syncing
        G3D::System::sleep(1);
    }

    {
        // Create an instance from the new class on the server
        {
            RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

            shared_ptr<PartInstance> ins = Creatable<Instance>::create<RBX::TheNewClass>();
            ins->setAnchored(true);
            ins->setName("TheNewClass");
            ins->setParent(serverDm->getWorkspace());
        }

        // Wait for it to replicate
        G3D::System::sleep(0.5);
    }

    // Confirm the client does not have it
    {
        RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);
        BOOST_CHECK(clientDm->getWorkspace()->findFirstChildByName("TheNewClass") == 0);
    }
}
#ifdef _WIN32

bool startClientProc(std::string& commandLine)
{
    if (RBX::Test::BaseGlobalFixture::fflagsAllOn)
    {
        commandLine.append(" --fflags=true");
    }
    else if (RBX::Test::BaseGlobalFixture::fflagsAllOff)
    {
        commandLine.append(" --fflags=false");
    }

    printf("Starting client proc with parameter: %s\n", commandLine.c_str());
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    TCHAR* tCharCmdline = new TCHAR[commandLine.size() + 1];
    tCharCmdline[commandLine.size()] = '\0';
    memcpy(tCharCmdline, commandLine.c_str(), sizeof(TCHAR)*commandLine.length());

    LPTSTR szCmdline = (LPTSTR)tCharCmdline;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    if (!CreateProcess(szPath,   //  module
        szCmdline,      // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // creation flag, 0 - reuse the current console
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf( "Child proc failed to start: (%d).\n", GetLastError() );
        return false;
    }
    return true;
}

bool isPropTestServer = false;

BOOST_AUTO_TEST_CASE(PropTestServer)
{
    // Note, PropTestClient and PropTestServer are twins. Please keep the timing synchronized.
    isPropTestServer = true;
    registerProps(true);
    ScopedFastFlagSetting debugProtoSyncFlag("DebugProtocolSynchronization", false);
    ScopedFastFlagSetting debugLocalRccServerConnection("DebugLocalRccServerConnection", false);
    ScopedFastFlagSetting debugForceRegenerateSchemaBitStream("DebugForceRegenerateSchemaBitStream", true);
    DataModelFixture serverDm;
    {
        NetworkFixture::startServer(serverDm);
    }

    std::string commandLine = "--log_level=test_suite --run_test=ProtocolSchema/PropTestClient";
    BOOST_REQUIRE(startClientProc(commandLine));

    // wait for client init
    G3D::System::sleep(1);

    shared_ptr<PartInstance> serverIns;
    {
        RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

        // Create an instance from the new class on the server
        BOOST_MESSAGE("Creating instance from TheNewClass...");
        serverIns = Creatable<Instance>::create<RBX::TheNewClass>();
        serverIns->setAnchored(true);
        serverIns->setName("TheNewClass");
        serverIns->setParent(serverDm->getWorkspace());
        serverIns->fastDynamicCast<RBX::TheNewClass>()->setExistingProp(1);
        serverIns->fastDynamicCast<RBX::TheNewClass>()->setTheNewProp(1);
    }

    // Wait for replication
    G3D::System::sleep(1);

    {
        RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

        // Change the property values
        BOOST_MESSAGE("Changing ExistingProp and TheNewProp...");
        serverIns->fastDynamicCast<RBX::TheNewClass>()->setExistingProp(2);
        serverIns->fastDynamicCast<RBX::TheNewClass>()->setTheNewProp(2);
    }

    // Wait for replication
    G3D::System::sleep(1);
}

BOOST_AUTO_TEST_CASE(PropTestClient)
{
    // Note, PropTestClient and PropTestServer are twins. Please keep the timing synchronized.
    if (isPropTestServer)
    {
        return;
    }
    registerProps(false);
	ScopedFastFlagSetting debugProtoSyncFlag("DebugProtocolSynchronization", false);
	ScopedFastFlagSetting debugLocalRccServerConnection("DebugLocalRccServerConnection", false);
	ScopedFastFlagSetting debugforceRegenerateSchemaBitStream("DebugForceRegenerateSchemaBitStream", true);
    DataModelFixture clientDm;
    {
        NetworkFixture::startClient(clientDm);
    }
    // Wait for connection
    G3D::System::sleep(0.5);

    // Wait for replication
    G3D::System::sleep(1);

    Instance* clientIns;
    {
        RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);

        // Confirm the instance creation
        clientIns = clientDm->getWorkspace()->findFirstChildByName("TheNewClass");
        BOOST_REQUIRE(clientIns);
        RBX::TheNewClass* newIns = clientIns->fastDynamicCast<RBX::TheNewClass>();
        BOOST_CHECK_EQUAL(newIns->getExistingProp(), 1);
        BOOST_CHECK_EQUAL(newIns->getTheNewProp(), 0);
    }

    // Wait for replication
    G3D::System::sleep(1);

    {
        RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);

        // Confirm the property updates
        RBX::TheNewClass* newIns = clientIns->fastDynamicCast<RBX::TheNewClass>();
        BOOST_CHECK_EQUAL(newIns->getExistingProp(), 2);
        BOOST_CHECK_EQUAL(newIns->getTheNewProp(), 0);
    }
}

bool isEventTestServer = false;

BOOST_AUTO_TEST_CASE(EventTestServer)
{
    // Note, EventTestClient and EventTestServer are twins. Please keep the timing synchronized.
    isEventTestServer = true;
    if (!isPropTestServer)
    {
        registerProps(true);
    }
    registerEvents(true);
    ScopedFastFlagSetting debugProtoSyncFlag("DebugProtocolSynchronization", false);
    ScopedFastFlagSetting debugLocalRccServerConnection("DebugLocalRccServerConnection", false);
    ScopedFastFlagSetting debugForceRegenerateSchemaBitStream("DebugForceRegenerateSchemaBitStream", true);
    DataModelFixture serverDm;
    {
        NetworkFixture::startServer(serverDm);
    }

    std::string commandLine = "--log_level=test_suite --run_test=ProtocolSchema/EventTestClient";
    BOOST_REQUIRE(startClientProc(commandLine));

    // wait for client init
    G3D::System::sleep(1);

    shared_ptr<PartInstance> serverIns;
    {
        RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

        // Create an instance from the new class on the server
        BOOST_MESSAGE("Creating instance from TheNewClass...");
        serverIns = Creatable<Instance>::create<RBX::TheNewClass>();
        serverIns->setAnchored(true);
        serverIns->setName("TheNewClass");
        serverIns->setParent(serverDm->getWorkspace());
    }

    // Wait for replication
    G3D::System::sleep(1);

    {
        RBX::DataModel::LegacyLock lock(&serverDm, RBX::DataModelJob::Write);

        // Fire events
        BOOST_MESSAGE("Firing ExistingEvent and TheNewEvent...");
        RBX::TheNewClass* newIns = serverIns->fastDynamicCast<RBX::TheNewClass>();
        newIns->connectExistingSignal();
        RBX::TheNewClass::event_ExistingEvent->fireAndReplicateEvent(newIns, 1);
        BOOST_CHECK(newIns->existingEvt.Wait(RBX::Time::Interval::from_seconds(1)));
        BOOST_CHECK_EQUAL(newIns->existingEventValue, 1);
        newIns->connectTheNewSignal();
        RBX::TheNewClass::event_TheNewEvent->fireAndReplicateEvent(newIns, 1);
        BOOST_CHECK(newIns->theNewEvt.Wait(RBX::Time::Interval::from_seconds(1)));
        BOOST_CHECK_EQUAL(newIns->theNewEventValue, 1);
    }

    // Wait for replication
    G3D::System::sleep(1);
}

BOOST_AUTO_TEST_CASE(EventTestClient)
{
    // Note, EventTestClient and EventTestServer are twins. Please keep the timing synchronized.
    if (isEventTestServer)
    {
        return;
    }
    registerProps(true);
    registerEvents(false);
    ScopedFastFlagSetting debugProtoSyncFlag("DebugProtocolSynchronization", false);
    ScopedFastFlagSetting debugLocalRccServerConnection("DebugLocalRccServerConnection", false);
    ScopedFastFlagSetting debugForceRegenerateSchemaBitStream("DebugForceRegenerateSchemaBitStream", true);
    DataModelFixture clientDm;
    {
        NetworkFixture::startClient(clientDm);
    }
    // Wait for connection
    G3D::System::sleep(0.5);

    // Wait for replication
    G3D::System::sleep(1);

    Instance* clientIns;
    // Confirm the instance creation
    {
        RBX::DataModel::LegacyLock lock(&clientDm, RBX::DataModelJob::Write);

        clientIns = clientDm->getWorkspace()->findFirstChildByName("TheNewClass");
        BOOST_REQUIRE(clientIns);
    }

    RBX::TheNewClass* newIns = clientIns->fastDynamicCast<RBX::TheNewClass>();

    // Wait for replication
    //G3D::System::sleep(1);
    {
        printf("Start listening for events..\n");
        // Confirm the event results
        rbx::signals::scoped_connection connection(newIns->connectExistingSignal());
        BOOST_CHECK(newIns->existingEvt.Wait(RBX::Time::Interval::from_seconds(2)));
        BOOST_CHECK_EQUAL(newIns->existingEventValue, 1);
        BOOST_CHECK_EQUAL(newIns->theNewEventValue, 0);
    }
}

#endif
BOOST_AUTO_TEST_SUITE_END()
