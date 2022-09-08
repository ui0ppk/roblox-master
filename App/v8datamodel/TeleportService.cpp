#include "stdafx.h"

#include "V8DataModel/DataModel.h"
#include "v8datamodel/TeleportService.h"
#include "v8datamodel/TeleportCallback.h"
#include "V8DataModel/GameBasicSettings.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "V8DataModel/InsertService.h"
#include "Util/http.h"
#include "util/RbxStringTable.h"

#include "VMProtect/VMProtectSDK.h"
#include "V8DataModel/Workspace.h"
#include "network/Player.h"
#include "network/Players.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "v8xml/WebParser.h"
#include "Script/Script.h"

#include "RobloxServicesTools.h"

DYNAMIC_FASTINTVARIABLE(TeleportRetryTimes, 5)
DYNAMIC_FASTFLAGVARIABLE(UserCameraZoomPersistThroughTeleport, false)
DYNAMIC_FASTFLAGVARIABLE(UserMouseLockSettingSaveTeleport, false)
DYNAMIC_FASTFLAGVARIABLE(GetLocalTeleportData, false)
FASTFLAGVARIABLE(UseBuildGenericGameUrl, true)

FASTFLAGVARIABLE(PlaceLauncherUsePOST, true)

using namespace RBX;

const char* const RBX::sTeleportService	= "TeleportService";

REFLECTION_BEGIN();
static Reflection::BoundYieldFuncDesc<TeleportService, shared_ptr<const Reflection::Tuple>(int)> func_GetPlayerPlaceInstanceAsync(&TeleportService::GetPlayerPlaceInstanceAsync, "GetPlayerPlaceInstanceAsync", "userId", Security::None);
static Reflection::BoundFuncDesc<TeleportService, void(int, std::string, shared_ptr<Instance>, std::string, Reflection::Variant, shared_ptr<Instance>)> func_TeleportToPlaceInstance(&TeleportService::TeleportToPlaceInstance, "TeleportToPlaceInstance", "placeId", "instanceId", "player", shared_ptr<Instance>(), "spawnName", std::string(), "teleportData", Reflection::Variant(), "customLoadingScreen", shared_ptr<Instance>(), Security::None);

static Reflection::BoundFuncDesc<TeleportService, void(int, std::string, shared_ptr<Instance>, Reflection::Variant, shared_ptr<Instance>)> func_TeleportToSpawnByName(&TeleportService::TeleportToSpawnByName, "TeleportToSpawnByName", "placeId", "spawnName", "player", shared_ptr<Instance>(), "teleportData", Reflection::Variant(), "customLoadingScreen", shared_ptr<Instance>(), Security::None);
static Reflection::BoundFuncDesc<TeleportService, void(int, shared_ptr<Instance>, Reflection::Variant, shared_ptr<Instance>)> func_Teleport(&TeleportService::Teleport, "Teleport", "placeId", "player", shared_ptr<Instance>(), "teleportData", Reflection::Variant(), "customLoadingScreen", shared_ptr<Instance>(), Security::None);

static Reflection::BoundYieldFuncDesc<TeleportService, std::string(int)> func_ReserveServer(&TeleportService::ReserveServer, "ReserveServer", "placeId", Security::None);
static Reflection::BoundFuncDesc<TeleportService, void(int, std::string, shared_ptr<const Instances>, std::string, Reflection::Variant, shared_ptr<Instance>)> func_TeleportToPrivateServer(&TeleportService::TeleportToPrivateServer, "TeleportToPrivateServer", "placeId", "reservedServerAccessCode", "players", "spawnName", std::string(), "teleportData", Reflection::Variant(), "customLoadingScreen", shared_ptr<Instance>(), Security::None);

static Reflection::BoundFuncDesc<TeleportService, Reflection::Variant(std::string)> func_GetTeleportSetting(&TeleportService::GetTeleportSetting, "GetTeleportSetting", "setting", Security::None);
static Reflection::BoundFuncDesc<TeleportService, void(std::string, Reflection::Variant)> func_SetTeleportSetting(&TeleportService::SetTeleportSetting, "SetTeleportSetting", "setting", "value", Security::None);

static Reflection::BoundCallbackDesc<bool(std::string,int,std::string)> callback_ConfirmationCallback("ConfirmationCallback", &TeleportService::confirmationCallback, "message", "placeId", "spawnName", Security::RobloxScript);
static Reflection::BoundCallbackDesc<void(std::string)> errorCallback("ErrorCallback", &TeleportService::showErrorCallback, "message", Security::RobloxScript);
static Reflection::BoundFuncDesc<TeleportService, void()> func_TeleportCancel(&TeleportService::TeleportCancel, "TeleportCancel", Security::RobloxScript);

