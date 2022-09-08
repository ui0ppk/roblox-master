#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <boost/test/parameterized_test.hpp>

#define BOOST_FILESYSTEM_NO_DEPRECATED 
#include <boost/filesystem.hpp>

#include "rbx/test/DataModelFixture.h"
#include "v8xml/Serializer.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/Test.h"
#include "v8datamodel/Workspace.h"
#include "V8DataModel/PhysicsSettings.h"
#include "script/script.h"
#include "script/ScriptContext.h"

#include "Network/GameConfigurer.h"
#include "../NetworkSettings.h"

#include "rbx/CEvent.h"
#include "rbx/test/ErrorLogsFixture.h"
#include "rbx/test/test_tools.h"
#include "rbx/make_shared.h"

using namespace RBX;
using namespace boost::filesystem;

extern void SetDefaultFilePath(const std::string &path);

class Tester
{
public:
	virtual ~Tester() {}
	virtual void run() = 0;
	virtual void wait() = 0;
};

#define RBX_SOURCE_TO_FILE(source) \
	source ? boost::unit_test::const_string(source->getFullName()) : BOOST_TEST_EMPTY_STRING       \
	/**/

#define RBX_TEST_PASSPOINT(source, line)                                  \
	::boost::unit_test::unit_test_log.set_checkpoint(           \
	RBX_SOURCE_TO_FILE(source),  \
	(std::size_t)line )                                 \
	/**/

#define RBX_TEST_CHECKPOINT( M, source, line )                              \
	::boost::unit_test::unit_test_log.set_checkpoint(           \
	RBX_SOURCE_TO_FILE(source),  \
	(std::size_t)line,                                          \
	(::boost::wrap_stringstream().ref() << M).str() )       \
	/**/

#define RBX_TEST_TOOL_IMPL( func, P, check_descr, TL, CT, source, line )            \
	::boost::test_tools::tt_detail::func(                               \
	P,                                                              \
	::boost::unit_test::lazy_ostream::instance() << check_descr,    \
	RBX_SOURCE_TO_FILE(source),  \
	(std::size_t)line,                                          \
	::boost::test_tools::tt_detail::TL,                             \
	::boost::test_tools::tt_detail::CT                              \
	/**/

#define RBX_CHECK_IMPL( P, check_descr, TL, CT, source, line )                  \
	do {                                                                \
	RBX_TEST_PASSPOINT(source, line);                                         \
	RBX_TEST_TOOL_IMPL( check_impl, P, check_descr, TL, CT, source, line ), 0 );\
	} while( ::boost::test_tools::dummy_cond )                          \

#define RBX_TEST_LOG_ENTRY( ll, source, line )                                                  \
	(::boost::unit_test::unit_test_log                                              \
	<< ::boost::unit_test::log::begin( RBX_SOURCE_TO_FILE(source), (std::size_t)line ))(ll)  \
	/**/

#define RBX_TEST_MESSAGE( M, source, line )                                 \
	BOOST_TEST_LOG_ENTRY( ::boost::unit_test::log_messages )    \
	<< (::boost::unit_test::lazy_ostream::instance() << M)      \
	/**/

class TestServiceTester : public Tester
{
	shared_ptr<TestService> test;
	CEvent event;
#ifdef _WIN32
	std::vector<HANDLE> clientProcessHandles;
#endif

	void onMessaged(std::string text, shared_ptr<Instance> source, int line)
	{
		::printf("- Message: %s\n", text.c_str() );

		RBX_TEST_MESSAGE(text, source, line);
	}

	void onWarning(bool condition, std::string description, shared_ptr<Instance> source, int line)
	{
		if(!condition)
		{
			::printf("- Warning: %s\n", description.c_str() );
		}

		RBX_CHECK_IMPL((condition), description, WARN, CHECK_MSG, source, line);
	}

	void onChecking(bool condition, std::string description, shared_ptr<Instance> source, int line)
	{
		RBX_CHECK_IMPL((condition), description, CHECK, CHECK_MSG, source, line);
	}

