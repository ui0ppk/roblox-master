//
//  PlaceLauncher.m
//  RobloxMobile
//
//  Created by David York on 9/25/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#include "PlaceLauncher.h"
#include "util/Statistics.h"
#include "RobloxView.h"
#include "RobloxInfo.h"
#include "JNIMain.h"
#include "RobloxUtilities.h"

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/TeleportService.h"
#include "v8datamodel/LoginService.h"
#include "V8DataModel/ContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/TextureContentProvider.h"
#include "v8datamodel/FastLogSettings.h"
#include "V8DataModel/UserInputService.h"
#include "script/ScriptContext.h"
#include "util/Statistics.h"
#include "util/standardout.h"
#include "util/MemoryStats.h"
#include "v8datamodel/Stats.h"


#include "util/RobloxGoogleAnalytics.h"
#include "util/Http.h"
#include "LogManager.h"
#include "rbx/Profiler.h"

#include <rapidjson/document.h>

DYNAMIC_LOGVARIABLE(PlaceLauncher, 0)
DYNAMIC_LOGGROUP(NetworkTrace)
LOGGROUP(PlayerShutdownLuaTimeoutSeconds)


DYNAMIC_FASTINT(AndroidInfluxHundredthsPercentage)

using namespace RBX;
using namespace RBX::JNI;

namespace
{
static const std::string kAndroidClientAppSettings = "AndroidAppSettings";
static const std::string kAndroidClientSettingsAPIKey = "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79";
static const std::string kStartGameURL = "%sGame/PlaceLauncher.ashx?request=%s&%s&isPartyLeader=false&gender=&isTeleport=false";
static const std::string kStartGameStatusURL = "%sGame/PlaceLauncher.ashx?request=CheckGameJobStatus&jobId=%s";
} // namespace

namespace RBX
{
    namespace JNI
    {
        extern std::string platformUserAgent; // Comes from LogManager
    }
}

PlaceLauncher::PlaceLauncher()
{
    teleporter.reset(new Teleporter(RBX::FunctionMarshaller::GetWindow()));
    RBX::TeleportService::SetCallback(teleporter.get());
};

PlaceLauncher& PlaceLauncher::getPlaceLauncher()
{
    static PlaceLauncher placeLauncher;
    return placeLauncher;
}

static std::string ReadStringValue(std::string &data, std::string name)
{
    std::string result;
    int pos = data.find(name);
    if (pos != -1)
    {
        int start = pos + name.length() + 3;
        int end = data.find(",", start);
        result = data.substr(start, end - start - 1);
    }

    pos = result.find("\\/"); // find first space
    while (pos != std::string::npos)
    {
        result.replace(pos, 2, "/");
        pos = result.find("\\/", pos + 1);
    }
    return result;
}

static RBX::ProtectedString fetchAndValidateScript(RBX::DataModel* dataModel, const std::string& urlScript)
{
	RBX::Security::Impersonator impersonate(RBX::Security::COM);

	std::ostringstream data;
	if (RBX::ContentProvider::isUrl(urlScript))
	{
		RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
		std::auto_ptr<std::istream> stream(RBX::ServiceProvider::create<RBX::ContentProvider>(dataModel)->getContent(RBX::ContentId(urlScript.c_str())));
		boost::iostreams::copy(*stream, data);
	}
	else
		return RBX::ProtectedString();

	RBX::ProtectedString verifiedSource;

	try
	{
		verifiedSource = RBX::ProtectedString::fromTrustedSource(data.str());
		RBX::ContentProvider::verifyScriptSignature(verifiedSource, true);
	}
	catch(std::bad_alloc& e)
	{
		throw;
	}
	catch(RBX::base_exception& e)
	{
		return RBX::ProtectedString();
	}

	return verifiedSource;
}

static void executeUrlJoinScript(boost::shared_ptr<RBX::Game> game, const std::string& urlScript)
{
	RBXASSERT(urlScript.find("join.ashx"));

	boost::shared_ptr<RBX::DataModel> dataModel = game->getDataModel();

	RBX::ProtectedString verifiedSource = fetchAndValidateScript(dataModel.get(), urlScript);
	if (verifiedSource.empty())
		return;

	RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);

	if (dataModel->isClosed())
		return;

	// new join script
	std::string dataString = verifiedSource.getSource();
	int firstNewLineIndex = dataString.find("\r\n");
	if (dataString[firstNewLineIndex+2] == '{')
	{
        RobloxUtilities::getRobloxUtilities().getGameTimer().reset();
		game->configurePlayer(RBX::Security::COM, dataString.substr(firstNewLineIndex+2));
		return;
	}

	// old join script
    RBX::ScriptContext* context = dataModel->create<RBX::ScriptContext>();
    context->executeInNewThread(RBX::Security::COM, verifiedSource, "Start Script");
}

