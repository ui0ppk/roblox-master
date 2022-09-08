#include <boost/test/unit_test.hpp>

#include "rbx/rbxtime.h"
#include "rbxformat.h"

using namespace boost;

BOOST_AUTO_TEST_SUITE(Format)

BOOST_AUTO_TEST_CASE(SmallString)
{
	std::string s;
	for (int i=0; i<125; ++i)
		s += "x";
	BOOST_CHECK( RBX::format("%s", s.c_str()) == s);
}

BOOST_AUTO_TEST_CASE(BigString)
{
	std::string s;
	for (int i=0; i<500; ++i)
		s += "x";
	BOOST_CHECK( RBX::format("%s", s.c_str()) == s);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(Time)

	template <RBX::Time::SampleMethod T>
	static void test()
	{
		BOOST_CHECK(true);
		for (int i=0; i<1e7; ++i)
			RBX::Time::now<T>();
	}

	BOOST_AUTO_TEST_CASE(Fast)
	{
		test<RBX::Time::Fast>();
	}

	BOOST_AUTO_TEST_CASE(Multimedia)
	{
		test<RBX::Time::Multimedia>();
	}

	BOOST_AUTO_TEST_CASE(Precise)
	{
		test<RBX::Time::Precise>();
	}

	template <RBX::Time::SampleMethod T>
	static void testAccuracy()
	{
		RBX::Time was = RBX::Time::now<T>();
#ifdef _DEBUG
		for (int i=0; i<1e4; ++i)
#else
		for (int i=0; i<1e6; ++i)
#endif
		{
			RBX::Time now = RBX::Time::now<T>();
			double sec = (now - was).seconds();
			BOOST_CHECK_GE(sec, 0.0);
			//BOOST_CHECK_LE(sec, 0.0011);
			was = now;
		}
	}

	BOOST_AUTO_TEST_CASE(FastAccuracy)
	{
		testAccuracy<RBX::Time::Fast>();
	}

	BOOST_AUTO_TEST_CASE(MultimediaAccuracy)
	{
		testAccuracy<RBX::Time::Multimedia>();
	}

	BOOST_AUTO_TEST_CASE(PreciseAccuracy)
	{
		testAccuracy<RBX::Time::Precise>();
	}

BOOST_AUTO_TEST_SUITE_END()