	void onCheckpoint(std::string description, shared_ptr<Instance> source, int line)
	{
		RBX_TEST_CHECKPOINT(description, source, line);
	}
public:
	TestServiceTester(shared_ptr<TestService> test)
		:test(test)
		,event(true)
	{
		// Hook up the callbacks
		test->onMessage = boost::bind(&TestServiceTester::onMessaged, this, _1, _2, _3);
		test->onWarn = boost::bind(&TestServiceTester::onWarning, this, _1, _2, _3, _4);
		test->onCheck = boost::bind(&TestServiceTester::onChecking, this, _1, _2, _3, _4);
		test->onCheckpoint = boost::bind(&TestServiceTester::onCheckpoint, this, _1, _2, _3);
		test->onStillWaiting = boost::bind(&TestServiceTester::onStillWaiting, this, _1);

	}
	void run()
	{
		test->run(
			boost::bind(&TestServiceTester::onDone, this),
			boost::bind(&TestServiceTester::onError, this, _1)
			);
	}

	void onStillWaiting(double timeout)
	{
		BOOST_MESSAGE(boost::format("Waiting up to %d seconds for the test to end") % timeout);
	}

	void onError(std::string message)
	{
		BOOST_ERROR(message);
		event.Set();
	}

	void onDone()
	{
		event.Set();
#ifdef _WIN32
		for (unsigned int i = 0; i < clientProcessHandles.size(); i++)
			TerminateProcess(clientProcessHandles[i], 2);
#endif
	}

	virtual void wait()
	{
		event.Wait();
	}

#ifdef _WIN32
	void collectProcessHandle(HANDLE processHandle)
	{
		clientProcessHandles.push_back(processHandle);
	}
#endif
};


class EmptyTester : public Tester
{
	RBX::Time::Interval timeout;
	CEvent event;
	DataModel* dataModel;

	static shared_ptr<const Reflection::Tuple> setEvent(CEvent* event)
	{
		event->Set();
		return shared_ptr<const Reflection::Tuple>();
	}

public:
	EmptyTester(DataModel* dataModel)
		:event(true)
		,dataModel(dataModel)
	{
		BOOST_TEST_MESSAGE("No test found. Running for 5 game seconds. Timout in 60 seconds wall time");
		timeout = RBX::Time::Interval::from_seconds(60);
	}

	void run()
	{
		ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(dataModel);

		RBX::Reflection::Tuple args(1);
		args.values[0] = rbx::make_shared<Lua::GenericFunction>(boost::bind(&EmptyTester::setEvent, &event));

		const char* clientScript = "done = ...\n"\
			"repeat until game:GetService('RunService').Stepped:wait() > 5\n"
			"done()\n";

		// Run!
		RBX::RunService* runService = RBX::ServiceProvider::create<RBX::RunService>(dataModel);
		BOOST_REQUIRE(runService);
		runService->run();

		RBX_REQUIRE_NO_EXECEPTION(scriptContext->executeInNewThread(RBX::Security::CmdLine_, ProtectedString::fromTrustedSource(clientScript), "End callback", args));

		// TODO: Should we log StandardOut errors as test errors?
	}

	virtual void wait()
	{
		BOOST_MESSAGE(boost::format("Waiting up to %d seconds for the test to end") % timeout.seconds());
		if (!event.Wait(timeout))
			BOOST_WARN_MESSAGE(false, "Test timed out");
	}

};


static std::string appSettings;
void setClientAppSettings(std::string s)
{
    appSettings = s;
}
std::string getClientAppSettings()
{
    return appSettings;
}

static std::string contentPath;
void setContentPath(std::string s)
{
    contentPath = s;
}
std::string getContentPath()
{
    return contentPath;
}

static bool clientTest = false;
static path clientTestFile;
static bool useFastLog;

