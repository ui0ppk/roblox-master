#include <iostream>

#include <v8datamodel/DataModel.h>
#include "v8xml/Serializer.h"
#include "v8tree/Service.h"
#include "Util/RunStateOwner.h"
#include "v8datamodel/GameBasicSettings.h"
#include "NetworkSettings.h"
#include "v8datamodel/DebugSettings.h"
#include "rbx/Profiler.h"
#include "util/ProtectedString.h"
#include "script/ScriptContext.h"
#include "v8datamodel/ContentProvider.h"
#include "boost/algorithm/string/find.hpp"
#include "script/script.h"
#include "script/ExitHandlers.h"

#include "RobloxPluginHost.h"
#include "util/HttpAsync.h"
#include "v8datamodel/FastLogSettings.h"

#include "RobloxServicesTools.h"
#include "v8datamodel/Test.h"


std::string executableLocation;

bool scriptActive;

RobloxPluginHost                m_pPluginHost;

enum argumentIndex
{
	UNKNOWN,
	FILELOCATION
};

bool loadFromStream(std::istream* stream, shared_ptr<RBX::DataModel> dataModel)
{
	//deserializing stream into datamodel
	if (!stream)
		return false;
	
	Serializer serializer;
	serializer.load(*stream, dataModel.get());

	dataModel->processAfterLoad();

	return true;
}

bool handleFileOpen(const std::string& fileName, shared_ptr<RBX::DataModel> dataModel)//, const std::string& script)
{
	//deserializing datamodel from file at location fileName
	std::ifstream stream(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!stream)
		return false;
	loadFromStream(&stream, dataModel);

	return true;
}

void onMessageOut(const RBX::StandardOutMessage& message)
{
	std::cout << message.message.c_str() << std::endl;
}

void onScriptExecutionFinish()
{
	scriptActive = false;
}

//We should send analytics up for execution failed
void onScriptExecutionFailed()
{
	scriptActive = false;
}

void runTestService(shared_ptr<RBX::DataModel> dataModel)
{
	{
		scriptActive = true;
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
		RBX::TestService* testService = RBX::ServiceProvider::find<RBX::TestService>(dataModel.get());

		if (!testService)
			return;

		testService->timeout = 60;
		testService->run(boost::bind(&onScriptExecutionFinish), boost::bind(&onScriptExecutionFailed));
	}
	
	while (scriptActive)
		Sleep(1000);
}


void fetchFlags()
{
	RBX::HttpFuture settingsFuture = FetchClientSettingsDataAsync(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY);
	try
	{
		LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, settingsFuture.get(), &RBX::ClientAppSettings::singleton());
	}
	catch (std::exception&)
	{
		std::cout << "Failed to load ClientAppSettings" << std::endl;
	}
}

void globalInit()
{
	m_pPluginHost = RobloxPluginHost(executableLocation);
	
	RBX::StandardOut::singleton()->messageOut.connect(&onMessageOut);

	SetBaseURL("www.roblox.com");

	RBX::GameBasicSettings::singleton();
	RBX::NetworkSettings::singleton();
	RBX::DebugSettings::singleton();
	
	RBX::GameBasicSettings::singleton().setStudioMode(true);
	
	RBX::Profiler::onThreadCreate("Main");

	RBX::TaskScheduler::singleton().setThreadCount(RBX::TaskSchedulerSettings::singleton().getThreadPoolConfig());
	
	std::string contentPath(executableLocation + "content");

	RBX::ContentProvider::setAssetFolder(contentPath.c_str());

	RBX::Http::init(RBX::Http::WinInet, RBX::Http::CookieSharingMultipleProcessesWrite | RBX::Http::CookieSharingMultipleProcessesRead);
}

int main(int argc, char *argv[])
{
	std::cout << "Model Analyzer" << std::endl;

	argumentIndex currentArgument = UNKNOWN;

	std::string fileLocation;

	bool helpRequested = false;

	executableLocation = argv[0];
	boost::replace_last(executableLocation, "RobloxModelAnalyzer.exe", "");

	for (int i = 1; i < argc; ++i)
	{
		std::string argum = argv[i];

		if (currentArgument != UNKNOWN)
		{
			if (currentArgument == FILELOCATION)
				fileLocation = argv[i];
			currentArgument = UNKNOWN;
		}
		else if (argum.compare("--fileLocation") == 0)
		{
			currentArgument = FILELOCATION;
		}
		else if (argum.compare("--help") == 0)
		{
			helpRequested = true;
		}
	}

	if (helpRequested)
	{
		std::cout << "----HELP----" << std::endl;
		std::cout << "--fileLocation [fileLocation]" << std::endl;

		return 0;
	}

	if (fileLocation.empty())
	{
		std::cout << "Inadequate command line arguments." << std::endl;
		std::cout << "Pass --help for more information" << std::endl;
		return 1;
	}

	globalInit();

	RBX::Security::Impersonator impersonate(RBX::Security::Anonymous);

	while (true)
	{
		fetchFlags();

		shared_ptr<RBX::Game> game;

		game.reset(new RBX::UnsecuredStudioGame(NULL,"Test", true));
		{
			RBX::Security::Impersonator impersonateForFileOpen(RBX::Security::COM);
			RBX::DataModel::LegacyLock lock(game->getDataModel(), RBX::DataModelJob::Write);
			if (!handleFileOpen(fileLocation, game->getDataModel()))
			{
				std::cout << "Failed to open file " << fileLocation << std::endl;
				return 1;
			}
		}

		runTestService(game->getDataModel());

		game->shutdown();
		game.reset();
	}
	

	return 0;
}