#include "stdafx.h"

#include "V8DataModel/Test.h"
#include "V8DataModel/PhysicsSettings.h"
#include "Network/../../NetworkSettings.h"
#include "V8DataModel/TimerService.h"
#include "Script/script.h"
#include "Script/ScriptContext.h"
#include "util/RunStateOwner.h"
#include "v8datamodel/DataModel.h"
#include <boost/format.hpp>
#include <ios>
#define BOOST_FILESYSTEM_NO_DEPRECATED 
#include <boost/filesystem.hpp>



namespace RBX
{
	const char* const sFunctionalTest = "FunctionalTest";
	const char* const sTestService = "TestService";

namespace Reflection
{
template<>
EnumDesc<FunctionalTest::Result>::EnumDesc()
:EnumDescriptor("FunctionalTestResult")
{
	addPair(FunctionalTest::Passed, "Passed");
	addPair(FunctionalTest::Warning, "Warning");
	addPair(FunctionalTest::Error, "Error");
}
template<>
FunctionalTest::Result& Variant::convert<FunctionalTest::Result>(void)
{
	return genericConvert<FunctionalTest::Result>();
}
}

template<>
bool RBX::StringConverter<FunctionalTest::Result>::convertToValue(const std::string& text, FunctionalTest::Result& value)
{
	return Reflection::EnumDesc<FunctionalTest::Result>::singleton().convertToValue(text.c_str(),value);
}


REFLECTION_BEGIN();
Reflection::BoundProp<std::string> descDescription("Description", "Data", &FunctionalTest::description);
Reflection::BoundProp<double> descTimeout("Timeout", "Settings", &FunctionalTest::timeout, Reflection::PropertyDescriptor::LEGACY);
Reflection::BoundProp<bool> descPhysicsEnvironmentalThrottle("PhysicsEnvironmentalThrottle", "Physics", &FunctionalTest::isPhysicsThrottled, Reflection::PropertyDescriptor::LEGACY);
Reflection::BoundProp<bool> descAllowSleep("AllowSleep", "Physics", &FunctionalTest::allowSleep, Reflection::PropertyDescriptor::LEGACY);
Reflection::BoundProp<bool> descIs30FpsThrottleEnabled("Is30FpsThrottleEnabled", "Physics", &FunctionalTest::is30FpsThrottleEnabled, Reflection::PropertyDescriptor::LEGACY);
Reflection::BoundProp<bool> prop_HasMigratedSettingsToTestService("HasMigratedSettingsToTestService", "Settings", &FunctionalTest::hasMigratedSettingsToTestService, Reflection::PropertyDescriptor::STREAMING);

static Reflection::BoundFuncDesc<FunctionalTest, void(std::string)> descWarn(&FunctionalTest::warn, "Warn", "message", "", Security::None);
static Reflection::BoundFuncDesc<FunctionalTest, void(std::string)> descPassed(&FunctionalTest::pass, "Pass", "message", "", Security::None);
static Reflection::BoundFuncDesc<FunctionalTest, void(std::string)> descFailed(&FunctionalTest::error, "Error", "message", "", Security::None);

static Reflection::BoundFuncDesc<FunctionalTest, void(std::string)> descPassedDeprecated(&FunctionalTest::pass, "Passed", "message", "", Security::None);
static Reflection::BoundFuncDesc<FunctionalTest, void(std::string)> descFailedDeprecated(&FunctionalTest::error, "Failed", "message", "", Security::None);

Reflection::BoundProp<std::string> prop_Description("Description", "Data", &TestService::description);
Reflection::BoundProp<double> prop_Timeout("Timeout", "Settings", &TestService::timeout);
Reflection::BoundProp<bool> prop_IsPhysicsEnvironmentalThrottled("IsPhysicsEnvironmentalThrottled", "Physics", &TestService::isPhysicsThrottled);
Reflection::BoundProp<bool> prop_IsSleepAllowed("IsSleepAllowed", "Physics", &TestService::allowSleep);
Reflection::BoundProp<bool> prop_Is30FpsThrottleEnabled("Is30FpsThrottleEnabled", "Physics", &TestService::is30FpsThrottleEnabled);
Reflection::BoundProp<bool> prop_AutoStart("AutoRuns", "Physics", &TestService::autoRuns);
Reflection::BoundProp<int> prop_NumberOfPlayers("NumberOfPlayers", "Settings", &TestService::numberOfPlayers);
Reflection::BoundProp<double> prop_SimulateLag("SimulateSecondsLag", "Settings", &TestService::lagSimulation);

#ifdef RBX_TEST_BUILD
Reflection::BoundProp<float> prop_PhysicsSendRate("PhysicsSendRate", "Settings", &TestService::physicsSendRate);
#endif

Reflection::BoundProp<int, Reflection::READONLY> TestService::TestCount("TestCount", "Results", &TestService::testCount);
Reflection::BoundProp<int, Reflection::READONLY> TestService::WarnCount("WarnCount", "Results", &TestService::warnCount);
Reflection::BoundProp<int, Reflection::READONLY> TestService::ErrorCount("ErrorCount", "Results", &TestService::errorCount);

static Reflection::BoundYieldFuncDesc<TestService, void()> func_Run(&TestService::run, "Run", Security::Plugin);
static Reflection::BoundFuncDesc<TestService, void(std::string, shared_ptr<Instance>, int)> func_Message(&TestService::message, "Message", "text", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void(std::string, shared_ptr<Instance>, int)> func_Checkpoint(&TestService::checkpoint, "Checkpoint", "text", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void(bool, std::string, shared_ptr<Instance>, int)> func_Warn(&TestService::warn, "Warn", "condition", "description", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void(bool, std::string, shared_ptr<Instance>, int)> func_Check(&TestService::check, "Check", "condition", "description", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void(bool, std::string, shared_ptr<Instance>, int)> func_Require(&TestService::require2, "Require", "condition", "description", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void(std::string, shared_ptr<Instance>, int)> func_Error(&TestService::error, "Error", "description", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void(std::string, shared_ptr<Instance>, int)> func_Fails(&TestService::fail, "Fail", "description", "source", shared_ptr<Instance>(), "line", 0, Security::None);
static Reflection::BoundFuncDesc<TestService, void()> func_Done(&TestService::done, "Done", Security::None);

// DoCommand calls getVerb instead of getWhitelistVerb, and thus is an easy exploit.
#ifdef RBX_TEST_BUILD
static Reflection::BoundFuncDesc<TestService, shared_ptr<const Reflection::ValueArray>()> func_GetCommandNames(&TestService::getCommandNames, "GetCommandNames", Security::LocalUser);
static Reflection::BoundFuncDesc<TestService, void(std::string)> func_DoCommand(&TestService::doCommand, "DoCommand", "name", Security::LocalUser);
static Reflection::BoundFuncDesc<TestService, bool(std::string)> func_IsCommandEnabled(&TestService::isCommandEnabled, "IsCommandEnabled", "name", Security::LocalUser);
static Reflection::BoundFuncDesc<TestService, bool(std::string)> func_IsCommandChecked(&TestService::isCommandChecked, "IsCommandChecked", "name", Security::LocalUser);
#endif

static Reflection::RemoteEventDesc<TestService, void(std::string, shared_ptr<Instance>, int)> desc_serverCollectResult(&TestService::serverCollectResultSignal, "ServerCollectResult", "text", "script", "line", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<TestService, void(bool, std::string, shared_ptr<Instance>, int)> desc_serverCollectConditionalResult(&TestService::serverCollectConditionalResultSignal, "ServerCollectConditionalResult", "condition", "text", "script", "line", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

TestService::TestService()
:description("")
,timeout(10)
,isPhysicsThrottled(true)
,allowSleep(true)
,is30FpsThrottleEnabled(true)
,running(false)
,runCount(0)
,testCount(0)
,warnCount(0)
,errorCount(0)
,autoRuns(true)
,numberOfPlayers(0)
,lagSimulation(0.0)
,clientTestService(false)
,scriptCount(0)
{
	setName(sTestService);

	RBX::NetworkSettings &ns = RBX::NetworkSettings::singleton();
	physicsSendRate = ns.getPhysicsSendRate();
}

TestService::~TestService()
{
}

void TestService::message( std::string text, shared_ptr<Instance> source, int line )
{
	if(isAClient())
		desc_serverCollectResult.fireAndReplicateEvent(this, text, source, line);
	if (onMessage)
		onMessage(text, source, line);
	else
		output(RBX::MESSAGE_INFO, source, line, text);
}

void TestService::onRemoteResult( std::string text, shared_ptr<Instance> source, int line )
{
	if (!isAClient())
	{
		RBXASSERT(numberOfPlayers > 0);
		onMessage(text, source, line);
	}
}

void TestService::checkpoint( std::string text, shared_ptr<Instance> source, int line )
{
	if (onCheckpoint)
		onCheckpoint(text, source, line);
	else
		output(RBX::MESSAGE_INFO, source, line, "checkpoint " + text);
}

void TestService::warn( bool condition, std::string description, shared_ptr<Instance> source, int line )
{
	incrementTest();
	if (!condition)
		incrementWarn();
	if (onWarn)
		onWarn(condition, description, source, line);
	else if (!condition)
		output(RBX::MESSAGE_WARNING, source, line, description);
}

void TestService::check( bool condition, std::string description, shared_ptr<Instance> source, int line )
{
	if(isAClient())
		desc_serverCollectConditionalResult.fireAndReplicateEvent(this, condition, description, source, line);

	incrementTest();
	if (!condition)
		incrementError();
	if (onCheck)
		onCheck(condition, description, source, line);
	else if (!condition)
		output(RBX::MESSAGE_ERROR, source, line, description);
}

void TestService::onRemoteConditionalResult( bool condition, std::string description, shared_ptr<Instance> source, int line )
{
	incrementTest();
	if (!condition)
		incrementError();
		
	onCheck(condition, description, source, line);
}

void TestService::error( std::string description, shared_ptr<Instance> source, int line )
{
	incrementTest();
	incrementError();
	if (onCheck)
		onCheck(false, description, source, line);
	else
		output(RBX::MESSAGE_ERROR, source, line, description);
}

void TestService::require2( bool condition, std::string description, shared_ptr<Instance> source, int line )
{
	incrementTest();
	if (!condition)
		incrementError();
	if (onCheck)
		onCheck(condition, description, source, line);
	else if (!condition)
		output(RBX::MESSAGE_ERROR, source, line, "fatal " + description);

	if (!condition)
		done();
}

void TestService::fail( std::string description, shared_ptr<Instance> source, int line )
{
	incrementTest();
	incrementError();
	if (onCheck)
		onCheck(false, description, source, line);
	else
		output(RBX::MESSAGE_ERROR, source, line, "fatal " + description);

	done();
}

void TestService::done()
{
	if (!running)
		return;

	running = false;

	if (onDone)
		onDone();
	else if (testCount == 0)
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "TestService: Doesn't include any assertions");
	else
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "TestService: Run completed, tests: %u, warns: %u, errors: %u", testCount, warnCount, errorCount);

	stop();
}

void TestService::stillWaiting(int runCount, double timeout)
{
	if (runCount != this->runCount)
		return;

	if (!running)
		return;

	if (onStillWaiting)
		onStillWaiting(timeout);
	else
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "TestService: Waiting a total of %g seconds for the test to end", timeout);
}

void TestService::onTimeout(int runCount, double timeout)
{
	if (runCount != this->runCount)
		return;

	if (!running)
		return;

	running = false;

	incrementTest();
	incrementError();

	if (onCheck)
		onCheck(false, RBX::format("Tests failed to complete in %g seconds", timeout), shared_ptr<Instance>(), 0);
	else
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "TestService: Tests failed to complete in %g seconds", timeout);

	if (onDone)
		onDone();

	stop();
}

void TestService::run(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (running)
		throw std::runtime_error("Run still in progress");

	// wire up the callback
	this->resumeFunction = resumeFunction;
	running = true;
	runCount++;

	testCount = 0;
	warnCount = 0;
	errorCount = 0;
	raisePropertyChanged(TestCount);
	raisePropertyChanged(WarnCount);
	raisePropertyChanged(ErrorCount);

	setConfiguration();

	// wire up a timeout
	{
		TimerService* timerService = ServiceProvider::find<TimerService>(this);
		if (!timerService)
			throw std::runtime_error("Unable to get TimerService");

		// Print out a message in case somebody is impatient
		if (timeout > 10.0)
			timerService->delay(boost::bind(&TestService::stillWaiting, shared_from(this), runCount, timeout), 5.0);

		timerService->delay(boost::bind(&TestService::onTimeout, shared_from(this), runCount, timeout), this->timeout);
	}
	
	// run!
	{
		RunService* runService = ServiceProvider::find<RunService>(this);
		if (!runService)
			throw std::runtime_error("Unable to get RunService");
		wasRunning = runService->isRunState();
		if (autoRuns)
			runService->run();
	}

	try
	{
		startScripts();
	}
	catch (...)
	{
		fail("Failed to start test scripts");
		throw;
	}
}

void TestService::incrementTest()
{
	testCount++;
	raisePropertyChanged(TestCount);
}

void TestService::incrementWarn()
{
	warnCount++;
	raisePropertyChanged(WarnCount);
}

void TestService::incrementError()
{
	errorCount++;
	raisePropertyChanged(ErrorCount);
}

void TestService::startScripts()
{
	shared_ptr<const Instances> instances = getChildren().read();
	if (!instances)
		return;
	scriptCount = 0;
	std::for_each(instances->begin(), instances->end(), boost::bind(&TestService::countScript, this, _1));
	std::for_each(instances->begin(), instances->end(), boost::bind(&TestService::startScript, this, _1));
}

void TestService::stopScripts()
{
	shared_ptr<const Instances> instances = getChildren().read();
	if (!instances)
		return;
	std::for_each(instances->begin(), instances->end(), boost::bind(&TestService::stopScript, this, _1));
}

void TestService::countScript( shared_ptr<Instance> instance )
{
	if (shared_dynamic_cast<Script>(instance))
		scriptCount++;
}

void TestService::startScript( shared_ptr<Instance> instance )
{
	if (shared_ptr<Script> script = shared_dynamic_cast<Script>(instance))
	{
		RBX::ScriptContext* scriptContext = RBX::ServiceProvider::create<RBX::ScriptContext>(this);
		if (!scriptContext)
			throw std::runtime_error("Unable to start test script");

		ScriptContext::ScriptStartOptions options;
		options.continuations.successHandler = boost::bind(&TestService::onScriptEnded, shared_from(this), runCount);
		options.continuations.errorHandler = boost::bind(&TestService::onScriptFailed, shared_from(this), runCount, _1, _2, _3, _4);
		// elevate the identity of this script to the current caller's identity:
		options.identity = RBX::Security::Context::current().identity;
		options.filter = boost::bind(&TestService::filterScript, this, _1);

		scriptContext->addScript(script, options);
	}
}

void TestService::stopScript( shared_ptr<Instance> instance )
{
	if (shared_ptr<Script> script = shared_dynamic_cast<Script>(instance))
	{
		RBX::ScriptContext* scriptContext = RBX::ServiceProvider::create<RBX::ScriptContext>(this);
		if (!scriptContext)
			throw std::runtime_error("Unable to stop test script");

		scriptContext->removeScript(script);
	}
}

void TestService::onScriptEnded(int runCount)
{
	if (runCount != this->runCount)
		return;

	if (--scriptCount == 0)
		done();
}

void TestService::onScriptFailed(int runCount, const char* message, const char* callStack, shared_ptr<BaseScript> source, int line)
{
	if (runCount != this->runCount)
		return;

	fail(message, source, line);
}

bool TestService::askForbidChild( const Instance* instance ) const
{
	// Only allow script
	return (dynamic_cast<const Script*>(instance) == NULL);
}

void TestService::setConfiguration()
{
	RBX::PhysicsSettings &ps = RBX::PhysicsSettings::singleton();
	oldSettings.eThrottle = ps.getEThrottle();
	oldSettings.allowSleep = ps.getAllowSleep();
	oldSettings.throttleAt30Fps = ps.getThrottleAt30Fps();
	oldSettings.wasLogAsserts = FLog::Asserts;
	
	RBX::NetworkSettings &ns = RBX::NetworkSettings::singleton();
	ns.setPhysicsSendRate(physicsSendRate);

	if (isPerformanceTest())
	{
		this->message("Running performance test");
		FLog::Asserts = 0;
	}

	if( isPhysicsThrottled )
		ps.setEThrottle( EThrottle::ThrottleDefaultAuto );
	else
		ps.setEThrottle( EThrottle::ThrottleDisabled );
	ps.setAllowSleep( allowSleep );
	ps.setThrottleAt30Fps( is30FpsThrottleEnabled );
}

void TestService::restoreConfiguration()
{
	FLog::Asserts = oldSettings.wasLogAsserts;

	RBX::PhysicsSettings &ps = RBX::PhysicsSettings::singleton();
	ps.setEThrottle( oldSettings.eThrottle );
	ps.setAllowSleep( oldSettings.allowSleep );
	ps.setThrottleAt30Fps( oldSettings.throttleAt30Fps );
}

void TestService::stop()
{
	stopScripts();

	if (!wasRunning)
		if (RunService* runService = ServiceProvider::find<RunService>(this))
			runService->pause();

	restoreConfiguration();
	if (resumeFunction)
		resumeFunction();
}

class MacroSubstituter
{
	std::stringstream str;
	std::stringstream result;
public:
	std::string getResult() const { 
		return result.str(); 
	}

	MacroSubstituter(const std::string& source)
		:str(source)
	{
		int lineNumber = 0;
		try
		{
			char buffer[1024];
			while (str.getline(buffer, 1024))
			{
				lineNumber++;
				//process the line
				std::string line = buffer;
				if (line.size() > 1000)
					throw std::runtime_error("Line too long");

				line += '\n';	// restore the newline character

				processLine(lineNumber, line);
			}
		}
		catch (std::exception& e)
		{
			throw ScriptContext::ScriptStartOptions::LuaSyntaxError(lineNumber, e);
		}
	}

private:
	void processLine(int lineNumber, const std::string& line)
	{
		if (doRBX_Test_Equality(lineNumber, line, "RBX_CHECK_EQUAL", "Check", "==", "!="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_CHECK_NE", "Check", "~=", "=="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_CHECK_GE", "Check", ">=", "<"))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_CHECK_LE", "Check", "<=", ">"))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_CHECK_GT", "Check", ">", "<="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_CHECK_LT", "Check", "<", ">="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_WARN_EQUAL", "Warn", "==", "!="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_WARN_NE", "Warn", "!=", "=="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_WARN_GE", "Warn", ">=", "<"))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_WARN_LE", "Warn", "<=", ">"))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_WARN_GT", "Warn", ">", "<="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_WARN_LT", "Warn", "<", ">="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_REQUIRE_EQUAL", "Require", "==", "!="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_REQUIRE_NE", "Require", "!=", "=="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_REQUIRE_GE", "Require", ">=", "<"))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_REQUIRE_LE", "Require", "<=", ">"))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_REQUIRE_GT", "Require", ">", "<="))
			return;
		if (doRBX_Test_Equality(lineNumber, line, "RBX_REQUIRE_LT", "Require", "<", ">="))
			return;

		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_WARN_MESSAGE", "Warn"))
			return;
		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_CHECK_MESSAGE", "Check"))
			return;
		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_REQUIRE_MESSAGE", "Require"))
			return;