static Reflection::BoundFuncDesc<TeleportService, Reflection::Variant()> func_GetLocalTeleportData(&TeleportService::getLocalPlayerTeleportData, "GetLocalPlayerTeleportData", Security::None);

static Reflection::BoundProp<bool> prop_CustomizedTeleportUI("CustomizedTeleportUI", category_Data, &TeleportService::customizedTeleportUI, Reflection::PropertyDescriptor::Attributes::deprecated());

Reflection::EventDesc<TeleportService, void(shared_ptr<Instance>, Reflection::Variant)> event_playerArrivedFromTeleport(&TeleportService::playerArrivedFromTeleportSignal, "LocalPlayerArrivedFromTeleport","loadingGui", "dataTable");
REFLECTION_END();

TeleportCallback *TeleportService::_callback = NULL;
std::string TeleportService::_spawnName;
std::string TeleportService::_baseUrl;
std::string TeleportService::_browserTrackerId;
bool TeleportService::_waitingForUserInput = false;

HeapValue<int> TeleportService::previousPlaceId(0);
HeapValue<int> TeleportService::previousCreatorId(0);
HeapValue<int> TeleportService::previousCreatorType(0);
bool TeleportService::teleported = false;
shared_ptr<Instance> TeleportService::customTeleportLoadingGui = shared_ptr<Instance>();
Reflection::Variant TeleportService::dataTable = Reflection::Variant();
TeleportService::SettingsMap TeleportService::settingsTable = TeleportService::SettingsMap();


namespace {
static void sendTeleportStats(const char* label)
{
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "Teleport", label);
}
}

static bool defCallback(std::string message, int placeId, std::string spawnName)
{
	return false;
}

static void defErrorCallback(std::string message)
{
}

TeleportService::TeleportService() :
    confirmationCallback(defCallback),
	showErrorCallback(defErrorCallback),
    requestingTeleport(false),
    url(""),
    customizedTeleportUI(false)
{
	setName("Teleport Service");
}

static std::string ReadStringValue(shared_ptr<const Reflection::ValueTable> jsonResult, std::string name) 
{
    Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<std::string>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected string result for %s", name.c_str()));
    }
}

static int ReadIntValue(shared_ptr<const Reflection::ValueTable> jsonResult, std::string name)
{
    Reflection::ValueTable::const_iterator itData = jsonResult->find(name);
    if (itData != jsonResult->end())
    {
        return itData->second.get<int>();
    }
    else
    {
        throw std::runtime_error(RBX::format("Unexpected int result for %s", name.c_str()));
    }
}

static shared_ptr<Instance> sanitizeCustomLoadingGui(shared_ptr<Instance> customLoadingGUI)
{
	if (customLoadingGUI)
	{
		shared_ptr<Instance> sanitizedCustomLoadingGUI = customLoadingGUI->clone(EngineCreator);
		customLoadingGUI->destroyDescendantsOfType<LuaSourceContainer>();
		return sanitizedCustomLoadingGUI;
	} 
	return shared_ptr<Instance>();
}

std::string &TeleportService::GetSpawnName()
{
	return _spawnName;
}

void TeleportService::ResetSpawnName()
{
	_spawnName = "";
}

void TeleportService::SetBaseUrl(const char *baseUrl)
{
	_baseUrl = std::string(baseUrl);
}

void TeleportService::SetBrowserTrackerId(const std::string& browserTrackerId)
{
	_browserTrackerId = browserTrackerId;
}

void TeleportService::SetCallback(TeleportCallback *callback) 
{ 
	_callback = callback; 
}

void TeleportService::TeleportCancel()
{
    _waitingForUserInput = false;
    requestingTeleport = false;
}

