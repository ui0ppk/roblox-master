
#pragma once

#include "rbx/test/test_tools.h"
#include "boost/test/test_observer.hpp"
#include "boost/test/framework.hpp"

namespace RBX { namespace Test {

	class BaseGlobalFixture : public boost::unit_test::test_observer
	{
		static rbx::atomic<int> assertCount;

		virtual void test_unit_finish( boost::unit_test::test_unit const&, unsigned long /* elapsed */ ) 
		{
			assertCount = 0;
		}

		static bool tooManyAssertionsCheck();
		static bool handleDebugAssert(const char* expression, const char* filename, int lineNumber);
		static bool handleFailure(const char* expression, const char* filename, int lineNumber);
	public:

        static bool fflagsAllOn;
        static bool fflagsAllOff;

		static void setAreAssertsTested(bool value)
		{
			FLog::Asserts = value ? 1 : 0;
		}

		BaseGlobalFixture();

		~BaseGlobalFixture()
		{
			boost::unit_test::framework::deregister_observer(*this);
		}

		// returns true if the arg is processed
		static bool processArg(const std::string arg);
	};


	class PerformanceTestFixture
	{
		const FLog::Channel oldValue;
	public:
		PerformanceTestFixture()
			:oldValue(FLog::Asserts)
		{
			BOOST_MESSAGE("Running performance test");
			BOOST_CHECK(true);	// to prevent "Test case doesn't include any assertions"

			FLog::Asserts = 0;
		}
		~PerformanceTestFixture()
		{
			FLog::Asserts = oldValue;
		}
	};

}}