class FileTester
{
	path file;
    boost::shared_ptr<GameConfigurer> gameConfigurer;

public:
	FileTester(path file):file(file) {}
	
	void test()
	{
		//OutputLoggingFixture g;
		boost::scoped_ptr<Tester> tester;

		// Create a game
		DataModelFixture fixture;

		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);

		{
			DataModel::LegacyLock lock(fixture.dataModel, DataModelJob::Write);

			boost::filesystem::path clientTestFilePath = file;
			clientTestFilePath.replace_extension(".clienttest");

			// Load the game data
			std::ifstream stream(file.string().c_str(), std::ios_base::in | std::ios_base::binary);
			Serializer serializer;
			serializer.load(stream, fixture.dataModel.get());
			LuaSourceContainer::blockingLoadLinkedScripts(fixture.dataModel->create<ContentProvider>(), fixture.dataModel.get());

			fixture.dataModel->processAfterLoad();

			// Find the test
			shared_ptr<TestService> test = shared_from(ServiceProvider::find<TestService>(fixture.dataModel.get()));
			if (test)
			{
				tester.reset(new TestServiceTester(test));
#ifdef _WIN32
				test->setIsAClient(clientTest);

				NetworkSettings::singleton().incommingReplicationLag = test->lagSimulation;

				if (test->isMultiPlayerTest() && !test->isAClient())
				{
					startServerHeadless(fixture.dataModel.get());

					for (int i = 0; i < test->getNumberOfPlayers(); i++)
					{
						boost::xtime xt;
						boost::xtime_get(&xt, boost::TIME_UTC_);
						xt.sec += 1;
						boost::thread::sleep(xt);
						TestServiceTester* tst = dynamic_cast<TestServiceTester*>(tester.get());
						if (tst)
							tst->collectProcessHandle(startLocalClient(i, clientTestFilePath));
					}
				}
				else if (test->isAClient())
				{
					connectClientHeadless(fixture.dataModel.get());
				}
#endif
			}
			else
				tester.reset(new EmptyTester(fixture.dataModel.get()));
			
			tester->run();
		}

		tester->wait();
	}

#ifdef _WIN32
	HANDLE startLocalClient(int clientIndex, boost::filesystem::path clientTestPath)
	{
		std::string pathName = clientTestPath.parent_path().string();
		std::string fileName = clientTestPath.filename().string();
		std::string commandLine = "--log_level=all --tests_path=" + pathName + " --run_test=" + fileName + " --client_test " + getClientAppSettings() + " " + getContentPath();
		if (useFastLog)
		{
			commandLine += " --fastlog";
		}
		TCHAR* tCharCmdline = new TCHAR[commandLine.size() + 1];
		tCharCmdline[commandLine.size()] = '\0';
		std::copy(commandLine.begin(), commandLine.end(), tCharCmdline);

		LPTSTR szCmdline = (LPTSTR)tCharCmdline;

		// exe directory
		TCHAR szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, MAX_PATH);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		// Start the child process. 
		if( CreateProcess( szPath,   //  module
			szCmdline,      // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			CREATE_NEW_CONSOLE,              // creation flag
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi )           // Pointer to PROCESS_INFORMATION structure
		) 
		{
			printf( "Client number (%d) successfully started.\n", clientIndex );
			delete tCharCmdline;
			return pi.hProcess;
		}
		else
		{
			printf( "Client start failed: (%d).\n", GetLastError() );
			delete tCharCmdline;
			return NULL;
		}
	}