		if (doRBX_Test_Throw(lineNumber, line, "RBX_WARN_THROW", "Warn"))
			return;
		if (doRBX_Test_Throw(lineNumber, line, "RBX_CHECK_THROW", "Check"))
			return;
		if (doRBX_Test_Throw(lineNumber, line, "RBX_REQUIRE_THROW", "Require"))
			return;

		if (doRBX_Test_NoThrow(lineNumber, line, "RBX_WARN_NO_THROW", "Warn"))
			return;
		if (doRBX_Test_NoThrow(lineNumber, line, "RBX_CHECK_NO_THROW", "Check"))
			return;
		if (doRBX_Test_NoThrow(lineNumber, line, "RBX_REQUIRE_NO_THROW", "Require"))
			return;

		if (doRBX_Test(lineNumber, line, "RBX_WARN", "Warn"))
			return;
		if (doRBX_Test(lineNumber, line, "RBX_CHECK", "Check"))
			return;
		if (doRBX_Test(lineNumber, line, "RBX_REQUIRE", "Require"))
			return;

		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_ERROR", "Error"))
			return;
		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_FAIL", "Fail"))
			return;
		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_MESSAGE", "Message"))
			return;
		if (doRBX_SimpleSubstitution(lineNumber, line, "RBX_CHECKPOINT", "Checkpoint"))
			return;

