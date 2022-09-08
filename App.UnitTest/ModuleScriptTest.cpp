#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"
#include "Script/ModuleScript.h"
#include "Script/Script.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/Value.h"

using namespace RBX;

BOOST_AUTO_TEST_SUITE(ModuleScriptTest)

BOOST_AUTO_TEST_CASE(ModuleScriptsDoNotRunOnTheirOwn)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"Instance.new(\"BUG\", Workspace) "
		"return 1"));
	moduleScript->setName("Module");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
		dm->create<ScriptContext>();
	}

	BOOST_CHECK(ModuleScript::NotRunYet == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(dm->getWorkspace()->findFirstChildByName("BUG") == NULL);
}

BOOST_AUTO_TEST_CASE(ScopeOfScriptInReturnedFunctions)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"return function () return script.Name .. \"2\" end"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"local v = Instance.new(\"StringValue\", Workspace) "
		"v.Name = require(Workspace.Module)() "));
	loaderScript->setName("Caller");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK_EQUAL(ModuleScript::CompletedSuccess, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(dm->getWorkspace()->findFirstChildByName("Module2") != NULL);
	BOOST_CHECK(dm->getWorkspace()->findFirstChildByName("Caller2") == NULL);
}

BOOST_AUTO_TEST_CASE(ReturnStatefulClosure)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local count = 0 "
		"return function (amt) "
		"    count = count + amt "
		"    if count > 10 then "
		"        local val = Instance.new(\"IntValue\", Workspace) "
		"        val.Value = count "
		"        val.Name = \"\" .. count "
		"    end "
		"end "));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"local fun = require(script.Parent.Module) "
		"fun(9) " // count == 9; no int value
		"fun(7) " // count == 16; int value
		"fun(-5) " // count == 11; int value
		));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK(ModuleScript::CompletedSuccess == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	BOOST_CHECK(NULL == dm->getWorkspace()->findFirstChildByName("9"));
	Instance* inst16 = dm->getWorkspace()->findFirstChildByName("16");
	BOOST_REQUIRE(inst16 != NULL);
	BOOST_CHECK_EQUAL(16, Instance::fastDynamicCast<IntValue>(inst16)->getValue());

	Instance* inst11 = dm->getWorkspace()->findFirstChildByName("11");
	BOOST_REQUIRE(inst11 != NULL);
	BOOST_CHECK_EQUAL(11, Instance::fastDynamicCast<IntValue>(inst11)->getValue());
}

BOOST_AUTO_TEST_CASE(SharedLoadResult)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local count = 0 "
		"return function (amt) "
		"    count = count + amt "
		"    if count > 10 then "
		"        local val = Instance.new(\"IntValue\", Workspace) "
		"        val.Value = count "
		"        val.Name = \"\" .. count "
		"    end "
		"end "));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript1 = Creatable<Instance>::create<Script>();
	loaderScript1->setEmbeddedCode(ProtectedString::fromTestSource(
		"local fun = require(script.Parent.Module) "
		"fun(9) " // count == 9; no int value
		));

	shared_ptr<Script> loaderScript2 = Creatable<Instance>::create<Script>();
	loaderScript2->setEmbeddedCode(ProtectedString::fromTestSource(
		"local fun = require(script.Parent.Module) "
		"fun(5) " // count == 14; create int value
		));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript1->setParent(dm->getWorkspace());
		loaderScript2->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK(ModuleScript::CompletedSuccess == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	Instance* inst14 = dm->getWorkspace()->findFirstChildByName("14");
	BOOST_REQUIRE(inst14 != NULL);
	BOOST_CHECK_EQUAL(14, Instance::fastDynamicCast<IntValue>(inst14)->getValue());
}

