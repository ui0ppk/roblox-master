#include "rbx/test/App.UnitTest.Lib.h"
#include "util/Statistics.h"
#include "v8datamodel/FastLogSettings.h"

#ifdef _WIN32
#include "LogManager.h"
#else
class MainLogManager {
public:
	MainLogManager(const char *productName, const char *crashExtension, const char *crashEventExtension) {}
};
#endif

using namespace RBX::Test;

BOOST_GLOBAL_FIXTURE(BaseGlobalFixture);
BOOST_GLOBAL_FIXTURE(AppGlobalFixture);

int BOOST_TEST_CALL_DECL
main( int argc, char* argv[] )
{
	char** filteredArgv = new char*[argc];
	int filteredArgc = 0;
	MainLogManager *logManager = NULL;

	extern void setTestPath(std::string s);
	extern void setAsClientTestInstance(void);
	extern void setWithFastLog(void);
    extern void setClientAppSettings(std::string s);
    extern void setContentPath(std::string s);

	for (int i = 0; i < argc; i++)
	{
		std::string arg(argv[i]);
		const char* pathArg = "--tests_path=";
		if (arg.find(pathArg) == 0)
			setTestPath(arg.substr(strlen(pathArg)));
		else
		{
			if (!RBX::Test::BaseGlobalFixture::processArg(argv[i]))
				if (!RBX::Test::AppGlobalFixture::processArg(argv[i]))
					filteredArgv[filteredArgc++] = argv[i];
		}

		if (arg.find("--help") == 0)
		{
			std::cout << "ROBLOX custom arguments:\n";
			std::cout << "   --tests_path=<dir> - directory to the Tests folder\n";
			return false;
		}

		if (arg.find("--client_test") == 0)
		{
			setAsClientTestInstance();
		}

		if (arg.find("--fastlog") == 0)
		{
			setWithFastLog();
			logManager = new MainLogManager("RobloxTest", ".test.dmp", ".test.crashevent");
		}

		if (arg.find("--client_app_settings") == 0)
		{
            setClientAppSettings(arg);
			std::string baseURL = "http://www.roblox.com/";
			if (arg.find("--client_app_settings=") == 0)
			{
				const char* settingsArg = "--client_app_settings=";
				std::string siteArg = arg.substr(strlen(settingsArg));
				if (siteArg ==  "gt1")
					baseURL = "www.gametest1.robloxlabs.com";
				else if (siteArg ==  "gt2")
					baseURL = "www.gametest2.robloxlabs.com";
				else if (siteArg ==  "gt3")
					baseURL = "www.gametest3.robloxlabs.com";
				else if (siteArg ==  "gt4")
					baseURL = "www.gametest4.robloxlabs.com";
				else
					baseURL = "http://www.roblox.com/";
			}
			SetBaseURL(baseURL);
			RBX::ClientAppSettings::Initialize();

			// disable influxdb reporting
			FLog::SetValue("HttpInfluxHundredthsPercentage", "0", FASTVARTYPE_ANY);			
		}

        if (arg.find("--content_path=") == 0)
        {
            setContentPath(arg);
        }
	}

	// prototype for user's unit test init function
#ifdef BOOST_TEST_ALTERNATIVE_INIT_API
	extern bool init_unit_test();

	boost::unit_test::init_unit_test_func init_func = &init_unit_test;
#else
	extern ::boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] );

	boost::unit_test::init_unit_test_func init_func = &init_unit_test_suite;
#endif

	int result = ::boost::unit_test::unit_test_main( init_func, filteredArgc, filteredArgv );
	delete [] filteredArgv;
	delete logManager;

	std::cout << "- All tests finished.  atexit output follows:";

	return result;
}