		result << line;
	}

	static void appendArg(std::vector<std::string>* args, std::string::const_iterator begin, std::string::const_iterator end)
	{
		std::string arg;
		arg.assign(begin, end);
		args->push_back(arg);
	}

	bool doRBX_Test_Equality(int lineNumber, const std::string& line, const char* macro, const char* f, const char* equality, const char* notEquality)
	{
		int i = line.find(macro);
		if (i < 0)
			return false;

		// RBX_CHECK_EQUAL(a, b) ===> if a == b then Game:GetService('TestService'):Check(true, [==[a == b]==]) else Game:GetService('TestService'):Check(false,[==[test a == b failed [1 != 2]]==]) end

		using namespace std;
		string::const_iterator start = line.begin() + i + strlen(macro);
		start = Lua::ArgumentParser::skipWhitespaces(start, line.end());

		if (*start != '(')
			throw std::runtime_error("Expected '('");

		vector<string> args;
		string::const_iterator stop = Lua::ArgumentParser::parseBracket(start, line.end(), boost::bind(&appendArg, &args, _1, _2));
		if (args.size() != 2)
			throw RBX::runtime_error("Expected 2 arguments but got %d", (int)args.size());

		// start points to (
		// stop points after )
		RBXASSERT(*start == '(');
		RBXASSERT(*(stop-1) == ')');

		// First write everything before RBX_CHECK
		result << line.substr(0, i);

		// Now substitute the function call
		result << "do ";
		             // picking aZZZZ and bZZZZ, which are unlikely to be used elsewhere
		result <<    "local aZZZZ = " << args[0] << " local bZZZZ = " << args[1] << " ";
		result <<    "if aZZZZ " << equality << " bZZZZ then ";
		result <<       "Game:GetService('TestService'):" << f << "(true, [==[" << args[0] << " " << equality << " " << args[1] << "]==], script, " << lineNumber << ") ";
		result <<    "else ";
		result <<       "Game:GetService('TestService'):" << f << "(false, [==[test " << args[0] << " " << equality << " " << args[1] << " failed []==] .. tostring(aZZZZ) .. ' " << notEquality << " ' .. tostring(bZZZZ) .. ']', script, " << lineNumber << ") ";
		result <<    "end ";
		result << "end";

		// Now the rest of the line
		copy(stop, line.end(), std::ostream_iterator<char>(result));

		return true;
	}

	bool doRBX_Test(int lineNumber, const std::string& line, const char* macro, const char* f)
	{
		int i = line.find(macro);
		if (i < 0)
			return false;

		// RBX_CHECK(a) ===> Game:GetService('TestService'):Check(a,[==[test failed: a]==])

		using namespace std;
		string::const_iterator start = line.begin() + i + strlen(macro);
		start = Lua::ArgumentParser::skipWhitespaces(start, line.end());

		if (*start != '(')
			throw std::runtime_error("Expected '('");

		string::const_iterator stop = Lua::ArgumentParser::parseBracket(start, line.end());

		// start points to (
		// stop points after )
		RBXASSERT(*start == '(');
		RBXASSERT(*(stop-1) == ')');

		// First write everything before RBX_CHECK
		result << line.substr(0, i);

		// Now substitute the function call
		result << "Game:GetService('TestService'):" << f << "(";

		// Now the test argument
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));

		// Now the stringized test
		result << ", [==[test failed: ";
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));
		result << "]==], script, " << lineNumber << ")";

		// Now the rest of the line
		copy(stop, line.end(), std::ostream_iterator<char>(result));

		return true;
	}

	bool doRBX_Test_Throw(int lineNumber, const std::string& line, const char* macro, const char* f)
	{
		int i = line.find(macro);
		if (i < 0)
			return false;

		// RBX_CHECK_THROW(a) ===> Game:GetService('TestService'):Check(ypcall(function() a end) == false, 'Exception expected in ' .. [==[a]==])

		using namespace std;
		string::const_iterator start = line.begin() + i + strlen(macro);
		start = Lua::ArgumentParser::skipWhitespaces(start, line.end());

		if (*start != '(')
			throw std::runtime_error("Expected '('");

		string::const_iterator stop = Lua::ArgumentParser::parseBracket(start, line.end());

		// start points to (
		// stop points after )
		RBXASSERT(*start == '(');
		RBXASSERT(*(stop-1) == ')');

		// First write everything before RBX_CHECK_THROW
		result << line.substr(0, i);

		// Now substitute the function call
		result << "Game:GetService('TestService'):" << f << "(ypcall(function() ";

		// Now the test argument
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));

		// Now the stringized test
		result << " end) == false, 'Exception expected in ' .. [==[";
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));
		result << "]==], script, " << lineNumber << ")";

		// Now the rest of the line
		copy(stop, line.end(), std::ostream_iterator<char>(result));

		return true;
	}


	bool doRBX_Test_NoThrow(int lineNumber, const std::string& line, const char* macro, const char* f)
	{
		int i = line.find(macro);
		if (i < 0)
			return false;

		// RBX_CHECK_NO_THROW(a) ===> Game:GetService('TestService'):Check(ypcall(function() a end) == true, 'Exception thrown by ' .. [==[a]==])

		using namespace std;
		string::const_iterator start = line.begin() + i + strlen(macro);
		start = Lua::ArgumentParser::skipWhitespaces(start, line.end());

		if (*start != '(')
			throw std::runtime_error("Expected '('");

		string::const_iterator stop = Lua::ArgumentParser::parseBracket(start, line.end());

		// start points to (
		// stop points after )
		RBXASSERT(*start == '(');
		RBXASSERT(*(stop-1) == ')');

		// First write everything before RBX_CHECK_THROW
		result << line.substr(0, i);

		// Now substitute the function call
		result << "Game:GetService('TestService'):" << f << "(ypcall(function() ";

		// Now the test argument
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));

		// Now the stringized test
		result << " end) == true, 'Exception thrown by ' .. [==[";
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));
		result << "]==], script, " << lineNumber << ")";

		// Now the rest of the line
		copy(stop, line.end(), std::ostream_iterator<char>(result));

		return true;
	}

	bool doRBX_SimpleSubstitution(int lineNumber, const std::string& line, const char* macro, const char* f)
	{
		int i = line.find(macro);
		if (i < 0)
			return false;

		// RBX_CHECK(a) ===> Game:GetService('TestService'):Check(a,[==[a]==])

		using namespace std;
		string::const_iterator start = line.begin() + i + strlen(macro);
		start = Lua::ArgumentParser::skipWhitespaces(start, line.end());

		if (*start != '(')
			throw std::runtime_error("Expected '('");

		string::const_iterator stop = Lua::ArgumentParser::parseBracket(start, line.end());

		// start points to (
		// stop points after )
		RBXASSERT(*start == '(');
		RBXASSERT(*(stop-1) == ')');

		// First write everything before RBX_CHECK
		result << line.substr(0, i);

		// Now substitute the function call
		result << "Game:GetService('TestService'):" << f << "(";

		// Now the message argument
		copy(start + 1, stop - 1, std::ostream_iterator<char>(result));

		// Now the source
		result << ", script, " << lineNumber << ")";

		// Now the rest of the line
		copy(stop, line.end(), std::ostream_iterator<char>(result));

		return true;
	}
};