BOOST_AUTO_TEST_CASE(CloneDoesNotShareResult)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local count = 0 "
		"return function (amt) "
		"    count = count + amt "
		"    if count > 10 then "
		"        local val = Instance.new(\"IntValue\", Workspace) "
		"        val.Value = count "
		"        val.Name = \"\" .. count "
		"    end "
		"end "));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript1 = Creatable<Instance>::create<Script>();
	loaderScript1->setEmbeddedCode(ProtectedString::fromTestSource(
		"local fun = require(script.Parent.Module) "
		"fun(9) " // count == 9; no int value
		));

	shared_ptr<Script> loaderScript2 = Creatable<Instance>::create<Script>();
	loaderScript2->setEmbeddedCode(ProtectedString::fromTestSource(
		"local fun = require(script.Parent.Module:Clone()) "
		"fun(5) " // count == 14; create int value
		));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript1->setParent(dm->getWorkspace());
		loaderScript2->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK(ModuleScript::CompletedSuccess == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	BOOST_CHECK(NULL == dm->getWorkspace()->findFirstChildByName("14"));
}

BOOST_AUTO_TEST_CASE(LoadModuleFromModule)
{
	shared_ptr<ModuleScript> deepModule = Creatable<Instance>::create<ModuleScript>();
	deepModule->setSource(ProtectedString::fromTestSource(
		"return function (v1) return function (v2) return \"Deep\" .. v2 .. \"Orig\" .. v1 end end"));
	deepModule->setName("DeepModule");

	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"return require(script.Parent.DeepModule)(\"L1String\")"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"local v = Instance.new(\"Part\", Workspace) "
		"v.Name = require(script.Parent.Module)(\"L2String\")"));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		deepModule->setParent(dm->getWorkspace());
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK(ModuleScript::CompletedSuccess == deepModule->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(ModuleScript::CompletedSuccess == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	BOOST_CHECK(dm->getWorkspace()->findFirstChildByName("DeepL2StringOrigL1String") != NULL);
}

BOOST_AUTO_TEST_CASE(ModuleScriptDoesNotLeakGlobals)
{
	shared_ptr<ModuleScript> caller = Creatable<Instance>::create<ModuleScript>();
	caller->setSource(ProtectedString::fromTestSource(
		"shallowVariable = 'FailureDepth1' "
		"require(script.Parent.CalleeModule) "
		"success = pcall(function() script.Name = deepVariable end) "
		"if not success then script.Name = 'RanCorrectly' end "
		"return 1"
		));
	caller->setName("CallerModule");

	shared_ptr<ModuleScript> callee = Creatable<Instance>::create<ModuleScript>();
	callee->setSource(ProtectedString::fromTestSource(
		"deepVariable = 'FailureDepth2' "
		"return deepVariable"));
	callee->setName("CalleeModule");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(script.Parent.CallerModule) "
		"success2 = pcall(function() script.Name = shallowVariable end) "
		"success3 = pcall(function() script.Name = deepVariable end) "
		"if not success2 and not success3 then script.Name = 'TopLevelCorrect' end"));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		caller->setParent(dm->getWorkspace());
		callee->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK(ModuleScript::CompletedSuccess == caller->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(ModuleScript::CompletedSuccess == callee->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK_EQUAL("RanCorrectly", caller->getName());
	BOOST_CHECK_EQUAL("CalleeModule", callee->getName());
	BOOST_CHECK_EQUAL("TopLevelCorrect", loaderScript->getName());
}

BOOST_AUTO_TEST_CASE(LoadModuleFromModuleWithYields)
{
	shared_ptr<ModuleScript> deepModule = Creatable<Instance>::create<ModuleScript>();
	deepModule->setSource(ProtectedString::fromTestSource(
		"wait() "
		"return function (v1) "
		"    wait() "
		"    return function (v2) "
		"        wait() "
		"        return \"Deep\" .. v2 .. \"Orig\" .. v1 "
		"    end "
		"end"));
	deepModule->setName("DeepModule");

	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"wait() "
		"local fun = require(script.Parent.DeepModule) "
		"wait() "
		"return fun(\"L1String\")"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"local v = Instance.new(\"Part\") "
		"v.Name = require(script.Parent.Module)(\"L2String\") "
		"v.Parent = Workspace"));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		deepModule->setParent(dm->getWorkspace());
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());
		ServiceProvider::create<RunService>(&dm)->run();
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK(ModuleScript::CompletedSuccess == deepModule->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(ModuleScript::CompletedSuccess == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	BOOST_CHECK(dm->getWorkspace()->findFirstChildByName("DeepL2StringOrigL1String") != NULL);
}

BOOST_AUTO_TEST_CASE(ModuleScriptSourceCannotBeChangedFromClient)
{
	const ProtectedString kOriginalSource = ProtectedString::fromTestSource("print 'OriginalSource'");
	
	DataModelFixture serverDm;
	shared_ptr<ModuleScript> serverModuleScript;
	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Write);
		serverModuleScript = Creatable<Instance>::create<ModuleScript>();
		serverModuleScript->setSource(kOriginalSource);
		serverModuleScript->setName("ModuleScript");
		serverModuleScript->setParent(serverDm->getWorkspace());
	}
	NetworkFixture::startServer(serverDm);

	DataModelFixture clientDm;
	NetworkFixture::startClient(clientDm, true);

	{
		DataModel::LegacyLock l(&clientDm, DataModelJob::Write);

		ModuleScript* clientModule = Instance::fastDynamicCast<ModuleScript>(
			clientDm->getWorkspace()->findFirstChildByName("ModuleScript"));
		BOOST_REQUIRE(clientModule);
		BOOST_REQUIRE_EQUAL(clientModule->getSource().getSource(),
			kOriginalSource.getSource());
		clientModule->setSource(ProtectedString::fromTestSource("ModifiedSource"));
	}

	// wait for replication
	G3D::System::sleep(.5f);

	{
		DataModel::LegacyLock l(&serverDm, DataModelJob::Read);

		BOOST_CHECK_EQUAL(serverModuleScript->getSource().getSource(),
			kOriginalSource.getSource());
	}
}

