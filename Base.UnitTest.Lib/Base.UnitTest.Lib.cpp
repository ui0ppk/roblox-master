
#include "rbx/test/Base.UnitTest.Lib.h"
#include "boost/test/unit_test_suite.hpp"
#include "JetBrains/teamcity_messages.h"

using namespace JetBrains;
BOOST_GLOBAL_FIXTURE(TeamcityFormatterRegistrar);

namespace RBX { namespace Test {
    
    bool BaseGlobalFixture::fflagsAllOn = false;
    bool BaseGlobalFixture::fflagsAllOff = false;

	BaseGlobalFixture::BaseGlobalFixture()
	{
		boost::unit_test::framework::register_observer(*this);
		if (!RBX::assertionHook())
			RBX::setAssertionHook(&handleDebugAssert);

		if (!RBX::failureHook())
			RBX::setFailureHook(&handleFailure);

		setAreAssertsTested(true);
	}

	bool BaseGlobalFixture::tooManyAssertionsCheck()
	{
		const long maxCount = 100;

		long count = assertCount++;

		if (count < maxCount)
			return false;

		if (count == maxCount)
			BOOST_MESSAGE("Too many assertions. No more will be reported for the current test");

		return true;
	}

	bool BaseGlobalFixture::handleDebugAssert(const char* expression, const char* filename, int lineNumber)
	{
		if (!tooManyAssertionsCheck())
		{
			std::string message = "Assertion Failed: ";
			message += expression;
			BOOST_CHECK_MESSAGE_SOURCE(false, message.c_str(), filename, lineNumber);
		}
		return false; // let other processing take place, too
	}
	
	bool BaseGlobalFixture::handleFailure(const char* expression, const char* filename, int lineNumber)
	{
		std::string message = "Assertion Failed: ";
		message += expression;
		BOOST_REQUIRE_MESSAGE_SOURCE(false, message.c_str(), filename, lineNumber);
		return true; // Don't let other processing take place. We're going to exit
	}

	static bool skipFastFlag(const char* flagName) {
		return
			// Test flags (never remove)
			strcmp(flagName, "TestFastFlag") == 0 ||
			strcmp(flagName, "TestTrueValue") == 0 ||
			strcmp(flagName, "TestFalseValue") == 0 ||
            // Debug flags
            strncmp(flagName, "Debug", 5) == 0 ||
			// Broken flags
			strcmp(flagName, "UseClusterCache") == 0;
	}

	static void setFFlag(const std::string& name, const std::string& varValue, void* context)
	{
		if (!skipFastFlag(name.c_str())) {
			bool value = *reinterpret_cast<bool*>(context);
			if(value && varValue == "false")
				FLog::SetValue(name, "true", FASTVARTYPE_ANY, false);
			if(!value && varValue == "true")
				FLog::SetValue(name, "false", FASTVARTYPE_ANY, false);
		}
	}

	static void setAllFFlags(bool value)
	{
        BaseGlobalFixture::fflagsAllOn = value;
        BaseGlobalFixture::fflagsAllOff = !value;
        FLog::ForEachVariable(&setFFlag, &value, FASTVARTYPE_ANY);
	}

	bool BaseGlobalFixture::processArg( const std::string arg )
	{
		if (arg.find("--help") == 0)
		{
			std::cout << "ROBLOX custom arguments:\n";
			std::cout << "   --fflags=true|false - override all FFlag values\n";
			return false;
		}

		const char* fflagArg = "--fflags=";
		if (arg.find(fflagArg) == 0)
		{
			std::string fflagValue = arg.substr(strlen(fflagArg));
			if (fflagValue == "true")
			{
				BOOST_MESSAGE("setting all FFlags to true");
				setAllFFlags(true);
			}
			else if (fflagValue == "false")
			{
				BOOST_MESSAGE("setting all FFlags to false");
				setAllFFlags(false);
			}
			else 
				throw boost::unit_test::framework::setup_error("bad fflag value. Must be true or false");
			return true;
		}
		return false;
	}




	rbx::atomic<int> BaseGlobalFixture::assertCount;
}}