std::string TestService::filterScript( const std::string& source )
{
	std::string result = MacroSubstituter(source).getResult();
	return result;
}

void TestService::output( MessageType type, shared_ptr<Instance> source, int line, const std::string& message)
{
	if (source)
		RBX::StandardOut::singleton()->printf(type, "TestService.%s(%d): %s", source->getName().c_str(), line, message.c_str());
	else
		RBX::StandardOut::singleton()->printf(type, "TestService: %s", message.c_str());
}

bool TestService::isPerformanceTest() const
{
	if (!is30FpsThrottleEnabled)
		return true;
	if (!isPhysicsThrottled)
		return true;
	return false;
}

#ifdef RBX_TEST_BUILD
void TestService::doCommand( std::string name )
{
	getVerb(name)->doIt(DataModel::get(this));
}

bool TestService::isCommandEnabled( std::string name )
{
	return getVerb(name)->isEnabled();
}

bool TestService::isCommandChecked( std::string name )
{
	return getVerb(name)->isChecked();
}

Verb* TestService::getVerb( std::string name )
{
	DataModel* dm = DataModel::get(this);
	if (!dm)
		throw std::runtime_error("Invalid operation");
	Verb* verb = dm->getVerb(name);
	if (!verb)
		throw RBX::runtime_error("Verb %s does not exist", name.c_str());
	return verb;
}
#endif

