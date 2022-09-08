#include "stdafx.h"
#include "GameConfigurer.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/CookiesEngineService.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/ChangeHistory.h"
#include "v8datamodel/InsertService.h"
#include "v8datamodel/SocialService.h"
#include "v8datamodel/GamePassService.h"
#include "v8datamodel/MarketplaceService.h"
#include "v8datamodel/TimerService.h"
#include "v8datamodel/Visit.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/Value.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/HttpService.h"
#include "V8DataModel/Folder.h"
#include "V8DataModel/ScreenGui.h"
#include "V8Datamodel/ContentProvider.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/TeleportService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "util/Statistics.h"
#include "RobloxServicesTools.h"
#include "script/Script.h"
#include "script/ScriptContext.h"
#include "script/ModuleScript.h"
#include "script/CoreScript.h"
#include "v8xml/WebParser.h"
#include "util/ScriptInformationProvider.h"
#include "util/RobloxGoogleAnalytics.h"
#include "Client.h"
#include "ClientReplicator.h"
#include "Marker.h"
#include "GamePerfMonitor.h"
#include "Network/Players.h"
#include "util/rbxrandom.h"
#include "../win/sharedlauncher.h"
#include <vector>

#include "StringConv.h"

#include "script/LuaVM.h"

#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"
#include "XStudioBuild.h"

FASTSTRINGVARIABLE(SocialServiceFriendUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=IsFriendsWith&playerid=%d&userid=%d")
FASTSTRINGVARIABLE(SocialServiceBestFriendUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=IsBestFriendsWith&playerid=%d&userid=%d")
FASTSTRINGVARIABLE(SocialServiceGroupUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=IsInGroup&playerid=%d&groupid=%d")
FASTSTRINGVARIABLE(SocialServiceGroupRankUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=GetGroupRank&playerid=%d&groupid=%d")
FASTSTRINGVARIABLE(SocialServiceGroupRoleUrl, "Game/LuaWebService/HandleSocialRequest.ashx?method=GetGroupRole&playerid=%d&groupid=%d")
FASTSTRINGVARIABLE(GamePassServicePlayerHasPassUrl, "Game/GamePass/GamePassHandler.ashx?Action=HasPass&UserID=%d&PassID=%d")
FASTSTRINGVARIABLE(MobileJoinRateFormatUrl, "Game/JoinRate.ashx?st=%d&i=%d&p=%d&c=%s&r=%s&d=%d&b=%d&platform=%s")
FASTFLAG(UseBuildGenericGameUrl)

DYNAMIC_FASTINTVARIABLE(JoinInfluxHundredthsPercentage, 0)

FASTFLAGVARIABLE(ClientABTestingEnabled, true)

DYNAMIC_FASTFLAG(UseR15Character)

using namespace RBX;

void GameConfigurer::parseArgs(const std::string& args)
{
	parameters = rbx::make_shared<const Reflection::ValueTable>();
	WebParser::parseJSONTable(args, parameters);
}

int GameConfigurer::getParamInt(const std::string& key)
{
	RBX::Reflection::ValueTable::const_iterator iter = parameters->find(key);
	if (iter != parameters->end())
		return iter->second.get<int>();

	return 0;
}

std::string GameConfigurer::getParamString(const std::string& key)
{
	RBX::Reflection::ValueTable::const_iterator iter = parameters->find(key);
	if (iter != parameters->end())
		return iter->second.get<std::string>();

	return std::string();
}

bool GameConfigurer::getParamBool(const std::string& key)
{
	RBX::Reflection::ValueTable::const_iterator iter = parameters->find(key);
	if (iter != parameters->end())
		return iter->second.get<bool>();

	return false;
}

void GameConfigurer::registerPlay(const std::string& key, int userId, int placeId)
{
	if (!getParamBool("CookieStoreEnabled"))
		return;

	if (CookiesService* cs = dataModel->create<CookiesService>())
	{
		if (cs->GetValue(key).empty())
		{
			std::string value = RBX::format("{{ \"userId\" : %d, \"placeId\" : %d, \"os\" : \"%s\" }}", userId, placeId, DebugSettings::singleton().osPlatform().c_str());
			cs->SetValue(key, value);
		}
	}
}

void GameConfigurer::setupUrls()
{
	std::string baseUrl = getParamString("BaseUrl");
	dataModel->create<InsertService>();
    
	if (FFlag::UseBuildGenericGameUrl)
	{
	SocialService* socialService = dataModel->create<SocialService>();
	socialService->setFriendUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceFriendUrl));
	socialService->setBestFriendUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceBestFriendUrl));
	socialService->setGroupUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceGroupUrl));
	socialService->setGroupRankUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceGroupRankUrl));
	socialService->setGroupRoleUrl(BuildGenericGameUrl(baseUrl, FString::SocialServiceGroupRoleUrl));
	dataModel->create<GamePassService>()->setPlayerHasPassUrl(BuildGenericGameUrl(baseUrl, FString::GamePassServicePlayerHasPassUrl));
}
	else
	{
		SocialService* socialService = dataModel->create<SocialService>();
		socialService->setFriendUrl(baseUrl + FString::SocialServiceFriendUrl);
		socialService->setBestFriendUrl(baseUrl + FString::SocialServiceBestFriendUrl);
		socialService->setGroupUrl(baseUrl + FString::SocialServiceGroupUrl);
		socialService->setGroupRankUrl(baseUrl + FString::SocialServiceGroupRankUrl);
		socialService->setGroupRoleUrl(baseUrl + FString::SocialServiceGroupRoleUrl);
		dataModel->create<GamePassService>()->setPlayerHasPassUrl(baseUrl + FString::GamePassServicePlayerHasPassUrl);
	}
}

