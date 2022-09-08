#include <boost/test/unit_test.hpp>

#include "FastLog.h"
#include "rbx/rbxTime.h"

using namespace boost;

LOGGROUP(UnitTestOn)
LOGGROUP(UnitTestOff)

LOGVARIABLE(UnitTestOn, 1)
LOGVARIABLE(UnitTestOff, 0)

DYNAMIC_FASTFLAGVARIABLE(DebugUnitDynamicFlag, true)

BOOST_AUTO_TEST_SUITE(FastLog)

	static int i = 23;
	static int increment() { return ++i; }

	BOOST_AUTO_TEST_CASE(TrailingSemicolon)
	{
		if (false)
			FASTLOG(FLog::UnitTestOn, "Just Message");
		else
			BOOST_CHECK(true);
	}

	BOOST_AUTO_TEST_CASE(Conditionals)
	{
		int i = 2;
		if (false)
			FASTLOG1(FLog::UnitTestOff, "Just Message %d", increment());
		else if (true)
			i = 4;

		BOOST_CHECK_EQUAL(i, 4);

		// If this test fails, then the macro was not written properly because
		// it exposes a partial if-then block. (One more reason MACROS are worse than templates :)
		//
		// Here is the incorrect implementation:
		// #define FASTLOG1(group,level,message,arg1) if(...) RBX::FastLog(...)
		//
		// Here is a correct implementation:
		// #define FASTLOG1(group,level,message,arg1) do { if(...) RBX::FastLog(...); } while (0)
		//
		// The do {} while (0) clause is one way to wrap the macro into a self-contained statement
		// IMPORTANT Notice that there is NO ";" at the end of the block. 
		// See http://stackoverflow.com/questions/1067226/c-multi-line-macro-do-while0-vs-scope-block
	}
	
	BOOST_AUTO_TEST_CASE(LazyEval)
	{
		i = 3;
		FASTLOG1(FLog::UnitTestOff, "Just Message %d", increment());
		BOOST_CHECK_EQUAL(i, 3);

		i = 13;
		FASTLOG1(FLog::Warning, "Warning %d", increment());
		BOOST_CHECK_EQUAL(i, 14);

		i = 23;
		FASTLOG1(FLog::Error, "Just Message %d", increment());
		BOOST_CHECK_EQUAL(i, 24);
	}
	
	BOOST_AUTO_TEST_CASE(BasicLog)
	{
		RBX::Time start = RBX::Time::now<RBX::Time::Multimedia>();
		
		std::string nullString = "", shortString = "aaaaa", longString = "very very long string";
		for(int i = 0; i < LOG_HISTORY; i++)
		{
			FASTLOG(FLog::UnitTestOn, "Just Message");
			FASTLOG1(FLog::UnitTestOn, "Message with argument %u", 1);
			FASTLOG3(FLog::UnitTestOn, "Message with pointer %p, number %u and %u", this, 3, 2);
			FASTLOGS(FLog::UnitTestOn, "String message - %s", "");
			FASTLOGS(FLog::UnitTestOn, "String message - %s", "AAAAA");
			FASTLOGS(FLog::UnitTestOn, "Large string message - %s", "01234567890123456789");

			FASTLOG1F(FLog::UnitTestOn, "Message with argument %f", 10.0f);
			FASTLOG3F(FLog::UnitTestOn, "Message with argument %f %f %f", 10.0f, 5.0f, -1.0f);

			FASTLOG(FLog::UnitTestOn, "Log");
			FASTLOG(FLog::UnitTestOn, "One more");

			FASTLOG(FLog::UnitTestOff, "Off");
			FASTLOG(FLog::UnitTestOff, "Off, but on");

			FASTLOGS(FLog::UnitTestOn, "Null string message - %s", nullString);
			FASTLOGS(FLog::UnitTestOn, "Short string message - %s", shortString);
			FASTLOGS(FLog::UnitTestOn, "Long string message - %s", longString);
		}

		RBX::Time finish = RBX::Time::now<RBX::Time::Multimedia>();
		FASTLOG1F(FLog::UnitTestOn, "Logging took %f seconds", (float)(finish - start).seconds());
	}

	FASTFLAGVARIABLE(TestFastFlag, false)

	BOOST_AUTO_TEST_CASE(FastFlags)
	{
		// We can't do the following test because the --fflags= arg might have overriden it
		//BOOST_CHECK_EQUAL(FFlag::TestFastFlag, false);
		FLog::SetValue("TestFastFlag", "True", FASTVARTYPE_STATIC);
		BOOST_CHECK_EQUAL(FFlag::TestFastFlag, true);
		FLog::SetValue("TestFastFlag", "False", FASTVARTYPE_STATIC);
		BOOST_CHECK_EQUAL(FFlag::TestFastFlag, false);
	}

	BOOST_AUTO_TEST_CASE(SettingUnknownGroups)
	{
		FLog::SetValue("UnknownGroup", "1", FASTVARTYPE_STATIC);
		FLog::SetValue("UnknownFlag", "true", FASTVARTYPE_STATIC);
		{
			FLog::Channel UnknownGroup = 0; 
			FLog::RegisterLogGroup("UnknownGroup", &UnknownGroup);

			bool UnknownFlag = 0; 
			FLog::RegisterFlag("UnknownFlag", &UnknownFlag);

			BOOST_CHECK_EQUAL(UnknownGroup, 1);
			BOOST_CHECK_EQUAL(UnknownFlag, true);
		}
	}

	BOOST_AUTO_TEST_CASE(DynamicVariables)
	{
		FLog::SetValue("DebugUnitDynamicFlag", "false", FASTVARTYPE_STATIC);
		BOOST_CHECK_EQUAL(DFFlag::DebugUnitDynamicFlag, true);
		FLog::SetValue("DebugUnitDynamicFlag", "false", FASTVARTYPE_DYNAMIC);
		BOOST_CHECK_EQUAL(DFFlag::DebugUnitDynamicFlag, false);		
	}

	FASTINTVARIABLE(TestFastInt, 3)
	BOOST_AUTO_TEST_CASE(FastInts)
	{
		FLog::SetValue("TestFastInt", "2", FASTVARTYPE_STATIC);
		BOOST_CHECK_EQUAL(FInt::TestFastInt, 2);
	}

    SYNCHRONIZED_FASTFLAGVARIABLE(TestSynchronizedFlag, false);
    BOOST_AUTO_TEST_CASE(SynchronizedFlagVariables)
    {
        FLog::SetValue("TestSynchronizedFlag", "true", FASTVARTYPE_STATIC);
        BOOST_CHECK_EQUAL(SFFlag::TestSynchronizedFlag, false);

        FLog::SetValue("TestSynchronizedFlag", "true", FASTVARTYPE_SYNC);
        BOOST_CHECK_EQUAL(SFFlag::TestSynchronizedFlag, true);
        BOOST_CHECK_EQUAL(*SFFlag::TestSynchronizedFlagIsSync, true);

		FLog::ResetSynchronizedVariablesState();
		BOOST_CHECK_EQUAL(*SFFlag::TestSynchronizedFlagIsSync, false);

        FLog::SetValueFromServer("TestSynchronizedFlag", "true");
        BOOST_CHECK_EQUAL(SFFlag::TestSynchronizedFlag, true);
        BOOST_CHECK_EQUAL(*SFFlag::TestSynchronizedFlagIsSync, true);
    }

    SYNCHRONIZED_FASTINTVARIABLE(UnitSynchronizedInt, 0);
    BOOST_AUTO_TEST_CASE(SynchronizedIntVariables)
    {
        FLog::SetValue("UnitSynchronizedInt", "1", FASTVARTYPE_STATIC);
        BOOST_CHECK_EQUAL(SFInt::UnitSynchronizedInt, 0);

        FLog::SetValue("UnitSynchronizedInt", "1", FASTVARTYPE_SYNC);
        BOOST_CHECK_EQUAL(SFInt::UnitSynchronizedInt, 1);

		// flag state was reset in the previous test, so IsSynce should be false here
		BOOST_CHECK_EQUAL(*SFInt::UnitSynchronizedIntIsSync, false);

        FLog::SetValueFromServer("UnitSynchronizedInt", "2");
        BOOST_CHECK_EQUAL(SFInt::UnitSynchronizedInt, 2);
        BOOST_CHECK_EQUAL(*SFInt::UnitSynchronizedIntIsSync, true);
    }


BOOST_AUTO_TEST_SUITE_END()