void TestService::onServiceProvider( ServiceProvider* oldProvider, ServiceProvider* newProvider )
{
	Super::onServiceProvider(oldProvider, newProvider);
	if (newProvider)
	{
		resultCollectConnection	= serverCollectResultSignal.connect(boost::bind(&TestService::onRemoteResult,this,_1,_2,_3));
		conditionalResultCollectConnection = serverCollectConditionalResultSignal.connect(boost::bind(&TestService::onRemoteConditionalResult,this,_1,_2,_3,_4));
	}
	else if (oldProvider)
	{
		resultCollectConnection.disconnect();
		conditionalResultCollectConnection.disconnect();
	}
}


static void addName(Reflection::ValueArray* array, const Name* name)
{
	array->push_back(name->str);
}

#ifdef RBX_TEST_BUILD
shared_ptr<const Reflection::ValueArray> TestService::getCommandNames()
{
	DataModel* dm = DataModel::get(this);
	if (!dm)
		throw std::runtime_error("Invalid operation");

	shared_ptr<Reflection::ValueArray> result = rbx::make_shared<Reflection::ValueArray>();

	dm->eachVerbName(boost::bind(&addName, result.get(), _1));

	return result;
}
#endif

FunctionalTest::FunctionalTest()
:description("?")
,timeout(60)
,isPhysicsThrottled(true)
,allowSleep(true)
,is30FpsThrottleEnabled(true)
,hasMigratedSettingsToTestService(false)
{
}