void TeleportService::TeleportImpl(shared_ptr<const Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI)
{
    RBXASSERT(!Workspace::serverIsPresent(this));

	dataTable = teleportInfo->at("teleportData");
	customTeleportLoadingGui = customLoadingGUI;
	if (customTeleportLoadingGui)
	{
		customTeleportLoadingGui->destroyDescendantsOfType<LuaSourceContainer>();
		customTeleportLoadingGui->setParent(NULL);
	}

    if (requestingTeleport)
    {
        return;
    }
	// check for being in studio here in case someone is trying to call this method directly without going through Teleport (see DE4204)
	if ( _callback && !_callback->isTeleportEnabled() )
	{
		showErrorCallback(STRING_BY_ID(NoTeleportInStudio));
		return;
	}

	try
	{
        requestingTeleport = true;
		RBXASSERT(!_baseUrl.empty());

#if defined(RBX_PLATFORM_DURANGO)
        char extraStuff[1024];
        sprintf_s(extraStuff, 1024, "&gamerTag=%s", _callback->xBox_getGamerTag().c_str()); // We'll probably drop this crap in the future. Ask me in 5 years.       -- Max
#else
        const char* extraStuff = "";
#endif

		if (FFlag::UseBuildGenericGameUrl)
		{
			if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToInstance)
			{
				// to-instance teleport
				_spawnName = teleportInfo->at("spawnName").get<std::string>();
				char urlBuf[2048] = {0};
				sprintf_s(urlBuf, 2048, "Game/PlaceLauncher.ashx?request=RequestGameJob&placeId=%d&gameId=%s&isPartyLeader=false&gender=&isTeleport=true%s", teleportInfo->at("placeId").get<int>(), teleportInfo->at("instanceId").get<std::string>().c_str(), extraStuff);
				url = BuildGenericGameUrl(_baseUrl, urlBuf);
			}
			else if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToPlace)
			{
				// to-place teleport
				_spawnName = teleportInfo->at("spawnName").get<std::string>();
				char urlBuf[2048] = {0};
				sprintf_s(urlBuf, 2048, "Game/PlaceLauncher.ashx?request=RequestGame&placeId=%d&isPartyLeader=false&gender=&isTeleport=true%s", teleportInfo->at("placeId").get<int>(), extraStuff);
				url = BuildGenericGameUrl(_baseUrl, urlBuf);
			}
			else if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToReservedServer)
			{
				// to-reserved server teleport
				_spawnName = teleportInfo->at("spawnName").get<std::string>();
				char urlBuf[2048] = {0};
				sprintf_s(urlBuf, 2048, "Game/PlaceLauncher.ashx?request=RequestPrivateGame&placeId=%d&accessCode=%s&linkCode=&privateGameMode=ReservedServer%s", teleportInfo->at("placeId").get<int>(), RBX::Http::urlEncode(teleportInfo->at("reservedServerAccessCode").get<std::string>()).c_str(), extraStuff);
				url = BuildGenericGameUrl(_baseUrl, urlBuf);
			}
		}
		else
		{
			if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToInstance)
			{
				// to-instance teleport
				char urlBuf[2048] = {0};
				sprintf_s(urlBuf, 2048, "%s/Game/PlaceLauncher.ashx?request=RequestGameJob&placeId=%d&gameId=%s&isPartyLeader=false&gender=&isTeleport=true%s", _baseUrl.c_str(), teleportInfo->at("placeId").get<int>(), teleportInfo->at("instanceId").get<std::string>().c_str(), extraStuff);
				url = urlBuf;
				_spawnName = teleportInfo->at("spawnName").get<std::string>();
			}
			else if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToPlace)
			{
				// to-place teleport
				_spawnName = teleportInfo->at("spawnName").get<std::string>();
				char urlBuf[2048] = {0};
				sprintf_s(urlBuf, 2048, "%s/Game/PlaceLauncher.ashx?request=RequestGame&placeId=%d&isPartyLeader=false&gender=&isTeleport=true%s", _baseUrl.c_str(), teleportInfo->at("placeId").get<int>(), extraStuff);
				url = urlBuf;
			}
			else if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToReservedServer)
			{
				// to-reserved server teleport
				char urlBuf[2048] = {0};
				sprintf_s(urlBuf, 2048, "%s/Game/PlaceLauncher.ashx?request=RequestPrivateGame&placeId=%d&accessCode=%s&linkCode=&privateGameMode=ReservedServer%s", _baseUrl.c_str(), teleportInfo->at("placeId").get<int>(), RBX::Http::urlEncode(teleportInfo->at("reservedServerAccessCode").get<std::string>()).c_str(), extraStuff);
				url = urlBuf;
				_spawnName = teleportInfo->at("spawnName").get<std::string>();
			}
		}

		if (!_browserTrackerId.empty())
			url += ("&browserTrackerId=" + _browserTrackerId);

#if defined(_NOOPT)
		StandardOut::singleton()->printf(MESSAGE_INFO, "Request: %s", url.c_str());