PlayerConfigurer::PlayerConfigurer() :
	testing(true),
	logAnalytics(false),
	isTouchDevice(false),
	connectResolved(false),
	connectionFailed(false),
	loadResolved(false),
	joinResolved(false),
	playResolved(true),
	waitingForCharacter(false),
	startTime(0),
	playStartTime(0),
	launchMode(-1)
{
}

PlayerConfigurer::~PlayerConfigurer()
{
	// make sure analytics are reported
	analyticsPoints.report("ClientJoin", DFInt::JoinInfluxHundredthsPercentage);
    
    for (auto& c: connections)
        c.disconnect();
}

void PlayerConfigurer::ifSeleniumThenSetCookie(const std::string& key, const std::string& value)
{
	if (getParamBool("SeleniumTestMode"))
	{
		if (CookiesService* cs = dataModel->create<CookiesService>())
		{
			cs->SetValue(key, value);
		}
	}
}

void PlayerConfigurer::showErrorWindow(const std::string& message, const std::string& errorType, const std::string& errorCategory)
{
	if (!loadResolved || !joinResolved)
	{
		double duration = G3D::System::time() - startTime;

		if (!loadResolved)
		{
			loadResolved = true;
			reportDuration("GameLoad", "Failure", duration, false);
		}

		if (!joinResolved)
		{
			joinResolved = true;
			reportDuration("GameJoin", errorCategory, duration, false);
		}

		// log error to GA
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "JoinFailurePlace", RBX::format("%s_%d", errorType.c_str(), getParamInt("PlaceId")).c_str());
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "JoinFailureIP", RBX::format("%s_%s", errorType.c_str(), getParamString("MachineAddress").c_str()).c_str());
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "JoinFailureVendor", RBX::format("%s_%d", errorType.c_str(), getParamInt("VendorId")).c_str());
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "JoinFailureDataCenter", RBX::format("%s_%d", errorType.c_str(), getParamInt("DataCenterId")).c_str());
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "GameJoin", "Failure");

		try
		{
			Analytics::EphemeralCounter::reportCounter("JoinFailure-"+errorType, 1);
		}
		catch (RBX::base_exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "http failed in showErrorWindow: %s", e.what());	
		}
	}
	else if (!playResolved)
	{
		double duration = G3D::System::time() - playStartTime;
		playResolved = true;
		reportDuration("GameDuration", errorCategory, duration, false);

		try
		{
			Analytics::EphemeralCounter::reportCounter("GameDisconnect-"+errorType, 1);
		}
		catch (RBX::base_exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "http failed in showErrorWindow: %s", e.what());	
		}
	}

	if (GuiService*gs = dataModel->create<GuiService>())
	{
		if (errorType != "Kick" || gs->getErrorMessage() == "")
			gs->setErrorMessage(message);
	}

	dataModel->setUiMessage(message);
}

static void ignoreResponse(std::string*, std::exception*)
{
}

