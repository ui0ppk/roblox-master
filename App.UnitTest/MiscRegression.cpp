#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/Test.h"
#include "script/script.h"

BOOST_FIXTURE_TEST_SUITE(MiscRegression, DataModelFixture)

BOOST_AUTO_TEST_CASE(DE2399)
{
	using namespace RBX;
	DataModel::LegacyLock lock(dataModel.get(), RBX::DataModelJob::Write);
	TestService* ts = ServiceProvider::create<TestService>(dataModel.get());
	BOOST_CHECK_EQUAL(ts->countDescendantsOfType<Script>(), 0);
	shared_ptr<Instance> s = Script::createInstance();
	s->setParent(ts);
	BOOST_CHECK_EQUAL(ts->countDescendantsOfType<Script>(), 1);
	dataModel->clearContents(true);
	BOOST_CHECK_EQUAL(ts->countDescendantsOfType<Script>(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