#endif

        // fire teleport signal
        if (Network::Player* player = Network::Players::findLocalPlayer(this))
        {
            player->onTeleportInternal(TeleportState_Started, teleportInfo);
        }
        // spawn the teleport thread
        retryTimer.reset();
        teleportThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&TeleportService::TeleportThreadImpl, shared_from(this), teleportInfo), "Teleport Thread")));
    }
    catch (RBX::base_exception& e)
    {
        _waitingForUserInput = false;
        requestingTeleport = false;
	    throw e;
    }
    _waitingForUserInput = false;
}

void TeleportService::TeleportThreadImpl(shared_ptr<const Reflection::ValueTable> teleportInfo)
{
    bool cancelable = true;
    int retries = DFInt::TeleportRetryTimes;
	shared_ptr<DataModel> dm = shared_from(DataModel::get(this));
	if (!dm)
	{
		return;
	}
    while (true)
    {
        if ((!requestingTeleport) && cancelable)
        {
            return;
        }
        else
        {
            try
            {
                std::string response = "";
                bool found = false;
                bool retryUsed = true;
                if (FFlag::PlaceLauncherUsePOST)
                {
                std::istringstream input("");
                    RBX::Http(url).post(input, RBX::Http::kContentTypeDefaultUnspecified, false, response);
                }
                else
                {
                    RBX::Http(url).get(response);
                }
                    
                std::stringstream jsonStream;
                jsonStream << response;
                shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());
                bool parseResult = WebParser::parseJSONTable(jsonStream.str(), jsonResult);
                if (parseResult)
                {
                    int status = ReadIntValue(jsonResult, "status");
                    if (status == 2)
                    {
                        cancelable = false;
                        found = true;
                    }
                    else
                    {
                        // 0 or 1 is not an error - it is a sign that we should wait
                        if (status == 0 || status == 1)
                        {
                            cancelable = false; // at this point, the teleport action cannot be canceled anymore because the web service could disconnect the player from current server at any time
                            // fire teleport signal
							try
							{
								DataModel::LegacyLock l(dm, DataModelJob::Write);

								if (Network::Player* player = Network::Players::findLocalPlayer(this))
								{
									player->onTeleportInternal(TeleportState_WaitingForServer, teleportInfo);
								}
							}
							catch (const RBX::base_exception&)
							{
							}
                            retryUsed = false;
                        }
                        // TODO more status code to indicate the failure reasons so we can better handle it

                        if (retryUsed)
                        {
                            retries --;
                        }
                    }
                }

                if (found)
                {
                    std::string au = ReadStringValue(jsonResult, "authenticationUrl");
                    std::string ticket = ReadStringValue(jsonResult, "authenticationTicket");;
                    std::string script;
                    
                    if (RBX::GameBasicSettings::singleton().inStudioMode())
                    {
						
						// get universe id
						Http getUniverseRequest(format("%s/universes/get-universe-containing-place?placeId=%d",
							ContentProvider::getUnsecureApiBaseUrl(_baseUrl).c_str(), teleportInfo->at("placeId").get<int>()));
						std::string universeJson;
						getUniverseRequest.get(universeJson);
						Reflection::Variant v;
						WebParser::parseJSONObject(universeJson, v);
						int universeId = v.cast<shared_ptr<const Reflection::ValueTable> >()->at(
							"UniverseId").cast<int>();

                        char urlBuf[2048] = {0};
						if (FFlag::UseBuildGenericGameUrl)
						{
							sprintf_s(urlBuf, 2048, "Game/Visit.ashx?placeID=%d&IsPlaySolo=1&FromTeleport=1&universeId=%d",
								teleportInfo->at("placeId").get<int>(), universeId);
							script = BuildGenericGameUrl(_baseUrl, urlBuf);
						}
						else
						{
							sprintf_s(urlBuf, 2048, "%s/Game/Visit.ashx?placeID=%d&IsPlaySolo=1&FromTeleport=1&universeId=%d",
								_baseUrl.c_str(), teleportInfo->at("placeId").get<int>(), universeId);
							script = urlBuf;
						}
                    }
                    else
                    {
                        script = ReadStringValue(jsonResult, "joinScriptUrl");
                    }

                    if (_callback != NULL)
                    {
                        // fire teleport signal
						try
						{
							DataModel::LegacyLock l(dm, DataModelJob::Write);
							if (Network::Player* player = Network::Players::findLocalPlayer(this))
							{
                            	player->onTeleportInternal(TeleportState_InProgress, teleportInfo);
							}

							previousPlaceId = dm->getPlaceID();
							previousCreatorId = dm->getCreatorID();
							previousCreatorType = dm->getCreatorType();
							teleported = true;
							_callback->doTeleport(au, ticket, script);
							_waitingForUserInput = false;
						}
						catch (const RBX::base_exception&)
						{
						}
                        return;
                    }
                }
                else if (retries<0)
                {
                    // fire teleport signal
					try
					{
						DataModel::LegacyLock l(dm, DataModelJob::Write);
						if (Network::Player* player = Network::Players::findLocalPlayer(this))
						{
                        
							RBXASSERT(teleportInfo->at("placeId").get<int>() >= -1);
							player->onTeleportInternal(TeleportState_Failed, teleportInfo);
						}
					}
					catch (const RBX::base_exception&)
					{
					}
					_waitingForUserInput = false;
					requestingTeleport = false;
                    return;
                }
            }
            catch (RBX::base_exception& e)
            {
                _waitingForUserInput = false;
                requestingTeleport = false;
                // invalid request
				try
				{
					DataModel::LegacyLock l(dm, DataModelJob::Write);
					if (Network::Player* player = Network::Players::findLocalPlayer(this))
					{
						player->onTeleportInternal(TeleportState_Failed, teleportInfo);
					}
				}
				catch (RBX::base_exception&)
				{
				}
                StandardOut::singleton()->printf(MESSAGE_ERROR, "Teleport exception: %s", e.what());
            }

            double timeToSleep = 1000.f - retryTimer.delta().msec();
            if (timeToSleep > 0.f)
            {
                boost::this_thread::sleep(timeToSleep);
            }
            retryTimer.reset();
        }
    }
}