void PlayerConfigurer::reportError(const std::string& error, const std::string& msg)
{
	StandardOut::singleton()->printf(MESSAGE_INFO, "***ERROR*** %s %s", error.c_str(), msg.c_str());
	if (!testing)
	{
		RBX::Visit* visit = dataModel->create<RBX::Visit>();
		visit->setUploadUrl(""); 
	}

	Network::Client* client = dataModel->create<Network::Client>();
	client->disconnect();

	if (TimerService* timer = dataModel->create<TimerService>()) {
		std::string errorMsg = RBX::format("Error: %s", error.c_str());
		timer->delay(boost::bind(&PlayerConfigurer::showErrorWindow, this, errorMsg, msg, "Other"), 4.0);
	}
}

void PlayerConfigurer::reportCounter(const std::string& counterNamesCSV, bool blocking)
{
	try 
	{
		Analytics::EphemeralCounter::reportCountersCSV(counterNamesCSV, blocking);
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_INFO, "reportCounter failed: %s", e.what());
	}
	
}

void PlayerConfigurer::reportStats(const std::string& category, float value)
{
	Analytics::EphemeralCounter::reportStats(category, value, false);
}

void PlayerConfigurer::reportDuration(const std::string& category, const std::string& result, double duration, bool blocking)
{
	int durationInMsec = G3D::iFloor(duration * 1000);
	int bytesReceived = -1;
		
	Instance* client = dataModel->create<Network::Client>();
	if (client->numChildren() > 0)
	{
		if (Network::Replicator* replicator = Instance::fastDynamicCast<Network::Replicator>(client->getChild(0)))
			bytesReceived = replicator->getMetricValue("Total Bytes Received");
	}

	try
	{
        // need to keep this until JoinRate.ashx no longer needs to track iOS join success
        // web uses this data for mobile sort to remove games from the sort that don't work on the specific device
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
        
        std::string url;
        if (FFlag::UseBuildGenericGameUrl)
        {
            url = RBX::format(BuildGenericGameUrl(getParamString("BaseUrl"),FString::MobileJoinRateFormatUrl).c_str(),
                getParamInt("VendorId"),
                getParamInt("UserId"),
                getParamInt("PlaceId"),
                category.c_str(),
                result.c_str(),
                durationInMsec,
                bytesReceived,
                DebugSettings::singleton().osPlatform().c_str());
        }
        else
        {
            url = RBX::format("%sGame/JoinRate.ashx?st=%d&i=%d&p=%d&c=%s&r=%s&d=%d&b=%d&platform=%s",
                getParamString("BaseUrl").c_str(),
                getParamInt("VendorId"),
                getParamInt("UserId"),
                getParamInt("PlaceId"),
                category.c_str(),
                result.c_str(),
                durationInMsec,
                bytesReceived,
                DebugSettings::singleton().osPlatform().c_str());
        }

        Http http(url);
        http.get(ignoreResponse);
#endif
		// new endpoint
		reportStats(category + "_" + result + "_Duration", durationInMsec);
		reportStats(category + "_" + result + "_BytesReceived", bytesReceived);

		if (DFInt::JoinInfluxHundredthsPercentage > 0)
		{
			RBX::Analytics::InfluxDb::Points durationAnalyticsPoint;
			durationAnalyticsPoint.addPoint("Category", category.c_str());
			durationAnalyticsPoint.addPoint("Result", result.c_str());
			durationAnalyticsPoint.addPoint("Duration", duration);
			durationAnalyticsPoint.report("GameDuration", DFInt::JoinInfluxHundredthsPercentage);
		}

		std::string dataCenterId = RBX::format("%d", getParamInt("DataCenterId"));

		// for kick rate by data center
		if (category == "GameDuration" && result == "Kick")
			reportStats("KickByDatacenter_" + dataCenterId, durationInMsec);

		// game join and game load by data center
		if ((category == "GameJoin" || category == "GameLoad") && result == "Success")
		{
			reportStats(category + "ByDatacenter_" + dataCenterId, durationInMsec);
			analyticsPoints.addPoint(category+"BytesReceived", bytesReceived);
		}
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_INFO, "reportDuration failed: %s", e.what());
    }
}

void PlayerConfigurer::requestCharacter(shared_ptr<Network::Replicator> replicator, shared_ptr<bool> isWaiting)
{
	(*isWaiting) = false;
	
	dataModel->setUiMessage("Requesting character");

	if (!loadResolved)
	{
		loadResolved = true;
		double duration = G3D::System::time() - startTime;
		reportDuration("GameLoad", "Success", duration, false);
		analyticsPoints.addPoint("GameLoad", RBX::Time::nowFastSec());
	}

	try
	{
		replicator->requestCharacter();
		dataModel->setUiMessage("Waiting for character");
		waitingForCharacter = true;
	}
	catch (RBX::base_exception& e)
	{
		reportError(e.what(), "W4C");
	}
}

