#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/Selection.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE( SelectionTest )

BOOST_AUTO_TEST_CASE(RemovingFromDataModelRemovesFromSelection)
{
	DataModelFixture dm;
	DataModel::LegacyLock lock(&dm, DataModelJob::Write);

	shared_ptr<BasicPartInstance> part = Creatable<Instance>::create<BasicPartInstance>();
	part->setParent(dm->getWorkspace());

	Selection* selection = dm->create<Selection>();
	selection->setSelection(part.get());

	BOOST_CHECK_EQUAL(1, selection->size());
	BOOST_CHECK_EQUAL(part.get(), selection->front().get());

	part->setParent(NULL);

	BOOST_CHECK_EQUAL(0, selection->size());
}

BOOST_AUTO_TEST_CASE(NotInDataModelBlockedFromSelection)
{
	DataModelFixture dm;
	DataModel::LegacyLock lock(&dm, DataModelJob::Write);

	shared_ptr<BasicPartInstance> part = Creatable<Instance>::create<BasicPartInstance>();

	Selection* selection = dm->create<Selection>();

	const size_t expectedSize = 0;

	selection->clearSelection();
	selection->setSelection(part.get());
	BOOST_CHECK_EQUAL(expectedSize, selection->size());

	selection->clearSelection();
	selection->addToSelection(part.get());
	BOOST_CHECK_EQUAL(expectedSize, selection->size());

	selection->clearSelection();
	selection->toggleSelection(part.get());
	BOOST_CHECK_EQUAL(expectedSize, selection->size());

	selection->clearSelection();
	part.reset();
}

BOOST_AUTO_TEST_SUITE_END()