static void joinGamePlaceId(StartGameParams sgp, shared_ptr<RBX::Game> game)
{
    RobloxUtilities::getRobloxUtilities().getGameTimer().reset();
    try
    {
        int retrys = 5;

        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"PlaceLauncher::joinGamePlaceId placeId = %i, userId = %d, accessCode = %s, gameId = %s, joinRequestType: %d\n", sgp.placeId, sgp.userId, sgp.accessCode.c_str(), sgp.gameId.c_str(), sgp.joinRequestType);

        const char* base = GetBaseURL().c_str();
        std::string requestParamsString, requestType;
        std::string formattedParams;
        switch (sgp.joinRequestType)
        {
            case JOIN_GAME_REQUEST_PLACEID:
                requestParamsString = "placeId=%d";
                formattedParams = RBX::format(requestParamsString.c_str(), sgp.placeId);
                requestType = "RequestGame";
                break;
            case JOIN_GAME_REQUEST_USERID:
                requestParamsString = "userId=%d";
                formattedParams = RBX::format(requestParamsString.c_str(), sgp.userId);
                requestType = "RequestFollowUser";
                break;
            case JOIN_GAME_REQUEST_PRIVATE_SERVER:
                requestParamsString = "placeId=%d&accessCode=%s";
                formattedParams = RBX::format(requestParamsString.c_str(), sgp.placeId, sgp.accessCode.c_str());
                requestType = "RequestPrivateGame";
                break;
            case JOIN_GAME_REQUEST_GAME_INSTANCE:
                requestParamsString = "placeId=%d&gameId=%s";
                formattedParams = RBX::format(requestParamsString.c_str(), sgp.placeId, sgp.gameId.c_str());
                requestType = "RequestGameJob";
                break;
        }

        std::string response;
        bool found = false;
        int status = -1;

        std::string url = RBX::format(kStartGameURL.c_str(), base, requestType.c_str(), formattedParams.c_str());
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"PlaceLauncher URL : %s\n", url.c_str());
        std::string jobId;
        while (retrys >= 0)
        {
            bool retryUsed = true;
            response = "";
            RBX::Http(url).get(response);

            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"PlaceLauncher Response : %s\n", response.c_str());

            rapidjson::Document doc;
            doc.Parse<rapidjson::kParseDefaultFlags>(response.c_str());

            RBXASSERT(doc.HasMember("status"));
            status = doc["status"].GetInt();

            // Place join status results
            // Waiting = 0,
            // Loading = 1,
            // Joining = 2,
            // Disabled = 3,
            // Error = 4,
            // GameEnded = 5,
            // GameFull = 6
            // UserLeft = 10
            // Restricted = 11

            if (2 == status)
            {
                // not most efficient way to check status,
                // but we have web calls so strstr is not a big deal
                found = true;
                break;
            }
            else if (0 == status || 1 == status)
            {
                // 0 or 1 is not an error - it is a sign that we should wait
                retryUsed = false;
                RBXASSERT(doc.HasMember("jobId"));
                jobId = doc["jobId"].GetString();

                // Check game status no matter what type of join we're attempting
				url = RBX::format(kStartGameStatusURL.c_str(), base, Http::urlEncode(jobId).c_str());
            }
            else
                break;

            int sleepTime = 250*1000;

            if (retryUsed)
            {
                --retrys;
                sleepTime = 999*1000;
            }

            ::usleep(sleepTime);
        }

        if (found)
        {
			executeUrlJoinScript(game, ReadStringValue(response, "joinScriptUrl"));
        }
        else
        {
            if (sgp.joinRequestType == JOIN_GAME_REQUEST_PLACEID)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlaceLauncher: Cannot connect to place %d, return from GetPlaceLauncher.ashx = %s", sgp.placeId, response.c_str());
            else if (sgp.joinRequestType == JOIN_GAME_REQUEST_USERID)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlaceLauncher: Cannot follow user %d, return from GetPlaceLauncher.ashx = %s", sgp.userId, response.c_str());
            else if (sgp.joinRequestType == JOIN_GAME_REQUEST_PRIVATE_SERVER)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlaceLauncher: Cannot join private server, accessCode = %s, return from GetPlaceLauncher.ashx = %s", sgp.accessCode.c_str(), response.c_str());
            else if (sgp.joinRequestType)
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlaceLauncher: Cannot join game instance, gameId = %s, return from GetPlaceLauncher.ashx = %s", sgp.gameId.c_str(), response.c_str());

            // Place join status results
            // Waiting = 0,
            // Loading = 1,
            // Joining = 2,
            // Disabled = 3,
            // Error = 4,
            // GameEnded = 5,
            // GameFull = 6
            // UserLeft = 10
            // Restricted = 11
            PlaceLauncher::handleStartGameFailure(status);
        }
    }
    catch (const RBX::base_exception& e)
    {
        if (sgp.joinRequestType == JOIN_GAME_REQUEST_PLACEID)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't join place %d, because %s\n", sgp.placeId, e.what());
        else if (sgp.joinRequestType == JOIN_GAME_REQUEST_USERID)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Exception thrown: Can't follow user %d, because %s\n", sgp.userId, e.what());
        else if (sgp.joinRequestType == JOIN_GAME_REQUEST_PRIVATE_SERVER)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlaceLauncher: Exception thrown: Cannot join private server, accessCode = %s, because %s", sgp.accessCode.c_str(), e.what());
        else if (sgp.joinRequestType)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "PlaceLauncher: Exception thrown: Cannot join game instance, gameId = %s, because %s", sgp.gameId.c_str(), e.what());

        //[[PlaceLauncher sharedInstance] handleStartGameFailure];
    }
}