void periodicallyZoomExtents(weak_ptr<DataModel> dmWeak, shared_ptr<bool> isWaiting)
{
    shared_ptr<DataModel> dm = dmWeak.lock();

	if (dm && *isWaiting)
	{
        dm->getWorkspace()->zoomToExtents();

        dm->create<TimerService>()->delay(boost::bind(&periodicallyZoomExtents, dmWeak, isWaiting), 0.5);
	}
}

void PlayerConfigurer::onGameClose()
{
	double duration = G3D::System::time() - startTime;

	if (!connectResolved)	
    {
		reportDuration("GameConnect", "Failure", duration, true);
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "GameJoin", "Failure");
    }
	else if (!loadResolved || !joinResolved)
	{
		if (!loadResolved)
		{
			loadResolved = true;
			reportDuration("GameLoad", "Cancel", duration, true);
		}

		if (!joinResolved)
		{
			joinResolved = true;
			reportDuration("GameJoin", "Cancel", duration, true);
		}

		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "GameJoin", "Cancel");
	}
	else if (!playResolved)
	{
		duration = G3D::System::time() - playStartTime;
		playResolved = true;
		reportDuration("GameDuration", "Success", duration, true);
		reportDuration("GameDuration_"+DebugSettings::singleton().osPlatform(), "Success", duration, true);
		Stats::reportGameStatus("LeftGame");
	}

}

void PlayerConfigurer::onDisconnection(const std::string& peer, bool lostConnection)
{
	if (peer.length() > 0)
	{
		if (lostConnection)
		{
			if (!connectionFailed)
				showErrorWindow("You have lost the connection to the game", "LostConnection", "LostConnection");
		}
		else
		{
			if (!connectionFailed)
			{
				showErrorWindow("This game has shut down", "Kick", "Kick");
				if (dataModel)
				{
					Network::Players *players = dataModel->find<Network::Players>();
					if (players)
					{
						Network::Player *p = players->getLocalPlayer();
						if (p) 
						{
							Instance *ps = p->findFirstChildByName("PlayerScripts");
							if (ps)
								ps->destroy();
						}
					}
				}
			}
		}
	}

	try 
	{
		std::string url = RBX::format("%s&disconnect=true", getParamString("PingUrl").c_str());
#if defined(RBX_PLATFORM_DURANGO)
        HttpAsync::get(url);
#else
		dataModel->httpGet(url, true);
#endif
	}
	catch (RBX::base_exception&)
	{
		// don't care
	}
}

void PlayerConfigurer::onConnectionAccepted(std::string url, shared_ptr<Instance> replicator)
{
	connectResolved = true;
	reportDuration("GameConnect", "Success", G3D::System::time() - startTime, false);
	analyticsPoints.addPoint("Connect", RBX::Time::nowFastSec());

	shared_ptr<bool> waitingForMarker(new bool(true));

	try 
	{
		if (!testing)
		{
			RBX::Visit* visit = dataModel->create<RBX::Visit>();
			visit->setPing(getParamString("PingUrl"), getParamInt("PingInterval"));
		}

		if (!getParamBool("GenerateTeleportJoin"))
			dataModel->setUiMessageBrickCount();
		else
			dataModel->setUiMessage("Teleporting...");

		boost::shared_ptr<Network::ClientReplicator> rep = Instance::fastSharedDynamicCast<Network::ClientReplicator>(replicator);

		connections.push_back(rep->disconnectionSignal.connect(boost::bind(&PlayerConfigurer::onDisconnection, this, _1, _2)));
		connections.push_back(rep->receivedGlobalsSignal.connect(boost::bind(&PlayerConfigurer::onReceivedGlobals, this)));

		connections.push_back(rep->gameLoadedSignal.connect(boost::bind(&PlayerConfigurer::onGameLoaded, this, waitingForMarker)));
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "onConnectionAccepted failed: %s", e.what());
		reportError(e.what(), "ConnectionAccepted");
	}

    dataModel->create<TimerService>()->delay(boost::bind(&periodicallyZoomExtents, weak_from(dataModel), waitingForMarker), 0);
}

void PlayerConfigurer::onConnectionFailed(const std::string& remoteAddress, int errorCode, const std::string& errorMsg)
{
	connectionFailed = true;
	std::string msg = RBX::format("Failed to connect to the Game. (ID = %d: %s)", errorCode, errorMsg.c_str());
	std::string errorType = RBX::format("ID%d", errorCode);
	showErrorWindow(msg, errorType, "Other");
}

