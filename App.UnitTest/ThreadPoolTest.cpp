#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "Util/ThreadPool.h"

using namespace RBX;

namespace
{
	static inline void sleepTask(int ms)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(ms));
	}
}

BOOST_AUTO_TEST_SUITE( ThreadPoolTest ) // avoid name conflict with "Tool"

BOOST_AUTO_TEST_CASE( ThreadPoolSuccessfulScheduleTest )
{
	ThreadPool *tp = new ThreadPool(1, BaseThreadPool::NoAction, 1);
	BOOST_CHECK_EQUAL(true, tp->schedule(boost::bind(&sleepTask, 1)));
}

BOOST_AUTO_TEST_CASE( ThreadPoolMaxScheduleTest )
{
	ThreadPool *tp = new ThreadPool(1, BaseThreadPool::NoAction, 1);
	bool success = true;
	for (size_t i = 0; i < 3; ++i)
	{
		success = success && tp->schedule(boost::bind(&sleepTask, 1000));
	}
	BOOST_CHECK_EQUAL(false, success);
}

BOOST_AUTO_TEST_CASE( ThreadPoolUnlimitedScheduleTest )
{
	ThreadPool *tp = new ThreadPool(1);
	bool success = true;
	for (size_t i = 0; i < 3; ++i)
	{
		success = success && tp->schedule(boost::bind(&sleepTask, 1000));
	}
	BOOST_CHECK_EQUAL(true, success);
}

BOOST_AUTO_TEST_SUITE_END()