void TeleportService::ServerTeleport(shared_ptr<Instance> characterOrPlayerInstance, shared_ptr<Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI)
{
    RBXASSERT(Workspace::serverIsPresent(this));
    if (characterOrPlayerInstance == NULL)
    {
        StandardOut::singleton()->printf(MESSAGE_WARNING, "Invalid player to teleport.");
        return;
    }
    Network::Player* player = characterOrPlayerInstance->fastDynamicCast<Network::Player>();
    if (!player)
    {
        player = Network::Players::getPlayerFromCharacter(characterOrPlayerInstance.get());
        if (player)
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING, "Passing Character instance into teleport functions is deprecated. Please use Player instance instead.");
        }
    }
    if (player)
    {
        {
			if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToPlace)
            {
                static boost::once_flag flag = BOOST_ONCE_INIT;
                boost::call_once(flag, boost::bind(&sendTeleportStats, "ToPlace"));
            }
            else if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToInstance)
            {
                static boost::once_flag flag = BOOST_ONCE_INIT;
                boost::call_once(flag, boost::bind(&sendTeleportStats, "ToInstance"));
            } 
			else if (teleportInfo->at("teleportType").get<TeleportType>() == TeleportType_ToReservedServer)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
                boost::call_once(flag, boost::bind(&sendTeleportStats, "ToReservedServer"));
			}
        }

		customLoadingGUI = sanitizeCustomLoadingGui(customLoadingGUI);
		if (customLoadingGUI)
		{
			customLoadingGUI->setParent(ServiceProvider::create<InsertService>(this)); //customLoadingGui needs to be parented to a Replication Container so it can be replicated to the client
		}

		player->onTeleportInternal(TeleportState_RequestedFromServer, teleportInfo, customLoadingGUI);
		if (customLoadingGUI)
		{
			customLoadingGUI->setParent(NULL);
		}
    }
    else
    {
        StandardOut::singleton()->printf(MESSAGE_WARNING, "Invalid player to teleport.");
    }
}

void TeleportService::Teleport(int placeId, shared_ptr<Instance> characterOrPlayerInstance, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI)
{
    TeleportToSpawnByName(placeId, "", characterOrPlayerInstance, teleportData, customLoadingGUI);
}

