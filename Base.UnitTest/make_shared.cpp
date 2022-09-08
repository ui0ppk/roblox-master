#include <boost/test/unit_test.hpp>

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "rbx/make_shared.h"

using namespace boost;

struct Counted : boost::noncopyable {
	static int count;
	Counted() { count++; }
	~Counted() { count--; }
};
int Counted::count = 0;

BOOST_AUTO_TEST_SUITE(MakeShared)

	BOOST_AUTO_TEST_CASE(Destructor)
	{
		BOOST_CHECK_EQUAL(0, Counted::count);
		shared_ptr<Counted> p = rbx::make_shared<Counted>();
		BOOST_CHECK_EQUAL(1, Counted::count);
		weak_ptr<Counted> w = p;
		BOOST_CHECK_EQUAL(1, Counted::count);
		p.reset();
		BOOST_CHECK_EQUAL(0, Counted::count);
	}

BOOST_AUTO_TEST_SUITE_END()


