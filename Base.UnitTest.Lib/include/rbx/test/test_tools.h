#pragma once

#include "boost/test/test_tools.hpp"
#include "rbx/Debug.h"
#include "rbx/atomic.h"
#include "rbx/rbxTime.h"
#include "boost/thread.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#define RBX_TEST_WITH_TIMEOUT(F, T) testWithTimeout(F, T, BOOST_STRINGIZE(F))

static void checkNoThrow(boost::function<void()> f, std::string stringized)
{
	try 
	{                                                                                           
		f(); 
	}
	catch(RBX::base_exception& ex)
	{                                                                                  
		BOOST_CHECK_IMPL( false, RBX::format("exception '%s' thrown by %s", ex.what(), stringized.c_str() ), CHECK, CHECK_MSG );      
	}                                                                                               
	catch( ... ) 
	{                                                                                  
		BOOST_CHECK_IMPL( false, RBX::format("exception thrown by %s", stringized.c_str() ), CHECK, CHECK_MSG );      
	}                                                                                               
}

static void testWithTimeout(boost::function<void()> f, RBX::Time::Interval timeout, std::string stringized)
{
	boost::thread t(boost::bind(&checkNoThrow, f, stringized));

	BOOST_CHECK_MESSAGE(
		t.timed_join(boost::posix_time::milliseconds((int)(timeout.seconds()*1000.0))), 
		RBX::format("Timeout of '%s' after %g seconds", stringized.c_str(), timeout.seconds())
		);
}

#define RBX_CHECK_NO_EXECEPTION_IMPL( S, TL )                                                       \
    try {                                                                                           \
        S;                                                                                          \
        BOOST_CHECK_IMPL( true, "no exceptions thrown by " BOOST_STRINGIZE( S ), TL, CHECK_MSG ); } \
	catch(RBX::base_exception& ex) {                                                                     \
	    std::string message = ex.what();                                                            \
        message += " thrown by " BOOST_STRINGIZE( S );                                              \
        BOOST_CHECK_IMPL( false, ex.what(), TL, CHECK_MSG );                                        \
    }                                                                                               \
/**/

#define RBX_WARN_NO_EXECEPTION( S )            RBX_CHECK_NO_EXECEPTION_IMPL( S, WARN )
#define RBX_CHECK_NO_EXECEPTION( S )           RBX_CHECK_NO_EXECEPTION_IMPL( S, CHECK )
#define RBX_REQUIRE_NO_EXECEPTION( S )         RBX_CHECK_NO_EXECEPTION_IMPL( S, REQUIRE )

#define BOOST_TEST_TOOL_IMPL2( func, P, check_descr, TL, CT, F, L )            \
	::boost::test_tools::tt_detail::func(                               \
	P,                                                              \
	::boost::unit_test::lazy_ostream::instance() << check_descr,    \
	F,                                         \
	(std::size_t)L,                                          \
	::boost::test_tools::tt_detail::TL,                             \
	::boost::test_tools::tt_detail::CT                              \
	/**/

#define BOOST_CHECK_IMPL2( P, check_descr, TL, CT, F, L )                  \
	do {                                                                \
	BOOST_TEST_PASSPOINT();                                         \
	BOOST_TEST_TOOL_IMPL2( check_impl, P, check_descr, TL, CT, F, L ), 0 );\
	} while( ::boost::test_tools::dummy_cond )                          \
	/**/

#define BOOST_CHECK_MESSAGE_SOURCE( P, M, F, L )     BOOST_CHECK_IMPL2( (P), M, CHECK, CHECK_MSG, F, L )
#define BOOST_REQUIRE_MESSAGE_SOURCE( P, M, F, L )     BOOST_CHECK_IMPL2( (P), M, REQUIRE, CHECK_MSG, F, L )

