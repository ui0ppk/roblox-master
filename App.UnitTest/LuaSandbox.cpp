#include <boost/test/unit_test.hpp>

#include "rbx/test/DataModelFixture.h"

#include "script/ScriptContext.h"
#include "script/Script.h"

using namespace RBX;

LOGGROUP(LuaScriptTimeoutSeconds)

void AssertScriptIsTrue(DataModel& dataModel, const char* script)
{
	ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(&dataModel);
	RBX::Reflection::Variant result = scriptContext->executeInNewThread(RBX::Security::GameScript_, ProtectedString::fromTestSource(script), "test", RBX::Reflection::Tuple())->at(0);
	BOOST_CHECK(result.convert<bool>());
}

// A sandboxed thread attempts (and fails) to hack the global wait() function
void HackWait(DataModel& dataModel, const char* waitHack)
{
	ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(&dataModel);

	// First script tries to modify the environment
	scriptContext->executeInNewThread(RBX::Security::GameScript_, ProtectedString::fromTestSource(waitHack), "test", RBX::Reflection::Tuple());

	// The second script should use the original meaning of "wait" because RBX::Security::GameScript_ is sandboxed
	RBX::Reflection::Variant result = scriptContext->executeInNewThread(RBX::Security::GameScript_, ProtectedString::fromTestSource("return type(wait) == 'function'"), "test", RBX::Reflection::Tuple())->at(0);
	
	BOOST_CHECK(result.convert<bool>());
}



BOOST_FIXTURE_TEST_SUITE(LuaSandbox, DataModelFixture)

	BOOST_AUTO_TEST_CASE(Hack_getfenv)
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		HackWait(*dataModel, "getfenv(0).wait = 'hacko!'");
		// This was a hack that a user created around 8/13/10
		HackWait(*dataModel, "_G = getmetatable(getfenv(1)).__index _G._G = _G _G.wait = 'Hacko'");
	}

	BOOST_AUTO_TEST_CASE(HackGlobal)
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		HackWait(*dataModel, "_G.wait = 'hacko!'");
	}

	BOOST_AUTO_TEST_CASE(HackMathLibrary)
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		AssertScriptIsTrue(*dataModel, "b = pcall(function() rawset(math, 'sin', 'hacko!') end) return not b");
		AssertScriptIsTrue(*dataModel, "return getmetatable(math) == nil");
	}

	BOOST_AUTO_TEST_CASE(HackStringMetatable)
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		AssertScriptIsTrue(*dataModel, "return getmetatable('') == 'The metatable is locked'");
	}

	static void testLuaTimeoutLuaSetting(shared_ptr<DataModel> dataModel)
	{
		RBX::Time start;

		shared_ptr<Script> script = Creatable<Instance>::create<Script>();
		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(dataModel.get());
			scriptContext->setTimeout(2);

			script->setEmbeddedCode(ProtectedString::fromTestSource("script.Name = 'foo' while true do end"));
			script->setParent(dataModel->getWorkspace());

			RBX::RunService* runService = RBX::ServiceProvider::create<RBX::RunService>(dataModel.get());

			start = RBX::Time::nowFast();
			runService->run();
		}

#if 1
		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			/*
			At this point we know that the script has started to run. 
			In fact, if we got the lock then we know the script was
			interrupted. If we never get here (and hang), then we know
			that the timeout code failed.
			*/
		}
#else
		// Wait for the script to start running
		while (true) 
		{
			G3D::System::sleep(0.1);
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			BOOST_MESSAGE("checking for foo");
			if (script->getName() == "foo")
			{
				BOOST_MESSAGE("found foo");
				break;
			}
		}

		/*
		At this point we know that the script has started to run. 
		In fact, if we got the lock then we know the script was
		interrupted. If we never get here (and hang), then we know
		that the timeout code failed.
		*/
#endif
		RBX::Time::Interval delta = RBX::Time::nowFast() - start;

		BOOST_CHECK_GE(delta, RBX::Time::Interval(2));
		BOOST_CHECK_LE(delta, RBX::Time::Interval(4));
	}

	BOOST_AUTO_TEST_CASE(LuaTimeoutLuaSetting)
	{
		FLog::LuaScriptTimeoutSeconds = 0;
		RBX_TEST_WITH_TIMEOUT(boost::bind(&testLuaTimeoutLuaSetting, dataModel), RBX::Time::Interval(10));
	}

	static void testLuaTimeoutFlogSetting(shared_ptr<DataModel> dataModel)
	{
		RBX::Time start;

		shared_ptr<Script> script = Creatable<Instance>::create<Script>();
		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			ServiceProvider::create<ScriptContext>(dataModel.get());

			script->setEmbeddedCode(ProtectedString::fromTestSource("script.Name = 'foo' while true do end"));
			script->setParent(dataModel->getWorkspace());

			RBX::RunService* runService = RBX::ServiceProvider::create<RBX::RunService>(dataModel.get());

			start = RBX::Time::nowFast();
			runService->run();
		}

#if 1
		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			/*
			At this point we know that the script has started to run. 
			In fact, if we got the lock then we know the script was
			interrupted. If we never get here (and hang), then we know
			that the timeout code failed.
			*/
		}
#else
		// Wait for the script to start running
		while (true) 
		{
			G3D::System::sleep(0.1);
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			BOOST_MESSAGE("checking for foo");
			if (script->getName() == "foo")
			{
				BOOST_MESSAGE("found foo");
				break;
			}
		}

		/*
		At this point we know that the script has started to run. 
		In fact, if we got the lock then we know the script was
		interrupted. If we never get here (and hang), then we know
		that the timeout code failed.
		*/
#endif
		RBX::Time::Interval delta = RBX::Time::nowFast() - start;

		BOOST_CHECK_GE(delta, RBX::Time::Interval(2));
		BOOST_CHECK_LE(delta, RBX::Time::Interval(4));
	}

	BOOST_AUTO_TEST_CASE(LuaTimeoutFlogSetting)
	{
		FLog::LuaScriptTimeoutSeconds = 2;
		RBX_TEST_WITH_TIMEOUT(boost::bind(&testLuaTimeoutFlogSetting, dataModel), RBX::Time::Interval(10));
	}


BOOST_AUTO_TEST_SUITE_END()