#endif

    void startServerHeadless(DataModel* dm)
    {
        const ProtectedString serverScript = ProtectedString::fromTrustedSource("loadfile('http://www.roblox.com/game/gameserver.ashx')(0, 53640)\n");

        DataModel::LegacyLock lock(dm, DataModelJob::Write);

        ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(dm);
        RBX::Reflection::Tuple args;
        scriptContext->executeInNewThread(RBX::Security::COM, serverScript, "Script", args);			
    }

    void connectClientHeadless(DataModel* dm)
    {
        std::string response;
        Http("http://www.roblox.com/game/join.ashx?UserID=0&serverPort=53640").get(response);

        int firstNewLineIndex = response.find("\r\n");
        if (response[firstNewLineIndex+2] == '{')
        {
            gameConfigurer.reset(new PlayerConfigurer());
            gameConfigurer->configure(Security::COM, dm, response.substr(firstNewLineIndex+2));
        }
        else
        {
            ProtectedString clientScript = ProtectedString::fromTrustedSource(response);

            DataModel::LegacyLock lock(dm, DataModelJob::Write);

            ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(dm);
            RBX::Reflection::Tuple args;
            scriptContext->executeInNewThread(RBX::Security::COM, clientScript, "Script", args);
        }

        printf("Client joined\n");
    }
};

static path removeWhitespace(path myPath)
{
	path::string_type pathString = myPath.native();
	for (unsigned int i = 0; i < pathString.length(); i++){
		if (pathString[i] == ' ') pathString[i] = '_';
	}
	path newPath(pathString);
	return newPath;
}

static bool gatherTestCases(path dir_path, boost::unit_test::test_suite* suite)
{
	bool foundOne = false;

	directory_iterator end_itr; // default construction yields past-the-end

	// First find files
	std::vector<path> v;
	for ( directory_iterator iter( dir_path ); iter != end_itr; ++iter )
	{
		if (is_directory(iter->status()))
			continue;
		path p = iter->path();
		if (p.extension() == ".rbxl" || p.extension() == ".rbxlx" || (clientTest && p.extension() == ".clienttest"))
		{
			// remove whitespaces from test level names
			path filename_without_spaces = removeWhitespace(p.filename());
			filename_without_spaces.replace_extension();
			shared_ptr<FileTester> tester(new FileTester(p));
			if (clientTest && p.extension() == ".clienttest")
			{
				clientTestFile = p;
				suite->add(boost::unit_test::make_test_case( &FileTester::test, filename_without_spaces.string() + ".clienttest", tester ));
			}
			else if (p.extension() == ".rbxl")
				suite->add(boost::unit_test::make_test_case( &FileTester::test, filename_without_spaces.string() + ".rbxl", tester ));
			else
				suite->add(boost::unit_test::make_test_case( &FileTester::test, filename_without_spaces.string() + ".rbxlx", tester ));

			foundOne = true;
		}
	}

	// Now recurse through sub-directories
	for ( directory_iterator iter( dir_path ); iter != end_itr; ++iter )
	{
		if (!is_directory(iter->status()))
			continue;
		path p = iter->path();
		boost::unit_test::test_suite* subSuite = new boost::unit_test::test_suite(p.filename().string());
		suite->add(subSuite);
		foundOne = gatherTestCases(p, subSuite) || foundOne;
	}

	return foundOne;
}


static const char* suiteName = "Game";

static path testPath;

void setTestPath(std::string s)
{
	testPath = s;
	if (!testPath.has_root_path())
	{
		testPath = initial_path<path>() / s;
	}
#ifdef RBX_TEST_BUILD
    SetDefaultFilePath(testPath.string());
#endif
}

void setAsClientTestInstance(void)
{
	clientTest = true;
}

void setWithFastLog(void)
{
	useFastLog = true;
}

extern bool init_unit_test()
{
	if (testPath.empty())
		testPath = initial_path<path>() / "Tests";

	const std::string pathString = testPath.string();

	boost::unit_test::test_suite* suite = &boost::unit_test::framework::master_test_suite();
	suite->p_name.value = suiteName;

	if (!is_directory(testPath))
		throw boost::unit_test::framework::setup_error(pathString + " does not exist");

	if (!gatherTestCases(testPath, suite))
		throw boost::unit_test::framework::setup_error("no rbxl or rbxlx files found in " + pathString);

	return true;
}

boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] )
{
	init_unit_test();
	return 0;
}

