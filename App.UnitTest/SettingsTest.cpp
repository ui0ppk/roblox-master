#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "util/SoundService.h"
#include "V8DataModel/GlobalSettings.h"
#include "V8Tree/Service.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE( SettingsTest )

class SettingsTestFixture : public DataModelFixture {
public:
	void reloadSettings() {
		// Restore the settings state after each test
		std::auto_ptr<RBX::Reflection::Tuple> result = execute(
			"return settings()");
		shared_ptr<Settings> settings = Instance::fastSharedDynamicCast<Settings>(
		result->at(0).cast<shared_ptr<Instance> >());

		BOOST_REQUIRE(settings.get());

		settings->loadState("TestSettings.txt");
	}
	SettingsTestFixture() : DataModelFixture() {
		reloadSettings();
	}
	virtual ~SettingsTestFixture() {
		reloadSettings();
	}
};

BOOST_AUTO_TEST_CASE( SettingsAreAvailableThroughLua )
{
	SettingsTestFixture dm;
	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"return settings()");
	BOOST_CHECK(!result->at(0).isVoid());
}

BOOST_AUTO_TEST_CASE( NetworkIsAutoAddedToSettings )
{
	SettingsTestFixture dm;
	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"return settings().Network");
	BOOST_CHECK(!result->at(0).isVoid());
}

BOOST_AUTO_TEST_CASE( SettingsAreSharedBetweenDataModels )
{
	SettingsTestFixture dm1;
	std::auto_ptr<RBX::Reflection::Tuple> result = dm1.execute(
		"return settings().Network.PrintTouches");
	BOOST_CHECK(!result->at(0).cast<bool>());

	SettingsTestFixture dm2;
	result = dm2.execute("return settings().Network.PrintTouches");
	BOOST_CHECK(!result->at(0).cast<bool>());

	dm1.execute("settings().Network.PrintTouches = true");
	result = dm1.execute("return settings().Network.PrintTouches");
	BOOST_CHECK(result->at(0).cast<bool>());

	result = dm2.execute("return settings().Network.PrintTouches");
	BOOST_CHECK(result->at(0).cast<bool>());
}

BOOST_AUTO_TEST_CASE( CanAccessMembersThroughBracketNotationWithoutTriggeringProtection )
{
	RBX::TaskSchedulerSettings::singleton();
	SettingsTestFixture dm;
	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"x = settings()[\"Task Scheduler\"].PriorityMethod return #settings():GetChildren()");
	BOOST_CHECK(result->at(0).cast<double>() > 0);
}

BOOST_AUTO_TEST_CASE( DifficultToCreateAServiceDirectlyUnderSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"return settings()");
	shared_ptr<Settings> settings = Instance::fastSharedDynamicCast<Settings>(
		result->at(0).cast<shared_ptr<Instance> >());

	BOOST_REQUIRE(settings.get());

	Soundscape::SoundService* sound = NULL;
	bool errored = false;
	try {
		 sound = settings->create<Soundscape::SoundService>();
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);
	BOOST_CHECK_EQUAL((Soundscape::SoundService*)NULL, sound);
	BOOST_CHECK_EQUAL((Soundscape::SoundService*)NULL,
		settings->find<Soundscape::SoundService>());
}

BOOST_AUTO_TEST_CASE( DifficultToCreateAPartDirectlyUnderSettings )
{
	SettingsTestFixture dm;
	std::auto_ptr<RBX::Reflection::Tuple> result
		= dm.execute("return #settings():GetChildren()");
	double childCount = result->at(0).cast<double>();

	BOOST_CHECK_GT(childCount, 0);

	bool errored = false;
	try {
		result = dm.execute("return Instance.new(\"Part\", settings())");
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);
	
	result = dm.execute("return #settings():GetChildren()");
	BOOST_CHECK_EQUAL(childCount, result->at(0).cast<double>());
}

