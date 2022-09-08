/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/DataModel.h"		// TODO - minimize these includes, and in the .h file
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/KeyframeSequenceProvider.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/Tool.h"
#include "V8DataModel/MouseCommand.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/Lighting.h"
#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/ContextActionService.h"
#include "V8DataModel/StarterPlayerService.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/SpawnLocation.h"
#include "V8DataModel/Visit.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/TextButton.h"
#include "V8DataModel/Test.h"
#include "V8DataModel/MegaCluster.h"
#include "V8datamodel/Stats.h"
#include "V8datamodel/PartInstance.h"

#include "V8DataModel/BadgeService.h"
#include "V8DataModel/ChatService.h"
#include "V8DataModel/CollectionService.h"
#include "V8DataModel/CookiesEngineService.h"
#include "V8DataModel/DebrisService.h"
#include "V8DataModel/FriendService.h"
#include "V8DataModel/GamePassService.h"
#include "V8DataModel/GeometryService.h"
#include "V8DataModel/GuiService.h"
#include "V8DataModel/ImageButton.h"
#include "V8DataModel/InsertService.h"
#include "V8DataModel/JointsService.h"
#include "V8DataModel/MarketplaceService.h"
#include "V8DataModel/PointsService.h"
#include "V8DataModel/PersonalServerService.h"
#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/RenderHooksService.h"
#include "V8DataModel/ScriptService.h"
#include "V8DataModel/SocialService.h"
#include "V8DataModel/TeleportService.h"
#include "V8DataModel/UserInputService.h"
#include "V8DataModel/HackDefines.h"
#include "v8datamodel/ReplicatedStorage.h"
#include "v8datamodel/RobloxReplicatedStorage.h"
#include "v8datamodel/ReplicatedFirst.h"
#include "v8datamodel/ServerScriptService.h"
#include "v8datamodel/ServerStorage.h"
#include "v8datamodel/AssetService.h"
#include "v8datamodel/DataStoreService.h"
#include "v8datamodel/HttpService.h"
#include "V8DataModel/LogService.h"
#include "V8DataModel/AdService.h"
#include "V8DataModel/NotificationService.h"
#include "V8DataModel/CSGDictionaryService.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/ScreenGui.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "v8datamodel/GamepadService.h"
#include "v8datamodel/TextBox.h"

#include "v8datamodel/Light.h"

#include "Humanoid/Humanoid.h"

#include "Network/Api.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Network/NetworkOwner.h"
#include "Network/GameConfigurer.h"

#include "V8World/Contact.h"
#include "V8Kernel/ContactConnector.h"
#include "V8Kernel/Kernel.h"
#include "V8World/ContactManager.h" // for fastclear

#include "v8xml/WebParser.h"

#include "Script/luainstancebridge.h"
#include "Script/scriptcontext.h"
#include "Script/Script.h"
#include "Script/CoreScript.h"

#include "Util/Statistics.h"
#include "Util/ScriptInformationProvider.h"
#include "Util/SimSendFilter.h"
#include "Util/SoundWorld.h"
#include "Util/SoundService.h"
#include "Util/StandardOut.h"
#include "Util/UserInputBase.h"
#include "Util/http.h"
#include "Util/profiling.h"
#include "Util/IMetric.h"
#include "Util/Statistics.h"
#include "Util/ContentFilter.h"
#include "Util/RobloxGoogleAnalytics.h"

#include "rbx/Log.h"
#include "FastLog.h"
#include "rbx/threadsafe.h"
#include "rbx/Countable.h"

#include "AppDraw/DrawPrimitives.h"

#include "g3d/format.h"

#include "RbxG3D/RbxTime.h"

#include "V8Xml/Serializer.h"
#include "V8Xml/XmlSerializer.h"
#include "boost/cast.hpp"
#include "boost/scoped_ptr.hpp"

#include "GfxBase/IAdornableCollector.h" 
#include <fstream>
#include "rbx/RbxDbgInfo.h"
#include <boost/format.hpp> 

#include "util/RbxStringTable.h"
#include "format_string.h"
#include "VMProtect/VMProtectSDK.h"

#include "RbxFormat.h"

#include "util/MemoryStats.h"

#include "../NetworkSettings.h"

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

#include "rbx/Profiler.h"

LOGGROUP(CloseDataModel)
LOGGROUP(LegacyLock)

FASTFLAGVARIABLE(UserBetterInertialScrolling, false)

DYNAMIC_FASTINT(SavePlacePerMinute)

DYNAMIC_FASTINTVARIABLE(OnCloseTimeoutInSeconds, 30)
DYNAMIC_FASTFLAGVARIABLE(AllowHideHudShortcut, false)
DYNAMIC_FASTFLAGVARIABLE(AllowHideHudShortcutDefault, true)
DYNAMIC_FASTFLAGVARIABLE(ProcessAcceleratorsBeforeGUINavigation, false)
DYNAMIC_FASTFLAGVARIABLE(DontProcessMouseEventsForGuiTarget, false)
DYNAMIC_FASTFLAGVARIABLE(CloseStatesBeforeChildRemoval, false)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled);
DYNAMIC_FASTFLAGVARIABLE(DataModelProcessHttpRequestResponseOnLockUseSubmitTask, true)

FASTFLAGVARIABLE(RelativisticCameraFixEnable, true)
FASTFLAG(UserAllCamerasInLua)

LOGVARIABLE(CyclicExecutiveTiming, 0)

LOGVARIABLE(CyclicExecutiveThrottling, 0)

DYNAMIC_FASTFLAG(UseR15Character)
DYNAMIC_LOGVARIABLE(R15Character, 0)