void PlayerConfigurer::onConnectionRejected()
{
	showErrorWindow("This game is not available. Please try another", "WrongVersion", "WrongVersion");
}

void PlayerConfigurer::onReceivedGlobals()
{
	analyticsPoints.addPoint("ReceivedGlobals", RBX::Time::nowFastSec());
}

void PlayerConfigurer::onGameLoaded(boost::shared_ptr<bool> isWaiting)
{
	*isWaiting = false;

	if (!loadResolved)
	{
		loadResolved = true;
		double duration = G3D::System::time() - startTime;
		reportDuration("GameLoad", "Success", duration, false);
		analyticsPoints.addPoint("GameLoad", RBX::Time::nowFastSec());
	}

	try
	{
		dataModel->setUiMessage("Waiting for character");
		waitingForCharacter = true;
	}
	catch (RBX::base_exception& e)
	{
		reportError(e.what(), "W4C");
	}
}

void PlayerConfigurer::onPlayerIdled(double time)
{
	if (time > 1200)
	{
		showErrorWindow(RBX::format("You were disconnected for being idle %d minutes", (int)(time/60)), "Idle", "Idle");
			
		if (Network::Client* client = dataModel->find<Network::Client>())
			client->disconnect();
	}
}

void PlayerConfigurer::setMessage(const std::string& msg)
{
	if (!getParamBool("GenerateTeleportJoin"))
		dataModel->setUiMessage(msg);
	else
		dataModel->setUiMessage("Teleporting...");
}

