

// This directive will define the main entry point
#define BOOST_TEST_MODULE Base
#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>

#include "rbx/Debug.h"

#include "rbx/test/Base.UnitTest.Lib.h"


#ifdef _WIN32
int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
	char** filteredArgv = new char*[argc];
	int filteredArgc = 0;

	for (int i = 0; i < argc; i++)
		if (!RBX::Test::BaseGlobalFixture::processArg(argv[i]))
			filteredArgv[filteredArgc++] = argv[i];

	// prototype for user's unit test init function
#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
    boost::unit_test::init_unit_test_func init_func = &init_unit_test;
#else
    boost::unit_test::init_unit_test_func init_func = &init_unit_test_suite;
#endif

    int result = ::boost::unit_test::unit_test_main( init_func, filteredArgc, filteredArgv );
	delete [] filteredArgv;
	return result;
}
#endif

using namespace RBX::Test;

BOOST_GLOBAL_FIXTURE(BaseGlobalFixture);