void PlaceLauncher::handleStartGameFailure(int status = 99)
{
    // Place join status results
    // 0, 1, or 2 is handled in joinGamePlaceId above

    // Disabled = 3,
    // Error = 4,
    // GameEnded = 5,
    // GameFull = 6
    // UserLeft = 10
    // Restricted = 11

    std::string errorMsgName; // this is the name of the error message string in res/values/strings.xml
    std::string fastLogMsg; // only used for logging; not displayed to the user
    switch (status)
    {
        case 3:
            errorMsgName = "GameStartFailureDisabled";
            fastLogMsg = "PlaceLauncher failure - game was disabled.";
            break;
        case 4:
            errorMsgName = "GameStartFailureError";
            fastLogMsg = "PlaceLauncher failure - game failed to start.";
            break;
        case 5:
            errorMsgName = "GameStartFailureGameEnded";
            fastLogMsg = "PlaceLauncher failure - game has ended.";
            break;
        case 6:
            errorMsgName = "GameStartFailureGameFull";
            fastLogMsg = "PlaceLauncher failure - game is full.";
            break;
        case 10:
            errorMsgName = "GameStartFailureUserLeft";
            fastLogMsg = "PlaceLauncher failure - the user you were following has left the game.";
            break;
        case 11:
            errorMsgName = "GameStartFailureRestricted";
            fastLogMsg = "PlaceLauncher failure - game not available for this platform.";
            break;
        case 99:
        default:
            errorMsgName = "GameStartFailureUnknown";
            fastLogMsg = "PlaceLauncher failure - unknown game start failure.";
            break;
    }
    RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "in handleGameStartFailure, msg is %s", errorMsgName.c_str());
    FASTLOG1(DFLog::PlaceLauncher, "%s", fastLogMsg.c_str());

    JNI::exitGameWithError(errorMsgName); // from JNIMain
}