BOOST_AUTO_TEST_CASE(ModuleDeletesScriptThatRequiredTheModule)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"wait() "
		"Workspace.Caller.Parent = nil "
		"return 1"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "
		"local v = Instance.new(\"Part\", Workspace) "));
	loaderScript->setName("Caller");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK_EQUAL(ModuleScript::CompletedSuccess, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(dm->getWorkspace()->findFirstChildByName("Caller") == NULL);
	BOOST_CHECK(dm->getWorkspace()->findFirstDescendantOfType<PartInstance>() == NULL);
}

BOOST_AUTO_TEST_CASE(ParseErrorInModuleScript)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"*ThisIS 12312 invalid syntax!"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "
		"local v = Instance.new(\"Part\", Workspace) "));
	loaderScript->setName("Caller");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK_EQUAL(ModuleScript::CompletedError, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	// check that the caller script was stopped at the require line
	BOOST_CHECK(dm->getWorkspace()->findFirstDescendantOfType<PartInstance>() == NULL);
}

BOOST_AUTO_TEST_CASE(ImmediateErrorInModuleScript)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local foo = nil "
		"foo.Parent = Workspace"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "
		"local v = Instance.new(\"Part\", Workspace) "));
	loaderScript->setName("Caller");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK_EQUAL(ModuleScript::CompletedError, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	// check that the caller script was stopped at the require line
	BOOST_CHECK(dm->getWorkspace()->findFirstDescendantOfType<PartInstance>() == NULL);
}

BOOST_AUTO_TEST_CASE(ErrorInModuleScriptAfterYield)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local foo = nil "
		"wait() "
		"foo.Parent = Workspace"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "
		"local v = Instance.new(\"Part\", Workspace) "));
	loaderScript->setName("Caller");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK_EQUAL(ModuleScript::CompletedError, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(dm->getWorkspace()->findFirstDescendantOfType<PartInstance>() == NULL);
}