void PlayerConfigurer::configure(RBX::Security::Identities identity, DataModel* dm, const std::string& args, int lm, const char* vrDevice)
{
	startTime = G3D::System::time();
	analyticsPoints.addPoint("Start", RBX::Time::nowFastSec());

	dataModel = dm;
	launchMode = lm;

    // Client-side A/B testing setup
    // urls to try:
    // https://api.gametest1.robloxlabs.com/users/get-experiment-enrollments  
    // https://api.gametest1.robloxlabs.com/users/get-studio-experiment-enrollments

    std::string baseUrl = RBX::ContentProvider::getUnsecureApiBaseUrl(GetBaseURL());

    // begin fetching now
    RBX::HttpFuture abTest1, abTest2;
    if (FFlag::ClientABTestingEnabled)
    {
        abTest1 = FetchABTestDataAsync(baseUrl + "users/get-experiment-enrollments");
        abTest2 = FetchABTestDataAsync(baseUrl + "users/get-studio-experiment-enrollments");
    }

	RBX::Security::Impersonator impersonate(identity);

	parseArgs(args);

	Http::gameID = getParamString("GameId");
	Analytics::setUserId(getParamInt("UserId"));
	TeleportService::SetBrowserTrackerId(getParamString("BrowserTrackerId"));

	reportCounter("GameJoinStart,GameJoinStart"+DebugSettings::singleton().osPlatform(), false);
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "GameJoin", "Start");
	Stats::reportGameStatus("JoiningGame");

	testing = (getParamString("ClientTicket").length() == 0);
	logAnalytics = (rand() % 100 == 1);

	if (DFFlag::UseR15Character)
		dataModel->create<Network::Client>();

	dataModel->setPlaceID(getParamInt("PlaceId"), getParamBool("IsRobloxPlace"));
	int universeId = getParamInt("UniverseId");
	dataModel->setUniverseId(universeId);
	dataModel->create<HttpService>();
	isTouchDevice = dm->create<UserInputService>()->getTouchEnabled();

	StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "! Joining game '%s' place %d at %s", getParamString("GameId").c_str(), getParamInt("PlaceId"), getParamString("MachineAddress").c_str());

	connections.push_back(dataModel->closingSignal.connect(boost::bind(&PlayerConfigurer::onGameClose, this)));

	dataModel->create<ChangeHistoryService>()->setEnabled(false);
	dataModel->create<ContentProvider>()->setThreadPool(16);

	setupUrls();

	DataModel::CreatorType creatorType;
	Reflection::EnumDesc<DataModel::CreatorType>::singleton().convertToValue(getParamString("CreatorTypeEnum").c_str(), creatorType);
	dataModel->setCreatorID(getParamInt("CreatorId"), creatorType);

	Network::Players* players = dataModel->create<Network::Players>();
	Network::Players::ChatOption chatOption;
	Reflection::EnumDesc<Network::Players::ChatOption>::singleton().convertToValue(getParamString("ChatStyle").c_str(), chatOption);
	players->setChatOption(chatOption);

	if (!DFFlag::UseR15Character)
		dataModel->create<Network::Client>();
	dataModel->create<Visit>();

	ifSeleniumThenSetCookie("SeleniumTest1", "Started join script");

	try
	{
		setMessage("Connecting to Server");

		Network::Client* client = dm->create<Network::Client>();

		connections.push_back(client->connectionAcceptedSignal.connect(boost::bind(&PlayerConfigurer::onConnectionAccepted, this, _1, _2)));
		connections.push_back(client->connectionRejectedSignal.connect(boost::bind(&PlayerConfigurer::onConnectionRejected, this)));
		connections.push_back(client->connectionFailedSignal.connect(boost::bind(&PlayerConfigurer::onConnectionFailed, this, _1, _2, _3)));

		client->setTicket(getParamString("ClientTicket"));

		ifSeleniumThenSetCookie("SeleniumTest2", "Successfully connected to server");

		client->setGameSessionID(getParamString("SessionId"));

		shared_ptr<Network::Player> player = 
			Instance::fastSharedDynamicCast<Network::Player>(client->playerConnect(getParamInt("UserId"), getParamString("MachineAddress"), getParamInt("ServerPort"), getParamInt("ClientPort"), -1));

		if (vrDevice)
			Network::Player::prop_VRDevice.setValue(player.get(), vrDevice);

		// prepare callback for when the Character appears
		playerChangedConnection = player->propertyChangedSignal.connect(boost::bind(&PlayerConfigurer::onPlayerChanged, this, _1));

		registerPlay(getParamString("CookieStoreFirstTimePlayKey"), getParamInt("UserId"), getParamInt("PlaceId"));
		if (TimerService* timer = dataModel->create<TimerService>()) {
			timer->delay(boost::bind(&PlayerConfigurer::registerPlay, this, getParamString("CookieStoreFiveMinutePlayKey"), getParamInt("UserId"), getParamInt("PlaceId")), 60*5.0);
		}

		player->setSuperSafeChat(getParamBool("SuperSafeChat"));
		player->setUnder13(getParamBool("IsUnknownOrUnder13"));
		Network::Player::MembershipType membershipType;
		Reflection::EnumDesc<Network::Player::MembershipType>::singleton().convertToValue(getParamString("MembershipType").c_str(), membershipType);
		player->setMembershipType(membershipType);
		player->setAccountAge(getParamInt("AccountAge"));

		connections.push_back(player->idledSignal.connect(boost::bind(&PlayerConfigurer::onPlayerIdled, this, _1)));
		
		try
		{
			player->setName(getParamString("UserName"));
		}
		catch (RBX::base_exception&)
		{
			// don't care, happens when called from studio cmd bar
		}
		
		player->setCharacterAppearance(getParamString("CharacterAppearance"));
		player->setFollowUserId(getParamInt("FollowUserId"));
		
		if (!testing)
		{
			RBX::Visit* visit = dataModel->create<RBX::Visit>();
			visit->setUploadUrl(""); 
		}
	}
	catch (RBX::base_exception& e)
	{
		reportError(e.what(), "CreatePlayer");
	}

	ifSeleniumThenSetCookie("SeleniumTest3", "Successfully created player");

	if (!testing)
	{
		gamePerfMonitor.reset(new GamePerfMonitor(getParamString("BaseUrl"), getParamString("GameId"), getParamInt("PlaceId"), getParamInt("UserId")));
		gamePerfMonitor->start(dataModel);
	}

	
	dataModel->setScreenshotSEOInfo(getParamString("ScreenShotInfo"));
	dataModel->setVideoSEOInfo(getParamString("VideoInfo"));

	ifSeleniumThenSetCookie("SeleniumTest4", "Finished join");

    if (FFlag::ClientABTestingEnabled)
    {
        try 
        {
            LoadABTestFromString(abTest1.get());
        } 
        catch(const std::exception& e1)
        {
            try 
            {
                LoadABTestFromString(abTest2.get());
            } 
            catch(const std::exception& e2)
            {
                FASTLOG2(FLog::Error, "Failed to load AB test data from both URLS: [%s] [%s]", e1.what(), e2.what());
            }
        }
    }
}

