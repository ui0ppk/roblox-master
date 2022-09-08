/*
 *  RobloxCpp.cpp
 *  MacClient
 *
 *  Created by Tony on 1/26/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


#include "Roblox.h"

#undef max
#undef min


#include "v8datamodel/datamodel.h"
#include "v8datamodel/workspace.h"
#include "v8datamodel/partinstance.h"

#include "v8datamodel/factoryregistration.h"
#include "Util/FileSystem.h"
#include "Util/Http.h"
#include "Util/Profiling.h"
#include "Util/Statistics.h"
#include "Util/MD5Hasher.h"
#include "util/RobloxGoogleAnalytics.h"
#include "v8datamodel/game.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/GlobalSettings.h"
#include "script/ScriptContext.h"
#include "script/LuaSettings.h"
#include "network/api.h"

#include "rbx/ProcessPerfCounter.h"
#include "rbx/Profiler.h"

#include "Gui/ProfanityFilter.h"

#include "boost/filesystem.hpp"

#include "MachineConfiguration.h"
#include <string>

FASTFLAG(GoogleAnalyticsTrackingEnabled)
DYNAMIC_LOGGROUP(GoogleAnalyticsTracking)
LOGGROUP(Network)
LOGGROUP(PlayerShutdownLuaTimeoutSeconds)

bool Roblox::initialized = false;
std::string Roblox::assetFolder;
Roblox::RunMode Roblox::runMode = Roblox::RUN_FILE;
rbx::signals::scoped_connection messageOutConnection;

RBX::ClientAppSettings Roblox::appSettings;

static boost::shared_ptr<RBX::Game> preloadedGame;
static boost::mutex preloadedGameMutex;
static bool preloadShuttingDown = false;

static boost::thread releaseGameThread;

extern "C" {
	void writeFastLogDumpHelper(const char *fileName, int numEntries)
	{
		FLog::WriteFastLogDump(fileName, numEntries);
	}	
};



static void do_preloadGame(bool isApp)
{
	// Avoid having 2 DataModels open at once
	// It should be safe, but may as well not play with fire
	releaseGameThread.join();
	
	boost::mutex::scoped_lock lock(preloadedGameMutex);
	if (preloadShuttingDown)
		return;
	if (!preloadedGame)
	{
		RBX::Time start = RBX::Time::now<RBX::Time::Fast>();
		preloadedGame.reset(new RBX::SecurePlayerGame(NULL, ::GetBaseURL().c_str()));
		RBX::Time stop = RBX::Time::now<RBX::Time::Fast>();
		double secs = (stop - start).seconds();
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Preloaded Game %gsec", secs);
	}
	else
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Already preloaded Game");
}

void Roblox::preloadGame(bool isApp)
{
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Requesting preload Game");
	boost::thread(boost::bind(&do_preloadGame, isApp));
}

extern std::string macBundlePath();

bool Roblox::globalInit(bool isApp)
{
	if (initialized)
	{
		return true;
	}
    RBX::Game::globalInit(false);
	
	std::string fileRobloxPlayer = macBundlePath() + "/Contents/MacOS/RobloxPlayer";
    
    // hash may have been overridden on command line, otherwise calculate from executable
    if (RBX::DataModel::hash.length() == 0)
    {
        RBX::DataModel::hash = RBX::CollectMd5Hash(fileRobloxPlayer);
	}

    RBX::ContentProvider::setAssetFolder(assetFolder.c_str());
	
	// Reset synchronized flags, they should be set by the server
	FLog::ResetSynchronizedVariablesState();
    
    {
        bool useCurl = rand() % 100 < RBX::ClientAppSettings::singleton().GetValueHttpUseCurlPercentageMacClient();
        FASTLOG1(FLog::Network, "Use CURL = %d", useCurl);
        RBX::Http::SetUseCurl(useCurl);
        
        RBX::Http::SetUseStatistics(true);
    }
    
	{
		int lottery = rand() % 100;
		FASTLOG1(DFLog::GoogleAnalyticsTracking, "Google analytics lottery number = %d", lottery);
		// initialize google analytics
		if (FFlag::GoogleAnalyticsTrackingEnabled && (lottery < RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsLoadPlayer()))
		{
			RBX::RobloxGoogleAnalytics::setCanUseAnalytics();
            
			RBX::RobloxGoogleAnalytics::init(RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyIDPlayer(),
                                             RBX::ClientAppSettings::singleton().GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());
		}
	}
    
    // must be after globalInit, where http pool is initialized
    RBX::postMachineConfiguration(::GetBaseURL().c_str(), 0);
	
	messageOutConnection = RBX::StandardOut::singleton()->messageOut.connect(&onMessageOut);
		
    RBX::GlobalAdvancedSettings::singleton()->loadState("");
    {
        RBX::Security::Impersonator impersonate(RBX::Security::RobloxGameScript_);
        RBX::GlobalBasicSettings::singleton()->loadState("");
    }
	
	RBX::Profiler::onThreadCreate("Main");
	
	// Initialize the TaskScheduler (after loading configs)
	RBX::TaskScheduler::singleton().setThreadCount(RBX::TaskSchedulerSettings::singleton().getThreadPoolConfig());

	initialized = true;

	preloadGame(isApp);
    
    return true;
}

boost::shared_ptr<RBX::Game> Roblox::getpreloadedGame(const bool isApp)
{
	boost::mutex::scoped_lock lock(preloadedGameMutex);
	if (!preloadedGame)
	{
		RBX::Time start = RBX::Time::now<RBX::Time::Fast>();
		preloadedGame.reset(new RBX::SecurePlayerGame(NULL, ::GetBaseURL().c_str()));
		RBX::Time stop = RBX::Time::now<RBX::Time::Fast>();
		double secs = (stop - start).seconds();
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Loaded Game %gsec", secs);
	}
	boost::shared_ptr<RBX::Game> temp = preloadedGame;
	preloadedGame.reset();
    
    if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
        temp->getDataModel()->create<RBX::ScriptContext>();
    
	return temp;
}



static void releaseGame(boost::shared_ptr<RBX::Game> game)
{
	RBX::GlobalBasicSettings::singleton()->saveState();
	game->shutdown();
}

void Roblox::relinquishGame(boost::shared_ptr<RBX::Game>& game)
{
    releaseGameThread.join();

    // Defer deletion of DataModel for later (so that we don't block)
    releaseGameThread = boost::thread(boost::bind(&releaseGame, game));
    
    game.reset();
}


void Roblox::globalShutdown()
{
	{
		boost::mutex::scoped_lock lock(preloadedGameMutex);
		preloadShuttingDown = true;
		if (preloadedGame)
			preloadedGame->shutdown();	// Don't rely on destructor, in case somebody else holds a reference
		preloadedGame.reset();
	}
	
	// Don't exit until Game has been shut down. Otherwise we might get crashes in static destructors
	releaseGameThread.join();

	if (!initialized)
		return;
	
	RBX::Game::globalExit();
	messageOutConnection.disconnect();
}


void doCrash()
{
	RBXCRASH();
}

void Roblox::testCrash()
{
	boost::shared_ptr<RBX::Instance> i = RBX::Creatable<RBX::Instance>::create<RBX::PartInstance>();
	i->propertyChangedSignal.connect(boost::bind(&doCrash));
	i->setName("blah");
	
}

void Roblox::setArgs(const char *gameFolder, const char *runMode)
{
	Roblox::assetFolder = std::string(gameFolder) + "/content/";
	
	if (strcmp(runMode,"c") == 0){
		Roblox::runMode = RUN_CLIENT;
	}
	else if (strcmp(runMode,"s") == 0){
		Roblox::runMode = RUN_SERVER;
	}
	else {
		Roblox::runMode = RUN_FILE;
	}
}

extern "C" {
	
	void setRobloxArgs(const char *gameFolder, const char *runMode)
	{
		Roblox::setArgs(gameFolder, runMode);
	}
}

// Utility functions

void Roblox::onMessageOut(const RBX::StandardOutMessage& message)
{
	switch (message.type)
	{
		case RBX::MESSAGE_INFO:
			printf("INFO: %s\n", message.message.c_str());
			break;
		case RBX::MESSAGE_WARNING:
			printf("WARNING: %s\n", message.message.c_str());
			break;
		case RBX::MESSAGE_ERROR:
			printf("ERROR: %s\n", message.message.c_str());
			break;
		default:
			printf("%s\n", message.message.c_str());
			break;
	}
}


static void handler(std::exception* ex, std::string file)
{
	if (ex)
		return;
	std::remove(file.c_str());
}

void uploadAndDeletFileAsync(const char* url, const char* file)
{
	boost::shared_ptr<std::fstream> data(new std::fstream(file, std::ios_base::in | std::ios_base::binary));
	size_t begin = data->tellg();
	data->seekg (0, std::ios::end);
	size_t end = data->tellg();
	if (end > begin)
	{
		data->seekg (0, std::ios::beg);
		
		std::string version;
		while (*data)
		{
			char buff[255];
			data->getline(buff, 155);
			std::string line = buff;
			
			if (line.substr(0, 8) == "Version:")
			{
				// "Version:         0.34.0.107 (107)"
				for (size_t i = 8; i < line.size(); ++i)
					if (line[i] != ' ')
					{
						line = line.substr(i);
						break;
					}
				
				// "0.34.0.107 (107)"
				for (size_t i = 0; i < line.size(); ++i)
					if (line[i] == ' ')
					{
						line = line.substr(0, i);
						// "0.34.0.107"
						while (true)
						{
							int j = line.find('.');
							if (j == std::string::npos)
								break;
							line = line.replace(j, 1, ",%20");
						}
						// "0,%2034,%200,%20107"
						version = line;
						break;
					}
			}
		}

		data->seekg (0, std::ios::beg);
	
		boost::filesystem::path p(file);
		std::string filename = "log_";
		// extract file "guid"
		filename += p.filename().string().substr(13, 17);
		filename += "%20";
		filename += version;
		filename += ".crash";

		std::string fullUrl = url;
		fullUrl += "?filename=" + filename;
		
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, "Uploading %s", fullUrl.c_str());	
		RBX::Http(fullUrl).post(data, RBX::Http::kContentTypeUrlEncoded, true, boost::bind(handler, _2, p.string()));
	}
}