namespace RBX {

static void dummyLoader(RBX::DataModel*) {}
int DataModel::LegacyLock::mainThreadId = ~0;

// DataModel::throttleAt30Fps should only affect the frequency at which Physics and Rendering runs. Nothing else.
bool DataModel::throttleAt30Fps = true;							// for debugging/benchmarking - default is true;
unsigned int DataModel::sendStats = 0;									// flipped if client is "hacking" 
std::string DataModel::hash; // contains the hash of the exe
boost::function<void(RBX::DataModel*)> DataModel::loaderFunc = boost::function<void(RBX::DataModel*)>(dummyLoader);

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<DataModel, void()> func_loadPlugins(&DataModel::loadPlugins, "LoadPlugins", Security::Roblox);

Reflection::EventDesc<DataModel, void(shared_ptr<Instance>, const Reflection::PropertyDescriptor*)> event_ItemChanged(&DataModel::itemChangedSignal, "ItemChanged", "object", "descriptor");

#if defined(RBX_STUDIO_BUILD) || defined (RBX_RCC_SECURITY) || defined (RBX_TEST_BUILD)
static Reflection::BoundFuncDesc<DataModel, shared_ptr<const Instances>(ContentId)> getContentFunctionOld(&DataModel::fetchAsset, "get", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, shared_ptr<const Instances>(ContentId)> getContentFunction(&DataModel::fetchAsset, "GetObjects", "url", Security::Plugin);
#endif

static const Reflection::BoundYieldFuncDesc<DataModel, bool(Instance::SaveFilter)>  savePlaceAsyncFunction(&DataModel::savePlaceAsync, "SavePlace", "saveFilter", DataModel::SAVE_ALL, Security::None);

static Reflection::BoundFuncDesc<DataModel, void(int)>  loadWorldFunction(&DataModel::loadWorld, "LoadWorld", "assetID", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void(int)>  loadGameFunction(&DataModel::loadGame, "LoadGame", "assetID", Security::LocalUser);

static Reflection::BoundFuncDesc<DataModel, void(ContentId)>  loadFunction(&DataModel::loadContent, "Load", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void(ContentId)>  saveFunction(&DataModel::save, "Save", "url", Security::Roblox);

static Reflection::BoundFuncDesc<DataModel, void(bool)> setRemoteBuildMode(&DataModel::setRemoteBuildMode, "SetRemoteBuildMode", "buildModeEnabled", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, bool()> getRemoteBuildMode(&DataModel::getRemoteBuildMode, "GetRemoteBuildMode", Security::None);
static Reflection::BoundFuncDesc<DataModel, void(std::string)> setServerSaveUrlFunction(&DataModel::setServerSaveUrl, "SetServerSaveUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void()> serverSaveFunction(&DataModel::serverSave, "ServerSave", Security::LocalUser);

// TODO: Security: Make this Trusted only:
// TODO: Remove the default value for synchronous, or make it true?  It is non-intuitive to be false by default
static const Security::Permissions kHttpPermission = Security::RobloxScript;
static Reflection::BoundYieldFuncDesc<DataModel, std::string(std::string)>  httpGetAsyncFunction(&DataModel::httpGetAsync, "HttpGetAsync", "url", kHttpPermission);
static Reflection::BoundYieldFuncDesc<DataModel, std::string(std::string, std::string, std::string)>  httpPostAsyncFunction(&DataModel::httpPostAsync, "HttpPostAsync", "url", "data", "contentType", "*/*", kHttpPermission);
static Reflection::BoundFuncDesc<DataModel, std::string(std::string, bool)>  httpGetFunction(&DataModel::httpGet, "HttpGet", "url", "synchronous", false, kHttpPermission);
static Reflection::BoundFuncDesc<DataModel, std::string(std::string, std::string, bool, std::string)>  httpPostFunction(&DataModel::httpPost, "HttpPost", "url", "data", "synchronous", false, "contentType", "*/*", kHttpPermission);

static Reflection::BoundFuncDesc<DataModel, shared_ptr<const Reflection::ValueArray>()>  getJobsInfo(&DataModel::getJobsInfo, "GetJobsInfo", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(std::string, std::string, std::string, std::string, std::string)>  func_reportMeasurement(&DataModel::reportMeasurement, "ReportMeasurement", "id", "key1", "value1", "key2", "value2", Security::RobloxScript);
static Reflection::BoundFuncDesc<DataModel, void(std::string, std::string, std::string, int)> googleAnalyticsFunction(&DataModel::luaReportGoogleAnalytics, "ReportInGoogleAnalytics", "category", "action", "custom", "label", "none", "value", 0, Security::RobloxScript);

#if defined(RBX_STUDIO_BUILD) || defined (RBX_RCC_SECURITY) || defined (RBX_TEST_BUILD)
static Reflection::BoundFuncDesc<DataModel, void(bool)> sanitizeFunction(&DataModel::clearContents, "ClearContent", "resettingSimulation", Security::LocalUser);
#endif
static Reflection::BoundFuncDesc<DataModel, void()> closeFunction(&DataModel::close, "Shutdown", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void()> toggleFunction(&DataModel::toggleToolsOff, "ToggleTools", Security::LocalUser);

static Reflection::PropDescriptor<DataModel, bool> prop_isPersonalServer("IsPersonalServer", category_State, &DataModel::getIsPersonalServer, &DataModel::setIsPersonalServer, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<DataModel, bool> prop_canSaveLocal("LocalSaveEnabled", category_State, &DataModel::canSaveLocal, NULL, Reflection::PropertyDescriptor::UI, Security::RobloxScript);

static Reflection::BoundYieldFuncDesc<DataModel, bool()> saveToRobloxFunction(&DataModel::saveToRoblox, "SaveToRoblox", Security::RobloxScript);
static Reflection::BoundCallbackDesc<bool()> requestShutdownCallback("RequestShutdown", &DataModel::requestShutdownCallback, Security::RobloxScript);
static Reflection::BoundFuncDesc<DataModel, void(bool)> finishShutdownFunction(&DataModel::completeShutdown, "FinishShutdown", "localSave", Security::RobloxScript);

static Reflection::BoundFuncDesc<DataModel, void(std::string)> func_SetUiMessage(&DataModel::setUiMessage, "SetMessage", "message", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void()> func_ClearUIMessage(&DataModel::clearUiMessage, "ClearMessage", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void()> func_SetUIMessageBrickCount(&DataModel::setUiMessageBrickCount, "SetMessageBrickCount", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, std::string()> func_GetUIMessage(&DataModel::getUpdatedMessageBoxText, "GetMessage", Security::None);

static Reflection::BoundFuncDesc<DataModel, shared_ptr<const Reflection::ValueArray>()>  getJobsExtendedStats(&DataModel::getJobsExtendedStats, "GetJobsExtendedStats", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, double(std::string, double)>  getJobTimePeakFraction(&DataModel::getJobTimePeakFraction, "GetJobTimePeakFraction", "jobname", "greaterThan", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, double(std::string, double)>  getJobIntervalPeakFraction(&DataModel::getJobIntervalPeakFraction, "GetJobIntervalPeakFraction", "jobname", "greaterThan", Security::Plugin);
// TODO: Make this part of settings, not per-document. After all, it is a global setting
static Reflection::BoundFuncDesc<DataModel, void(double)>  setJobsExtendedStatsWindow(&DataModel::setJobsExtendedStatsWindow, "SetJobsExtendedStatsWindow", "seconds", Security::LocalUser);

static Reflection::BoundFuncDesc<DataModel, void(int)>  setPlaceVersion(&DataModel::setPlaceVersion, "SetPlaceVersion", "placeId", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(int, bool)>  setPlaceId(&DataModel::setPlaceID, "SetPlaceId", "placeId", "robloxPlace", false, Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(int, bool)>  setPlaceIdDeprecated(&DataModel::setPlaceID, "SetPlaceID", "placeID", "robloxPlace", false, Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(std::string)> setGameInstanceId(&DataModel::setGameInstanceID, "SetGameInstanceId", "instanceID", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(int)> setUniverseId(&DataModel::setUniverseId, "SetUniverseId", "universeId", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(int, DataModel::CreatorType)>  setCreatorId(&DataModel::setCreatorID, "SetCreatorId", "creatorId", "creatorType", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(int, DataModel::CreatorType)>  setCreatorIdDeprecated(&DataModel::setCreatorID, "SetCreatorID", "creatorID", "creatorType", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(DataModel::Genre)>  func_setGenre(&DataModel::setGenre, "SetGenre", "genre", Security::Plugin);
static Reflection::BoundFuncDesc<DataModel, void(DataModel::GearGenreSetting, int)>  func_setGear(&DataModel::setGear, "SetGearSettings", "genreRestriction", "allowedGenres", Security::Plugin);

static Reflection::RefPropDescriptor<DataModel, Workspace> prop_Workspace("Workspace", category_Data, &DataModel::getWorkspace, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::RefPropDescriptor<DataModel, Workspace> prop_workspace("workspace", category_Data, &DataModel::getWorkspace, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(prop_Workspace));
Reflection::RefPropDescriptor<DataModel, Instance> DataModel::prop_lighting("lighting", category_Data, &DataModel::getLightingDeprecated, NULL, Reflection::PropertyDescriptor::Attributes::deprecated());

static Reflection::PropDescriptor<DataModel, int> prop_placeId("PlaceId", category_State, &DataModel::getPlaceID, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<DataModel, int> prop_placeVersion("PlaceVersion", category_State, &DataModel::getPlaceVersion, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<DataModel, int> prop_creatorId("CreatorId", category_State, &DataModel::getCreatorID, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<DataModel, bool> prop_forceR15("ForceR15", category_State, &DataModel::getForceR15, &DataModel::setForceR15, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::Roblox);
static Reflection::EnumPropDescriptor<DataModel, DataModel::CreatorType> prop_creatorType("CreatorType", category_State, &DataModel::getCreatorType, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::EnumPropDescriptor<DataModel, DataModel::Genre> prop_genre("Genre", category_State, &DataModel::getGenre, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::EnumPropDescriptor<DataModel, DataModel::GearGenreSetting> prop_gearGenreSetting("GearGenreSetting", category_State, &DataModel::getGearGenreSetting, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::BoundFuncDesc<DataModel, bool(DataModel::GearType)>  func_isGearTypeAllowed(&DataModel::isGearTypeAllowed, "IsGearTypeAllowed", "gearType", Security::None);
static Reflection::EventDesc<DataModel, void()> event_allowedGearTypeChanged(&DataModel::allowedGearTypeChanged, "AllowedGearTypeChanged");

static Reflection::PropDescriptor<DataModel, std::string> prop_getJobId("JobId", "JobInfo", &DataModel::getJobId, NULL, Reflection::PropertyDescriptor::UI);

static Reflection::BoundFuncDesc<DataModel, void(std::string)> setScreenshotSEOInfoFunction(&DataModel::setScreenshotSEOInfo, "SetScreenshotInfo", "info", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void(std::string)> setVideoSEOInfoFunction(&DataModel::setVideoSEOInfo, "SetVideoInfo", "info", Security::LocalUser);

static Reflection::EventDesc<DataModel, void(bool)> event_graphicsQualityChangeRequest(&DataModel::graphicsQualityShortcutSignal, "GraphicsQualityChangeRequest","betterQuality");

static Reflection::EventDesc<DataModel, void()> event_GameLoaded(&DataModel::gameLoadedSignal, "Loaded");
static Reflection::BoundFuncDesc<DataModel, bool()> gameLoadedFunction(&DataModel::getIsGameLoaded, "IsLoaded", Security::None);

static Reflection::BoundFuncDesc<DataModel, void(std::string, std::string)> addStatFunction(&DataModel::addCustomStat, "AddStat", "displayName", "stat", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void(std::string)> removeStatFunction(&DataModel::removeCustomStat, "RemoveStat", "stat", Security::LocalUser);
static Reflection::BoundFuncDesc<DataModel, void()> writeStatSettingsFunction(&DataModel::writeStatsSettings, "SaveStats", Security::LocalUser);

static Reflection::BoundAsyncCallbackDesc<
	DataModel, shared_ptr<const Reflection::Tuple>(void)> callback_onClose("OnClose", &DataModel::onCloseCallback, &DataModel::onCloseCallbackChanged);

static Reflection::PropDescriptor<DataModel, std::string> desc_VIPServerId("VIPServerId", category_Data, &DataModel::getVIPServerId, NULL);
static Reflection::BoundFuncDesc<DataModel, void(std::string)> func_setVIPServerId(&DataModel::setVIPServerId, "SetVIPServerId", "newId", Security::LocalUser);
static Reflection::PropDescriptor<DataModel, int> desc_VIPServerOwnerId("VIPServerOwnerId", category_Data, &DataModel::getVIPServerOwnerId, NULL);
static Reflection::BoundFuncDesc<DataModel, void(int)> func_setVIPServerOwnerId(&DataModel::setVIPServerOwnerId, "SetVIPServerOwnerId", "newId", Security::LocalUser);
REFLECTION_END();

bool DataModel::BlockingDataModelShutdown = true;
unsigned int DataModel::perfStats = 0;									// flipped if client is "hacking" 

const char *const sDataModel = "DataModel";

namespace Reflection
{
	template<>
	EnumDesc<DataModel::CreatorType>::EnumDesc()
	:EnumDescriptor("CreatorType")
	{
		addPair(DataModel::CREATOR_USER, "User");
		addPair(DataModel::CREATOR_GROUP, "Group");
	}

	template<>
	DataModel::CreatorType& Variant::convert<DataModel::CreatorType>(void)
	{
		return genericConvert<DataModel::CreatorType>();
	}

	
	template<>
	EnumDesc<DataModel::Genre>::EnumDesc()
	:EnumDescriptor("Genre")
	{
		addPair(DataModel::GENRE_ALL			, "All");
		addPair(DataModel::GENRE_TOWN_AND_CITY  , "TownAndCity");
		addPair(DataModel::GENRE_FANTASY		, "Fantasy");
		addPair(DataModel::GENRE_SCI_FI		    , "SciFi");
		addPair(DataModel::GENRE_NINJA			, "Ninja");
		addPair(DataModel::GENRE_SCARY			, "Scary");
		addPair(DataModel::GENRE_PIRATE		    , "Pirate");
		addPair(DataModel::GENRE_ADVENTURE		, "Adventure");
		addPair(DataModel::GENRE_SPORTS		    , "Sports");
		addPair(DataModel::GENRE_FUNNY			, "Funny");
		addPair(DataModel::GENRE_WILD_WEST		, "WildWest");
		addPair(DataModel::GENRE_WAR			, "War");
		addPair(DataModel::GENRE_SKATE_PARK	    , "SkatePark");
		addPair(DataModel::GENRE_TUTORIAL		, "Tutorial");
	}

	template<>
	DataModel::Genre& Variant::convert<DataModel::Genre>(void)
	{
		return genericConvert<DataModel::Genre>();
	}

	
	template<>
	EnumDesc<DataModel::GearGenreSetting>::EnumDesc()
	:EnumDescriptor("GearGenreSetting")
	{
		addPair(DataModel::GEAR_GENRE_ALL, "AllGenres");
		addPair(DataModel::GEAR_GENRE_MATCH, "MatchingGenreOnly");
	}

	template<>
	DataModel::GearGenreSetting& Variant::convert<DataModel::GearGenreSetting>(void)
	{
		return genericConvert<DataModel::GearGenreSetting>();
	}

	
	template<>
	EnumDesc<DataModel::GearType>::EnumDesc()
	:EnumDescriptor("GearType")
	{
		addPair(DataModel::GEAR_TYPE_MELEE_WEAPONS			, "MeleeWeapons");
		addPair(DataModel::GEAR_TYPE_RANGED_WEAPONS			, "RangedWeapons");
		addPair(DataModel::GEAR_TYPE_EXPLOSIVES				, "Explosives");
		addPair(DataModel::GEAR_TYPE_POWER_UPS				, "PowerUps");
		addPair(DataModel::GEAR_TYPE_NAVIGATION_ENHANCERS	, "NavigationEnhancers");
		addPair(DataModel::GEAR_TYPE_MUSICAL_INSTRUMENTS	, "MusicalInstruments");
		addPair(DataModel::GEAR_TYPE_SOCIAL_ITEMS			, "SocialItems");
		addPair(DataModel::GEAR_TYPE_BUILDING_TOOLS			, "BuildingTools");
		addPair(DataModel::GEAR_TYPE_PERSONAL_TRANSPORT		, "Transport");
	}

	template<>
	DataModel::GearType& Variant::convert<DataModel::GearType>(void)
	{
		return genericConvert<DataModel::GearType>();
	}

	template<>
	EnumDesc<Instance::SaveFilter>::EnumDesc()
		:EnumDescriptor("SaveFilter")
	{
		addPair(Instance::SAVE_ALL		, "SaveAll");
		addPair(Instance::SAVE_WORLD	, "SaveWorld");
		addPair(Instance::SAVE_GAME		, "SaveGame");
	}

	template<>
	Instance::SaveFilter& Variant::convert<Instance::SaveFilter>(void)
	{
		return genericConvert<Instance::SaveFilter>();
	}
}

template<>
bool RBX::StringConverter<DataModel::CreatorType>::convertToValue(const std::string& text, DataModel::CreatorType& value)
{
	return Reflection::EnumDesc<DataModel::CreatorType>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool RBX::StringConverter<DataModel::Genre>::convertToValue(const std::string& text, DataModel::Genre& value)
{
	return Reflection::EnumDesc<DataModel::Genre>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool RBX::StringConverter<DataModel::GearGenreSetting>::convertToValue(const std::string& text, DataModel::GearGenreSetting& value)
{
	return Reflection::EnumDesc<DataModel::GearGenreSetting>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool RBX::StringConverter<DataModel::GearType>::convertToValue(const std::string& text, DataModel::GearType& value)
{
	return Reflection::EnumDesc<DataModel::GearType>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool RBX::StringConverter<DataModel::SaveFilter>::convertToValue(const std::string& text, DataModel::SaveFilter& value)
{
	return Reflection::EnumDesc<DataModel::SaveFilter>::singleton().convertToValue(text.c_str(),value);
}


class DataModel::GenericJob : public DataModelJob
{
public:
	weak_ptr<DataModel> const dataModel;
	rbx::timestamped_safe_queue<Task> tasks;

	GenericJob(shared_ptr<DataModel> dataModel, const char* name, TaskType taskType)
		:DataModelJob(name, taskType, false, dataModel, Time::Interval(0))
		,dataModel(dataModel)
	{
	}
private:
	virtual Time::Interval sleepTime(const Stats& stats)
	{
		return tasks.size() > 0 ? Time::Interval(0) : Time::Interval::max();
	}

	void step(Task& task)
	{
		shared_ptr<DataModel> dm(dataModel.lock());
		if (!dm)
			return;

        task(dm.get());
	}

	//
	// *** THIS IS BEING USED AS A NEAR *IMMEDIATE* CALLBACK BY THE LEGACY LOCK.        ***
	// *** HOWEVER THE ERROR CALCUALTION IS GARBAGE FOR THAT, INITIALLY RETURNING ZERO. ***
	//
	virtual Job::Error error(const Stats& stats)
	{
		Error result;

		const int size = tasks.size();
		if (size)
		{
			// Estimate the total error to be headTime * n / 2
			result.error = tasks.head_waittime_sec(stats.timeNow) * (1 + size) / 2.0;
			
			// These tasks are "urgent" because they are often used 
			// with LegacyLock to marshal tasks with the UI or Web Service requests
            result.urgent = size > 0;
		}
		return result;
	}

	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
	{
		FASTLOG3(FLog::TaskSchedulerRun, "GenericJob::Step dataModel(%d) type(%d) taskCount(%d)", dataModel.lock().get(), taskType, tasks.size());
		
		switch (taskType)
		{
		case Write:
		case DataIn:
		case PhysicsIn:
			{
				shared_ptr<DataModel> dm(dataModel.lock());
				if (!dm)
					return TaskScheduler::Done;
				// TODO: clean up, non-scoped write request? 
				DataModel::scoped_write_request request(dm.get());

				// need to call processTasks here because of the scoped write request
				processTasks();
			}
			break;
		default:
			{
				processTasks();
			}
			break;
		}
		return TaskScheduler::Stepped;
	}

	void processTasks()
	{
		FASTLOG(FLog::DataModelJobs, "Process tasks start");
		Task task;
		int size = tasks.size();
		int count = 0;
		while ((count < size) && (tasks.pop_if_present(task))) // need to keep count because task can add more tasks
		{
			step(task);
			count++;

			if (TaskScheduler::singleton().isCyclicExecutive())
			{
				continue;
			}

			RBX::Time timeNow = RBX::Time::now<RBX::Time::Fast>();
			if (TaskScheduler::priorityMethod != TaskScheduler::FIFO)
				break;
			else if (tasks.head_waittime_sec(timeNow) < 0.2)
			{
				// FIFO mode: if we are able to process incoming tasks in a timely manner, process remaining tasks next frame
				break;
			}
		}
		FASTLOG(FLog::DataModelJobs, "Process tasks finish");
	}
};

static std::string tempTag()
{
    static rbx::atomic<int> count = 0;
	return RBX::format("DataModel-%d", (int)++count);
}

bool DataModel::canSave(const RBX::Instance* instance)
{
	//If the UploadURL is empty, they don't have permissions to save.  Shut. It. Down.
	if(Visit* visit = ServiceProvider::find<Visit>(instance))
		return !visit->getUploadUrl().empty();

	return true;
}

bool DataModel::serverSavePlace(const SaveFilter saveFilter, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	// construct the url
	std::string parameters = "?assetId=" + boost::lexical_cast<std::string>(placeID);
	parameters += "&isAppCreation=true";
	std::string baseUrl = ServiceProvider::create<ContentProvider>(this)->getBaseUrl();

	// save the place
	return uploadPlace(baseUrl + "ide/publish/UploadExistingAsset" + parameters, saveFilter, boost::bind(resumeFunction, true), errorFunction);
}

namespace {
	static void sendSavePlaceStats()
	{
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "SavePlace");
	}
} // namespace

void DataModel::savePlaceAsync(const SaveFilter saveFilter, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (placeID <= 0)
	{
		errorFunction("Game:SavePlace placeID is not valid!");
		return;
	}

	if(!savePlaceThrottle.checkLimit())
	{
		errorFunction("Game:SavePlace requests limit reached");
		return;
	}

	if(Network::Players::frontendProcessing(this))
	{
		errorFunction("Game:SavePlace can only be called from a server script, aborting save function");
		return;
	}
	else if(Network::Players::backendProcessing(this))
	{
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendSavePlaceStats, flag);
		}
		serverSavePlace(saveFilter, resumeFunction, errorFunction);

	}
	else
	{
		errorFunction("Game:SavePlace could not determine if client or server");
		return;
	}
}

void DataModel::completeShutdown(bool saveLocal)
{
	if(shutdownRequestedCount > 0){
		if(saveLocal && canSave(this)){
			if(Verb* verb = workspace->getWhitelistVerb("Shutdown", "Client", "AndSave")){
                Verb::doItWithChecks(verb, NULL);
			}
		}
		else{
			if(Verb* verb = workspace->getWhitelistVerb("Shutdown", "Client", "")){
                Verb::doItWithChecks(verb, NULL);
			}
		}
	}
}

DataModel::RequestShutdownResult DataModel::requestShutdown(bool useLuaShutdownForSave)
{
	if(!canSave(this))
    {
        Verb* verb = getWorkspace()->getWhitelistVerb("Toggle", "Play", "Mode");
		if ( verb && verb->isChecked() )
			return CLOSE_LOCAL_SAVE;

		//We are in play mode... so don't save
		return CLOSE_NO_SAVE_NEEDED;	
	}

	Visit* visit = RBX::ServiceProvider::find<Visit>(this);
	if (!visit || visit->getUploadUrl().empty())
	{
		return CLOSE_LOCAL_SAVE;
	}

	if(useLuaShutdownForSave && requestShutdownCallback){
		try
		{
			shutdownRequestedCount++;
			return requestShutdownCallback() ? CLOSE_REQUEST_HANDLED : CLOSE_NOT_HANDLED;
		}
		catch(RBX::base_exception&)
		{
			return CLOSE_NOT_HANDLED;
		}
	}

	return CLOSE_NOT_HANDLED;
}

TaskScheduler::Arbiter* DataModel::getSyncronizationArbiter()
{
	return this;
}
    
void DataModel::doDataModelSetup(shared_ptr<DataModel> dataModel, bool startHeartbeat, bool shouldShowLoadingScreen)
{
	shared_ptr<DataModel::GenericJob> j;
	j = shared_ptr<DataModel::GenericJob>(new DataModel::GenericJob(dataModel, "Write Marshalled", DataModelJob::Write));
	j->cyclicExecutive = false;
	dataModel->genericJobs[DataModelJob::Write] = j;
	TaskScheduler::singleton().add(j);
	j = shared_ptr<DataModel::GenericJob>(new DataModel::GenericJob(dataModel, "Read Marshalled", DataModelJob::Read));
	j->cyclicExecutive = false;
	dataModel->genericJobs[DataModelJob::Read] = j;
	TaskScheduler::singleton().add(j);
	j = shared_ptr<DataModel::GenericJob>(new DataModel::GenericJob(dataModel, "None Marshalled", DataModelJob::None));
	j->cyclicExecutive = false;
	dataModel->genericJobs[DataModelJob::None] = j;
	TaskScheduler::singleton().add(j);

	// TODO: Do we need this lock here? If initializing has no side effects
	//       like starting a task, then it shouldn't be necessary.
	LegacyLock lock(dataModel, DataModelJob::Write);

	RBX::DataModel::LegacyLock::mainThreadId = GetCurrentThreadId();

	dataModel->initializeContents(startHeartbeat);
	dataModel->isInitialized = true;
	dataModel->suppressNavKeys = false;

	// get loading script going ASAP so it shows on first render
	if (shouldShowLoadingScreen && !(TeleportService::didTeleport() && TeleportService::getCustomTeleportLoadingGui()))
	{
		try
		{
			if (boost::optional<ProtectedString> source = CoreScript::fetchSource("LoadingScript"))
			{
				if (ScriptContext* sc = RBX::ServiceProvider::create<ScriptContext>(dataModel.get()))
				{
					sc->executeInNewThread(RBX::Security::RobloxGameScript_, *source, "LoadingScript");
				}
			}
		}
		catch(std::exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "LoadingScript Error: %s", e.what());
		}
	}

	FASTLOG1(FLog::CloseDataModel, "DataModel created: %p", dataModel.get());
}
    
shared_ptr<DataModel> DataModel::createDataModel(bool startHeartbeat, RBX::Verb* lockVerb, bool shouldShowLoadingScreen)
{
	FASTLOG1(FLog::CloseDataModel, "Create DataModel - heartbeat(%d)", startHeartbeat);
		
	shared_ptr<DataModel> dataModel = Creatable<Instance>::create<RBX::DataModel>(lockVerb);
	
	doDataModelSetup(dataModel, startHeartbeat, shouldShowLoadingScreen);
    return dataModel;
}

bool DataModel::onlyJobsLeftForThisArbiterAreGenericJobs() {
	std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
	TaskScheduler::singleton().getJobsInfo(jobs);
	std::vector<boost::shared_ptr<const TaskScheduler::Job> >::iterator itr =
		jobs.begin();

	while (itr != jobs.end()) {
		if ((*itr)->getArbiter().get() == this) {
			bool anyMatch = false;
			for (int i = 0; i < DataModelJob::TaskTypeMax && !anyMatch; ++i) {
				if (genericJobs[i] == *itr) {
					anyMatch = true;
				}
			}
			RBXASSERT(anyMatch);
			if (!anyMatch) {
				return false;
			}
		}
		itr++;
	}
	return true;
}

void DataModel::doCloseDataModel(shared_ptr<DataModel> dataModel)
{
	FASTLOG1(FLog::CloseDataModel, "doCloseDataModel - %p", dataModel.get());

	RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);

	// Turn off ChangeHistoryService to avoid unecessary recording of state
	if (ChangeHistoryService* ch = ServiceProvider::find<ChangeHistoryService>(dataModel.get()))
		ch->setEnabled(false);

	FASTLOG(FLog::CloseDataModel, "Raising close..");
	dataModel->raiseClose();	// TODO: Demote this to MFC-only project???

	RBXASSERT(dataModel->isInitialized);
	dataModel->isInitialized = false;

	FASTLOG(FLog::CloseDataModel, "Removing all players");
	if(Network::Players* players = ServiceProvider::find<Network::Players>(dataModel.get()))
		players->removeAllChildren();

	FASTLOG(FLog::CloseDataModel, "Clearing contents...");
	// Ensure that all content is wiped out
	dataModel->clearContents(false);


	FASTLOG(FLog::CloseDataModel, "Visiting children with unlockParent...");
	dataModel->visitChildren(boost::bind(&Instance::unlockParent, _1));

	FASTLOG(FLog::CloseDataModel, "Removing all Children...");
	dataModel->removeAllChildren();

	FASTLOG(FLog::CloseDataModel, "Resetting workspace...");
	dataModel->workspace.reset();		

	FASTLOG(FLog::CloseDataModel, "Resetting runService...");
	dataModel->runService.reset();

	FASTLOG(FLog::CloseDataModel, "Resetting starterPackService...");
	dataModel->starterPackService.reset();

	FASTLOG(FLog::CloseDataModel, "Resetting starterGuiService...");
	dataModel->starterGuiService.reset();

	FASTLOG(FLog::CloseDataModel, "Resetting starterPlayerService...");
	dataModel->starterPlayerService.reset();

	FASTLOG(FLog::CloseDataModel, "Resetting guiRoot...");
	dataModel->guiRoot.reset();

	FASTLOG(FLog::CloseDataModel, "Clearing services...");
	dataModel->clearServices();

	RBXASSERT(dataModel->onlyJobsLeftForThisArbiterAreGenericJobs());

	FASTLOG(FLog::CloseDataModel, "Removing GenericJobs...");
	{
		bool blockingRemove = DataModel::BlockingDataModelShutdown && RBX::DebugSettings::singleton().blockingRemove;
		for (GenericJobs::iterator iter = dataModel->genericJobs.begin(); iter!=dataModel->genericJobs.end(); ++iter)
		{
			if(blockingRemove)
				TaskScheduler::singleton().removeBlocking(*iter);
			else
				TaskScheduler::singleton().remove(*iter);
			iter->reset();
		}
	}

	FASTLOG(FLog::CloseDataModel, "Close DataModel Done!");
}

static void closeCallbackContinuation(shared_ptr<CEvent> event)
{
	FASTLOG(FLog::CloseDataModel, "Close Callback finished");
	FASTLOG1(FLog::CloseDataModel, "Close callback resets event - %p", event.get());
	event->Set();
}

static void errorCallbackContinuation(std::string error, shared_ptr<CEvent> event)
{
	FASTLOGS(FLog::CloseDataModel, "Close Callback error - %s", error);
	FASTLOG1(FLog::CloseDataModel, "Close callback resets event on error - %p", event.get());
	event->Set();
}


void DataModel::closeDataModel(shared_ptr<DataModel> dataModel)
{
	FASTLOG1(FLog::CloseDataModel, "closeDataModel - %p", dataModel.get());

	if (!dataModel)
		return;


	if(dataModel->onCloseCallback) 
	{
		FASTLOG1(FLog::CloseDataModel, "Setting up onClose callback - %p", dataModel.get());

		RBXASSERT(!dataModel->currentThreadHasWriteLock());
		shared_ptr<CEvent> waitEvent(new CEvent(false));

		{
			LegacyLock lock(dataModel, DataModelJob::Write);
			if(Workspace::clientIsPresent(dataModel.get()))
				waitEvent->Set();
			else
			{
				FASTLOG(FLog::CloseDataModel, "Calling onClose callback");
				dataModel->onCloseCallback(boost::bind(closeCallbackContinuation, waitEvent), boost::bind(errorCallbackContinuation, _1, waitEvent));
			}
		}

		bool result = waitEvent->Wait(Time::Interval((double)DFInt::OnCloseTimeoutInSeconds));
		FASTLOG1(FLog::CloseDataModel, "Wait returned %u", result);
	}

	LegacyLock lock(dataModel, DataModelJob::Write);
	FASTLOG(FLog::CloseDataModel, "Finished waiting on lock");
	doCloseDataModel(dataModel);
}

void DataModel::onCloseCallbackChanged(const CloseCallback& oldValue)
{
	if(onCloseCallback && oldValue)
	{
		onCloseCallback = oldValue;
		throw RBX::runtime_error("OnClose is already set");
	}
}

// TODO: Refactor: This class is HORRIBLE. GET RID OF THE BLOB
//                 Also, it is dangerous to initialize all this stuff in the contructor
//                 Because DataModel isn't owned by a shared_ptr yet. So, code that
//                 calls shared_from(dataModel) will fail
//                 My only idea at the moment to get around it is to have a DataModel::initializeContet
//                 function, or perhaps wrap the whole thing into a static "CreateDataModel" function
//                 that constructs DataModel, and creates essential services.
//                 All these verbs should be taken out and put into a UI library (which isn't needed by
//                 by the WebService, for example.
DataModel::DataModel(RBX::Verb* lockVerb) 
	:Super("Game"),
	VerbContainer(NULL),
	shutdownRequestedCount(0),
	lockVerb(lockVerb),
	write_requested(0),
	writeRequestingThread(0),
	read_requested(0),
	dirty(false),
	isInitialized(false),
	isContentLoaded(false),
    isShuttingDown(false),
	runningInStudio(false),
	isStudioRunMode(false),
	checkedExperimentalFeatures(false),
	drawId(0),
	placeID(0),
	gameInstanceID(""),
	placeVersion(0),
	creatorID(0),
	universeId(0),
	physicsStepID(0),
	totalWorldSteps(0),
	creatorType(DataModel::CREATOR_USER),
	remoteBuildMode(false),
	numPartInstances(0),
	numInstances(0),
	mouseOverGui(false),
	networkMetric(NULL),
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
	workspace(Creatable<Instance>::create<Workspace>(this)),
	guiRoot(Creatable<Instance>::create<GuiRoot>()),
	forceArrowCursor(true),
	isGameLoaded(false),
	areCoreScriptsLoaded(false),
	game(NULL),
    networkStatsWindowsOn(false),
#pragma warning(pop)
	dataModelInitTime(Time::nowFast()),
	savePlaceThrottle(&DFInt::SavePlacePerMinute),
	vipServerId(""),
	vipServerOwnerId(0),
	renderGuisActive(DFFlag::AllowHideHudShortcutDefault),
	canRequestUniverseInfo(false),
	universeDataRequested(false),
    forceR15(false)
{
	if (TaskScheduler::singleton().isCyclicExecutive())
	{
		TaskScheduler::singleton().DataModel30fpsThrottle = DataModel::throttleAt30Fps;
	}


	count++;
	lockParent();

	guiBuilder.Initialize(this);
}
		
bool DataModel::getRemoteBuildMode()
{
	return false;
}

void DataModel::setRemoteBuildMode(bool remoteBuildMode) 
{ 
}

void DataModel::setScreenshotSEOInfo(std::string value)
{
	screenshotSEOInfo = value;
}

void DataModel::setVideoSEOInfo(std::string value)
{
	videoSEOInfo = value;
}

std::string DataModel::getScreenshotSEOInfo()
{
	return screenshotSEOInfo;
}

// TODO: Refactor. this is gross. can we get away without it?  Or put all other stuff here, too?
void DataModel::initializeContents(bool startHeartbeat)
{
	RBXASSERT(!isInitialized);
	workspace->setAndLockParent(this);
	guiRoot->setAndLockParent(this);

	setIsPersonalServer(false);

	// Add basic services that are always there
	ServiceProvider::create<NonReplicatedCSGDictionaryService>();
	ServiceProvider::create<CSGDictionaryService>();
	ServiceProvider::create<LogService>();
	ServiceProvider::create<ContentProvider>();
	ServiceProvider::create<ContentFilter>();
	ServiceProvider::create<KeyframeSequenceProvider>();
	ServiceProvider::create<GuiService>();
	ServiceProvider::create<ChatService>();
	ServiceProvider::create<MarketplaceService>();
	ServiceProvider::create<PointsService>();
    ServiceProvider::create<AdService>();
    ServiceProvider::create<NotificationService>();
	ServiceProvider::create<ReplicatedFirst>();
	ServiceProvider::create<HttpRbxApiService>();

	starterPlayerService = shared_from(ServiceProvider::create<StarterPlayerService>());

	starterPackService = shared_from(ServiceProvider::create<StarterPackService>());

	starterGuiService = shared_from(ServiceProvider::create<StarterGuiService>());

	coreGuiService = shared_from(ServiceProvider::create<CoreGuiService>());
	coreGuiService->createRobloxScreenGui();

	if (TeleportService* ts = ServiceProvider::create<TeleportService>(this))
	{
		if (TeleportService::getCustomTeleportLoadingGui() && TeleportService::didTeleport())
		{
			if (RBX::ScreenGui* loadingGui = Instance::fastDynamicCast<RBX::ScreenGui>(TeleportService::getCustomTeleportLoadingGui().get()))
			{
				ts->setTempCustomTeleportLoadingGui(loadingGui->clone(EngineCreator));
				ts->getTempCustomTeleportLoadingGui()->setParent(coreGuiService->getRobloxScreenGui().get());
				ts->setCustomTeleportLoadingGui(loadingGui->clone(EngineCreator));
			}
		}
	}

	runService = shared_from(ServiceProvider::create<RunService>());

	ServiceProvider::create<Soundscape::SoundService>();
	
	workspace->setDefaultMouseCommand();

	workspace->setVerbParent(this);

	isPersonalServer = false;

	if (startHeartbeat)
		runService->start();

	// No need to save the connection. The run service will be deleted before we are
	runService->runTransitionSignal.connect(boost::bind(&DataModel::onRunTransition, this, _1));

	// TODO: Do we really need to pre-create these services? It adds #include hell to this file.
	//       Most services should be created on-demand, even in a Lua startup script, but not here!
	ServiceProvider::create<JointsService>();
	ServiceProvider::create<CollectionService>();
	ServiceProvider::create<PhysicsService>();
	ServiceProvider::create<BadgeService>();
	ServiceProvider::create<GeometryService>();
	ServiceProvider::create<FriendService>();
	ServiceProvider::create<RenderHooksService>();
	ServiceProvider::create<InsertService>();
	ServiceProvider::create<SocialService>();
	ServiceProvider::create<GamePassService>();
	ServiceProvider::create<DebrisService>();
	ServiceProvider::create<ScriptInformationProvider>();
	ServiceProvider::create<CookiesService>();
	ServiceProvider::create<TeleportService>();
	ServiceProvider::create<PersonalServerService>();
	ServiceProvider::create<Network::Players>();		// We always need this, because it saves state
    userInputService = shared_from(ServiceProvider::create<UserInputService>());
    contextActionService = shared_from(ServiceProvider::create<ContextActionService>()); // store a ref since we use this in processInputObject
	ServiceProvider::create<ScriptService>();
	ServiceProvider::create<AssetService>();

}

void DataModel::loadCoreScripts(const std::string &altStarterScript)
{
	if (!areCoreScriptsLoaded) 
	{
		areCoreScriptsLoaded = true;
		std::string baseUrl = ServiceProvider::create<ContentProvider>(this)->getBaseUrl();

		if (!baseUrl.empty() && baseUrl[baseUrl.size() - 1] != '/')
			baseUrl += '/';

		StudioConfigurer sc;
		sc.starterScript = altStarterScript;
		sc.configure(Security::COM, this, format("{ \"BaseUrl\": \"%s\" }", baseUrl.c_str()));
	}
}


void DataModel::startCoreScripts(bool buildInGameGui, const std::string &altStarterScript)
{
	// Build Gui - all verbs should be set 
	guiBuilder.buildGui(this->workspace.get(), buildInGameGui);
	loadCoreScripts(altStarterScript);
}


rbx::atomic<int> DataModel::count;

DataModel::~DataModel() 
{
	count--;
	if(placeID != 0)
		RbxDbgInfo::RemovePlace(placeID);
}



struct DataModel::LegacyLock::Implementation : boost::noncopyable
{
	// To marshal the task to this thread we use a pair of locks (acquire and release)
	// Since creating mutexes are expensive, we push spent Events into a pool
	// for re-use
	struct Events
	{
		CEvent acquiredLock;
		CEvent releasedLock;
		Events()
			:acquiredLock(false)
			,releasedLock(false)
		{}
	};
	SAFE_STATIC(rbx::safe_queue< shared_ptr<Events> >, eventsPool)

	// The job in this thread that has currently acquired a lock (if any)
	// This thread specific pointer is used to re-entrancy checks
	SAFE_STATIC(rbx::thread_specific_reference<GenericJob>, currentJob)

	DataModel* const dataModel;

	// The job being used for scheduling:
	GenericJob* job;
	// The previous job, if any, being used for scheduling:
	GenericJob* previousJob;
	// The mutexes used to marshal the task across threads:
	shared_ptr<Events> events;

	boost::scoped_ptr<DataModel::scoped_write_transfer> writeTransfer;

	static void task(shared_ptr<Events> events)
	{
		RBXPROFILER_SCOPE("Jobs", "LegacyLock");

		// Signal the thread that constructed the Implementation object to proceed
		events->acquiredLock.Set();

		// Wait for Implementation object to go out of scope (task complete)
		events->releasedLock.Wait();

		// Recycle the events pair
		eventsPool().push(events);
	}

	Implementation(DataModel* dataModel, boost::shared_ptr<GenericJob> job)
		:job(job.get())
		,dataModel(dataModel)
	{
		RBXASSERT(job);
		previousJob = currentJob().get();
		// Re-entrancy test. No need to wait if we're recursively locked on this DataModel
		if (this->job != previousJob)
		{
			FASTLOG2(FLog::LegacyLock, "LegacyLock::Begin, DataModel: (%p), type(%d)", dataModel, job->taskType);
			// Set the current job for this thread
			currentJob().reset(this->job);

			// Get/create a pair of events for marshalling
			if (!eventsPool().pop_if_present(events))
				events.reset(new Events()); 

			// Submit the proxy task for scheduling.
			job->tasks.push(boost::bind(&Implementation::task, events));
			TaskScheduler::singleton().reschedule(job);

			// Wait for the task to signal acquiredLock
			events->acquiredLock.Wait();

			if (job->taskType == DataModelJob::Write)
				writeTransfer.reset(new DataModel::scoped_write_transfer(dataModel));

			FASTLOG2(FLog::LegacyLock, "LegacyLock::Acquired type(%d), job (%s), events (%p)", job->taskType, events.get());
		}
		else
		{
			if (job->taskType == DataModelJob::Write)
				RBXASSERT(dataModel->currentThreadHasWriteLock());

			FASTLOG2(FLog::LegacyLock, "LegacyLock::RecursivelyAcquired DataModel: (%p), type(%d)", dataModel, job->taskType);
		}
	}

	~Implementation()
	{
		FASTLOG2(FLog::LegacyLock, "LegacyLock::~Implementation type(%d), events (%p)", job->taskType, events.get());

		if (job != previousJob)
		{
			if (job->taskType == DataModelJob::Write)
				writeTransfer.reset();

			// The task has completed, so undo state
			currentJob().reset(previousJob);
			// Signal the proxy task that we're done
			events->releasedLock.Set();
			FASTLOG2(FLog::LegacyLock, "LegacyLock::Released DataModel: (%p), type(%d)", dataModel, job->taskType);
		}
		else
		{
			if (job->taskType == DataModelJob::Write)
				RBXASSERT(dataModel->currentThreadHasWriteLock());

			FASTLOG2(FLog::LegacyLock, "LegacyLock::RecursivelyReleased DataModel: (%p), type(%d)", dataModel, job->taskType);
		}
	}
};



DataModel::LegacyLock::Impersonator::Impersonator(shared_ptr<DataModel> dataModel, DataModelJob::TaskType taskType)
	:previousJob(Implementation::currentJob().get())
{
	Implementation::currentJob().reset(dataModel->getGenericJob(taskType).get());
}
DataModel::LegacyLock::Impersonator::~Impersonator()
{
	Implementation::currentJob().reset(previousJob);
}

DataModel::LegacyLock::LegacyLock(boost::shared_ptr<DataModel> dataModel, DataModelJob::TaskType taskType)
	// TODO: What if an invalid DataModelJob::Function is passed in???
	:implementation(dataModel ? new Implementation(dataModel.get(), dataModel->getGenericJob(taskType)) : 0)
{
    //assert(GetCurrentThreadId() != mainThreadId);
}

DataModel::LegacyLock::LegacyLock(DataModel* dataModel, DataModelJob::TaskType taskType)
	:implementation(dataModel ? new Implementation(dataModel, dataModel->getGenericJob(taskType)) : 0) 
{
    //assert(GetCurrentThreadId() != mainThreadId);
}

DataModel::LegacyLock::~LegacyLock()
{
}

shared_ptr<DataModel::GenericJob> DataModel::tryGetGenericJob(DataModelJob::TaskType taskType)
{
	RBX::mutex::scoped_lock lock(genericJobsLock);
	return genericJobs[taskType];
}

shared_ptr<DataModel::GenericJob> DataModel::getGenericJob(DataModelJob::TaskType taskType)
{
	shared_ptr<DataModel::GenericJob> job = tryGetGenericJob(taskType);
	if (!job)
	{
		// This can also happen if the job is requested after the data model is closed.
		throw std::runtime_error("Can't submit requested task type");
	}
	return job;
}

void DataModel::submitTask(Task task, DataModelJob::TaskType taskType)
{
	if (shared_ptr<GenericJob> job = tryGetGenericJob(taskType))
	{
		job->tasks.push(task);
		TaskScheduler::singleton().reschedule(job);
	}
}

void DataModel::raiseClose()
{
	closingSignal();
	closingLateSignal();
}

void DataModel::traverseDataModelReporting(shared_ptr<Instance> node)
{	
	std::string className = node->getClassNameStr();
	if(dataModelReportingData.find(className) == dataModelReportingData.end()){
		dataModelReportingData[className] = 0;
	}
	dataModelReportingData[className]++;
}

void DataModel::setServerSaveUrl(std::string url)
{
	serverSaveUrl = url;
}

bool DataModel::canSaveLocal() const
{
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

void DataModel::saveToRoblox(boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(canSave(this)){
		if (Visit* visit = RBX::ServiceProvider::find<Visit>(this))
		{
			std::string uploadUrl = visit->getUploadUrl();
			if(uploadUrl.find("http") == 0){
				//needs to start with http
				internalSaveAsync(ContentId(uploadUrl), resumeFunction);
				return;
			}
		}
	}

	resumeFunction(false);
}

void DataModel::save(ContentId contentId)
{
	if(canSave(this)){
		internalSave(contentId);
	}
	saveFinishedSignal();
}


void DataModel::HttpHelper(std::string* response, std::exception* exception, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(response){
		resumeFunction(*response);
	}
	else{
		errorFunction(exception->what());
	}
}

void DataModel::doHttpGet(const std::string& url, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBX::Http(url).get(boost::bind(&HttpHelper, _1, _2, resumeFunction, errorFunction));
}

std::string DataModel::doHttpGet(const std::string& url)
{
	std::string response;
	RBX::Http(url).get(response);
	return response;
}

std::string DataModel::doHttpPost(const std::string& url, const std::string& data, const std::string& contentType)
{
	std::string response;
	std::istringstream stream(data);
	Http(url).post(stream, contentType, data.size() > 256, response);
	return response;
}

void DataModel::doHttpPost(const std::string& url, const std::string& data, const std::string& contentType, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	Http(url).post(data, contentType, data.size() > 256,
		boost::bind(&HttpHelper, _1, _2, resumeFunction, errorFunction));
}

static void httpResumeCallback(const std::string& response)
{
}

static void httpErrorCallback(const std::string& url, const std::string& error)
{
    StandardOut::singleton()->printf(MESSAGE_ERROR, "%s: %s", url.c_str(), error.c_str());
}

std::string DataModel::httpGet(std::string url, bool synchronous)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	robloxScriptModifiedCheck(RBX::httpGetFunction.security);
	if (url.size()>0)
	{
		if (synchronous)
        {
			return doHttpGet(url);
		}
		else 
		{
            doHttpGet(url, httpResumeCallback, boost::bind(httpErrorCallback, url, _1));
		}
	}
	return "";
}
void DataModel::httpGetAsync(std::string url, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	if (url.size() == 0)
	{
		errorFunction("Empty URL");
		return;
	}
	robloxScriptModifiedCheck(RBX::httpGetAsyncFunction.security);
	doHttpGet(url, resumeFunction, errorFunction);
}

void DataModel::reportMeasurement(std::string id, std::string key1, std::string value1, std::string key2, std::string value2)
{
	ReportStatistic(GetBaseURL(), id, key1, value1, key2, value2);
}

std::string DataModel::httpPost(std::string url, std::string data, bool synchronous, std::string optionalContentType)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue

	robloxScriptModifiedCheck(RBX::httpPostFunction.security);
	if (synchronous)
	{
		return doHttpPost(url, data, optionalContentType);
	}
	else
	{
		doHttpPost(url, data, optionalContentType, httpResumeCallback, boost::bind(httpErrorCallback, url, _1));
		return "";
	}
}

void DataModel::httpPostAsync(std::string url, std::string data, std::string optionalContentType, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	
	if (url.size() == 0)
	{
		errorFunction("Empty URL");
		return;
	}

	robloxScriptModifiedCheck(RBX::httpPostAsyncFunction.security);
	doHttpPost(url, data, optionalContentType, resumeFunction, errorFunction);
}

void DataModel::luaReportGoogleAnalytics(std::string category, std::string action, std::string label, int value)
{
	std::string encodedCategory = Http::urlEncode(category);
	std::string encodedAction = Http::urlEncode(action);
	std::string encodedLabel = Http::urlEncode(label);
	RobloxGoogleAnalytics::trackEvent(encodedCategory.c_str(), encodedAction.c_str(), encodedLabel.c_str(), value);
}

std::auto_ptr<std::istream> DataModel::loadAssetIdIntoStream(int assetID)
{
	// construct the url
	std::string parameters = "asset/?id=" + boost::lexical_cast<std::string>(assetID);
	std::string url = ServiceProvider::create<ContentProvider>(this)->getBaseUrl() + parameters;

	StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "DataModel loading from: %s", url.c_str());

	return std::auto_ptr<std::istream>(ServiceProvider::create<ContentProvider>(this)->getContent( ContentId(url) ));
}

void DataModel::loadWorld(int assetID)
{
	if(!Network::Players::backendProcessing(this))
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING, "Game:LoadWorld should only be called from a server script");
		return;
	}

	std::auto_ptr<std::istream> stream = loadAssetIdIntoStream(assetID);

	if(workspace)
	{
		workspace->clearTerrain();
		workspace->removeAllChildren();
	}

	if (Lighting* lighting = ServiceProvider::find<Lighting>(this))
		lighting->removeAllChildren();
	if (Soundscape::SoundService* service = ServiceProvider::find<Soundscape::SoundService>(this))
		service->removeAllChildren();
	if (ServerStorage* service = ServiceProvider::find<ServerStorage>(this))
		service->removeAllChildren();
	if (ReplicatedStorage* service = ServiceProvider::find<ReplicatedStorage>(this))
		service->removeAllChildren();
	if (RobloxReplicatedStorage* service = ServiceProvider::find<RobloxReplicatedStorage>(this))
		service->removeAllChildren();
	if (ReplicatedFirst* service = ServiceProvider::find<ReplicatedFirst>(this))
		service->removeAllChildren();

	Serializer serializer;
	serializer.load(*stream, this);
	LuaSourceContainer::blockingLoadLinkedScripts(create<ContentProvider>(), this);

	processAfterLoad();
}

void DataModel::loadGame(int assetID)
{
	if(!Network::Players::backendProcessing(this))
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING, "Game:LoadWorld should only be called from a server script");
		return;
	}

	std::auto_ptr<std::istream> stream = loadAssetIdIntoStream(assetID);

	if (ServerScriptService* service = ServiceProvider::find<ServerScriptService>(this))
		service->removeAllChildren();
	if (starterGuiService)
		starterGuiService->removeAllChildren();
	if (starterPackService)
		starterPackService->removeAllChildren();
	if (starterPlayerService)
		starterPlayerService->removeAllChildren();

	Serializer serializer;
	serializer.load(*stream, this);
	LuaSourceContainer::blockingLoadLinkedScripts(create<ContentProvider>(), this);
}

void DataModel::loadContent(ContentId contentId)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue

	if (isContentLoaded)
	{
		Analytics::GoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_ERROR, "loadContent re-entrant", contentId.getAssetId().c_str());
		return;
	}

	StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "DataModel Loading %s", contentId.c_str());

	G3D::RealTime t1 = G3D::System::time(); // time in seconds
	std::auto_ptr<std::istream> stream(ServiceProvider::create<ContentProvider>(this)->getContent(contentId, "Place"));
	G3D::RealTime t2 = G3D::System::time();

    // post-load check on RCC.  An exploit that can overwrite pointers on RCC could jump to the
    // middle of this function.
	if (isContentLoaded)
	{
        stream.release();
		Analytics::GoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_ERROR, "loadContent re-entrant post", contentId.getAssetId().c_str());
		return;
	}

	Serializer serializer;
	serializer.load(*stream, this);
	LuaSourceContainer::blockingLoadLinkedScripts(create<ContentProvider>(), this);

	G3D::RealTime t3 = G3D::System::time();
	processAfterLoad();
	G3D::RealTime t4 = G3D::System::time();
    
	workspace->setStatsSyncHttpGetTime(t2 - t1);
	workspace->setStatsXMLLoadTime(t3 - t2);
	workspace->setStatsJoinAllTime(t4 - t3);

	workspace->createTerrain();
	workspaceLoadedSignal();

	isContentLoaded = true; // there should be a better way to do this.

}

void DataModel::processAfterLoad()
{
	workspace->joinAllHack();

	if (DFFlag::MaterialPropertiesEnabled)
	{
		// In case we have decided to make New Physical Properties the default, 
		// we want to migrate parts by default.
		if (workspace->getUsingNewPhysicalProperties())
		{
			RBX::PartInstance::convertToNewPhysicalPropRecursive(this);
		}
	}
}

#if defined(RBX_STUDIO_BUILD) || defined(RBX_RCC_SECURITY) || defined(RBX_TEST_BUILD)
shared_ptr<const Instances> DataModel::fetchAsset(ContentId contentId) 
{
	RBXASSERT(isInitialized);    // If hit show to David or Erik - threading issue

	ContentProvider* cp = create<ContentProvider>();
	std::auto_ptr<std::istream> stream(cp->getContent(contentId));

	// Return all the new items in a table
	shared_ptr<Instances> result(new Instances());

	// Read the items
	Serializer().loadInstances(*stream, *result);
	LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, *result);

	return result;
}
#endif

void DataModel::onChildAdded(Instance* child)
{
	Super::onChildAdded(child);

	if (Network::Players::isNetworkClient(child))  
	{
		// TODO: Hack
		// For now, this hook sets the mouse command to the NULL command, regardless of whether there is a player/character or not
		// This prevents the arrow tool from coming up on a multiplayer load
		workspace->setNullMouseCommand();
	}
}


bool DataModel::askAddChild(const Instance* instance) const
{
	return dynamic_cast<const Service*>(instance)!=NULL;
}


double DataModel::getGameTime() const
{
	RBXASSERT(runService);
	return runService->gameTime();
}



double DataModel::getSmoothFps() const
{
	RBXASSERT(runService);
	return runService->smoothFps();
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Rendering
//

void DataModel::computeGuiInset(Adorn* adorn)
{	

	Vector4 guiInset;
	if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(this))
	{
		guiInset = guiService->getGlobalGuiInset(); 
	}
	else
	{
		guiInset = Vector4(0, 0, 0, 0);
	}

	adorn->setUserGuiInset(guiInset);
}

void DataModel::renderPlayerGui(Adorn* adorn)
{
	RBX::Network::Player* player = RBX::Network::Players::findLocalPlayer(this);
	if ( !Network::Players::isCloudEdit(this) && player ) 
	{
		for (size_t i = 0; i < player->numChildren(); i++) 
		{
			Instance *child = player->getChild(i);
			if (PlayerGui* playergui = Instance::fastDynamicCast<PlayerGui>(child))
				playergui->render2d(adorn);
		}
	}
	else
		starterGuiService->render2d(adorn);
}

void DataModel::renderGuiRoot(Adorn* adorn)
{
    guiBuilder.updateGui();
	guiRoot->render2d(adorn);
}

std::string DataModel::getUpdatedMessageBoxText()
{
		std::string printMessage = "";
		if (uiMessage == "[[[progress]]]")
		{
            printMessage =		"Loading...   Instances: "
                            + getMetric("numInstances")
                            +   "   Bricks: "  
							+ getMetric("numPrimitives")
							+	"   Connectors: "
							+	getMetric("numJoints");

			MegaClusterInstance* terrain = Instance::fastDynamicCast<MegaClusterInstance>(
				workspace->getTerrain());
			unsigned int cellCount = 0;
			if (terrain) {
				cellCount = terrain->getNonEmptyCellCount();
			}
			if (cellCount > 0) {
				std::string result(15, '\0');
				snprintf(&result[0], 15, "%.3f mil", cellCount / 1000000.0f);
		
				printMessage += "   Voxels: " + result;
			}
		}
		else if(uiMessage != "[[[progress]]]")
			printMessage = uiMessage;

		return printMessage;
}

bool DataModel::canRenderMouse()
{
	if (userInputService->getLastInputType() == InputObject::TYPE_TOUCH)
	{
		return false;
	}

	if (userInputService->getOverrideMouseIconBehavior() == UserInputService::OVERRIDE_BEHAVIOR_FORCEHIDE)
	{
		return false;
	}
	else if (userInputService->getOverrideMouseIconBehavior() == UserInputService::OVERRIDE_BEHAVIOR_FORCESHOW)
	{
		return true;
	}

	if(!userInputService->getMouseIconEnabled())
	{
		return false;
	}

	bool playerExists = (RBX::Network::Players::findLocalPlayer(this) != NULL) && !Network::Players::isCloudEdit(this);

	if ( UserInputService::isTenFootInterface() )
	{
		return userInputService->getMouseIconEnabled();
	}

	return (!RBX::MouseCommand::isAdvArrowToolEnabled() || playerExists) && userInputService->getMouseEnabled();
}

void DataModel::renderPass2d(Adorn* adorn, IMetric* graphicsMetric) 
{
	RBXPROFILER_SCOPE("Render", "Pass2d");

	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue

	if (!isInitialized)
		return;
	if (!adorn)
		return;
	if (!graphicsMetric)
		return;

	tempMetric = graphicsMetric;

	//compute header/footer to restrict the Gui (used for MFC chat, todo:remove this when we have lua chat)
	computeGuiInset(adorn);

	// 1. Render 2D world items (names, cursors, ScreenGuis, user items in workspace, etc.)
	workspace->render2d(adorn);

	if (!DFFlag::AllowHideHudShortcut || renderGuisActive)
	{
		// 2. Render the User created gui - either inside the PlayerGui, or the StarterGui
		renderPlayerGui(adorn);

		// 3. Render the ROBLOX gui elements
		coreGuiService->render2d(adorn);

		// 4. Render old school guiRoot (don't add anything to this, only debug stats are shown here: Shift-F1 and Shift-F2)
		renderGuiRoot(adorn);

		// 5. Finally, render the mouse
		renderMouse(adorn);
	}

	tempMetric = NULL;	// be safe, never use again unless set
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
}

ContentId DataModel::getRenderMouseCursor()
{
	if (forceArrowCursor)
	{
		if (!MouseCommand::isAdvArrowToolEnabled() || 
			(RBX::Network::Players::findLocalPlayer(this) || Network::Players::serverIsPresent(this)))
		{
			return userInputService->getDefaultMouseCursor(true);
		}
		else 
		{
			return userInputService->getDefaultMouseCursor(true);
		}
	} 
	else
	{
		ContentId workspaceCursor = workspace->getCursor();
		if (workspaceCursor.isNull())
		{
			return userInputService->getDefaultMouseCursor(true);
		}
		return workspaceCursor;
	}
}

void DataModel::renderMouse(Adorn* adorn)
{
	if (Profiler::isCapturingMouseInput())
		return;

	ControllerService* service = ServiceProvider::create<ControllerService>(this);
	if(!service)
		return;

	if (UserInputService::IsUsingNewKeyboardEvents())
	{
		// don't need userinputbase for SDL
	}
	else
	{
#ifndef RBX_PLATFORM_UWP
		UserInputBase* hardwareDevice = service->getHardwareDevice();
        if (hardwareDevice)
        {
		hardwareDevice->setCursorId(adorn, getRenderMouseCursor());
#endif
	}
    }

	if (!canRenderMouse())
	{
		return;
	}

    bool waiting = false;

    // lazy connect to signal
    if (!unbindResourceSignal.connected())
    {
        unbindResourceSignal = adorn->getUnbindResourcesSignal().connect(boost::bind(&DataModel::onUnbindResourceSignal, this));
    }
    
    renderCursor = adorn->createTextureProxy(getRenderMouseCursor(), waiting, false, "Mouse Cursor");
	if (!renderCursor && userInputService)
	{
		renderCursor = adorn->createTextureProxy(userInputService->getDefaultMouseCursor(false), waiting);
	}

    if (renderCursor && userInputService->getCurrentMousePosition())
    {
        Rect2D rect = adorn->getTextureSize(renderCursor);

        // Center the texture on the cursor position
        rect = rect - rect.center() + userInputService->getCurrentMousePosition()->get2DPosition();
			
		rect = Rect2D::xyxy( Math::roundVector2(rect.x0y0()), Math::roundVector2(rect.x1y1()) );

        adorn->setTexture(0, renderCursor);
        adorn->rect2d(rect, G3D::Color4(1,1,1,1));
        adorn->setTexture(0, TextureProxyBaseRef());
    }
}
    
void DataModel::onUnbindResourceSignal()
{
    renderCursor.reset();
    unbindResourceSignal.disconnect();
}

void DataModel::renderPass3dAdorn(Adorn* adorn)
{
	RBXPROFILER_SCOPE("Render", "Pass3dAdorn");

	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	if (!isInitialized) {
		return;
	}

	// a couple of local variables for the ctrl-F1 hud
	drawId++;

	//
	if (!adorn) {
		RBXASSERT(0);
		return;
	}

	std::vector<AdornableDepth> sortedAdorn;
	workspace->render3dAdorn(adorn);
	workspace->append3dSortedAdorn(sortedAdorn);

	RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this);
	if (RBX::Network::Player* player = players->getLocalPlayer()) {
		if (PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>()) {
			playerGui->render3dAdorn(adorn);
			playerGui->append3dSortedAdorn(sortedAdorn, workspace->getConstCamera());
		}
	}
	else{
		starterGuiService->render3dAdorn(adorn);
		starterGuiService->append3dSortedAdorn(sortedAdorn, workspace->getConstCamera());
	}

	coreGuiService->render3dAdorn(adorn);
	coreGuiService->append3dSortedAdorn(sortedAdorn, workspace->getConstCamera());

	std::sort(sortedAdorn.begin(), sortedAdorn.end());

	FASTLOG1(FLog::AdornRenderStats, "Rendering 3D Sorted Adorn Items, %u items", sortedAdorn.size());

	for (auto& ad: sortedAdorn)
		ad.adornable->render3dSortedAdorn(adorn);

	if (!forceArrowCursor) {
		workspace->getCurrentMouseCommand()->render3dAdorn(adorn);
	}
	
	if (Workspace::showEPhysicsRegions) {
		ServiceProvider::create<RBX::Network::Players>(this)->renderDPhysicsRegions(adorn);
	}

	if (Workspace::showStreamedRegions)
	{
        adorn->setObjectToWorldMatrix(CoordinateFrame());
		if (RBX::Network::Player* player = players->getLocalPlayer())
			player->renderStreamedRegion(adorn);
	}

    if (Workspace::showPartMovementPath)
    {
        adorn->setObjectToWorldMatrix(CoordinateFrame());
        if (RBX::Network::Player* player = players->getLocalPlayer())
            player->renderPartMovementPath(adorn);
    }

	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
}

//Updates non-reflected state for camera and whatnot
void DataModel::renderStep(float timeIntervalSeconds)
{
	RBXPROFILER_SCOPE("Render", "Step");

	if (runService)
	{
		runService->earlyRenderSignal();
	}

    RBXASSERT(workspace);
    if (FFlag::RelativisticCameraFixEnable && workspace)
    {
        if (Camera* camera = workspace->getCamera())
        {
            // Relativistic Bug (DE9363): 
            //
            // Camera was being updated after the UI processing occurred, however the SurfaceGUI processing (which is part of the UI pipeline) required
            // updated camera to perform raycasts. This resulted in a bug where the raycast would use the camera from previous frame - not good if the 
            // camera was attached to a moving object with SurfaceGUIs on it.
            //
            // The correct way would be to split out the UI processing that modifies the camera (e.g. movement, panning) into a separate call and do that before
            // the camera update, and perform the rest afterwards.
            //
            // So this ugly hack will hang along for some indeterminate amount of time... Or perhaps for eternity.

            camera->step(timeIntervalSeconds);
            userInputService->onRenderStep();
            camera->step(0);
        }
    }
    else
    {
        userInputService->onRenderStep();
    }

	if (runService)
	{
		runService->renderStepped(timeIntervalSeconds, false);
	}

	// Need to step subject update after lua controls
	if (Camera* camera = workspace->getCamera())
	{
		camera->stepSubject();
	}
}


float DataModel::physicsStep(float timeInterval, double dt, double dutyDt, int numThreads)
{
	if (!isInitialized) {
		RBXASSERT(0);
		return 0.0f;					// 12/05/07 - possibly seeing this being called during destruction in other thread?
	}

// On Mac there is nothing equivalent of _CrtCheckMemory to validate the state of the heap
// Instead use this on XCode http://developer.apple.com/library/ios/#documentation/Performance/Conceptual/ManagingMemory/Articles/MallocDebug.html
#ifdef _WIN32
	RBXASSERT_IF_VALIDATING(_CrtCheckMemory());
#endif
	bool isCyclicExecutive = RBX::TaskScheduler::singleton().isCyclicExecutive();
	float dtCyclicExecutive = 0.0f;
	if( isCyclicExecutive )
	{
		int nSteps = workspace->updatePhysicsStepsRequiredForCyclicExecutive(dt);
		if( nSteps == 0 && DataModel::throttleAt30Fps)
		{
			return 0.0f;
		}

		dtCyclicExecutive = (float)nSteps * Constants::worldDt();
	}

	// TaskSchedulerCyclicExecutive:  This should simply be triggered after every
	// 1/30s.  Not changing it just yet.

	bool longStep =  physicsStepID % 2 == 0;

	Profiling::Mark mark(*workspace->profileDataModelStep, true, true);

	float timeIntervalUsed = 0.0f;

	// Physics Step
	RBX::Network::GameMode gameMode = Network::Players::getGameMode(this);
	if (updatePhysicsInstructions(gameMode))
	{
		if ( (gameMode == Network::DPHYS_CLIENT || gameMode == Network::CLIENT || gameMode == Network::VISIT_SOLO ) && !isCyclicExecutive )
		{
			runService->gameStepped( Constants::uiDt(), longStep);
		}
		else if (longStep && !isCyclicExecutive)
		{
			runService->gameStepped( Constants::longUiStepDt(), true);
		}
		else if (isCyclicExecutive)
		{
			runService->gameStepped(dtCyclicExecutive, true);
		}

		{
			RBX::Profiling::Mark mark2(*workspace->profileWorkspaceAssemble, true);
			workspace->assemble();		// must do assembly here, because "handle fallen parts" may disassemble.  Also, runService->gameStepped may disassemble
		}
		{	
			Profiling::Mark mark2(*workspace->profileWorkspaceStep, true, true);
			
			Network::Player* dPhysPlayer = (gameMode == Network::DPHYS_CLIENT) ? Network::Players::findLocalPlayer(this) : NULL;
			
			FASTLOG3F(FLog::CyclicExecutiveThrottling, "DataModel::PhysicsStep dt: %f, dtCyclicExecutive: %f, dutyDt: %f", dt, dtCyclicExecutive, dutyDt);
			if (isCyclicExecutive)
			{
				physicsInstructions.setCyclicThrottles(dPhysPlayer, workspace.get(), dtCyclicExecutive, dt, dutyDt);
			}
			else
			{
				physicsInstructions.setThrottles(dPhysPlayer, workspace.get(), dt, dutyDt);
			}
			timeIntervalUsed = workspace->physicsStep(longStep, timeInterval, numThreads);
			FASTLOG1F(FLog::CyclicExecutiveTiming, "DataModel::physicsStepped: %4.7f", timeIntervalUsed);
		}
	}
	else
	{
		// Assemble always
		RBX::Profiling::Mark mark2(*workspace->profileWorkspaceAssemble, true);
		workspace->assemble();
	}

    if (gameMode == RBX::Network::DPHYS_GAME_SERVER && (physicsStepID % 4) == 0)
    {
        // only server records the movement history, once every 3 physics steps
        workspace->updateHistory();
    }
	
	++physicsStepID;

// On Mac there is nothing equivalent of _CrtCheckMemory to validate the state of the heap
// Instead use this on XCode http://developer.apple.com/library/ios/#documentation/Performance/Conceptual/ManagingMemory/Articles/MallocDebug.html
#ifdef _WIN32
	RBXASSERT_IF_VALIDATING(_CrtCheckMemory());
#endif

	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue

	return timeIntervalUsed;
}




// TODO: make these objects
bool DataModel::updatePhysicsInstructions(Network::GameMode gameMode)
{
	SimSendFilter& simSendFilter = workspace->getWorld()->getSimSendFilter();
	simSendFilter.networkAddress = RBX::Network::Players::findLocalSimulatorAddress(this);
	physicsInstructions.requestedDutyPercent = 0.0;
	physicsInstructions.bandwidthExceeded = false;

	switch (gameMode)
	{
	case Network::VISIT_SOLO:
	case Network::EDIT:
		{
			RBXASSERT(simSendFilter.networkAddress == Network::NetworkOwner::Unassigned());
			simSendFilter.mode = SimSendFilter::EditVisit;
			physicsInstructions.requestedDutyPercent = PhysicsInstructions::visitSoloDutyPercent();
			return (runService->getRunState() == RS_RUNNING);
		}
	case Network::GAME_SERVER:	
		{
			RBXASSERT(simSendFilter.networkAddress == Network::NetworkOwner::Unassigned());
			simSendFilter.mode = SimSendFilter::Server;
			physicsInstructions.requestedDutyPercent = PhysicsInstructions::regularServerDutyPercent();
			return (runService->getRunState() == RS_RUNNING);
		}
	case Network::DPHYS_GAME_SERVER:	
		{
			RBXASSERT(simSendFilter.networkAddress == Network::NetworkOwner::Server());
			simSendFilter.mode = SimSendFilter::dPhysServer;
			simSendFilter.region.clearEmpty();
			physicsInstructions.requestedDutyPercent = PhysicsInstructions::dPhysicsServerDutyPercent();
			return (runService->getRunState() == RS_RUNNING);
		}
	case Network::CLIENT:
	case Network::WATCH_ONLINE:
		{
			RBXASSERT(simSendFilter.networkAddress == Network::NetworkOwner::Unassigned());
			simSendFilter.mode = SimSendFilter::Client;
			physicsInstructions.requestedDutyPercent = PhysicsInstructions::zeroDutyPercent();
			return false;
		}
	case Network::DPHYS_CLIENT:
		{
			RBXASSERT(Network::Players::clientIsPresent(this, true));
			simSendFilter.mode = SimSendFilter::dPhysClient;
			ServiceProvider::create<RBX::Network::Players>(this)->buildClientRegion(simSendFilter.region);
			physicsInstructions.requestedDutyPercent = PhysicsInstructions::dPhysicsClientEThrottleDutyPercent();

			physicsInstructions.bandwidthExceeded = Network::Player::physicsOutBandwidthExceeded(this);
			physicsInstructions.networkBufferHealth = Network::Player::getNetworkBufferHealth(this);
			return true;
		}
	default:
		{
			RBXASSERT(0);
			return false;
		}
	}
}

void DataModel::checkFetchExperimentalFeatures()
{
	if(getPlaceID() == 0)
	{
		FASTLOG(FLog::CloseDataModel, "Place Id is zero, skipping fetch");
		return;
	}

	if(checkedExperimentalFeatures)
		return;

	ContentProvider* cp = find<ContentProvider>();
	if(!cp)
	{
		RBXASSERT(false);
		return;
	}
	
	std::string response;

	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		apiService->get(RBX::format("game/GetAllowedExperimentalFeatures?placeId=%i", getPlaceID()), true, PRIORITY_EXTREME, response);
	}

	std::stringstream rawStream(response);
	shared_ptr<const Reflection::ValueTable> result;
	if(!WebParser::parseJSONTable(response, result))
	{
		FASTLOG(FLog::CloseDataModel, "Can't parse JSON");
		RBXASSERT(false);
		checkedExperimentalFeatures = true;
		return;
	}

	checkedExperimentalFeatures = true;
}

void DataModel::onRunTransition(RunTransition event)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	if (!isInitialized) {
		return;
	}

	switch (event.newState)
	{
	case RS_STOPPED:
		// only compiles in if we have a timer

		if (event.oldState == RS_RUNNING) {
			workspace->reset();
		}
		break;

	case RS_RUNNING:
		workspace->start();
		break;

	case RS_PAUSED:
		workspace->stop();
		break;
	}
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
}

///////////////////////////////////////////////////////////////////
// inquiry....

static void notificationCallbackError(std::string error)
{
}

static void notificationCallbackSuccess(DataModel* dm)
{
	dm->setRenderGuisActive(false);
}

GuiResponse DataModel::processAccelerators(const shared_ptr<InputObject>& event)		// true if used the event
{
	Verb* verb = NULL;
	bool preventSinking = false;

    if (networkStatsWindowsOn)
    {
        switch (event->getUserInputType())
        {
            case InputObject::TYPE_KEYBOARD:
                {
					// we only want key down events
					if(!event->isKeyDownEvent())
						break;

					switch (event->getKeyCode())
					{
					case SDLK_1:
						if (event->isKeyPressedWithShiftEvent(event->getKeyCode()))
							guiBuilder.nextNetworkStats();
						break;
					default:
						break;
					}
                }
            default:
                break;
        }
    }

	switch (event->getUserInputType())
	{
	case InputObject::TYPE_KEYBOARD:
		{
			RBXASSERT(event->isKeyDownEvent() || event->isKeyUpEvent());

			if(event->isKeyUpEvent())
			{
				switch(event->getKeyCode())
				{
				case SDLK_LALT:
				case SDLK_RALT:
					RBX::GameBasicSettings::singleton().setFreeLook(false);
					break;
				default:
					RBX::GameBasicSettings::singleton().setFreeLook(false);
					break;
				}
				break;
			}

			// if not keydown at this point, bail out
			if(!event->isKeyDownEvent())
				break;

			{
				if (ServiceProvider::find<GuiService>()->processKeyDown(event))
				{
					return GuiResponse::sunk();
				}
				else 
				{
					bool isCustomCameraType = false;
					if (const Camera* camera = workspace->getConstCamera())
					{
						isCustomCameraType = (camera->getCameraType() == Camera::CUSTOM_CAMERA);
					}
					bool shouldDoCameraVerbs = !isCustomCameraType;
                    if (const Camera* camera = workspace->getConstCamera())
                    {
                        if (FFlag::UserAllCamerasInLua && camera->hasClientPlayer())
                        {
                            shouldDoCameraVerbs = false;
                        }
                    }

					switch (event->getKeyCode())
					{
					case SDLK_i:		
						if (shouldDoCameraVerbs)
						{
							verb = workspace->getWhitelistVerb("Camera", "Zoom", "In");	
						}
						break;
					case SDLK_o:			
						if (shouldDoCameraVerbs)
						{
							verb = workspace->getWhitelistVerb("Camera", "Zoom", "Out");		
						}
						break;
					case SDLK_COMMA:
						// Camera panning is hard-coded to be always enabled, so we just do a quick check before we get a camera panning verb, so this doesn't break the Scriptable Camera
						if (shouldDoCameraVerbs && workspace->getConstCamera()->getCameraType() != Camera::LOCKED_CAMERA)
						{
							verb = workspace->getWhitelistVerb("Camera", "Pan", "Left");		
						}
						break;
					case SDLK_PERIOD:		
						if (shouldDoCameraVerbs && workspace->getConstCamera()->getCameraType() != Camera::LOCKED_CAMERA) 
						{
							verb = workspace->getWhitelistVerb("Camera", "Pan", "Right");
						}
						break;
					case SDLK_PAGEUP:	
						if (shouldDoCameraVerbs)
						{
							verb = workspace->getWhitelistVerb("Camera", "Tilt", "Up"); 
						}  
						break;
					case SDLK_PAGEDOWN:		
						if (shouldDoCameraVerbs)
						{
							verb = workspace->getWhitelistVerb("Camera", "Tilt", "Down"); 
						}
						break;
					case SDLK_BACKSPACE:
						{
							if (RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this))
							{
								if (RBX::Network::Player* player = players->getLocalPlayer()) 
								{
									Tool::dropAll(player);
								}
							}
							break;
						}

					case SDLK_EQUALS:
						{
							if (ModelInstance* character = Network::Players::findLocalCharacter(this))
							{
								Accoutrement::dropAll(character);
							}
							break;
						}

					/**************************************/

					case SDLK_F1:
                        if ( !RBX::GameBasicSettings::singleton().inStudioMode() )
                        {
						    if (event->mod==0)
							    verb = getWhitelistVerb("H", "el", "p");
						    else
							    verb = getWhitelistVerb("S", "ta", "ts");
                        }
						break;

					case SDLK_F2:
						if ( !RBX::GameBasicSettings::singleton().inStudioMode() && event->isKeyPressedWithShiftEvent(event->getKeyCode()) )
							verb = getWhitelistVerb("", "Render", "Stats");
						/*else
							verb = getWhitelistVerb("TogglePlayMode");*/
						// TODO: turn on above statement when f2 studio hack no longer is an issue

						break;
					case SDLK_F3:
						if ( !RBX::GameBasicSettings::singleton().inStudioMode() && event->isKeyPressedWithShiftEvent(event->getKeyCode()) )
							verb = getWhitelistVerb("Network", "", "Stats");
						break;

					case SDLK_F4:
						if ( !RBX::GameBasicSettings::singleton().inStudioMode() && event->isKeyPressedWithShiftEvent(event->getKeyCode()) )
							verb = getWhitelistVerb("Physics", "Stats", "");
						break;

					case SDLK_F5: // TODO: use for run command in studio
						if ( !RBX::GameBasicSettings::singleton().inStudioMode() && event->isKeyPressedWithShiftEvent(event->getKeyCode()) )
							verb = getWhitelistVerb("Summary", "" , "Stats");
						break;

					case SDLK_F6:
						if ( !RBX::GameBasicSettings::singleton().inStudioMode() && event->isKeyPressedWithShiftEvent(event->getKeyCode()) )
							verb = getWhitelistVerb("Custom", "Stats", "");
						break;
					case SDLK_F7:
						if (DFFlag::AllowHideHudShortcut && !RBX::GameBasicSettings::singleton().getUsedHideHudShortcut())
						{
							if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(this))
							{
								if (!guiService->notificationCallback.empty())
								{
									RBX::GameBasicSettings::singleton().setUsedHideHudShortcut(true);
									guiService->notificationCallback("HUD Hidden", "Press F7 again when you want to unhide all GUIs", boost::bind(notificationCallbackSuccess, this), boost::bind(notificationCallbackError, _1));
								}
							}
						}
						else
						{
							renderGuisActive = !renderGuisActive;
						}
						break;
                    case SDLK_F8:
                        if ( event->isKeyPressedWithCtrlEvent(event->getKeyCode()) )
                        {
                            if( getWorkspace()->getWorld() && getWorkspace()->getWorld()->getKernel() )
                            {
                                getWorkspace()->getWorld()->getKernel()->dumpLog( !event->isKeyPressedWithShiftEvent(event->getKeyCode()) );
                            }
                        }
                        break;

					case SDLK_F10:
						if(event->isKeyPressedWithShiftEvent(event->getKeyCode()))
							graphicsQualityShortcutSignal(false);
						else
							graphicsQualityShortcutSignal(true);
						break;

					case SDLK_F11:
						verb = getWhitelistVerb("Toggle", "Full", "Screen");
						break;

					case SDLK_F12:
						verb = getWhitelistVerb("Record", "Toggle", "");
						break;

					case SDLK_SYSREQ: // This is technically print ...... directx doesn't have a print key code :(
					case SDLK_PRINT:
						verb = getWhitelistVerb("", "Screen", "shot");
						break;
					case SDLK_LALT:
					case SDLK_RALT:
						RBX::GameBasicSettings::singleton().setFreeLook(true); // if we are in camlock, holding alt (option on mac) allows us to pan camera freely
						break;

					case SDLK_ESCAPE:
						if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(this))
							guiService->escapeKeyPressed();
						break;
                    default:
                        break;
					}
				}
			}
			break;
		}
	default:	break;
	}
	if (verb && verb->isEnabled()) {
        Verb::doItWithChecks(verb, this);
		if (!preventSinking) 
			return GuiResponse::sunk();
	}

	return GuiResponse::notSunk();
}

GuiResponse DataModel::processPlayerGui(const shared_ptr<InputObject>& event)
{
	RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this);

	if (RBX::Network::Player* player = players->getLocalPlayer())
		if (PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>())
			return playerGui->process(event);

	return GuiResponse::notSunk();
}

GuiResponse DataModel::processCameraCommands(const shared_ptr<InputObject>& event)
{
    Verb* verb = NULL;
    
    
    if (FFlag::UserAllCamerasInLua)
    {
        if  (const Camera* camera = workspace->getConstCamera())
        {
            if (camera->hasClientPlayer())
            {
                return GuiResponse::notSunk();
            }
        }
    }
    
	if (!FFlag::UserBetterInertialScrolling)
	{
		switch(event->getUserInputType())
		{
		case InputObject::TYPE_MOUSEWHEEL:
			{
				if  (const Camera* camera = workspace->getConstCamera())
				{
					if (camera->getCameraType() == Camera::CUSTOM_CAMERA)
					{
						if (RBX::Network::Players::findLocalPlayer(this))
						{
							break;
						}
					}
				}
				
				if (userInputService && !userInputService->isCtrlDown() && !userInputService->isShiftDown() && !userInputService->isAltDown())
				{
					if (event->isMouseWheelForward())
						verb = workspace->getWhitelistVerb("Camera", "Zoom", "In");
					else if(event->isMouseWheelBackward())
						verb = workspace->getWhitelistVerb("Camera", "Zoom", "Out");
				}

				break;
			}
			default:
				break;
		}
	}

	if (FFlag::UserBetterInertialScrolling)
	{
		if (event->isMouseWheelForward() || event->isMouseWheelBackward())
		{
			if  (Camera* camera = workspace->getCamera())
			{
				camera->zoom(event->getPosition().z);
			}
		}
	}
	else
	{
		if (verb && verb->isEnabled())
		{
			Verb::doItWithChecks(verb, this);
		}
	}

    return GuiResponse::notSunk(); // don't sink this event when we fire it
}
    
void DataModel::setInitialScreenSize(RBX::Vector2 newScreenSize)
{
    if (shared_ptr<RBX::ScreenGui> coreScreenGui = coreGuiService->getRobloxScreenGui())
    {
        coreScreenGui->setBufferedViewport(Rect2D(newScreenSize));
    }
}

GuiResponse DataModel::processDevGamepadEvent(const shared_ptr<InputObject>& event)
{
	if (GamepadService* gamepadService = ServiceProvider::find<RBX::GamepadService>(this))
	{
		return gamepadService->processDev(event);
	}

	return GuiResponse::notSunk();
}

GuiResponse DataModel::processCoreGamepadEvent(const shared_ptr<InputObject>& event)
{
	if (GamepadService* gamepadService = ServiceProvider::find<RBX::GamepadService>(this))
	{
		return gamepadService->processCore(event);
	}

	return GuiResponse::notSunk();
}

GuiResponse DataModel::processProfilerEvent(const shared_ptr<InputObject>& event)
{
	if (event->isMouseEvent())
	{
		unsigned int flags = Profiler::Flag_MouseMove;

		int mouseX = event->getRawPosition().x;
		int mouseY = event->getRawPosition().y;
		int mouseWheel = 0;
		int mouseButton = 0;

		if (event->isMouseWheelEvent())
		{
			flags |= Profiler::Flag_MouseWheel;
			mouseWheel = event->getRawPosition().z;
		}

		if (event->isMouseDownEvent())
		{
			flags |= Profiler::Flag_MouseDown;
			mouseButton = event->getUserInputType() - InputObject::TYPE_MOUSEBUTTON1;
		}

		if (event->isMouseUpEvent())
		{
			flags |= Profiler::Flag_MouseUp;
			mouseButton = event->getUserInputType() - InputObject::TYPE_MOUSEBUTTON1;
		}

		return Profiler::handleMouse(flags, mouseX, mouseY, mouseWheel, mouseButton) ? GuiResponse::sunk() : GuiResponse::notSunk();
	}

	if (event->isKeyDownEvent())
	{
        unsigned int ctrlOrCmd = KMOD_LCTRL | KMOD_RCTRL | KMOD_LMETA | KMOD_RMETA;

		if (event->isKeyDownEvent(SDLK_F6) && (event->mod & ctrlOrCmd))
			return Profiler::toggleVisible() ? GuiResponse::sunk() : GuiResponse::notSunk();

		if (event->isKeyDownEvent(SDLK_p) && (event->mod & ctrlOrCmd))
			return Profiler::togglePause() ? GuiResponse::sunk() : GuiResponse::notSunk();
	}

	return GuiResponse::notSunk();
}

GuiResponse DataModel::processGuiTarget(const shared_ptr<InputObject>& event)
{
	GuiResponse response = GuiResponse::notSunk();

	if (shared_ptr<Instance> guiTarget = guiTargetInstance.lock())
	{
		FASTLOG1(FLog::UserInputProfile, "Handing to GUI target: %p", guiTarget.get());
		if (TextBox* textbox = fastDynamicCast<TextBox>(guiTarget.get()))
		{
			if (event->isKeyEvent() || event->isGamepadEvent())
			{
				response = textbox->process(event);
			}
			else if (!DFFlag::DontProcessMouseEventsForGuiTarget)
			{
				response = textbox->preProcess(event);
			}
		}
		else if (GuiLayerCollector* guiCollector = fastDynamicCast<GuiLayerCollector>(guiTarget.get()))
		{
			response = guiCollector->process(event);
		}
	}

	return response;
}
    
bool DataModel::processEvent(const shared_ptr<InputObject>& event)
{
	RBXASSERT(event->getUserInputType() != InputObject::TYPE_NONE);
	if (!isInitialized)
		return false;

	if (event->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
		mouseStats.mouseMove.sample();

	// Hack #2
	switch(event->getUserInputType())
	{
		case InputObject::TYPE_MOUSEBUTTON2:
			if (event->getUserInputState() == InputObject::INPUT_STATE_END) // button up
			{
				FASTLOG(FLog::UserInputProfile, "Cancelling Right mouse pan");
				workspace->cancelRightMousePan();
			}
			break;         
		default:
			break;
	}

	bool isInMenu = false;
	if (RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(this))
	{
		isInMenu = guiService->getMenuOpen();
	}

	GuiResponse response = GuiResponse();
	shared_ptr<Instance> guiTarget = guiTargetInstance.lock();

	if (!guiTarget)
		setSuppressNavKeys(false);		// no target - make sure we no longer suppress asdf, which we do when a TextBox gets the focus/is target

	FASTLOG(FLog::UserInputProfile, "Starting handing out events...");

	if (event->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
	{
		mouseOverInteractable = weak_ptr<GuiObject>();
	}

	mouseOverGui = false;
	if (!response.wasSunk())
	{
		FASTLOG(FLog::UserInputProfile, "Handing to Profiler");
		response = processProfilerEvent(event);
	}
	if (DFFlag::ProcessAcceleratorsBeforeGUINavigation)
	{

	if (!isInMenu)
		{
			FASTLOG(FLog::UserInputProfile, "Handing to Accelerators");
			response = processAccelerators(event);
		}
	}

    if (!response.wasSunk())
    {
        response = processGuiTarget(event);
    }

	// todo: remove guiRoot when profiler has all the same stats
	if (!response.wasSunk())
	{
		FASTLOG(FLog::UserInputProfile, "Handing to GUI root");
		response = guiRoot->process(event);
	}

	if (!response.wasSunk() && (renderGuisActive || !DFFlag::AllowHideHudShortcut)) 
	{
		FASTLOG(FLog::UserInputProfile, "Handing to Core Gui");
		response = coreGuiService->process(event);
		mouseOverGui = response.getMouseWasOver();
	}

	if (!response.wasSunk() && event->isKeyDownEvent() && !suppressNavKeys)
	{
		if (GamepadService* gamepadService = RBX::ServiceProvider::create<GamepadService>(this))
		{
			FASTLOG(FLog::UserInputProfile, "Handing to gamepadService for keyboard event");
			response = gamepadService->trySelectGuiObject(userInputService->getKeyboardGuiSelectionDirection(event));
		}
	}

	if (!response.wasSunk())
	{
		FASTLOG(FLog::UserInputProfile, "Handing to processCoreGamepadEvent");
		response = processCoreGamepadEvent(event);
	}

	if (!response.wasSunk())
	{
		FASTLOG(FLog::UserInputProfile, "Handing to ContextActionService Core Bindings");
		response = contextActionService->processCoreBindings(event);
	}

	if (!DFFlag::ProcessAcceleratorsBeforeGUINavigation)
	{
		if (!response.wasSunk()) 
		{
			FASTLOG(FLog::UserInputProfile, "Handing to Accelerators");
			response = processAccelerators(event);
		}
	}

	if (!response.wasSunk() && (renderGuisActive || !DFFlag::AllowHideHudShortcut) && !isInMenu)
	{
		FASTLOG(FLog::UserInputProfile, "Handing to Player Gui");
		response = processPlayerGui(event);
		mouseOverGui = response.getMouseWasOver();
		
		if ( response.getMouseWasOver())
		{
			if (GuiButton* responseButton = fastDynamicCast<GuiButton>(response.getTarget().get()))
			{
				mouseOverInteractable = weak_from(responseButton);
			}
			if (TextBox* responseBox = fastDynamicCast<TextBox>(response.getTarget().get()))
			{
				mouseOverInteractable = weak_from(responseBox);
			}
		}
	}

	if (!response.wasSunk() && !isInMenu)
	{
		FASTLOG(FLog::UserInputProfile, "Handing to processDevGamepadEvent");
		response = processDevGamepadEvent(event);
	}
	
	if (!response.wasSunk())
	{
		FASTLOG(FLog::UserInputProfile, "Handing to ContextActionService Dev Bindings");
		response = contextActionService->processDevBindings(event, isInMenu);
	}

    if (!response.wasSunk() && !isInMenu) 
	{
        FASTLOG(FLog::UserInputProfile, "Handing to Camera Process");
        response = processCameraCommands(event);
    }

	InputObject::UserInputType lastType = userInputService->getLastInputType();
	if (GamepadService::getGamepadIntForEnum(lastType) == -1)
	{
		forceArrowCursor = (response.wasSunk() && guiTarget && (guiTarget.get() != workspace.get())) || mouseOverInteractable.lock();
	}

	const bool wasSunkBeforeHandingToTool = response.wasSunk();
    
	if (!response.wasSunk() && !isInMenu && !event->isTouchEvent() && event->isPublicEvent())
    {
			FASTLOG(FLog::UserInputProfile, "Handing to Workspace");
			response = workspace->process(event);
        }

	// Check for player idle (if we have a mouse event or a key was pressed, signal we aren't idle)
	if (event->isMouseEvent() || event->isKeyDownEvent() || event->isGamepadEvent())
		Network::Player::onLocalPlayerNotIdle(this);

	if (DFFlag::DontProcessMouseEventsForGuiTarget)
	{
		// If a textbox is currently focused and the target is not that textbox then release focus from that textbox
		if (TextBox* textbox = fastDynamicCast<TextBox>(guiTarget.get()))
		{
			if (textbox->getFocused() && (guiTarget != response.getTarget()))
			{
				textbox->releaseFocus(false, event);
			}
		}
	}

	guiTargetInstance = response.getTarget();
	
	FASTLOG(FLog::UserInputProfile, "DM event processed");
    
    return wasSunkBeforeHandingToTool;
}
    
void DataModel::processWorkspaceEvent(const shared_ptr<InputObject>& event)
{
	GuiResponse response = GuiResponse();

    FASTLOG(FLog::UserInputProfile, "Handing to Workspace via processWorkspaceEvent");
    response = workspace->process(event);
}
    
bool DataModel::processInputObject(shared_ptr<InputObject> event)
{
	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
	if (!isInitialized || !event)
		return false;
	
	bool processedEvent = false;

	if (event->getUserInputType() != InputObject::TYPE_NONE)
		processedEvent = processEvent(event);

	InputObjectProcessed(event);

    if (Network::Player* player = Network::Players::findLocalPlayer(this))
    {
        if (shared_ptr<Mouse> playerMouse = player->getMouse())
        {
            if(!processedEvent || event->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
            {
                playerMouse->update(event);
            }
            else if (event->isMouseEvent())
            {
                playerMouse->cacheInputObject(event);
            }
        }
    }

	RBXASSERT(isInitialized);    //  Show to David or Erik - threading issue
    
	return processedEvent;
}

void DataModel::close()
{
	Verb* verb = getWhitelistVerb("E", "xi", "t");
	if (verb==NULL)
		throw std::runtime_error("Couldn't find Exit Verb");
	Verb::doItWithChecks(verb, NULL);
}

void DataModel::addCustomStat(std::string name, std::string value)
{
	guiBuilder.addCustomStat(name, value);
}

void DataModel::removeCustomStat(std::string str)
{
	guiBuilder.removeCustomStat(str);
}

void DataModel::writeStatsSettings()
{
	guiBuilder.saveCustomStats();
}


void DataModel::clearContents(bool resettingSimulation)
{
	// TODO: Should Service have a "clearContents()" function?

	ScriptContext* sc = find<ScriptContext>();
	if (sc)
	{
		if (DFFlag::CloseStatesBeforeChildRemoval)
		{
			FASTLOG(FLog::CloseDataModel, "Closing Script Context");
			sc->closeStates(resettingSimulation);
			ScriptContext::propScriptsDisabled.setValue(sc, false);
		}
		else
		{
			sc->setPreventNewConnections();
		}
	}

	if (workspace)
	{
		// unlock terrain
		if( workspace->getTerrain() ) workspace->getTerrain()->unlockParent();

		workspace->setMouseCommand(shared_ptr<MouseCommand>());

		if(workspace->getWorld() && workspace->getWorld()->getContactManager())
		{
			// spatial hash is slow to remove. do a special trick when we know we are removing everything.
			workspace->getWorld()->getContactManager()->fastClear();
		}

		if(World* world = workspace->getWorld()){
			FASTLOG3(FLog::CloseDataModel, "Clearing World -- %d Bodies, %d Points, %d Constraints", world->getNumBodies(), world->getNumPoints(), world->getNumConstraints());
			FASTLOG3(FLog::CloseDataModel, "Clearing World -- %d HashNodes, %d MaxBucketSize, %d NumLinkCalls", world->getNumHashNodes(), world->getMaxBucketSize(), world->getNumLinkCalls());
			FASTLOG3(FLog::CloseDataModel, "Clearing World -- %d Contacts, %d Joints, %d Primitives", world->getNumContacts(), world->getNumJoints(), world->getNumPrimitives());
		}

		FASTLOG3(FLog::CloseDataModel, "Clearing Workspace -- %d Instances, %d Parts, %d Scripts", workspace->countDescendantsOfType<Instance>(), workspace->countDescendantsOfType<PartInstance>(), workspace->countDescendantsOfType<BaseScript>());		
		// TODO: Should we simply remove children of ALL service???
		workspace->removeAllChildren();
		workspace->setCurrentCamera(NULL);
	}

	if (starterPackService){
		FASTLOG1(FLog::CloseDataModel, "Clearing StarterPack -- %d Instances", starterPackService->countDescendantsOfType<Instance>());		
		starterPackService->removeAllChildren();
	}

	if (starterPlayerService){
		FASTLOG1(FLog::CloseDataModel, "Clearing StarterPlayer -- %d Instances", starterPlayerService->countDescendantsOfType<Instance>());		
		starterPlayerService->removeAllChildren();
	}

	if (starterGuiService){
		FASTLOG1(FLog::CloseDataModel, "Clearing StarterGui -- %d Instances", starterGuiService->countDescendantsOfType<Instance>());		
		starterGuiService->removeAllChildren();
	}

	// only remove core gui if we are closing doc
	if (coreGuiService && !resettingSimulation){
		FASTLOG1(FLog::CloseDataModel, "Clearing CoreGui -- %d Instances", coreGuiService->countDescendantsOfType<Instance>());		
		coreGuiService->removeAllChildren();
	}

	if (Teams* teams = ServiceProvider::find<Teams>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing Teams -- %d Instances", teams->countDescendantsOfType<Instance>());		
		teams->removeAllChildren();
	}

	if (SpawnerService* ss = ServiceProvider::find<SpawnerService>(this))
		ss->ClearContents();

	if (ChangeHistoryService* ch = ServiceProvider::find<ChangeHistoryService>(this)){
		FASTLOG1(FLog::CloseDataModel, "Clearing ChangeHistoryService -- %d waypoints ", ch->getWaypointCount());
		ch->clearWaypoints();
	}

	if (Lighting* lighting = ServiceProvider::find<Lighting>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing Lighting -- %d Instances", lighting->countDescendantsOfType<Instance>());		
		lighting->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<JointsService>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing JointsService -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<TestService>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing TestService -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<ServerScriptService>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing ServerScriptService -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<ReplicatedStorage>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing ReplicatedStorage -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<RobloxReplicatedStorage>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing RobloxReplicatedStorage -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<ReplicatedFirst>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing ReplicatedFirst -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (Instance* service = ServiceProvider::find<ServerStorage>(this)) {
		FASTLOG1(FLog::CloseDataModel, "Clearing ServerStorage -- %d Instances", service->countDescendantsOfType<Instance>());		
		service->removeAllChildren();
	}

	if (sc && !DFFlag::CloseStatesBeforeChildRemoval)
	{
		FASTLOG(FLog::CloseDataModel, "Closing Script Context");
		sc->closeStates(resettingSimulation);
		ScriptContext::propScriptsDisabled.setValue(sc, false);
	}
}


static std::string report(double time)
{
	if (time > 0.0)
	{
		char buffer[256];
		std::string t = Log::formatTime(time);
		int cpu = static_cast<int>(100 * time * 30.0);
		sprintf(buffer, "%i %%  %s (%.3gfps)", cpu, t.c_str(), 1.0/time);
		return buffer;
	}
	else
		return Log::formatTime(time);
}

void DataModel::setNetworkMetric(IMetric* metric)
{
	networkMetric = metric;
}
    
double DataModel::getMetricValue(const std::string& metric) const
{
    if (metric == "Render FPS")
    {
        return tempMetric->getMetricValue("Render FPS");
    }
    if (metric == "Render CPU")
	{
        return 100.0 * tempMetric->getMetricValue("Render Duty");
	}
    if (metric == "Render Time")
    {
        return 1000.0 * tempMetric->getMetricValue("Render Job Time");
    }
    if (metric == "Physics FPS")
    {
        return runService->smoothFps();
    }
    if (metric == "Physics CPU")
	{
        return 100.0 * runService->physicsCpuFraction();
	}
    if (metric == "Physics Time")
    {
        return 1000.0 * runService->physicsAverageStep();
    }
    if (metric == "Network Receive CPU")
    {
        return networkMetric ? networkMetric->getMetricValue("Network Receive CPU") : 0.0;
    }
    if (metric == "Network Receive Time")
    {
        return networkMetric ? networkMetric->getMetricValue("Network Receive Time") : 0.0;
    }
    if (metric == "Frame Time")
    {
        return tempMetric->getMetricValue("Delta Between Renders");
    }
    if (metric == "Effective FPS")
    {
        return 1000.0 / tempMetric->getMetricValue("Delta Between Renders");
    }

	Stats::StatsService* stats = find<Stats::StatsService>();
	if (stats)
	{ 
		shared_ptr<Stats::Item> network = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network"));
		if (network && network->numChildren() > 1)
		{
			if (Instance* item = network->getChild(1))
			{
				if (metric == "Received Physics Packets")
				{
					return item->findFirstChildByName("Received Physics Packets")->fastDynamicCast<Stats::Item>()->getValue();
				}
				if (metric == "Data Ping")
				{
					return item->findFirstChildByName("Data Ping")->fastDynamicCast<Stats::Item>()->getValue();
				}
			}
            else
            {
                return 0.0;
            }
		}
        else
        {
            return 0.0;
        }
	}
	else
	{
		return 0.0;
	}
    
    throw RBX::runtime_error("%s is not a valid metric.", metric.c_str());
}

std::string DataModel::getMetric(const std::string& valueName) const
{
	World* world = workspace->getWorld();
	
	if (valueName.compare(0, 11, "RenderStats") == 0) return tempMetric->getMetric(valueName);
	if (valueName.compare(0, 3, "FRM") == 0) return RBX::format("%.3g", tempMetric->getMetricValue(valueName));

	Stats::StatsService* stats = find<Stats::StatsService>();
	if (stats)
	{ 
		shared_ptr<Stats::Item> network = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network"));
		if (network && network->numChildren() > 1)
		{
			if (Instance* item = network->getChild(1))
			{
				// raknet
				if (Stats::Item* raknetItem = Instance::fastDynamicCast<Stats::Item>(item->findFirstChildByName("Stats")->findFirstChildByName(valueName)))
					return RBX::format("%.0f", raknetItem->getValue());
				
				// replicator
				if (valueName == "RakNetPing")
				{
					return item->findFirstChildByName("Ping")->fastDynamicCast<Stats::Item>()->getStringValue();
				}
                else if (valueName == "GeneralStats")
                {
                    return RBX::format("%.0f, %.2fms"
                        , item->findFirstChildByName("Send kBps")->findFirstChildByName("MtuSize")->fastDynamicCast<Stats::Item>()->getValue()
                        , item->findFirstChildByName("Data Ping")->fastDynamicCast<Stats::Item>()->getValue()
                        );
                }
                else if (valueName == "OutgoingStats")
                {
                    return RBX::format("%.2f"
                        , item->findFirstChildByName("Send kBps")->fastDynamicCast<Stats::Item>()->getValue()
                        );
                }
                else if (valueName == "IncomingStats")
                {
                    return RBX::format("%.2f, %.2f, %.0f"
                        , item->findFirstChildByName("Receive kBps")->fastDynamicCast<Stats::Item>()->getValue()
                        , item->findFirstChildByName("Received Packets")->fastDynamicCast<Stats::Item>()->getValue()
                        , item->findFirstChildByName("Packet Queue")->fastDynamicCast<Stats::Item>()->getValue()
                        );
                }
                else if (valueName == "InData")
                {
                    Stats::Item* receivedDataPackets = item->findFirstChildByName("Received Data Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = receivedDataPackets->getValue();
                    float avgSize = receivedDataPackets->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB", kBps, numPacket, avgSize);
                }
                else if (valueName == "InPhysics")
                {
                    Stats::Item* inPhysics = item->findFirstChildByName("Received Physics Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = inPhysics->getValue();
                    float avgSize = inPhysics->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB, %.2f", kBps, numPacket, avgSize
                        , inPhysics->findFirstChildByName("Average Lag")->fastDynamicCast<Stats::Item>()->getValue());
                }
                else if (valueName == "InTouches")
                {
                    Stats::Item* inTouches = item->findFirstChildByName("Received Touch Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = inTouches->getValue();
                    float avgSize = inTouches->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB", kBps, numPacket, avgSize);
                }
                else if (valueName == "InClusters")
                {
                    Stats::Item* inClusters = item->findFirstChildByName("Received Cluster Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = inClusters->getValue();
                    float avgSize = inClusters->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB", kBps, numPacket, avgSize);
                }
                else if (valueName == "OutPhysics")
                {
                    Stats::Item* outPhysics = item->findFirstChildByName("Sent Physics Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = outPhysics->getValue();
                    float avgSize = outPhysics->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB, %.2f%%", kBps, numPacket, avgSize
                        , outPhysics->findFirstChildByName("Throttle")->fastDynamicCast<Stats::Item>()->getValue());
                }
                else if (valueName == "OutData")
                {
                    Stats::Item* outProps = item->findFirstChildByName("Sent Data Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = outProps->getValue();
                    float avgSize = outProps->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB, %.2f%%", kBps, numPacket, avgSize, outProps->findFirstChildByName("Throttle")->fastDynamicCast<Stats::Item>()->getValue());
                }
                else if (valueName == "OutTouches")
                {
                    Stats::Item* outTouches = item->findFirstChildByName("SentTouchPackets")->fastDynamicCast<Stats::Item>();
                    float numPacket = outTouches->getValue();
                    float avgSize = outTouches->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB, %.0f", kBps, numPacket, avgSize, outTouches->findFirstChildByName("WaitingTouches")->fastDynamicCast<Stats::Item>()->getValue());
                }
                else if (valueName == "OutClusters")
                {
                    Stats::Item* outClusters = item->findFirstChildByName("Sent Cluster Packets")->fastDynamicCast<Stats::Item>();
                    float numPacket = outClusters->getValue();
                    float avgSize = outClusters->findFirstChildByName("Size")->fastDynamicCast<Stats::Item>()->getValue();
                    float kBps = numPacket * avgSize / 1000.f;
                    return RBX::format("%.2f, %.2f, %.2fB", kBps, numPacket, avgSize);
                }
                else if (valueName == "InPhysicsDetails")
                {
                    std::string result = "";
                    if (Stats::Item* inPhysicsDetails = item->findFirstChildByName(valueName)->fastDynamicCast<Stats::Item>())
                    {
                        inPhysicsDetails->visitChildren(boost::bind(&GuiBuilder::buildNetworkStatsOutput, _1, &result));
                    }
                    return result;
                }
                else if (valueName == "OutPhysicsDetails")
                {
                    std::string result = "";
                    if (Stats::Item* outPhysicsDetails = item->findFirstChildByName(valueName)->fastDynamicCast<Stats::Item>())
                    {
                        outPhysicsDetails->visitChildren(boost::bind(&GuiBuilder::buildNetworkStatsOutput, _1, &result));
                    }
                    return result;
                }
                else if (valueName == "InDataDetails")
                {
                    std::string result = "";
                    if (Stats::Item* inDataDetails = item->findFirstChildByName("Received Data Types")->fastDynamicCast<Stats::Item>())
                    {
                        inDataDetails->visitChildren(boost::bind(&GuiBuilder::buildNetworkStatsOutput, _1, &result));
                    }
                    return result;
                }
                else if (valueName == "OutDataDetails")
                {
                    std::string result = "";
                    if (Stats::Item* inDataDetails = item->findFirstChildByName("Send Data Types")->fastDynamicCast<Stats::Item>())
                    {
                        inDataDetails->visitChildren(boost::bind(&GuiBuilder::buildNetworkStatsOutput, _1, &result));
                    }
                    return result;
                }
				else if (valueName == "FreeMemory")
				{
					float result =  RBX::NetworkSettings::singleton().getFreeMemoryMBytes();
					return RBX::format("%.2fMB", result);
				}
				else if (valueName == "MemoryLevel")
				{
					int result = RBX::MemoryStats::slowCheckMemoryLevel(((RBX::MemoryStats::memsize_t)RBX::NetworkSettings::singleton().getExtraMemoryUsedInMB())*1024*1024);
					return RBX::format("%d", result);
				}
				else if (valueName == "ReceivedStreamData")
				{
					std::string result = "";
					if (Stats::Item* streamStats = item->findFirstChildByName("Received Stream Data")->fastDynamicCast<Stats::Item>())
					{
						streamStats->visitChildren(boost::bind(&GuiBuilder::buildSimpleStatsOutput, _1, &result));
					}
					return result;
				}
			}
		}

		shared_ptr<Stats::Item> httpQueue = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("HttpQueue_ContentProvider"));
		if (httpQueue)
		{
			if (valueName == "HttpTimeInQueue")
			{
				return RBX::format("%.2f msec", httpQueue->findFirstChildByName("Average time in queue")->fastDynamicCast<Stats::Item>()->getValue());
			}
			else if (valueName == "HttpProcessTime")
			{
				return RBX::format("%.2f msec", httpQueue->findFirstChildByName("Average process time")->fastDynamicCast<Stats::Item>()->getValue());
			}
			else if (valueName == "HttpSlowReq")
			{
				return RBX::format("%.0f", httpQueue->findFirstChildByName("Num slow requests")->fastDynamicCast<Stats::Item>()->getValue());
			}
		}

	}

	if (valueName == "physicsMode") {
		return Network::Players::getDistributedPhysicsEnabled() ? "Experimental Physics" : "Standard Physics";
	}

	else if (valueName == "Physics")		// This is the frequency at which physics job is actually executed
	{
		boost::format fmt("%.1f/s %.1f msec %d%%");
		fmt % runService->smoothFps() % (1000.0 * runService->physicsAverageStep()) % (int)(100.0 * runService->physicsCpuFraction());
		return fmt.str();
	}

	else if (valueName == "PhysicsReal") // This is the effective physics frame-rate considering steps skipped by environmental throttling
	{
		boost::format fmt("%.1f/s  throttle@%d%%(%d)");
		fmt % (runService->smoothFps() * world->getEnvironmentSpeed()) % (int)(100.0 * world->getEnvironmentSpeed()) % world->getFRMThrottle();
		return fmt.str();
	}

	else if (valueName == "Heartbeat")		
	{
		boost::format fmt("%.1f/s %.1f msec %d%%");
		fmt % runService->heartbeatFps() % (1000.0 * runService->heartbeatAverageStep()) % (int)(100.0 * runService->heartbeatCpuFraction());
		return fmt.str();
	}

	else if (valueName == "Network Send" || valueName == "Network Receive")
	{
		return networkMetric ? networkMetric->getMetric(valueName) : "";
	}

	else if (valueName == "Render")		
	{
		boost::format fmt("%.1f/s %.1f msec %d%%");
		fmt % tempMetric->getMetricValue("Render FPS") % (1000.0 * tempMetric->getMetricValue("Render Job Time")) % (int)(100.0 * tempMetric->getMetricValue("Render Duty"));
		return fmt.str();
	}

	else if (valueName == "MouseMove")		
	{
		boost::format fmt("%.1f/s -> %.1f/s");
		fmt % mouseStats.osMouseMove.rate() % mouseStats.mouseMove.rate();
		return fmt.str();
	}

    else if      (valueName == "Effective FPS")              return RBX::format("%.1f/s", 1000.0 / tempMetric->getMetricValue("Delta Between Renders"));
	else if		(valueName == "Render Nominal FPS")         return RBX::format("%.1f/s", tempMetric->getMetricValue(valueName));
	else if		(valueName == "Delta Between Renders")		return RBX::format("%.3g", tempMetric->getMetricValue(valueName));
	else if		(valueName == "Total Render")				return RBX::format("%.3g", tempMetric->getMetricValue(valueName));
	else if		(valueName == "Render Prepare")				return RBX::format("%.3g", tempMetric->getMetricValue(valueName));
	else if		(valueName == "Present Time")				return RBX::format("%.3g", tempMetric->getMetricValue(valueName));
	else if		(valueName == "GPU Delay")				    return RBX::format("%.3g", tempMetric->getMetricValue(valueName));

	else if		(valueName == "RequestQueueSize")			return  StringConverter<int>::convertToString(ServiceProvider::create<ContentProvider>()->getRequestQueueSize());

	else if		(valueName == "drawId")						return StringConverter<int>::convertToString(drawId);
	else if		(valueName == "worldId")					return StringConverter<int>::convertToString(world->getWorldStepId());
	else if		(valueName == "Graphics Mode")				return tempMetric->getMetric(valueName);
	else if		(valueName == "Particles")					return StringConverter<int>::convertToString(static_cast<int>(tempMetric->getMetricValue(valueName)));
	else if		(valueName == "Shadow Casters")				return StringConverter<int>::convertToString(static_cast<int>(tempMetric->getMetricValue(valueName)));
	else if		(valueName == "Anti-Aliasing")				return tempMetric->getMetric(valueName);
	else if		(valueName == "Bevels")						return tempMetric->getMetric(valueName);
	else if		(valueName == "Video Memory")			    return Log::formatMem(static_cast<int>(tempMetric->getMetricValue(valueName)));

	else if		(valueName == "Player Radius")				
	{	
		RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this);
		int numPlayers = players->getNumPlayers();
		if (numPlayers == 0)
			return std::string("N/A");
		float sumRadius = 0.0f;
		float sumMaxRaius = 0.0f;
		shared_ptr<const Instances> myPlayers = players->getPlayers();
		Instances::const_iterator iter = myPlayers->begin();
		Instances::const_iterator end = myPlayers->end();
		for (; iter!=end; ++iter)
		{
			RBX::Network::Player* player = boost::polymorphic_downcast<RBX::Network::Player*>(iter->get());
			sumRadius += player->getSimulationRadius();
			sumMaxRaius += player->getMaxSimulationRadius();
		}
		boost::format fmt("%d / %d");
		fmt % static_cast<int>(sumRadius / numPlayers) % static_cast<int>(sumMaxRaius / numPlayers);
		return fmt.str();
	}
	else if		(valueName == "numPrimitives")				return StringConverter<int>::convertToString(numPartInstances);	
	else if		(valueName == "numMovingPrimitives")		return StringConverter<int>::convertToString(workspace->getNumberMoving());	
	else if		(valueName == "numJoints")
    {
        int numJoints = world->getNumJoints();
        if (JointsService* jointsService = ServiceProvider::find<JointsService>(this))
        {
            numJoints += jointsService->numChildren();
        }
        return StringConverter<int>::convertToString(numJoints);
    }
    else if      (valueName == "numInstances")
    {
        return StringConverter<int>::convertToString(Diagnostics::Countable<Instance>::getCount());
    }
	else if		(valueName == "numContactsAll")				
	{	
		boost::format fmt("%d  %d  %d  %d");
		fmt % world->getNumContacts() % 
			  world->getMetric(IWorldStage::NUM_CONTACTSTAGE_CONTACTS) %
			  world->getMetric(IWorldStage::NUM_STEPPING_CONTACTS) %
			  world->getMetric(IWorldStage::NUM_TOUCHING_CONTACTS);
		return fmt.str();	
	}
	else if		(valueName == "numContacts")				return StringConverter<int>::convertToString(world->getNumContacts());	
	else if		(valueName == "numContactsInCollisionStage") return StringConverter<int>::convertToString(world->getMetric(IWorldStage::NUM_CONTACTSTAGE_CONTACTS));	
	else if		(valueName == "numSteppingContacts")		return StringConverter<int>::convertToString(world->getMetric(IWorldStage::NUM_STEPPING_CONTACTS));
	else if		(valueName == "numTouchingContacts")		return StringConverter<int>::convertToString(world->getMetric(IWorldStage::NUM_TOUCHING_CONTACTS));
	else if		(valueName == "maxTreeDepth")				return StringConverter<int>::convertToString(world->getMetric(IWorldStage::MAX_TREE_DEPTH));
	else if		(valueName == "contactPairHitRatio")		return StringConverter<float>::convertToString(BlockBlockContact::pairHitRatio());	
	else if		(valueName == "contactFeatureHitRatio")		return StringConverter<float>::convertToString(BlockBlockContact::featureHitRatio());	

	else if		(valueName == "numLinkCalls")				return StringConverter<int>::convertToString(world->getNumLinkCalls());	
	else if		(valueName == "numHashNodes")				return StringConverter<int>::convertToString(world->getNumHashNodes());	
	else if		(valueName == "maxBucketSize")				return StringConverter<int>::convertToString(world->getMaxBucketSize());	
	else if		(valueName == "environmentSpeed")			return StringConverter<float>::convertToString(world->getEnvironmentSpeed());	

	
	else if (valueName == "Break Time" ||
		valueName == "Assembler Time" ||
		valueName == "Filter Time" ||
		valueName == "UI Step" ||
		valueName == "Broadphase" ||
		valueName == "Collision" ||
		valueName == "Wake" ||
		valueName == "Sleep" ||
		valueName == "Joint Update" ||
		valueName == "Joint Sleep" ||
		valueName == "Kernel Bodies" ||
		valueName == "Kernel Connectors" )
	{
		if (stats)
		{ 
			shared_ptr<Stats::Item> workspace = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Workspace"));
			if( workspace && workspace->numChildren() > 0)
			{
				shared_ptr<Stats::Item> dataModelStep = shared_from_polymorphic_downcast<Stats::Item>(workspace->findFirstChildByName("DataModel Step"));
				if( dataModelStep && dataModelStep->numChildren() > 0)
				{
					shared_ptr<Stats::Item> workspaceStep = shared_from_polymorphic_downcast<Stats::Item>(dataModelStep->findFirstChildByName("Workspace Step"));
					if( workspaceStep && workspaceStep->numChildren() > 0)
					{
						shared_ptr<Stats::Item> worldStep = shared_from_polymorphic_downcast<Stats::Item>(workspaceStep->findFirstChildByName("World Step"));
						if (worldStep && worldStep->numChildren() > 0)
						{
							RBX::Instance* profilerItem = worldStep->findFirstChildByName(valueName);
							RBX::Stats::Item* profilerStatItem = Instance::fastDynamicCast<RBX::Stats::Item>(profilerItem);
							if(profilerStatItem)
								return profilerStatItem->getStringValue();
						}
					}
				}
			}
		}
	}
	

	// these are here for fun and to throw off the competition!
	else if		(valueName == "solverIterations")	return StringConverter<int>::convertToString(world->getKernel()->fakeDeceptiveSolverIterations());
	else if		(valueName == "matrixSize")			return StringConverter<int>::convertToString(world->getKernel()->fakeDeceptiveMatrixSize());


	else if		(valueName == "pGSSolverActive")
	{
		return StringConverter<bool>::convertToString(world->getUsingPGSSolver());
	}

	else if		(valueName == "numBodiesAll")
	{
		boost::format fmt("%d %d %d %d %d %d");
		fmt % world->getKernel()->numBodiesMax() %
			  world->getKernel()->numFreeFallBodies() %
			  world->getKernel()->numContactBodies() %
			  world->getKernel()->numJointBodies() %
			  world->getKernel()->numRealTimeBodies() %
			  world->getKernel()->numLeafBodies();
		return fmt.str();
	}

	else if		(valueName == "numConnectorsAll")
	{
		boost::format fmt("%d %d %d %d %d %d");
		fmt % world->getKernel()->numConnectors() %
			  world->getKernel()->numContactConnectors() %
			  world->getKernel()->numJointConnectors() %
			  world->getKernel()->numRealTimeConnectors() %
			  world->getKernel()->numSecondPassConnectors() %
			  world->getKernel()->numHumanoidConnectors();
		return fmt.str();
	}

	else if		(valueName == "numBodies")				return StringConverter<int>::convertToString(world->getKernel()->numBodies());
	else if		(valueName == "maxBodies")				return StringConverter<int>::convertToString(world->getKernel()->numBodiesMax());	
	else if		(valueName == "numLeafBodies")			return StringConverter<int>::convertToString(world->getKernel()->numLeafBodies());	
	else if		(valueName == "numConnectorsAndPoints")
	{
		boost::format fmt("%d %d");
		fmt % world->getKernel()->numConnectors() % world->getKernel()->numPoints();
		return fmt.str();
	}

	else if		(valueName == "numIterations")
	{
		boost::format fmt("%d %d (%f %f)");
		fmt % world->getKernel()->numMaxIterations() % world->getKernel()->numIterations()
			% world->getKernel()->getMaxSolverError() % world->getKernel()->getSolverError();
		return fmt.str();
	}

	else if		(valueName == "numConstraints")			return StringConverter<int>::convertToString(world->getKernel()->numConnectors());	
	else if		(valueName == "numPoints")				return StringConverter<int>::convertToString(world->getKernel()->numPoints());	
	else if		(valueName == "percentConnectorsActive")	return StringConverter<float>::convertToString(ContactConnector::percentActive());

	else if		(valueName == "energyBody")		return StringConverter<float>::convertToString(world->getKernel()->bodyKineticEnergy());	
	else if		(valueName == "energyConnector")return StringConverter<float>::convertToString(world->getKernel()->connectorSpringEnergy());	
    else if		(valueName == "energyTotal")	return StringConverter<float>::convertToString(world->getKernel()->totalKineticEnergy());	

	//RBXASSERT(0);
	return "?";
}

DataModel* DataModel::get(Instance* context)
{
	return context ? Instance::fastDynamicCast<DataModel>(context->getRootAncestor()) : NULL;
}
const DataModel* DataModel::get(const Instance* context)
{
	return context ? Instance::fastDynamicCast<DataModel>(context->getRootAncestor()) : NULL;
}

static void appendJobInfo(DataModel* dataModel, shared_ptr<const TaskScheduler::Job> job, Reflection::ValueArray* result)
{
	if (job->getArbiter().get()!=dataModel)
		return;

	
	shared_ptr<Reflection::ValueArray> info(rbx::make_shared<Reflection::ValueArray>());
	info->push_back(job->name);
	info->push_back(job->averageDutyCycle());
	info->push_back(job->averageStepsPerSecond());
	info->push_back(job->averageStepTime());
	info->push_back(job->averageError());
	info->push_back(job->isRunning());

	result->push_back(shared_ptr<const RBX::Reflection::ValueArray>(info));
}

shared_ptr<const Reflection::ValueArray> DataModel::getJobsInfo()
{
	// Returns some nice diagnostic information
	shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>());
	{
		shared_ptr<Reflection::ValueArray> info(rbx::make_shared<Reflection::ValueArray>());
		info->push_back(std::string("name"));
		info->push_back(std::string("averageDutyCycle"));
		info->push_back(std::string("averageStepsPerSecond"));
		info->push_back(std::string("averageStepTime"));
		info->push_back(std::string("averageError"));
		info->push_back(std::string("isRunning"));

		result->push_back(shared_ptr<const RBX::Reflection::ValueArray>(info));
	}
	std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
	TaskScheduler::singleton().getJobsInfo(jobs);
	std::for_each(jobs.begin(), jobs.end(), boost::bind(appendJobInfo, this, _1, result.get()));
	return result;
}

void DataModel::setCreatorID(int creatorID, CreatorType creatorType)
{
	if(this->creatorID != creatorID)
	{
		this->creatorID = creatorID;
		raisePropertyChanged(prop_creatorId);
	}
	if(this->creatorType != creatorType)
	{
		this->creatorType = creatorType;
		raisePropertyChanged(prop_creatorType);
	}
}

int DataModel::getPlaceIDOrZeroInStudio()
{
	return runningInStudio ? 0 : getPlaceID();
}

static void updateFlagsOnPlaceFilter(const std::string& name, const std::string& varValue, void* context)
{
    if (!varValue.empty())
    {
        std::string prefix = "PlaceFilter_";
        std::size_t offset = name.find(prefix.c_str());
        if (offset != std::string::npos)
        {
            offset += prefix.length();
            DataModel* dm = reinterpret_cast<DataModel*>(context);
            std::string flagName = name.substr(offset);
            std::vector<std::string> places;
            boost::split(places, varValue, boost::is_any_of(";"));
            for (std::size_t i=1; i<places.size(); i++)
            {
                std::string placeIdStr = places[i];
                if (dm->getPlaceID() == atoi(placeIdStr.c_str()))
                {
                    FLog::SetValue(flagName, places[0] /*value*/, FASTVARTYPE_ANY, true);
                    break;
                }
            }
        }
    }
}

void DataModel::setPlaceID(int placeID, bool robloxPlace)
{
	FASTLOG1(FLog::CloseDataModel, "Setting place ID %u", placeID);
	RbxDbgInfo::AddPlace(placeID);
	
	if(this->placeID != placeID)
	{
		this->placeID = placeID;

		FLog::ForEachVariable(&updateFlagsOnPlaceFilter, this, FASTVARTYPE_ANY);

		raisePropertyChanged(prop_placeId);
	}

	if(ScriptContext* sc = create<ScriptContext>())
		sc->setRobloxPlace(robloxPlace);

	Http::placeID = RBX::format("%d", placeID);

	Analytics::setPlaceId(placeID);
	RobloxGoogleAnalytics::setPlaceID(placeID);
}


static void gameStartInfoLoadedHelperSuccess(weak_ptr<DataModel> dm, std::string json)
{
	FASTLOG(DFLog::R15Character, "DataModel::gameStartInfoLoadedHelperSuccess");

	shared_ptr<DataModel> dm_shared = dm.lock();
	if (dm_shared.get() == NULL)
		return;

	if (json.empty()) 
	{
		dm_shared->universeDataLoaded.set_value();
		dm_shared->clearUniverseDataRequested();
		return;
	}

	FASTLOGS(DFLog::R15Character, "DataModel::gameStartInfoLoadedHelperSuccess %s", json.c_str());

	RBX::Reflection::Variant v;
	if (RBX::WebParser::parseJSONObject(json, v))
	{
		bool forceR15 = v.cast<shared_ptr<const Reflection::ValueTable> >()->at("r15Morphing").cast<bool>();
		dm_shared->setForceR15(forceR15);
	}

	dm_shared->universeDataLoaded.set_value();
	dm_shared->clearUniverseDataRequested();
}

static void gameStartInfoLoadedHelperError(weak_ptr<DataModel> dm,  std::string error)
{
	FASTLOG(DFLog::R15Character, "DataModel::gameStartInfoLoadedHelperError");

	shared_ptr<DataModel> dm_shared = dm.lock();
	if (dm_shared.get() == NULL)
		return;

	FASTLOGS(DFLog::R15Character, "DataModel::gameStartInfoLoadedHelperError %s", error.c_str());

	// didn't get settings, just do nothing for now
	dm_shared->universeDataLoaded.set_value();
	dm_shared->clearUniverseDataRequested();
}

void DataModel::setForceR15(bool v) 
{ 
	if (v != forceR15) 
	{
		forceR15 = v; 
		raisePropertyChanged(prop_forceR15);
	}
}

void DataModel::setCanRequestUniverseInfo(bool value)
{
	FASTLOG(DFLog::R15Character, "DataModel::requestGameStartInfo setCanRequestUniverseInfo");

	if (value != canRequestUniverseInfo)
	{
		FASTLOG(DFLog::R15Character, "DataModel::requestGameStartInfo setCanRequestUniverseInfo setting");
		canRequestUniverseInfo = value;
		if (canRequestUniverseInfo && universeDataRequested && Network::Players::backendProcessing(this))
		{
			requestGameStartInfo();
		}
	}
}

void DataModel::requestGameStartInfo()
{
	FASTLOG(DFLog::R15Character, "DataModel::requestGameStartInfo universeDataRequested");

	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::create<RBX::HttpRbxApiService>(this))
	{
		apiService->getAsync( RBX::format("universes/%d/game-start-info", universeId),false, RBX::PRIORITY_DEFAULT,
			boost::bind(&gameStartInfoLoadedHelperSuccess, weak_from(this), _1),
			boost::bind(&gameStartInfoLoadedHelperError, weak_from(this), _1));
	}	
}


void DataModel::setUniverseId(int uId) 
{ 
	FASTLOG1(DFLog::R15Character, "DataModel::setUniverseId %d", uId);
	if (uId != universeId)
	{
		universeId = uId; 

		if (DFFlag::UseR15Character && Network::Players::backendProcessing(this))
		{
			FASTLOG(DFLog::R15Character, "DataModel::setUniverseId universeDataRequested");
			universeDataLoaded = boost::promise<void>();
			universeDataRequested = true;
			if (canRequestUniverseInfo)
				requestGameStartInfo();
		}
	}
}

void DataModel::setIsStudio(bool runningInStudio)
{
	this->runningInStudio = runningInStudio;
}

void DataModel::setIsRunMode(bool value)
{
	this->isStudioRunMode = value;
}

void DataModel::setPlaceVersion(int placeVersion)
{
	FASTLOG1(FLog::CloseDataModel, "Setting place version %u", placeVersion);
	if(this->placeVersion != placeVersion)
	{
		this->placeVersion = placeVersion;
		raisePropertyChanged(prop_placeVersion);
	}
}
bool DataModel::isGearTypeAllowed(GearType gearType)
{
	return (allowedGearTypes & (1 << gearType)) != 0;
}
void DataModel::loadPlugins()
{
	throw RBX::runtime_error("load plugins not supported");
}
void DataModel::setGenre(Genre genre)
{
	if(this->genre != genre)
	{
		this->genre = genre;
		raisePropertyChanged(prop_genre);
	}
}
void DataModel::setGear(GearGenreSetting gearGenreSetting, int allowedGearTypes)
{
	if(this->gearGenreSetting != gearGenreSetting)
	{
		this->gearGenreSetting = gearGenreSetting ;
		raisePropertyChanged(prop_gearGenreSetting);
	}

	if(this->allowedGearTypes != allowedGearTypes)
	{
		this->allowedGearTypes = allowedGearTypes;
		allowedGearTypeChanged();
	}
}

std::string DataModel::getVIPServerId() const
{
	if (RBX::Network::Players::clientIsPresent(this))
	{
		RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "VIPServerID checked on client, but only set on server.");
	}

	return vipServerId; 
}

void DataModel::setVIPServerId(std::string value)
{
	if (value != vipServerId)
	{
		vipServerId = value;
		raisePropertyChanged(desc_VIPServerId);
	}
}

int DataModel::getVIPServerOwnerId() const
{
	if (RBX::Network::Players::clientIsPresent(this))
	{
		RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "VIPServerOwnerID checked on client, but only set on server.");
	}

	return vipServerOwnerId; 
}

void DataModel::setVIPServerOwnerId(int value)
{
	if (value != vipServerOwnerId)
	{
		vipServerOwnerId = value;
		raisePropertyChanged(desc_VIPServerOwnerId);
	}
}

void DataModel::gameLoaded()
{
	if(!getIsGameLoaded())
		setIsGameLoaded(true);
}


void DataModel::setJobsExtendedStatsWindow(double seconds)
{
	TaskScheduler::singleton().setJobsExtendedStatsWindow(seconds);
}

static void appendJobExtendedStats(DataModel* dataModel, shared_ptr<const TaskScheduler::Job> job, Reflection::ValueArray* result)
{
	if (job->getArbiter().get()!=dataModel)
		return;

	shared_ptr<Reflection::ValueArray> info(rbx::make_shared<Reflection::ValueArray>());

	WindowAverageDutyCycle<>::Stats stats = job->getDutyCycleWindow().getStats();
	info->push_back(job->name);

	info->push_back(stats.time.average);
	info->push_back(stats.time.variance);
	info->push_back((double)stats.time.samples);
	info->push_back(stats.interval.average.seconds());
	info->push_back(stats.interval.variance.seconds());
	info->push_back((double)stats.interval.samples);
	info->push_back(stats.dutyfraction);

	result->push_back(shared_ptr<const RBX::Reflection::ValueArray>(info));

}

shared_ptr<const Reflection::ValueArray> DataModel::getJobsExtendedStats()
{
	// Returns some nice diagnostic information
	shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>());
	{
		
		shared_ptr<Reflection::ValueArray> info(rbx::make_shared<Reflection::ValueArray>());
		info->push_back(std::string("name"));
		info->push_back(std::string("time.average"));
		info->push_back(std::string("time.variance"));
		info->push_back(std::string("time.samples"));
		info->push_back(std::string("interval.average"));
		info->push_back(std::string("interval.variance"));
		info->push_back(std::string("interval.samples"));
		info->push_back(std::string("dutyfraction"));

		result->push_back(shared_ptr<const RBX::Reflection::ValueArray>(info));
	}
	std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
	TaskScheduler::singleton().getJobsInfo(jobs);
	std::for_each(jobs.begin(), jobs.end(), boost::bind(appendJobExtendedStats, this, _1, result.get()));
	return result;
}

static void getJobTimePeakFractionFunc(DataModel* dataModel, shared_ptr<const TaskScheduler::Job> job, std::string& jobname, double greaterThan, double* result)
{
	if (job->getArbiter().get()!=dataModel)
		return;

	const WindowAverageDutyCycle<>& data = job->getDutyCycleWindow();

	*result = data.countTimesGreaterThan(Time::Interval(greaterThan));
	*result /= data.timesamples();
}


double DataModel::getJobTimePeakFraction(std::string jobname, double greaterThan)
{
	double result = -1.0;
	std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
	TaskScheduler::singleton().getJobsByName(jobname, jobs);
	std::for_each(jobs.begin(), jobs.end(), boost::bind(getJobTimePeakFractionFunc, this, _1, jobname, greaterThan, &result));
	return result;
}

static void getJobIntervalPeakFractionFunc(DataModel* dataModel, shared_ptr<const TaskScheduler::Job> job, std::string& jobname, double greaterThan, double* result)
{
	if (job->getArbiter().get()!=dataModel)
		return;

	const WindowAverageDutyCycle<>& data = job->getDutyCycleWindow();

	*result = data.countIntervalsGreaterThan(Time::Interval(greaterThan));
	*result /= data.intervalsamples();
}


double DataModel::getJobIntervalPeakFraction(std::string jobname, double greaterThan)
{
	double result = -1.0;
	std::vector<boost::shared_ptr<const TaskScheduler::Job> > jobs;
	TaskScheduler::singleton().getJobsByName(jobname, jobs);
	std::for_each(jobs.begin(), jobs.end(), boost::bind(getJobIntervalPeakFractionFunc, this, _1, jobname, greaterThan, &result));
	return result;
}

/*override*/ void DataModel::onChildChanged(Instance* instance, const PropertyChanged& event) {
	RBXASSERT(write_requested==1);
	Super::onChildChanged(instance, event);
	this->setDirty(true);
    if (!itemChangedSignal.empty())
    	itemChangedSignal(shared_from(instance), &event.getDescriptor());
}
/*override*/ void DataModel::onDescendantAdded(Instance* instance) {
	RBXASSERT(write_requested==1);

	// keep track of how instances this datamodel has
	numInstances++;
	if (Instance::fastDynamicCast<PartInstance>(instance))
		numPartInstances++;

	Super::onDescendantAdded(instance);
	this->setDirty(true);
}
/*override*/ void DataModel::onDescendantRemoving(const shared_ptr<Instance>& instance) {
	RBXASSERT(write_requested==1);

	numInstances--;
	if (Instance::fastDynamicCast<PartInstance>(instance.get()))
		numPartInstances--;

	Super::onDescendantRemoving(instance);
	this->setDirty(true);
}

int DataModel::getNumPlayers() const
{
	const Network::Players* players = find<Network::Players>();
	if (!players)
		return 0;
	return
		players->getNumPlayers();
}

void DataModel::setUiMessage(std::string message)
{
	uiMessage = message;
}

void DataModel::toggleToolsOff()
{
	Verb* verb = getWhitelistVerb("Toggle", "Play", "Mode");

	// Only toggle if tools are enabled, so user can't use it to turn tools back on
	if (verb!=NULL && verb->isChecked()){
		Verb::doItWithChecks(verb, NULL);
	}
}

void DataModel::clearUiMessage() 
{
	uiMessage = "";
}

void DataModel::setUiMessageBrickCount()
{
	uiMessage = "[[[progress]]]";
}

void DataModel::TakeScreenshotTask(weak_ptr<RBX::DataModel> weakDataModel)
{
	if (shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock())
	{
		dataModel->screenshotSignal();
	}
}

void DataModel::ScreenshotReadyTask(weak_ptr<RBX::DataModel> weakDataModel, const std::string &filename)
{
	if (shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock())
	{
		dataModel->screenshotReadySignal(filename);
	}
}

void DataModel::ScreenshotUploadTask(weak_ptr<RBX::DataModel> weakDataModel, bool finished)
{
	if (shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock())
	{
		dataModel->screenshotUploadSignal(finished);
	}
}

void DataModel::ShowMessage(weak_ptr<RBX::DataModel> weakDataModel, int slot, const std::string &message, double duration)
{
	if (shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock())
	{
		if(shared_ptr<RBX::CoreGuiService> coreGuiService = shared_from(dataModel->find<RBX::CoreGuiService>()))
		{
			coreGuiService->displayOnScreenMessage(slot, message, duration);
		}
	}
}

bool DataModel::currentThreadHasWriteLock() const
{
	// TODO: Do we need a mutex around this?
	return writeRequestingThread == GetCurrentThreadId();
}

bool DataModel::currentThreadHasWriteLock(Instance* context)
{
    DataModel* dm = get(context);
    return !dm || dm->currentThreadHasWriteLock();
}

Instance* DataModel::getLightingDeprecated() const
{
	return ServiceProvider::find<Lighting>(this);
}

DataModel::scoped_write_request::scoped_write_request(Instance* context)
	: dataModel(DataModel::get(context))
{
	if (dataModel)
	{
		RBX::mutex::scoped_lock lock(dataModel->debugLock);

		RBXASSERT(dataModel->write_requested == 0);
		RBXASSERT(dataModel->read_requested == 0);
		RBXASSERT(dataModel->writeRequestingThread == 0);

		dataModel->write_requested = 1;
		dataModel->writeRequestingThread = GetCurrentThreadId();
	}
}

DataModel::scoped_write_request::~scoped_write_request()
{
	if (dataModel)
	{
		RBX::mutex::scoped_lock lock(dataModel->debugLock);

		RBXASSERT(dataModel->write_requested == 1);
		RBXASSERT(dataModel->read_requested == 0);
		RBXASSERT(dataModel->writeRequestingThread == GetCurrentThreadId());

		dataModel->write_requested = 0;
		dataModel->writeRequestingThread = 0;
	}
}

// Place this code around tasks that write to a DataModel
DataModel::scoped_read_request::scoped_read_request(Instance* context)
	: dataModel(DataModel::get(context))
{
	if (dataModel)
	{
		RBX::mutex::scoped_lock lock(dataModel->debugLock);

		RBXASSERT(dataModel->write_requested == 0);

		dataModel->read_requested++;
	}
}

DataModel::scoped_read_request::~scoped_read_request()
{
	if (dataModel)
	{
		RBX::mutex::scoped_lock lock(dataModel->debugLock);

		RBXASSERT(dataModel->write_requested == 0);
		RBXASSERT(dataModel->read_requested > 0);

		dataModel->read_requested--;
	}
}

DataModel::scoped_write_transfer::scoped_write_transfer(Instance* context)
	: dataModel(DataModel::get(context))
	, oldWritingThread(0)
{
	if (dataModel)
	{
		RBX::mutex::scoped_lock lock(dataModel->debugLock);

		RBXASSERT(dataModel->write_requested == 1);
		RBXASSERT(dataModel->writeRequestingThread != GetCurrentThreadId());
		RBXASSERT(dataModel->writeRequestingThread != 0);

		oldWritingThread = dataModel->writeRequestingThread;
		dataModel->writeRequestingThread = GetCurrentThreadId();
	}
}

DataModel::scoped_write_transfer::~scoped_write_transfer()
{
	if (dataModel)
	{
		RBX::mutex::scoped_lock lock(dataModel->debugLock);

		RBXASSERT(dataModel->write_requested == 1);
		RBXASSERT(dataModel->writeRequestingThread == GetCurrentThreadId());

		dataModel->writeRequestingThread = oldWritingThread;
	}
}


unsigned int DataModel::allHackFlagsOredTogether() {
    unsigned int result = 0;
#if !defined(RBX_STUDIO_BUILD)
	VMProtectBeginMutation("18");
	boost::mutex::scoped_lock l(hackFlagSetMutex);
	
	for (boost::unordered_set<unsigned int>::iterator itr = hackFlagSet.begin();
			itr != hackFlagSet.end(); ++itr) {
		result |= *itr;
	}
	VMProtectEnd();
#endif
	return result;
}

void DataModel::processHttpRequestResponseOnLock(DataModel *dataModel, std::string* response, std::exception* exception, boost::function<void(shared_ptr<std::string>,shared_ptr<std::exception> exception)> onLockAcquired)
{
	if (dataModel == NULL)
	{
		return;
	}

	shared_ptr<std::string> responseCopy(response ? new std::string(*response) : NULL);
	shared_ptr<std::exception> exceptionCopy(exception ? new std::exception(*exception) : NULL);
	if (DFFlag::DataModelProcessHttpRequestResponseOnLockUseSubmitTask)
	{
		dataModel->submitTask(
			boost::bind(onLockAcquired, responseCopy, exceptionCopy),
			RBX::DataModelJob::TaskType::Write);
	}
	else
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		onLockAcquired(responseCopy, exceptionCopy);
	}
}

} // namespace