void PlayerConfigurer::onPlayerChanged(const Reflection::PropertyDescriptor* propertyDescriptor)
{
	if (propertyDescriptor->name == "Character")
	{
		dataModel->clearUiMessage();
		waitingForCharacter = false;

		playerChangedConnection.disconnect();

		if (!joinResolved)
		{
			joinResolved = true;
			reportDuration("GameJoin", "Success", G3D::System::time() - startTime, false);
			analyticsPoints.addPoint("GameJoin", RBX::Time::nowFastSec());

			std::string successCounterByOsAndLaunchType = "GameJoinSuccess_" + DebugSettings::singleton().osPlatform();
			if (launchMode == SharedLauncher::Play)
			{
				successCounterByOsAndLaunchType += "_Plugin";
				analyticsPoints.addPoint("Mode", "Play");
			}
			else if (launchMode == SharedLauncher::Play_Protocol)
			{
				successCounterByOsAndLaunchType += "_Protocol";
				analyticsPoints.addPoint("Mode", "ProtocolPlay");
			}

			reportCounter("GameJoinSuccess,GameJoin"+DebugSettings::singleton().osPlatform()+","+successCounterByOsAndLaunchType, false);
			Stats::reportGameStatus("InGame");

			if (Network::Players* players = dataModel->find<Network::Players>())
				analyticsPoints.addPoint("CharacterAutoSpawn", players->getCharacterAutoSpawnProperty());
			else
				analyticsPoints.addPoint("CharacterAutoSpawn", false);

			MegaClusterInstance *megaCluster = Instance::fastDynamicCast<MegaClusterInstance>(dataModel->getWorkspace()->getTerrain());
            bool hasTerrain = megaCluster && megaCluster->isAllocated();
			analyticsPoints.addPoint("HasTerrain", hasTerrain);
			analyticsPoints.addPoint("StreamingEnabled", dataModel->getWorkspace()->getNetworkStreamingEnabled());

			if (TeleportService::getPreviousPlaceId() > 0)
				analyticsPoints.addPoint("Mode", "Teleport", true);

			// send to influxdb
			analyticsPoints.report("ClientJoin", DFInt::JoinInfluxHundredthsPercentage);

			playStartTime = G3D::System::time();
			playResolved = false;
		}

		if (gamePerfMonitor)
		{
			// delay collecting network stats by 2 mins after character is resolved
			if (TimerService* timer = dataModel->create<TimerService>()) {
				timer->delay(boost::bind(&GamePerfMonitor::setPostDiagStats, gamePerfMonitor, true), 2*60);
			}
		}
	}
}

bool isHidden(const boost::filesystem::path &p)
{
	boost::filesystem::path name = p.filename();
	if(name != ".." &&
		name != "."  &&
		name.c_str()[0] == '.')
	{
		return true;
	}

	return false;
}


bool StudioConfigurer::findModulesAndLoad(const std::string& baseModulePath, const boost::filesystem::path& dir_path, boost::unordered_map<std::string, ProtectedString>& coreModules)
{
	if ( !boost::filesystem::exists( dir_path ) ) 
		return false;

	boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
	for ( boost::filesystem::directory_iterator itr( dir_path ); itr != end_itr; ++itr )
	{
		if ( is_directory(itr->status()) && !isHidden(itr->path()) )
		{
			if ( findModulesAndLoad(baseModulePath, itr->path(), coreModules) )
				return true;
		}

		else if ( is_regular_file(itr->status()) )
		{ 
			boost::filesystem::path currentPath = itr->path();
			std::string filePath = currentPath.string();
			
            if (filePath.find(".lua") != std::string::npos)
            {
                size_t pathLocation = filePath.find(baseModulePath);
                if (pathLocation != std::string::npos)
                {
                    filePath = filePath.substr(pathLocation + baseModulePath.length() + 1);
                    filePath = filePath.substr(0,filePath.length() - 4);
                }

                //load module
                if (!filePath.empty())
                {
                    ProtectedString source = RBX::CoreScript::fetchSource("/Modules/" + filePath).get();
                    if (!source.empty())
                    {
                        coreModules[filePath] = source;
                    }
                }
            }
		}
	}
	return false;
}