void TeleportService::TeleportToSpawnByName(int placeId, std::string spawnName, shared_ptr<Instance> characterOrPlayerInstance, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI)
{
	if (placeId <= 0)
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING, "Cannot teleport to invalid place id %d. Aborting teleport.", placeId);
		return;
	}
	shared_ptr<Reflection::ValueTable> teleportInfo(new Reflection::ValueTable);
	(*teleportInfo)["placeId"]					= placeId;
	(*teleportInfo)["spawnName"]				= spawnName;
	(*teleportInfo)["instanceId"]				= std::string();
	(*teleportInfo)["reservedServerAccessCode"] = std::string();
	(*teleportInfo)["teleportType"]				= TeleportType_ToPlace;
	(*teleportInfo)["teleportData"]				= teleportData;

    bool isServer = Workspace::serverIsPresent(this);
    if (isServer)
    {
        ServerTeleport(characterOrPlayerInstance, teleportInfo, customLoadingGUI);
        return;
    }

    // client implementation begins

    if (!isServer)
    {
		customLoadingGUI = sanitizeCustomLoadingGui(customLoadingGUI);
        TeleportImpl(teleportInfo, customLoadingGUI);
    }
}

void TeleportService::ProcessGetPlayerPlaceInstanceResultsSuccess(std::string response, 
											boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(4));
	int targetPlaceId;
	std::string targetGameId;
	std::string errorMsg = "";

	bool succeed = true;

	// parse the result
	if (!response.empty())
	{
		std::stringstream jsonStream;
		jsonStream << response;
		shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());
		bool parseResult = WebParser::parseJSONTable(jsonStream.str(), jsonResult);
		if (!parseResult)
		{
			succeed = false;
		}
		else
		{
			Reflection::ValueTable::const_iterator itData = jsonResult->find("PlaceId");
			if (itData == jsonResult->end())
			{
				succeed = false;
			}
			if (itData->second.isNumber())
			{
				targetPlaceId = itData->second.get<int>();
			}
			else
			{
				succeed = false;
			}

			itData = jsonResult->find("GameId");
			if (itData == jsonResult->end())
			{
				succeed = false;
			}
			if (itData->second.isString())
			{
				targetGameId = itData->second.get<std::string>();
			}
			else
			{
				succeed = false;
			}
		}
	}
	else
	{
		succeed = false;
	}
	if (!succeed)
	{
		errorMsg = "Unexpected result";
	}
	DataModel* dm = DataModel::get(this);
	if (succeed)
	{
		result->values[0] = (errorMsg.size()==0);
		result->values[1] = errorMsg;
		result->values[2] = targetPlaceId;
		result->values[3] = targetGameId;
		dm->submitTask(boost::bind(resumeFunction, result), DataModelJob::Write);
	}
	else
	{
		dm->submitTask(boost::bind(errorFunction, errorMsg), DataModelJob::Write);
	}
}
void TeleportService::ProcessGetPlayerPlaceInstanceResultsError(std::string error, boost::function<void(std::string)> errorFunction)
{
	// if user is offline or the response will be HTTP 500 which will be treated as exception
	DataModel* dm = DataModel::get(this);
	dm->submitTask(boost::bind(errorFunction, error), DataModelJob::Write);
}

void TeleportService::GetPlayerPlaceInstanceAsync(int playerId, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		char urlBuf[2048] = {0};
		DataModel* dm = DataModel::get(this);
		int placeId = dm->getPlaceID();
		sprintf_s(urlBuf, 2048, "universes/get-player-place-instance?currentPlaceId=%d&userId=%d", placeId, playerId);
		url = urlBuf;

#if defined(_NOOPT)
		StandardOut::singleton()->printf(MESSAGE_INFO, "Request: %s", url.c_str());
#endif

		apiService->getAsync(url, true, RBX::PRIORITY_SERVER_ELEVATED,
			boost::bind(&TeleportService::ProcessGetPlayerPlaceInstanceResultsSuccess, this, _1, resumeFunction, errorFunction), 
			boost::bind(&TeleportService::ProcessGetPlayerPlaceInstanceResultsError, this, _1, errorFunction) );
	}
}

void TeleportService::TeleportToPlaceInstance(int placeId, std::string instanceId, shared_ptr<Instance> characterOrPlayerInstance, std::string spawnName, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI)
{
	shared_ptr<Reflection::ValueTable> teleportInfo(new Reflection::ValueTable);
	(*teleportInfo)["placeId"]					= placeId;
	(*teleportInfo)["spawnName"]				= spawnName;
	(*teleportInfo)["instanceId"]				= instanceId;
	(*teleportInfo)["reservedServerAccessCode"] = std::string();
	(*teleportInfo)["teleportType"]				= TeleportType_ToInstance;
	(*teleportInfo)["teleportData"]				= teleportData;

    bool isServer = Workspace::serverIsPresent(this);
    if (isServer)
    {
        ServerTeleport(characterOrPlayerInstance, teleportInfo, customLoadingGUI);
        return;
    }

    // client implementation begins
    if (!isServer)
    {
		customLoadingGUI = sanitizeCustomLoadingGui(customLoadingGUI);
        TeleportImpl(teleportInfo, customLoadingGUI);
    }
}

