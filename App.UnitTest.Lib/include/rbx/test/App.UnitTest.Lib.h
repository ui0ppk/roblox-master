
#include <boost/test/unit_test.hpp>

#include "rbx/test/Base.UnitTest.Lib.h"

namespace RBX {
	namespace Test {
		// Define this fixture in your project:
		// BOOST_GLOBAL_FIXTURE(AppGlobalFixture);
		class AppGlobalFixture
		{
		public:
			AppGlobalFixture();

			// returns true if the arg is processed
			static bool processArg(const std::string arg);
		};
	}
}