BOOST_AUTO_TEST_CASE( DifficultToMoveAPartDirectlyUnderSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result
		= dm.execute("return #settings():GetChildren()");
	double childCount = result->at(0).cast<double>();

	BOOST_CHECK_GT(childCount, 0);

	bool errored = false;
	try {
		result = dm.execute(
			"foo = Instance.new(\"Part\") foo.Parent = settings() return foo");
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);

	result = dm.execute("return #settings():GetChildren()");
	BOOST_CHECK_EQUAL(childCount, result->at(0).cast<double>());
}

BOOST_AUTO_TEST_CASE( DifficultToCreateAPartTransitivelyUnderSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result
		= dm.execute("return #settings().Network:GetChildren()");
	double childCount = result->at(0).cast<double>();

	BOOST_CHECK_EQUAL(childCount, 0);

	bool errored = false;
	try {
		std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
			"return Instance.new(\"Part\", settings().Network)");
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);

	result = dm.execute("return #settings().Network:GetChildren()");
	BOOST_CHECK_EQUAL(childCount, result->at(0).cast<double>());
}

BOOST_AUTO_TEST_CASE( DifficultToMoveAPartTransitivelyUnderSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result
		= dm.execute("return #settings().Network:GetChildren()");
	double childCount = result->at(0).cast<double>();

	BOOST_CHECK_EQUAL(childCount, 0);

	bool errored = false;
	try {
		std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
			"foo = Instance.new(\"Part\") foo.Parent = settings().Network return foo");
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);

	result = dm.execute("return #settings().Network:GetChildren()");
	BOOST_CHECK_EQUAL(childCount, result->at(0).cast<double>());
}

BOOST_AUTO_TEST_CASE( SelectionDoesNotNukeSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"return settings()");
	shared_ptr<Settings> settings = Instance::fastSharedDynamicCast<Settings>(
		result->at(0).cast<shared_ptr<Instance> >());

	BOOST_REQUIRE(settings.get());

	Selection* sel = settings->create<Selection>();
	BOOST_CHECK(sel != NULL);
	BOOST_CHECK(sel->getParent() == settings.get());

	result = dm.execute("return #settings():GetChildren()");
	// make sure that other settings weren't nuked
	BOOST_CHECK(1 < result->at(0).cast<double>());
}

BOOST_AUTO_TEST_CASE( AddingPartUnderSelectionDoesNukeSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"return settings()");
	shared_ptr<Settings> settings = Instance::fastSharedDynamicCast<Settings>(
		result->at(0).cast<shared_ptr<Instance> >());

	Selection* sel = settings->create<Selection>();

	shared_ptr<PartInstance> part = Creatable<Instance>::create<PartInstance>();
	bool errored = false;
	try {
		part->setParent(sel);
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);

	BOOST_CHECK_EQUAL((RBX::Instances*)NULL, sel->getChildren().read().get());
}

BOOST_AUTO_TEST_CASE( AddingSelectionWithPartChildDoesNukeSettings )
{
	SettingsTestFixture dm;

	std::auto_ptr<RBX::Reflection::Tuple> result = dm.execute(
		"return settings()");
	shared_ptr<Settings> settings = Instance::fastSharedDynamicCast<Settings>(
		result->at(0).cast<shared_ptr<Instance> >());

	Selection* sel = settings->create<Selection>();

	shared_ptr<PartInstance> part = Creatable<Instance>::create<PartInstance>();
	sel->unlockParent();
	sel->setParent(NULL);
	part->setParent(sel);
	bool errored = false;
	try {
		sel->setParent(settings.get());
	} catch (std::runtime_error& ) {
		errored = true;
	}
	BOOST_CHECK(errored);

	// special cleanup: because we called ::create in settings, sel should
	// end up back in settings before these test objects are destroyed
	part->setParent(NULL);
	sel->setAndLockParent(settings.get());
}



BOOST_AUTO_TEST_SUITE_END()
