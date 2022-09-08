#include <boost/test/unit_test.hpp>

#include "rbx/test/TimeoutFixture.h"
#include "RBX/CEvent.h"


#ifdef RBX_PLATFORM_IOS
#define TIMEOUT 60
#else
#define TIMEOUT 20
#endif

using namespace boost;


BOOST_FIXTURE_TEST_SUITE(CEvent, RBX::Test::TimeoutFixture<TIMEOUT>)


	BOOST_AUTO_TEST_CASE(CEventManual)
	{
		RBX::CEvent e(true);
		for (int i = 0; i < 100000; ++i)
		{
			e.Set();
			e.Wait();
		}
	}

	static void fCEventAuto(RBX::CEvent& e1, RBX::CEvent& e2)
	{
		for (int i = 0; i < 100000; ++i)
		{
			e1.Wait();
			e2.Set();
		}
	}

	BOOST_AUTO_TEST_CASE(CEventAuto)
	{
		RBX::CEvent e1(false);
		RBX::CEvent e2(false);
		boost::thread t(fCEventAuto, boost::ref(e1), boost::ref(e2));
		for (int i = 0; i < 100000; ++i)
		{
			e1.Set();
			e2.Wait();
		}
		t.join();
	}

	void waitTest(double lower, double seconds, double upper)
	{
		RBX::CEvent e(true);
		RBX::Time start = RBX::Time::now<RBX::Time::Precise>();
		e.Wait(RBX::Time::Interval::from_seconds(seconds));
		RBX::Time::Interval waitTime = RBX::Time::now<RBX::Time::Precise>() - start;

#ifdef __APPLE__ 
		BOOST_WARN_GT(waitTime.seconds(), lower);
		BOOST_WARN_LT(waitTime.seconds(), upper);
#else
		BOOST_CHECK_GT(waitTime.seconds(), lower);
		BOOST_CHECK_LT(waitTime.seconds(), upper);
#endif
	}

	BOOST_AUTO_TEST_CASE(CEventTimeout)
	{
		waitTest(0.5, 1, 1.5);
		waitTest(1, 2, 3);
		waitTest(2, 3, 4);
	}

BOOST_AUTO_TEST_SUITE_END()


