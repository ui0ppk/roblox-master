#pragma once

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

namespace RBX { namespace Test {

template<int timeout>
class TimeoutFixture
{
	boost::thread thread;
	volatile bool done;
	boost::condition_variable cond;
	boost::mutex mut;

	void monitor()
	{
		boost::system_time const time = boost::get_system_time() + boost::posix_time::seconds(timeout);
		{
			boost::unique_lock<boost::mutex> lock(mut);
			if (!done)
				BOOST_REQUIRE_MESSAGE(cond.timed_wait(lock, time), "Timeout reached!");
		}
	}
public:
	TimeoutFixture()
		:done(false)
	{
		boost::unique_lock<boost::mutex> lock(mut);
		thread = boost::thread(boost::bind(&TimeoutFixture::monitor, this));
	}

	~TimeoutFixture()
	{
		{
			boost::unique_lock<boost::mutex> lock(mut);
			done = true;
			cond.notify_one();
		}
		thread.join();
	}
};

}}