void StudioConfigurer::loadCoreModules()
{	
	RBX::CoreGuiService* coreGuiService = RBX::ServiceProvider::find<RBX::CoreGuiService>(dataModel);

	if (!coreGuiService)
	{
		return;
	}

	shared_ptr<RBX::ScreenGui> robloxScreenGui = coreGuiService->getRobloxScreenGui();
	if (!robloxScreenGui)
	{
		return;
	}

	shared_ptr<RBX::Folder> moduleScriptFolder = RBX::Creatable<Instance>::create<RBX::Folder>();
	moduleScriptFolder->setName("Modules");
	moduleScriptFolder->setRobloxLocked(true);
	moduleScriptFolder->setParent(RBX::Instance::fastDynamicCast<RBX::Instance>(robloxScreenGui.get()));
	
	boost::unordered_map<std::string, ProtectedString> coreModules;

	if (LuaVM::canCompileScripts())
	{
		const std::string path = RBX::BaseScript::hasCoreScriptReplacements() ? RBX::BaseScript::adminScriptsPath + "/Modules" :
																				ContentProvider::assetFolder() + "scripts/Modules";

		boost::filesystem::path filePath(path);
		if ( !boost::filesystem::exists(filePath) )
		{
			return;
		}

		findModulesAndLoad(filePath.string(), filePath, coreModules);
	}
	else
	{
		boost::unordered_map<std::string, std::string> byteCodeModules = LuaVM::getBytecodeCoreModules();
		for (boost::unordered_map<std::string, std::string>::iterator iter = byteCodeModules.begin(); iter != byteCodeModules.end(); ++iter)
		{
			coreModules[RBX::rot13((*iter).first)] = ProtectedString::fromBytecode((*iter).second);
		}
	}

	for (boost::unordered_map<std::string, ProtectedString>::iterator iter = coreModules.begin(); iter != coreModules.end(); ++iter)
	{
		shared_ptr<ModuleScript> moduleScript = RBX::Creatable<Instance>::create<ModuleScript>();
		
		std::string name = (*iter).first;
		shared_ptr<RBX::Folder> lastFolder = moduleScriptFolder;

		boost::filesystem::path namePath(name);
		if (namePath.has_parent_path())
		{
			const std::string delimiter = "\\";

			while (name.find("/") != std::string::npos)
			{
				name.replace(name.find("/"), 1, delimiter);
			}

			size_t pos = 0;
			int level = 0;
			std::string token;
			while ((pos = name.find(delimiter)) != std::string::npos) 
			{
				token = name.substr(0, pos);
				name.erase(0, pos + delimiter.length());

				if (Instance* findFolder = moduleScriptFolder->findFirstChildByNameRecursive(token))
				{
					if (RBX::Folder* folderInstance = Instance::fastDynamicCast<RBX::Folder>(findFolder))
					{
						lastFolder = shared_from(folderInstance);
					}
				}
				else
				{
					shared_ptr<RBX::Folder> newFolder = RBX::Creatable<Instance>::create<RBX::Folder>();
					newFolder->setName(token);
					newFolder->setRobloxLocked(true);
					if (level == 0)
					{
						newFolder->setParent(moduleScriptFolder.get());
					}
					else
					{
						newFolder->setParent(lastFolder.get());
					}
					lastFolder = newFolder;
				}
				level++;
			}
		}

		moduleScript->setName(name);
		moduleScript->setSource((*iter).second);
		moduleScript->setRobloxLocked(true);

		moduleScript->setParent(lastFolder.get());
	}
}

void StudioConfigurer::configure(RBX::Security::Identities identity, DataModel* dm, const std::string& args, int launchMode, const char* vrDevice)
{
	dataModel = dm;
	parseArgs(args);

	std::string baseUrl = getParamString("BaseUrl");

	dataModel->create<InsertService>();

	setupUrls();

	// do not load core modules for cloud edit
	if (Network::Players::isCloudEdit(dataModel))
		return;

	if (Network::Players::frontendProcessing(dataModel))
		loadCoreModules();

#if defined(RBX_PLATFORM_DURANGO)
	if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
	{
        if(starterScript.empty()) 
            starterScript = "StarterScript";
        scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
        return;
	}
#endif

#if defined(RBX_STUDIO_BUILD) && ENABLE_XBOX_STUDIO_BUILD
    if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
    {
        starterScript = "XStarterScript";
        scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
        return;
    }
#endif


	// this will be called in case of old play solo
	if (ScriptContext* scriptContext = dataModel->create<ScriptContext>())
	{
		if (Network::Players::backendProcessing(dataModel)) 
		{
			starterScript = "ServerStarterScript";
			scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
		} 

		if (Network::Players::frontendProcessing(dataModel)) 
		{
			starterScript = "StarterScript";
			scriptContext->addCoreScriptLocal(starterScript, shared_ptr<Instance>());
		}
	}
} 