bool PlaceLauncher::prepareGame( const StartGameParams& sgp )
{
    StandardOut::singleton()->printf(MESSAGE_INFO, "Asset Path in Place Launch: %s", RobloxInfo::getRobloxInfo().getAssetFolderPath().c_str());
    FASTLOG(DFLog::PlaceLauncher, "PlaceLauncher prepareGame - START");
    RBX::ContentProvider::setAssetFolder(RobloxInfo::getRobloxInfo().getAssetFolderPath().c_str());

    Http::rbxUserAgent = platformUserAgent;
    RBX::Game::globalInit(false);
    RBX::TeleportService::SetBaseUrl(GetBaseURL().c_str());

    /*
    if(needsOnline)
    {
        Reachability* reachability = [Reachability reachabilityForInternetConnection];
        NetworkStatus remoteStatus = [reachability currentReachabilityStatus];

        if(remoteStatus == NotReachable)
        {
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: No Network Connection available");
            [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"ConnectionError", nil)];
            return false;
        }
        else if(remoteStatus == ReachableViaWWAN)
        {
            // if user preference is for wifi only, don't allow this
            NSUserDefaults *userDefaults =[NSUserDefaults standardUserDefaults];
            if([[userDefaults stringForKey:@"wifionly_preference"] boolValue])
            {
                [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"WiFiOnlyError", nil)];
                return false;
            }
        }
    }*/

    std::string ios = "i";
    ios += "o";
    ios += "s";
    ios = ios + "," + ios; // slight obfuscation

    RBX::DataModel::hash = ios;

    {
        RBX::Security::Impersonator impersonate(RBX::Security::RobloxGameScript_);
        RBX::GlobalBasicSettings::singleton()->loadState("");
    }

	RBX::Profiler::onThreadCreate("Main");

    RBX::TaskScheduler::singleton().setThreadCount(RBX::TaskSchedulerSettings::singleton().getThreadPoolConfig());

    FASTLOG(DFLog::PlaceLauncher, "PlaceLauncher prepareGame - END");
    return true;
}

void initClientSettings()
{
    // This needs to be put in a manager and cached
    std::string clientSettingsData;
    std::string androidAppSettingsData;
    FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &clientSettingsData);
    FetchClientSettingsData(kAndroidClientAppSettings.c_str(), kAndroidClientSettingsAPIKey.c_str(), &androidAppSettingsData);
    RBX::ClientAppSettings settings = RBX::ClientAppSettings::singleton();
    LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, clientSettingsData, &RBX::ClientAppSettings::singleton());
    LoadClientSettingsFromString(kAndroidClientAppSettings.c_str(), androidAppSettingsData, &RBX::ClientAppSettings::singleton());

    // Reset synchronized flags, they should be set by the server
    FLog::ResetSynchronizedVariablesState();
}

static void initControlView(RobloxView* rbxView, bool isTouchDevice)
{
	if(DataModel* dm = rbxView->getDataModel().get())
	{
		if(RBX::UserInputService* inputService = RBX::ServiceProvider::create<RBX::UserInputService>(dm))
		{
			inputService->setTouchEnabled(isTouchDevice);
		}
	}
}

void PlaceLauncher::finishGameSetup( boost::shared_ptr<RBX::Game> game)
{
    rbxView = RobloxView::create_view(currentGame, gameParams.view, gameParams.viewWidth, gameParams.viewHeight);
    rbxView->getDataModel()->submitTask(boost::bind(initControlView, rbxView, gameParams.isTouchDevice), RBX::DataModelJob::Write);
}


shared_ptr<RBX::Game> PlaceLauncher::setupGame( const StartGameParams& sgp )
{
    FASTLOG(DFLog::PlaceLauncher, "PlaceLauncher setUpGame");
    if(isCurrentlyPlayingGame)
            return shared_ptr<RBX::Game>();

    isCurrentlyPlayingGame = true;

    initClientSettings();
    RobloxGoogleAnalytics::lotteryInit(
            ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyIDPlayer(),
            ClientAppSettings::singleton().GetValueGoogleAnalyticsThreadPoolMaxScheduleSize(),
            ClientAppSettings::singleton().GetValueGoogleAnalyticsLoadPlayer());
    StandardOut::singleton()->printf(MESSAGE_INFO, "account = %s, schedule = %d",
            ClientAppSettings::singleton().GetValueGoogleAnalyticsAccountPropertyIDPlayer(),
            ClientAppSettings::singleton().GetValueGoogleAnalyticsThreadPoolMaxScheduleSize());

    RBX::Http::SetUseStatistics(true);

    if(prepareGame( sgp ))
    {
        FASTLOGS(DFLog::PlaceLauncher, "Game's base URL: %s", GetBaseURL().c_str());
        currentGame = shared_ptr<RBX::Game>( new RBX::SecurePlayerGame(NULL, GetBaseURL().c_str()) );

        rbxView = RobloxView::create_view(currentGame, sgp.view, sgp.viewWidth, sgp.viewHeight);

        rbxView->getDataModel()->submitTask(boost::bind(initControlView, rbxView, gameParams.isTouchDevice), RBX::DataModelJob::Write);

         if (FLog::PlayerShutdownLuaTimeoutSeconds > 0)
                currentGame->getDataModel()->create<RBX::ScriptContext>();

        return currentGame;
    }

    return shared_ptr<RBX::Game>();
}