BOOST_AUTO_TEST_CASE(ErrorInNestedModule)
{
	shared_ptr<ModuleScript> deepModule = Creatable<Instance>::create<ModuleScript>();
	deepModule->setSource(ProtectedString::fromTestSource(
		"local f = nil "
		"f.Parent = Workspace "));
	deepModule->setName("DeepModule");

	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local fun = require(script.Parent.DeepModule) "
		"return fun(\"L1String\")"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"local v = Instance.new(\"Part\") "
		"v.Name = require(script.Parent.Module)(\"L2String\") "
		"v.Parent = Workspace"));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		deepModule->setParent(dm->getWorkspace());
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	G3D::System::sleep(.5f);

	BOOST_CHECK(ModuleScript::CompletedError == deepModule->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(ModuleScript::CompletedError == moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	BOOST_CHECK(dm->getWorkspace()->findFirstDescendantOfType<PartInstance>() == NULL);
}

BOOST_AUTO_TEST_CASE(ModuleDoesNotReturnResult)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local foo = 2"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "
		"Instance.new(\"Part\", Workspace)"));
	loaderScript->setName("Caller");

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK_EQUAL(ModuleScript::CompletedError, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	BOOST_CHECK(dm->getWorkspace()->findFirstDescendantOfType<PartInstance>() == NULL);
}

BOOST_AUTO_TEST_CASE(ConnectionsInModulesAreNotBrokenWhenIncluderStops)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local p = Workspace.Part "
		"local s = Workspace.Script "
		"p.Changed:connect(function() s.Name = s.Name .. 'x' end) "
		"return 1"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "));

	shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		part->setParent(dm->getWorkspace());
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK_EQUAL(ModuleScript::CompletedSuccess, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		part->setName(part->getName() + "x");
		BOOST_CHECK_EQUAL("Scriptx", loaderScript->getName());
	}

	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		loaderScript->setDisabled(true);
		part->setName(part->getName() + "x");
		BOOST_CHECK_EQUAL("Scriptxx", loaderScript->getName());
	}
}

BOOST_AUTO_TEST_CASE(ConnectionInModulesAreBrokenOnModuleError)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"local p = Workspace.Part "
		"local s = Workspace.Script "
		"p.Changed:connect(function() s.Name = s.Name .. 'x' end) "
		"p = nil "
		"p.Name = 'foo' "
		"return 1"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"require(Workspace.Module) "));

	shared_ptr<Instance> part = Creatable<Instance>::create<BasicPartInstance>();

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		part->setParent(dm->getWorkspace());
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	BOOST_CHECK_EQUAL(ModuleScript::CompletedError, moduleScript->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());

	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		part->setName(part->getName() + "x");
		// check that the callback did not fire
		BOOST_CHECK_EQUAL("Script", loaderScript->getName());
	}
}

BOOST_AUTO_TEST_CASE(RequireManyModules)
{
	shared_ptr<ModuleScript> moduleScript = Creatable<Instance>::create<ModuleScript>();
	moduleScript->setSource(ProtectedString::fromTestSource(
		"print('Hio!') "
		"return 1"));
	moduleScript->setName("Module");

	shared_ptr<Script> loaderScript = Creatable<Instance>::create<Script>();
	loaderScript->setEmbeddedCode(ProtectedString::fromTestSource(
		"for i=1,100 do "
		"  local m = Workspace.Module:Clone() "
		"  m.Parent = Workspace "
		"  m.Name = m.Name .. i "
		"  require(m) "
		"end"));

	DataModelFixture dm;
	{
		DataModel::LegacyLock l(&dm, DataModelJob::Write);
		moduleScript->setParent(dm->getWorkspace());
		loaderScript->setParent(dm->getWorkspace());

		ServiceProvider::create<RunService>(&dm)->run();
	}

	DataModel::LegacyLock l(&dm, DataModelJob::Read);
	for (int i = 1; i <= 100; ++i)
	{
		ModuleScript* m = Instance::fastDynamicCast<ModuleScript>(
			dm->getWorkspace()->findFirstChildByName(format("Module%d", i)));
		BOOST_REQUIRE(m);
		BOOST_CHECK_EQUAL(ModuleScript::CompletedSuccess, m->vmState(
		dm->find<ScriptContext>()->getGlobalState(Security::GameScript_)).getCurrentState());
	}
}

BOOST_AUTO_TEST_SUITE_END()