void TeleportService::ProcessGrantAccessSuccess(shared_ptr<const Instances> players, shared_ptr<Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI, std::string response)
{
	for (Instances::const_iterator iter = players->begin(); iter != players->end(); ++iter)
	{
		if (Network::Player* player = (*iter)->fastDynamicCast<Network::Player>())
		{
			ServerTeleport(shared_from(player), teleportInfo, customLoadingGUI);
		}
	}
}

void TeleportService::ProcessGrantAccessError(shared_ptr<const Instances> players, shared_ptr<Reflection::ValueTable> teleportInfo, std::string error)
{
	StandardOut::singleton()->printf(MESSAGE_ERROR, "Teleport to ReservedServer failed : %s", error.c_str());
	for (Instances::const_iterator iter = players->begin(); iter != players->end(); ++iter)
	{
		if (Network::Player* player = (*iter)->fastDynamicCast<Network::Player>())
		{
			player->onTeleportInternal(TeleportState_Failed, teleportInfo);
		}
	}
}

void TeleportService::TeleportToPrivateServer(int placeId, std::string reservedServerAccessCode, shared_ptr<const Instances> players, std::string spawnName, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI)
{
	if (Workspace::serverIsPresent(this))
	{
		std::stringstream playerIdsStream;
		bool playerFound = false;

		for (Instances::const_iterator iter = players->begin(); iter != players->end(); ++iter)
		{
			if (Network::Player* player = (*iter)->fastDynamicCast<Network::Player>())
			{
				playerIdsStream << (playerFound ? "&" : "")<< "playerIds=" << player->getUserID();
				playerFound = true;
			}
		}

		char urlBuf[2048] = {0};
		sprintf_s(urlBuf, 2048, "reservedservers/grantaccess?reservedServerAccessCode=%s", RBX::Http::urlEncode(reservedServerAccessCode).c_str());
		url = urlBuf;

		if (playerFound)
		{
			if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
			{

#if defined(_NOOPT)
				StandardOut::singleton()->printf(MESSAGE_INFO, "Request: %s", url.c_str());
#endif

				shared_ptr<Reflection::ValueTable> teleportInfo(new Reflection::ValueTable);
				(*teleportInfo)["placeId"]					= placeId;
				(*teleportInfo)["spawnName"]				= spawnName;
				(*teleportInfo)["instanceId"]				= std::string();
				(*teleportInfo)["reservedServerAccessCode"] = reservedServerAccessCode;
				(*teleportInfo)["teleportType"]				= TeleportType_ToReservedServer;
				(*teleportInfo)["teleportData"]				= teleportData;

				apiService->postAsync(url, playerIdsStream.str(), true, RBX::PRIORITY_SERVER_ELEVATED, HttpService::APPLICATION_URLENCODED,
					boost::bind(&TeleportService::ProcessGrantAccessSuccess, this, players, teleportInfo, customLoadingGUI, _1), 
					boost::bind(&TeleportService::ProcessGrantAccessError, this, players, teleportInfo, _1) );
			}
		}
		else
		{
			StandardOut::singleton()->print(MESSAGE_ERROR, "TeleportService::TeleportToPrivateServer must be passed an array of players");
		}
	} 
	else
	{
		StandardOut::singleton()->print(MESSAGE_ERROR, "TeleportService::TeleportToPrivateServer can only be called from the server");
	}
}


void TeleportService::ProcessReserveServerResultsSuccess(std::string response, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	std::string reservedServerAccessCode;
	std::string errorMsg = "";

	// parse the result
	if (!response.empty())
	{
		std::stringstream jsonStream;
		jsonStream << response;
		shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());
		bool parseResult = WebParser::parseJSONTable(jsonStream.str(), jsonResult);
		if (!parseResult)
		{
			errorMsg = "ReserveServer error: Could not parse JSON result";
		}
		else
		{
			Reflection::ValueTable::const_iterator itData = jsonResult->find("ReservedServerAccessCode");
			if (itData == jsonResult->end())
			{
				errorMsg = "ReserveServer error: No ReservedServerAccessCode returned";
			}
			if (itData->second.isString())
			{
				reservedServerAccessCode = itData->second.get<std::string>();
			}
			else
			{
				errorMsg = "ReserveServer error: ReservedServerAccessCode is not of type string";
			}
		}
	}
	else
	{
		errorMsg = "ReserveServer error: Response is empty";
	}

	DataModel* dm = DataModel::get(this);
	if (errorMsg == "")
	{
		dm->submitTask(boost::bind(resumeFunction, reservedServerAccessCode), DataModelJob::Write);
	}
	else
	{
		dm->submitTask(boost::bind(errorFunction, errorMsg), DataModelJob::Write);
	}
}

