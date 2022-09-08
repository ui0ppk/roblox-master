#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "rbx/signal.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/CustomEvent.h"
#include "V8DataModel/CustomEventReceiver.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Selection.h"
#include "V8DataModel/Workspace.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/XmlElement.h"
#include "V8Xml/XmlSerializer.h"

#include <sstream>

using namespace RBX;

struct CallCounter {
	float lastValue;
	int calls;
	CallCounter() : lastValue(0), calls(0) {}
	void call(float value) {
		++calls;
		lastValue = value;
	}
};

struct ConnectedListener {
	int calls;
	shared_ptr<Instance> sentValue;
	ConnectedListener() : calls(0), sentValue() {}
	void call(shared_ptr<Instance> value) {
		++calls;
		sentValue = value;
	}
	void resetForNextCase() {
		calls = 0;
		sentValue.reset();
	}
};

struct InScopeHelper {
	CustomEvent* evt;
	CustomEventReceiver* receiver;
	InScopeHelper(CustomEvent* evt, CustomEventReceiver* receiver)
			: evt(evt), receiver(receiver) {}
	bool inScope(Instance* instance) {
		return instance == evt || instance == receiver;
	}
};

static void copyPaste(CustomEvent* evt, CustomEventReceiver* receiver,
		DataModelFixture& fixture, Instance* targetParent, Instances* items) {
	std::auto_ptr<XmlElement> root(Serializer::newRootElement());
	InScopeHelper inScope(evt, receiver);
	if (evt) {
		root->addChild(evt->writeXml(boost::bind(&InScopeHelper::inScope, inScope, _1),
				RBX::SerializationCreator));
	}
	if (receiver) {
		root->addChild(receiver->writeXml(boost::bind(&InScopeHelper::inScope, inScope, _1),
				RBX::SerializationCreator));
	}

	std::stringstream ss(std::stringstream::in | std::stringstream::out);
	TextXmlWriter machine(ss);
	machine.serialize(root.get());

	Serializer().loadInstances(ss, *items);

	fixture->getWorkspace()->insertInstances(
			*items, targetParent, INSERT_TO_TREE, SUPPRESS_PROMPTS);
}

BOOST_AUTO_TEST_SUITE(CustomEvents)

BOOST_AUTO_TEST_CASE( CreatingStateSanityCheck )
{
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	BOOST_REQUIRE_EQUAL((Instance *)NULL, evt->getParent());
	BOOST_REQUIRE_EQUAL((Instance *)NULL, evt->getParent());
	BOOST_REQUIRE_EQUAL(0, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());
}

BOOST_AUTO_TEST_CASE( AttachedListMaintenance )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());

	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(1, receiver.use_count());
	BOOST_REQUIRE(receiver.unique());

	{
		shared_ptr<const Instances> receivers(evt->getAttachedReceivers());
		BOOST_REQUIRE_EQUAL(1, receivers->size());
		BOOST_REQUIRE_EQUAL(receiver.get(), receivers->at(0).get());
	}

	{
		receiver->setSource(NULL);
		shared_ptr<const Instances> receivers = evt->getAttachedReceivers();
		BOOST_REQUIRE_EQUAL(0, receivers->size());
	}

	{
		// re-attach, then delete receiver while it is attached
		receiver->setSource(evt.get());
		shared_ptr<const Instances> receivers = evt->getAttachedReceivers();
		BOOST_REQUIRE_EQUAL(1, receivers->size());
	}

	BOOST_REQUIRE(receiver.unique());
	receiver.reset();
	shared_ptr<const Instances> receivers = evt->getAttachedReceivers();
	BOOST_REQUIRE_EQUAL(0, receivers->size());

	// TODO - test add/remove order
}