void FunctionalTest::pass(std::string message)
{
	if (!testService)
		throw std::runtime_error("Can't find TestService");
	testService->check(true, message);
	testService->done();
}

void FunctionalTest::warn(std::string message)
{
	if (!testService)
		throw std::runtime_error("Can't find TestService");
	testService->warn(false, message);
	testService->done();
}

void FunctionalTest::error(std::string message)
{
	if (!testService)
		throw std::runtime_error("Can't find TestService");
	testService->check(false, message);
	testService->done();
}

void FunctionalTest::onServiceProvider( ServiceProvider* oldProvider, ServiceProvider* newProvider )
{
	Super::onServiceProvider(oldProvider, newProvider);
	if (newProvider)
	{
		testService = shared_from(newProvider->find<TestService>());
		if (!testService)
		{
			testService = shared_from(newProvider->create<TestService>());
			if (hasMigratedSettingsToTestService)
			{
				// This is an old file with no TestService. We'll create one and migrate the configuration data here:
				testService->description = this->description;
				testService->timeout = this->timeout;
				testService->isPhysicsThrottled = this->isPhysicsThrottled;
				testService->allowSleep = this->allowSleep;
				testService->is30FpsThrottleEnabled = this->is30FpsThrottleEnabled;
				this->hasMigratedSettingsToTestService = true;
			}
		}
	}
}

}