void TeleportService::ProcessReserveServerResultsError(std::string error, boost::function<void(std::string)> errorFunction)
{
	DataModel* dm = DataModel::get(this);
	dm->submitTask(boost::bind(errorFunction, error), DataModelJob::Write);
}

void TeleportService::ReserveServer(int placeId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (Workspace::serverIsPresent(this))
	{
		if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
		{
			char urlBuf[2048] = {0};
			sprintf_s(urlBuf, 2048, "reservedservers/create?placeId=%d", placeId);
			url = urlBuf;

#if defined(_NOOPT)
			StandardOut::singleton()->printf(MESSAGE_INFO, "Request: %s", url.c_str());
#endif

			apiService->postAsync(url, "", true, RBX::PRIORITY_SERVER_ELEVATED, HttpService::APPLICATION_URLENCODED,
				boost::bind(&TeleportService::ProcessReserveServerResultsSuccess, this, _1, resumeFunction, errorFunction), 
				boost::bind(&TeleportService::ProcessReserveServerResultsError, this, _1, errorFunction) );
		}
	}
	else
	{
		errorFunction("TeleportService:ReserveServer can only be called by the server.");
		return;
	}
}

void TeleportService::SetTeleportSetting(std::string key, Reflection::Variant value)
{
	TeleportService::settingsTable[key] = value;
}

Reflection::Variant TeleportService::GetTeleportSetting(std::string key)
{
	TeleportService::SettingsMap::iterator iter = TeleportService::settingsTable.find(key);
	if (iter != TeleportService::settingsTable.end())
	{
		return (*iter).second;
	}
	return Reflection::Variant();
}

Reflection::Variant TeleportService::getLocalPlayerTeleportData()
{
	if (DFFlag::GetLocalTeleportData)
	{
		return TeleportService::getDataTable();
	}
	else
	{
		throw std::runtime_error("GetLocalPlayerTeleportData is not currently enabled");
	}
}

namespace RBX{
    namespace Reflection {
        template<>
        EnumDesc<TeleportService::TeleportState>::EnumDesc()
            :EnumDescriptor("TeleportState")
        {
            addPair(TeleportService::TeleportState_RequestedFromServer, "RequestedFromServer");
            addPair(TeleportService::TeleportState_Started,             "Started");
            addPair(TeleportService::TeleportState_WaitingForServer,    "WaitingForServer");
            addPair(TeleportService::TeleportState_Failed,              "Failed");
            addPair(TeleportService::TeleportState_InProgress,          "InProgress");
        }

        template<>
        TeleportService::TeleportState& Variant::convert<TeleportService::TeleportState>(void)
        {
            return genericConvert<TeleportService::TeleportState>();
        }
		
        template<>
        EnumDesc<TeleportService::TeleportType>::EnumDesc()
            :EnumDescriptor("TeleportType")
        {
            addPair(TeleportService::TeleportType_ToPlace,				"ToPlace");
            addPair(TeleportService::TeleportType_ToInstance,			"ToInstance");
            addPair(TeleportService::TeleportType_ToReservedServer,     "ToReservedServer");
        }

		template<>
        TeleportService::TeleportType& Variant::convert<TeleportService::TeleportType>(void)
        {
            return genericConvert<TeleportService::TeleportType>();
        } 
    }

    template<>
    bool StringConverter<TeleportService::TeleportState>::convertToValue(const std::string& text, TeleportService::TeleportState& value)
    {
        return Reflection::EnumDesc<TeleportService::TeleportState>::singleton().convertToValue(text.c_str(),value);
    }

    template<>
    bool StringConverter<TeleportService::TeleportType>::convertToValue(const std::string& text, TeleportService::TeleportType& value)
    {
        return Reflection::EnumDesc<TeleportService::TeleportType>::singleton().convertToValue(text.c_str(),value);
    }
}//namespace RBX::Reflection