BOOST_AUTO_TEST_CASE( SendValuesOnlyWhenConnected )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());

	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(0, callCounter->calls);

	evt->setCurrentValue(0);
	BOOST_REQUIRE_EQUAL(0, callCounter->calls);

	// set value of attached event
	evt->setCurrentValue(.5f);
	BOOST_REQUIRE_EQUAL(.5f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.5f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.5f, callCounter->lastValue);

	// change receiver to be attached to new event with same value
	shared_ptr<CustomEvent> evt2(Creatable<Instance>::create<CustomEvent>());
	evt2->setCurrentValue(.5f);
	receiver->setSource(evt2.get());
	BOOST_REQUIRE_EQUAL(.5f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.5f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(.5f, evt2->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.5f, callCounter->lastValue);

	// change value of event that used to be attached
	evt->setCurrentValue(.75f);
	BOOST_REQUIRE_EQUAL(.75f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.5f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(.5f, evt2->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.5f, callCounter->lastValue);

	// change value of newly attached event
	evt2->setCurrentValue(.25f);
	BOOST_REQUIRE_EQUAL(.75f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.25f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(.25f, evt2->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(2, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.25f, callCounter->lastValue);

	// change attached event that has different current value
	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(.75f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.75f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(.25f, evt2->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(3, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.75f, callCounter->lastValue);

	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( ValueClamping )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());

	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	evt->setCurrentValue(-100);
	BOOST_REQUIRE_EQUAL(0, evt->getPersistedCurrentValue());

	evt->setCurrentValue(100);
	BOOST_REQUIRE_EQUAL(1, evt->getPersistedCurrentValue());

	BOOST_REQUIRE_EQUAL(0, callCounter->calls);
	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(1, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(1, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(1, receiver->getCurrentValue());

	// different number clamped to same number -> no update
	evt->setCurrentValue(7);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(1, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(1, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(1, receiver->getCurrentValue());

	// set to clamped value from unclamped value
	evt->setCurrentValue(1);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(1, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(1, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(1, receiver->getCurrentValue());

	// connect to source that was set to a different value, but the different
	// value is clamped to the same value the receiver had before -> no update
	shared_ptr<CustomEvent> evt2(Creatable<Instance>::create<CustomEvent>());
	evt2->setCurrentValue(30);
	receiver->setSource(evt2.get());
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(1, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(1, evt2->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(1, receiver->getCurrentValue());

	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( SettingParentToNullDoesBreakConnection )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	evt->setCurrentValue(.1f);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.1f, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.1f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.1f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	evt->setParent(NULL);
	evt->setCurrentValue(.2f);
	BOOST_REQUIRE_EQUAL(2, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.2f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(0, evt->getAttachedReceivers()->size());

	// putting the event back in the workspace doesn't repair connection
	evt->setParent(partParent.get());
	BOOST_REQUIRE_EQUAL(2, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.2f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(0, evt->getAttachedReceivers()->size());

	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(3, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.2f, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.2f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.2f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	receiver->setParent(NULL);
	evt->setCurrentValue(.3f);
	BOOST_REQUIRE_EQUAL(4, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.3f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(0, evt->getAttachedReceivers()->size());

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( BreakingConnectionSetsReceiverToZero )
{
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	receiver->setSource(evt.get());

	evt->setCurrentValue(.1f);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.1f, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.1f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.1f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	// Break connection by setting source, sender had non-zero value
	receiver->setSource(NULL);
	BOOST_REQUIRE_EQUAL(2, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.1f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(0, evt->getAttachedReceivers()->size());

	// break connection by setting source, sender had zero value
	evt->setCurrentValue(0);
	receiver->setSource(evt.get());
	receiver->setSource(NULL);
	BOOST_REQUIRE_EQUAL(2, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(0, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(0, evt->getAttachedReceivers()->size());

	evt->setCurrentValue(.2f);
	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(3, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.2f, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.2f, evt->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.2f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	// Break connection by removing sender from workspace sender had non-zero value
	evt->remove();
	BOOST_REQUIRE_EQUAL((Instance *) NULL, receiver->getSource());
	BOOST_REQUIRE_EQUAL(4, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());

	// break connection by removing sender from workspace, sender had zero value
	evt = Creatable<Instance>::create<CustomEvent>();
	evt->setParent(partParent.get());
	receiver->setSource(evt.get());
	evt->setParent(NULL);
	BOOST_REQUIRE_EQUAL(4, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());

	// break connection by removing receiver from workspace
	evt->setParent(partParent.get());
	evt->setCurrentValue(.3f);
	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(5, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.3f, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(.3f, receiver->getCurrentValue());

	receiver->remove();
	BOOST_REQUIRE_EQUAL(6, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, callCounter->lastValue);
	BOOST_REQUIRE_EQUAL(0, receiver->getCurrentValue());

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( SenderRemembersLastState )
{
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	evt->setCurrentValue(.5f);

	DataModelFixture fixture;
	Instances pasted;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());

	copyPaste(evt.get(), NULL, fixture, partParent.get(), &pasted);

	BOOST_REQUIRE_EQUAL(1, pasted.size());
	shared_ptr<CustomEvent> pastedEvent(Instance::fastSharedDynamicCast<CustomEvent>(pasted.at(0)));
	BOOST_REQUIRE(evt != pastedEvent);
	BOOST_REQUIRE_EQUAL(.5f, pastedEvent->getPersistedCurrentValue());

	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	receiver->setSource(pastedEvent.get());
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.5f, callCounter->lastValue);

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
}

///////////////////////////////////////////////////////////////////////////////
// LUA cloning

BOOST_AUTO_TEST_CASE( CloneEvent )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	shared_ptr<CustomEvent> clonedEvent(
			Instance::fastSharedDynamicCast<CustomEvent>(evt->luaClone()));
	BOOST_REQUIRE(clonedEvent);
	BOOST_REQUIRE_EQUAL(0, clonedEvent->getAttachedReceivers()->size());
	BOOST_REQUIRE_EQUAL(evt.get(), receiver->getSource());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	// verify that connections act as expected
	clonedEvent->setCurrentValue(1.0f);
	BOOST_REQUIRE_EQUAL(0, callCounter->calls);
	evt->setCurrentValue(1.0f);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( CloneReceiver )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	shared_ptr<CustomEventReceiver> clonedReceiver(
			Instance::fastSharedDynamicCast<CustomEventReceiver>(receiver->luaClone()));
	BOOST_REQUIRE(clonedReceiver);
	BOOST_REQUIRE_EQUAL(evt.get(), clonedReceiver->getSource());
	BOOST_REQUIRE_EQUAL(evt.get(), receiver->getSource());
	BOOST_REQUIRE_EQUAL(2, evt->getAttachedReceivers()->size());

	// verify that connections act as expected
	evt->setCurrentValue(1.0f);
	BOOST_REQUIRE_EQUAL(1.0f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(1.0f, clonedReceiver->getCurrentValue());

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( CloneEventAndReceiver )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	shared_ptr<Instance> clone(partParent->clone(RBX::ScriptingCreator));
	shared_ptr<BasicPartInstance> clonedParent(
			Instance::fastSharedDynamicCast<BasicPartInstance>(clone));
	shared_ptr<const Instances> children(clonedParent->getChildren2());
	BOOST_REQUIRE_EQUAL(2, children->size());
	shared_ptr<CustomEvent> clonedEvent(
			Instance::fastSharedDynamicCast<CustomEvent>(children->at(0)));
	shared_ptr<CustomEventReceiver> clonedReceiver(
			Instance::fastSharedDynamicCast<CustomEventReceiver>(children->at(1)));

	BOOST_REQUIRE(clonedEvent);
	BOOST_REQUIRE(clonedReceiver);
	BOOST_REQUIRE_EQUAL(1, clonedEvent->getAttachedReceivers()->size());
	BOOST_REQUIRE_EQUAL(clonedEvent.get(), clonedReceiver->getSource());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());
	BOOST_REQUIRE_EQUAL(evt.get(), receiver->getSource());
	BOOST_REQUIRE_EQUAL(0, callCounter->calls);

	evt->setCurrentValue(.1f);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(0, clonedEvent->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(0, clonedReceiver->getCurrentValue());

	clonedEvent->setCurrentValue(.2f);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);
	BOOST_REQUIRE_EQUAL(.2f, clonedEvent->getPersistedCurrentValue());
	BOOST_REQUIRE_EQUAL(.2f, clonedReceiver->getCurrentValue());

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

///////////////////////////////////////////////////////////////////////////////
// Studio Copy & Paste

BOOST_AUTO_TEST_CASE( StudioCopyEvent )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	Instances pasted;
	copyPaste(evt.get(), NULL, fixture, partParent.get(), &pasted);
	BOOST_REQUIRE_EQUAL(1, pasted.size());
	shared_ptr<CustomEvent> pastedEvent(
			Instance::fastSharedDynamicCast<CustomEvent>(pasted.at(0)));
	BOOST_REQUIRE(pastedEvent);
	BOOST_REQUIRE_EQUAL(0, pastedEvent->getAttachedReceivers()->size());
	BOOST_REQUIRE_EQUAL(evt.get(), receiver->getSource());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	// verify that connections act as expected
	pastedEvent->setCurrentValue(1.0f);
	BOOST_REQUIRE_EQUAL(0, callCounter->calls);
	evt->setCurrentValue(1.0f);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( StudioCopyReceiver )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	Instances pasted;
	copyPaste(NULL, receiver.get(), fixture, partParent.get(), &pasted);
	BOOST_REQUIRE_EQUAL(1, pasted.size());
	shared_ptr<CustomEventReceiver> pastedReceiver(
			Instance::fastSharedDynamicCast<CustomEventReceiver>(pasted.at(0)));
	BOOST_REQUIRE(pastedReceiver);

	BOOST_REQUIRE_EQUAL((Instance *)NULL, pastedReceiver->getSource());
	BOOST_REQUIRE_EQUAL(evt.get(), receiver->getSource());
	BOOST_REQUIRE_EQUAL(1, evt->getAttachedReceivers()->size());

	// verify that connections act as expected
	evt->setCurrentValue(1.0f);
	BOOST_REQUIRE_EQUAL(1.0f, receiver->getCurrentValue());
	BOOST_REQUIRE_EQUAL(0, pastedReceiver->getCurrentValue());

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

BOOST_AUTO_TEST_CASE( StudioCopyEventAndReceiver )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<CallCounter> callCounter(new CallCounter);
	rbx::signals::connection conn(
			receiver->sourceValueChanged.connect(boost::bind(&CallCounter::call, callCounter.get(), _1)));

	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	receiver->setSource(evt.get());

	Instances pasted;
	copyPaste(evt.get(), receiver.get(), fixture, partParent.get(), &pasted);
	BOOST_REQUIRE_EQUAL(2, pasted.size());
	shared_ptr<CustomEvent> pastedEvent(
			Instance::fastSharedDynamicCast<CustomEvent>(pasted.at(0)));
	shared_ptr<CustomEventReceiver> pastedReceiver(
			Instance::fastSharedDynamicCast<CustomEventReceiver>(pasted.at(1)));

	BOOST_REQUIRE(pastedEvent);
	BOOST_REQUIRE(evt != pastedEvent);
	BOOST_REQUIRE(pastedReceiver);
	BOOST_REQUIRE(receiver != pastedReceiver);

	BOOST_REQUIRE_EQUAL(1, pastedEvent->getAttachedReceivers()->size());
	BOOST_REQUIRE_EQUAL(pastedReceiver.get(), pastedEvent->getAttachedReceivers()->at(0).get());
	BOOST_REQUIRE_EQUAL(pastedEvent.get(), pastedReceiver->getSource());

	// verify that connections act as expected
	BOOST_REQUIRE_EQUAL(0, pastedReceiver->getCurrentValue());
	pastedEvent->setCurrentValue(1.0);
	BOOST_REQUIRE_EQUAL(1, pastedReceiver->getCurrentValue());

	// TODO: also attach a call counter to pastedReceiver

	BOOST_REQUIRE_EQUAL(0, callCounter->calls);
	evt->setCurrentValue(1.0);
	BOOST_REQUIRE_EQUAL(1, callCounter->calls);

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();
	conn.disconnect();
}

///////////////////////////////////////////////////////////////////////////////
// Connect & Disconnect events

BOOST_AUTO_TEST_CASE( ConnectAndDisconnectSignalsTriggering )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEvent> evt2(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<ConnectedListener> receiverConnected(new ConnectedListener);
	scoped_ptr<ConnectedListener> receiverDisconnected(new ConnectedListener);
	scoped_ptr<ConnectedListener> eventConnected(new ConnectedListener);
	scoped_ptr<ConnectedListener> eventDisconnected(new ConnectedListener);
	rbx::signals::connection conn1(
			evt->receiverConnected.connect(boost::bind(&ConnectedListener::call, receiverConnected.get(), _1)));
	rbx::signals::connection conn2(
			evt->receiverDisconnected.connect(boost::bind(&ConnectedListener::call, receiverDisconnected.get(), _1)));
	rbx::signals::connection conn3(
			receiver->eventConnected.connect(boost::bind(&ConnectedListener::call, eventConnected.get(), _1)));
	rbx::signals::connection conn4(
			receiver->eventDisconnected.connect(boost::bind(&ConnectedListener::call, eventDisconnected.get(), _1)));
	
	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());
	
	// before connection, no events should have been raised
	BOOST_REQUIRE_EQUAL(0, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(0, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(0, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(0, eventDisconnected->calls);

	// connect event to receiver
	receiver->setSource(evt.get());
	BOOST_REQUIRE_EQUAL(1, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(receiver, receiverConnected->sentValue);
	BOOST_REQUIRE_EQUAL(0, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(1, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(evt, eventConnected->sentValue);
	BOOST_REQUIRE_EQUAL(0, eventDisconnected->calls);

	// reset
	receiverConnected->resetForNextCase();
	eventConnected->resetForNextCase();

	// manually disconnect
	receiver->setSource(NULL);
	BOOST_REQUIRE_EQUAL(0, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(1, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(receiver, receiverDisconnected->sentValue);
	BOOST_REQUIRE_EQUAL(0, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(1, eventDisconnected->calls);
	BOOST_REQUIRE_EQUAL(evt, eventDisconnected->sentValue);

	// reset and reconnect to evt
	receiver->setSource(evt.get());
	receiverConnected->resetForNextCase();
	receiverDisconnected->resetForNextCase();
	eventConnected->resetForNextCase();
	eventDisconnected->resetForNextCase();

	// clone receiver should create a connect event
	shared_ptr<CustomEventReceiver> clonedReceiver(
			Instance::fastSharedDynamicCast<CustomEventReceiver>(receiver->luaClone()));
	BOOST_REQUIRE_EQUAL(1, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(clonedReceiver, receiverConnected->sentValue);
	BOOST_REQUIRE_EQUAL(0, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(0, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(0, eventDisconnected->calls);

	// disconnect clone and reset
	clonedReceiver->setSource(NULL);
	receiverConnected->resetForNextCase();
	receiverDisconnected->resetForNextCase();
	eventConnected->resetForNextCase();
	eventDisconnected->resetForNextCase();

	// disconnect by setting source to a different event
	receiver->setSource(evt2.get());
	BOOST_REQUIRE_EQUAL(0, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(1, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(receiver, receiverDisconnected->sentValue);
	BOOST_REQUIRE_EQUAL(1, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(evt2, eventConnected->sentValue);
	BOOST_REQUIRE_EQUAL(1, eventDisconnected->calls);
	BOOST_REQUIRE_EQUAL(evt, eventDisconnected->sentValue);

	// reset and reconnect to evt
	receiver->setSource(evt.get());
	receiverConnected->resetForNextCase();
	receiverDisconnected->resetForNextCase();
	eventConnected->resetForNextCase();
	eventDisconnected->resetForNextCase();

	// disconnect by removing evt from datamodel
	evt->setParent(NULL);
	BOOST_REQUIRE_EQUAL(0, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(1, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(receiver, receiverDisconnected->sentValue);
	BOOST_REQUIRE_EQUAL(0, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(1, eventDisconnected->calls);
	BOOST_REQUIRE_EQUAL(evt, eventDisconnected->sentValue);

	// connect again and reset
	evt->setParent(partParent.get());
	receiver->setSource(evt.get());
	receiverConnected->resetForNextCase();
	receiverDisconnected->resetForNextCase();
	eventConnected->resetForNextCase();
	eventDisconnected->resetForNextCase();

	// disconnect by removing receiver from datamodel
	receiver->remove();
	BOOST_REQUIRE_EQUAL(0, receiverConnected->calls);
	BOOST_REQUIRE_EQUAL(1, receiverDisconnected->calls);
	BOOST_REQUIRE_EQUAL(receiver, receiverDisconnected->sentValue);
	BOOST_REQUIRE_EQUAL(0, eventConnected->calls);
	BOOST_REQUIRE_EQUAL(1, eventDisconnected->calls);
	BOOST_REQUIRE_EQUAL(evt, eventDisconnected->sentValue);

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();

	conn1.disconnect();
	conn2.disconnect();
	conn3.disconnect();
	conn4.disconnect();
}

struct ConnectStateChecker {
	shared_ptr<CustomEvent> evt;
	shared_ptr<CustomEventReceiver> receiver;
	bool checkedReceiverAdded;
	bool checkedReceiverRemoved;
	bool checkedEventAdded;
	bool checkedEventRemoved;

	ConnectStateChecker(shared_ptr<CustomEvent> evt, shared_ptr<CustomEventReceiver> receiver)
		: evt(evt), receiver(receiver), checkedReceiverAdded(false),
			checkedReceiverRemoved(false), checkedEventAdded(false), checkedEventRemoved(false) {}

	void verifyReceiverAlreadyAdded(shared_ptr<Instance> instance) {
		shared_ptr<CustomEventReceiver> castReceiver = Instance::fastSharedDynamicCast<CustomEventReceiver>(instance);
		BOOST_REQUIRE_EQUAL(receiver, castReceiver);

		shared_ptr<const Instances> attached = evt->getAttachedReceivers();
		BOOST_REQUIRE_EQUAL(1, attached->size());
		BOOST_REQUIRE_EQUAL(receiver.get(), attached->at(0).get());
		checkedReceiverAdded = true;
	}

	void verifyReceiverAlreadyRemoved(shared_ptr<Instance> instance) {
		shared_ptr<CustomEventReceiver> castReceiver = Instance::fastSharedDynamicCast<CustomEventReceiver>(instance);
		BOOST_REQUIRE_EQUAL(receiver, castReceiver);

		shared_ptr<const Instances> attached = evt->getAttachedReceivers();
		BOOST_REQUIRE(attached->empty());
		checkedReceiverRemoved = true;
	}

	void verifyEventAlreadyAdded(shared_ptr<Instance> instance) {
		shared_ptr<CustomEvent> castEvent = Instance::fastSharedDynamicCast<CustomEvent>(instance);
		BOOST_REQUIRE_EQUAL(evt, castEvent);

		BOOST_REQUIRE_EQUAL(evt.get(), receiver->getSource());
		checkedEventAdded = true;
	}

	void verifyEventAlreadyRemoved(shared_ptr<Instance> instance) {
		shared_ptr<CustomEvent> castEvent = Instance::fastSharedDynamicCast<CustomEvent>(instance);
		BOOST_REQUIRE_EQUAL(evt, castEvent);

		BOOST_REQUIRE_EQUAL((Instance *) NULL, receiver->getSource());
		checkedEventRemoved = true;
	}
};

BOOST_AUTO_TEST_CASE( ConnectAndDisconnectSignalHappensAfterInternalStateUpdate )
{
	// create event + receiver
	shared_ptr<CustomEvent> evt(Creatable<Instance>::create<CustomEvent>());
	shared_ptr<CustomEventReceiver> receiver(
			Creatable<Instance>::create<CustomEventReceiver>());
	scoped_ptr<ConnectStateChecker> checker(new ConnectStateChecker(evt, receiver));
	
	rbx::signals::connection conn1(
			evt->receiverConnected.connect(boost::bind(
			&ConnectStateChecker::verifyReceiverAlreadyAdded, checker.get(), _1)));
	rbx::signals::connection conn2(
			evt->receiverDisconnected.connect(boost::bind(
			&ConnectStateChecker::verifyReceiverAlreadyRemoved, checker.get(), _1)));
	rbx::signals::connection conn3(
			receiver->eventConnected.connect(boost::bind(
			&ConnectStateChecker::verifyEventAlreadyAdded, checker.get(), _1)));
	rbx::signals::connection conn4(
			receiver->eventDisconnected.connect(boost::bind(
			&ConnectStateChecker::verifyEventAlreadyRemoved, checker.get(), _1)));

	DataModelFixture fixture;
	RBX::DataModel::LegacyLock lock(&fixture, RBX::DataModelJob::Write);
	shared_ptr<BasicPartInstance> partParent(Creatable<Instance>::create<BasicPartInstance>());
	partParent->setParent(fixture->getWorkspace());
	evt->setParent(partParent.get());
	receiver->setParent(partParent.get());

	// verify precondition
	BOOST_REQUIRE(!checker->checkedReceiverAdded);
	BOOST_REQUIRE(!checker->checkedReceiverRemoved);
	BOOST_REQUIRE(!checker->checkedEventAdded);
	BOOST_REQUIRE(!checker->checkedEventRemoved);

	receiver->setSource(evt.get());
	receiver->setSource(NULL);

	BOOST_REQUIRE(checker->checkedReceiverAdded);
	BOOST_REQUIRE(checker->checkedReceiverRemoved);
	BOOST_REQUIRE(checker->checkedEventAdded);
	BOOST_REQUIRE(checker->checkedEventRemoved);

	// remove the part parent from the workspace to make sure the
	// parent/child references aren't affecting test cleanup.
	partParent->Instance::remove();

	checker.reset();
	conn1.disconnect();
	conn2.disconnect();
	conn3.disconnect();
	conn4.disconnect();
}


BOOST_AUTO_TEST_SUITE_END()