shared_ptr<RBX::Game> PlaceLauncher::setupPreloadedGame( const StartGameParams& sgp )
{
    FASTLOG(DFLog::PlaceLauncher, "PlaceLauncher SetupPreloadedGame");
    return setupGame( sgp );
}

bool PlaceLauncher::startGame( boost::function0 <void>  scriptFunction , shared_ptr<RBX::Game> preloadedGame)
{
    // Start the script thread!
    boost::thread(RBX::thread_wrapper(scriptFunction, "GameStartScript"));
    return true;
}


bool PlaceLauncher::startGame( const StartGameParams& sgp )
{
	gameParams = sgp;
    FASTLOG(DFLog::PlaceLauncher, "PlaceLauncher StartGame");
    shared_ptr<RBX::Game> game = setupPreloadedGame( sgp );
    if( game != shared_ptr<RBX::Game>())
    {
    	currentGame = game;
    	return startGame(boost::bind(&joinGamePlaceId, sgp, game), game);
    }

    return true;
}

void PlaceLauncher::leaveGame ( bool userRequestedLeave)
{
    RBX::Time::Interval gamePlayInterval = RobloxUtilities::getRobloxUtilities().getGameTimer().reset();
    RBX::Analytics::InfluxDb::Points points;
    points.addPoint("SessionReport" , "AndroidSuccess");
    points.addPoint("FreeMemoryKB" , RBX::MemoryStats::freeMemoryBytes());
    points.addPoint("UsedMemoryKB" , RBX::MemoryStats::usedMemoryBytes());
    points.addPoint("PlayTime" , gamePlayInterval.seconds());

    points.report("Android-RobloxPlayer-SessionReport", DFInt::AndroidInfluxHundredthsPercentage);
    deleteRobloxView(true);
}


void PlaceLauncher::deleteRobloxView( bool resetCurrentGame )
{
    if(resetCurrentGame)
    {
        // remove all shared ptr to games so it can properly be destroyed
        currentGame.reset();
    }

    if( teleporter )
    {
    	teleporter.reset();
    }

    // do actual destruction of the Roblox View, only if it isn't NULL
    if (rbxView)
    {
        // Destroy the view synchronously
        // Take care to zero-out view pointer beforehand to support reentrancy
        RobloxView* view = rbxView;
        rbxView = NULL;
        delete view;
    }
}

static void joinGameTeleport(std::string url, std::string ticket, std::string script, shared_ptr<RBX::Game> game)
{
    try
    {
        // get authentication URL
        std::string compound = url;

        if (!ticket.empty())
        {
            compound += "?suggest=";
            compound += ticket;
        }

        // issue an authentication request
        std::string result;

        try
        {
            RBX::Http http(compound.c_str());
            http.setAuthDomain(GetBaseURL().c_str());
            http.get(result);
        }
        catch (const RBX::base_exception& e)
        {
        }

        // run game
        executeUrlJoinScript(game, script);

    }
    catch (const RBX::base_exception& e)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"PlaceLauncher: Teleport failed: %s\n", e.what());
    }
}

void PlaceLauncher::teleport( std::string ticket, std::string authUrl, std::string script)
{
    if (!rbxView) return;

    // first do an empty reset so serviceprovider oldProvider is called first
    currentGame.reset();
    currentGame.reset(new RBX::SecurePlayerGame(NULL, GetBaseURL().c_str()));

    rbxView->replaceGame(currentGame);

    rbxView->getDataModel()->submitTask(boost::bind(initControlView, rbxView, gameParams.isTouchDevice), RBX::DataModelJob::Write);

    // Don't use submit task on data model for this thread, the script fetched in
    // the spawned thread does data model setup.
    // joinGameTeleport will take ownership of the game.
    boost::thread joinScriptThread(boost::bind(&joinGameTeleport, authUrl, ticket, script, currentGame));
}

void PlaceLauncher::shutDownGraphics()
{
	if (!rbxView) return;
	rbxView->shutDownGraphics(currentGame);
}

void PlaceLauncher::startUpGraphics(void *wnd, unsigned int width, unsigned int height)
{
	if (!rbxView) return;
	rbxView->startUpGraphics(currentGame, wnd, width, height);
}

