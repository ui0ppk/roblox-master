#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "rbx/test/ScopedFastFlagSetting.h"
#include "Script/script.h"
#include "Script/ScriptContext.h"
#include "Util/ProtectedString.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8Tree/Instance.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(LuaYieldingTest)

// This test is based on an actual bug found in our yielding system
BOOST_AUTO_TEST_CASE(WrongResumerWhenYieldingInPCall)
{
	shared_ptr<Script> script = Creatable<Instance>::create<Script>();
	script->setEmbeddedCode(ProtectedString::fromTestSource(
		"pcall(function () "
		"	return Workspace:WaitForChild('PartA') "
		"end) "
		"local p = Workspace:WaitForChild('PartB') "
		"script.Name = p.Name"));

	shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		
		script->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();

		part->setName("PartA");
		part->setParent(dm->getWorkspace());
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK_EQUAL(script->getName(), "Script");

	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		
		part->setParent(NULL);
		part->setName("PartB");
		part->setParent(dm->getWorkspace());
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK_EQUAL(script->getName(), "PartB");
}

BOOST_AUTO_TEST_CASE(CanYPCallCFunctionWithYield)
{
	shared_ptr<Script> script = Creatable<Instance>::create<Script>();
	script->setEmbeddedCode(ProtectedString::fromTestSource(
		"succ, p = ypcall(Workspace.WaitForChild, Workspace, 'PartA') "
		"script.Name = p.Name"));

	shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		
		script->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	G3D::System::sleep(.15f);

	BOOST_REQUIRE_EQUAL(script->getName(), "Script");

	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);

		part->setName("PartA");
		part->setParent(dm->getWorkspace());
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK_EQUAL(script->getName(), "PartA");
}

BOOST_AUTO_TEST_CASE(PerfPcallPrint)
{
	shared_ptr<Script> script = Creatable<Instance>::create<Script>();
	script->setEmbeddedCode(ProtectedString::fromTestSource(
		"i = 8000000 "
		"while i > 0 do "
		"	pcall(print) "
		"	i = i - 1 "
		"end"));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		
		script->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}
}

BOOST_AUTO_TEST_SUITE_END()
