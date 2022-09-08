/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#include "Network/Player.h"

#include "Client.h"
#include "ClientReplicator.h"
#include "Network/Players.h"
#include "RobloxServicesTools.h"
#include "V8DataModel/GuiService.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/PlayerScripts.h"
#include "V8DataModel/TextButton.h"
#include "V8Datamodel/Tool.h"
#include "V8Datamodel/ModelInstance.h"
#include "V8Datamodel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/ImageButton.h"
#include "V8DataModel/ImageLabel.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/SpawnLocation.h"
#include "V8DataModel/CharacterAppearance.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/SocialService.h"
#include "V8DataModel/TimerService.h"
#include "V8DataModel/DataModelMesh.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/Value.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/PersonalServerService.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "Gui/ProfanityFilter.h"
#include "Util/LuaWebService.h"
#include "Humanoid/Humanoid.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "V8World/DistributedPhysics.h"
#include "Security/SecurityContext.h"
#include "v8xml/xmlserializer.h"
#include "v8xml/serializer.h"
#include "v8xml/WebSerializer.h"
#include "Script/Script.h"
#include "Script/CoreScript.h"
#include "Script/ScriptContext.h"
#include "Util/StandardOut.h"
#include "Util/Region2.h"
#include "Util/Http.h"
#include "Util/AsyncHttpQueue.h"
#include "Util/Statistics.h"
#include "FastLog.h"
#include "format_string.h"
#include "v8world/ContactManager.h"
#include "Util.h"
#include <rapidjson/document.h>
#include "v8xml/WebParser.h"

#include "GfxBase/Adorn.h"
#include "AppDraw/DrawAdorn.h"
#include "PersistentDataStore.h"

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include<v8datamodel/FriendService.h>

#include "V8DataModel/PlayerMouse.h" // for future differences between PlayerMouse and Mouse objects
#include "V8DataModel/UserInputService.h" 
#include "Server.h"
#include "NetworkOwnerJob.h"
#include "V8DataModel/TeleportService.h"

const char* const RBX::Network::sPlayer = "Player";
static const char* const kPlaceIdUrlParamFormat = "&serverplaceid=%d";

static const char* const kValidCharacterAppearancePaths[] =
{
	"asset/characterfetch.ashx",
	"asset/avataraccoutrements.ashx"
};

using namespace RBX;
using namespace RBX::Network;

DYNAMIC_LOGVARIABLE(PlayerChatInfoExponentialBackoffLimitMultiplier, 9)

DYNAMIC_LOGGROUP(NetworkJoin)
DYNAMIC_LOGGROUP(PartStreamingRequests)

DYNAMIC_FASTFLAG(UseStarterPlayerHumanoid)
DYNAMIC_FASTFLAG(UseStarterPlayerCharacterScripts)
DYNAMIC_FASTFLAG(UseStarterPlayerCharacter)

DYNAMIC_FASTFLAG(LoadGuisWithoutChar);
DYNAMIC_FASTFLAGVARIABLE(ApiCapitalizationChanges, false)

DYNAMIC_FASTFLAGVARIABLE(LoadStarterGearWithoutLoadCharacter, false)
DYNAMIC_FASTFLAGVARIABLE(ValidateCharacterAppearanceUrl, false)
DYNAMIC_FASTFLAGVARIABLE(FilterKickMessage, false)

DYNAMIC_FASTFLAGVARIABLE(UseR15Character, false)
DYNAMIC_FASTFLAGVARIABLE(CloudEditDisablePlayerDestroy, false)

static const int idleCheckFrequency = 30; // check for idleness every X seconds
static const Time::Interval idlePeriod(2*60);		  // length of time a player must be inactive to qualify as being "idle"
static float defaultDisplayDistance(100.0f);

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Player, void()>    func_loadData(&Player::loadData, "LoadData", Security::LocalUser);
static Reflection::BoundFuncDesc<Player, void()>    func_saveData(&Player::saveData, "SaveData", Security::LocalUser);
static Reflection::BoundFuncDesc<Player, void()>    func_saveLeaderboardData(&Player::saveLeaderboardData, "SaveLeaderboardData", Security::LocalUser);

static Reflection::PropDescriptor<Player, bool>					prop_groupBuildTools("HasBuildTools", category_Data, &Player::getHasGroupBuildTools, &Player::setHasGroupBuildTools, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<Player, int>					prop_personalServerRank("PersonalServerRank", category_Data, &Player::getPersonalServerRank, &Player::setPersonalServerRank, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<Player, std::string()>	func_getWebPersonalServerRank(&Player::getWebPersonalServerRank, "GetWebPersonalServerRank", Security::LocalUser);
static Reflection::BoundYieldFuncDesc<Player, bool(int)>		func_setPersonalServerRank(&Player::setWebPersonalServerRank, "SetWebPersonalServerRank","rank", Security::WritePlayer);


static Reflection::PropDescriptor<Player, int>		prop_dataComplexity("DataComplexity", category_Data, &Player::getDataComplexity, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Player, int>		prop_dataComplexityLimit("DataComplexityLimit", category_Data, &Player::getDataComplexityLimit, &Player::setDataComplexityLimit, Reflection::PropertyDescriptor::UI, Security::LocalUser);

static Reflection::PropDescriptor<Player, bool> prop_dataReady("DataReady", category_Data, &Player::getDataReady, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::BoundYieldFuncDesc<Player, bool()> func_waitForDataReady(&Player::waitForDataReady, "WaitForDataReady", Security::None);
static Reflection::BoundYieldFuncDesc<Player, bool()> func_dep_waitForDataReady(&Player::waitForDataReady, "waitForDataReady", Security::None, Reflection::Descriptor::Attributes::deprecated(func_waitForDataReady));

static Reflection::BoundFuncDesc<Player, void(shared_ptr<Instance>)> func_requestFriendship(&Player::requestFriendship, "RequestFriendship", "player", Security::RobloxScript);
static Reflection::BoundFuncDesc<Player, void(shared_ptr<Instance>)> func_revokeFriendship(&Player::revokeFriendship, "RevokeFriendship", "player", Security::RobloxScript);
static Reflection::RemoteEventDesc<Player, void(bool,int)> desc_remoteFriendServiceSignal(&Player::remoteFriendServiceSignal, "RemoteFriendServiceSignal", "makeFriends", "userId", Security::RobloxScript, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::BoundFuncDesc<Player, shared_ptr<Instance>()> func_getMouse(&Player::getMouseInstance, "GetMouse", Security::None);

static Reflection::BoundFuncDesc<Player, std::string(std::string)>    func_loadString(&Player::loadString, "LoadString", "key", Security::None);
static Reflection::BoundFuncDesc<Player, void(std::string, std::string)>    func_saveString(&Player::saveString, "SaveString", "key", "value", Security::None);
static Reflection::BoundFuncDesc<Player, bool(std::string)>    func_loadBoolean(&Player::loadBoolean, "LoadBoolean", "key", Security::None);
static Reflection::BoundFuncDesc<Player, void(std::string, bool)>    func_saveBoolean(&Player::saveBoolean, "SaveBoolean", "key", "value", Security::None);
static Reflection::BoundFuncDesc<Player, double(std::string)>    func_loadNumber(&Player::loadNumber, "LoadNumber", "key", Security::None);
static Reflection::BoundFuncDesc<Player, void(std::string, double)>    func_saveNumber(&Player::saveNumber, "SaveNumber", "key", "value", Security::None);
static Reflection::BoundFuncDesc<Player, shared_ptr<Instance>(std::string)>    func_loadInstance(&Player::loadInstance, "LoadInstance", "key", Security::None);
static Reflection::BoundFuncDesc<Player, void(std::string, shared_ptr<Instance>)>    func_saveInstance(&Player::saveInstance, "SaveInstance", "key", "value", Security::None);

static Reflection::BoundFuncDesc<Player, std::string(std::string)>    dep_loadString(&Player::loadString, "loadString", "key", Security::None, Reflection::Descriptor::Attributes::deprecated(func_loadString));
static Reflection::BoundFuncDesc<Player, void(std::string, std::string)>    dep_saveString(&Player::saveString, "saveString", "key", "value", Security::None, Reflection::Descriptor::Attributes::deprecated(func_saveString));
static Reflection::BoundFuncDesc<Player, bool(std::string)>    dep_loadBoolean(&Player::loadBoolean, "loadBoolean", "key", Security::None, Reflection::Descriptor::Attributes::deprecated(func_loadBoolean));
static Reflection::BoundFuncDesc<Player, void(std::string, bool)>    dep_saveBoolean(&Player::saveBoolean, "saveBoolean", "key", "value", Security::None, Reflection::Descriptor::Attributes::deprecated(func_saveBoolean));
static Reflection::BoundFuncDesc<Player, double(std::string)>    dep_loadNumber(&Player::loadNumber, "loadNumber", "key", Security::None, Reflection::Descriptor::Attributes::deprecated(func_loadNumber));
static Reflection::BoundFuncDesc<Player, void(std::string, double)>    dep_saveNumber(&Player::saveNumber, "saveNumber", "key", "value", Security::None, Reflection::Descriptor::Attributes::deprecated(func_saveNumber));
static Reflection::BoundFuncDesc<Player, shared_ptr<Instance>(std::string)>    dep_loadInstance(&Player::loadInstance, "loadInstance", "key", Security::None, Reflection::Descriptor::Attributes::deprecated(func_loadInstance));
static Reflection::BoundFuncDesc<Player, void(std::string, shared_ptr<Instance>)>    dep_saveInstance(&Player::saveInstance, "saveInstance", "key", "value", Security::None, Reflection::Descriptor::Attributes::deprecated(func_saveInstance));

#ifdef _ADVANCED_PERSISTENCE_TYPES_
static Reflection::BoundFuncDesc<Player, shared_ptr<const Reflection::ValueArray>(std::string)>    func_loadList(&Player::loadList, "LoadList", "key", Security::None);
static Reflection::BoundFuncDesc<Player, void(std::string, shared_ptr<const Reflection::ValueArray>)>    func_saveList(&Player::saveList, "SaveList", "key", "value", Security::None);
static Reflection::BoundFuncDesc<Player, shared_ptr<const RBX::Reflection::ValueMap>(std::string)>    func_loadTable(&Player::loadTable, "LoadTable", "key", Security::None);
static Reflection::BoundFuncDesc<Player, void(std::string, shared_ptr<const RBX::Reflection::ValueMap>)>    func_saveTable(&Player::saveTable, "SaveTable", "key", "value", Security::None);
#endif

static Reflection::BoundFuncDesc<Player, void(Vector3, bool)>  moveCharacterFunction(&Player::move, "Move", "walkDirection", "relativeToCamera", false, Security::None);

// MoveCharacter + JumpCharacter only used by lua control core scripts
static Reflection::BoundFuncDesc<Player, void(Vector2,float)>    moveLocalCharacterFunction(&Player::luaMoveCharacter, "MoveCharacter", "walkDirection", "maxWalkDelta", Security::RobloxScript);
static Reflection::BoundFuncDesc<Player, void()>    jumpLocalCharacterFunction(&Player::luaJumpCharacter, "JumpCharacter", Security::RobloxScript);

static Reflection::BoundFuncDesc<Player, void(bool)>    loadCharacterFunction(&Player::luaLoadCharacter, "LoadCharacter", "inGame", true, Security::None);
static Reflection::BoundFuncDesc<Player, void()>    removeCharacterFunction(&Player::removeCharacter, "RemoveCharacter", Security::LocalUser);
static Reflection::BoundFuncDesc<Player, void(bool)>    func_SetUnder13(&Player::setUnder13, "SetUnder13", "value", Security::Roblox);
static Reflection::BoundFuncDesc<Player, bool()>    func_GetUnder13(&Player::getUnder13, "GetUnder13", Security::RobloxScript);
static Reflection::BoundFuncDesc<Player, void(bool)>    func_SetSuperSafeChat(&Player::setSuperSafeChat, "SetSuperSafeChat", "value", Security::Plugin);
static Reflection::BoundFuncDesc<Player, void(Player::MembershipType)>  func_SetMembershipType(&Player::setMembershipType, "SetMembershipType", "membershipType", Security::RobloxScript);
static Reflection::BoundFuncDesc<Player, void(int)>  func_SetAccountAge(&Player::setAccountAge, "SetAccountAge", "accountAge", Security::Plugin);
static Reflection::BoundFuncDesc<Player, void(std::string)>    func_Kick(&Player::kick, "Kick", "message", "", Security::None);
static Reflection::BoundFuncDesc<Player, std::string()> func_GetGameSessionID(&Player::getGameSessionID, "GetGameSessionID", Security::Roblox);
static Reflection::RemoteEventDesc<Player, void(std::string)> event_setShutdownMessage(&Player::setShutdownMessageSignal, 
																								 "SetShutdownMessage", "message", Security::Roblox, 
																								 Reflection::RemoteEventCommon::REPLICATE_ONLY, 
																								 Reflection::RemoteEventCommon::CLIENT_SERVER);

// This property is used by the replication framework to propagate user positions within a multi-player game
static Reflection::RefPropDescriptor<Player, ModelInstance> prop_Character("Character", category_Data, &Player::getDangerousCharacter, &Player::setCharacter);
static Reflection::PropDescriptor<Player, std::string> prop_characterAppearance("CharacterAppearance", category_Data, &Player::getCharacterAppearance, &Player::setCharacterAppearance);
static Reflection::PropDescriptor<Player, bool> prop_canLoadCharacterAppearance("CanLoadCharacterAppearance", category_Behavior, &Player::getCanLoadCharacterAppearance, &Player::setCanLoadCharacterAppearance, Reflection::PropertyDescriptor::SCRIPTING, Security::None);
static Reflection::BoundFuncDesc<Player, void()>  func_RemoveCharacterAppearance(&Player::removeCharacterAppearanceScript, "ClearCharacterAppearance", Security::None);
static Reflection::BoundFuncDesc<Player, void(shared_ptr<Instance>)> func_LoadCharacterAppearanceScript(&Player::loadCharacterAppearanceScript, "LoadCharacterAppearance","assetInstance", Security::None);

static Reflection::RefPropDescriptor<Player, SpawnLocation> prop_RespawnLocation("RespawnLocation", category_Data, &Player::getDangerousRespawnLocation, &Player::setRespawnLocation);
Reflection::PropDescriptor<Player, int> Player::prop_userId("UserId", category_Data, &Player::getUserID, &Player::setUserId, Reflection::PropertyDescriptor::SCRIPTING);
Reflection::PropDescriptor<Player, int> Player::prop_userIdDeprecated("userId", category_Data, &Player::getUserID, &Player::setUserId, Reflection::PropertyDescriptor::Attributes::deprecated(Player::prop_userId, Reflection::PropertyDescriptor::SCRIPTING));
static Reflection::BoundFuncDesc<Player, float(Vector3)> func_CharacterDistance(&Player::distanceFromCharacter, "DistanceFromCharacter", "point", Security::None);

static Reflection::BoundFuncDesc<Player, FriendService::FriendStatus(shared_ptr<Instance>)> func_GetFriendStatus(&Player::getFriendStatus, "GetFriendStatus", "player", Security::RobloxScript);

static Reflection::BoundYieldFuncDesc<Player, bool(int)> func_IsFriendsWith(&Player::isFriendsWith, "IsFriendsWith", "userId", Security::None);
static Reflection::BoundYieldFuncDesc<Player, bool(int)> dep_IsFriendsWith(&Player::isFriendsWith, "isFriendsWith", "userId", Security::None, Reflection::Descriptor::Attributes::deprecated(func_IsFriendsWith));
static Reflection::BoundYieldFuncDesc<Player, bool(int)> func_IsBestFriendsWith(&Player::isBestFriendsWith, "IsBestFriendsWith", "userId", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundYieldFuncDesc<Player, bool(int)> func_IsInGroup(&Player::isInGroup, "IsInGroup", "groupId", Security::None);
static Reflection::BoundYieldFuncDesc<Player, shared_ptr<const Reflection::ValueArray>(int)> func_getFriendsOnline(&Player::getFriendsOnline, "GetFriendsOnline", "maxFriends", 200, Security::None); 

// New Group Stats API
static Reflection::BoundYieldFuncDesc<Player, int(int)> func_getRankInGroup(&Player::getRankInGroup, "GetRankInGroup", "groupId", Security::None);
static Reflection::BoundYieldFuncDesc<Player, std::string(int)> func_getRoleInGroup(&Player::getRoleInGroup, "GetRoleInGroup", "groupId", Security::None);

// TODO: Security - put RBX::Security::WritePlayer checks on these properties (like userId has)
Reflection::PropDescriptor<Player, bool> Player::prop_SuperSafeChat("SuperSafeChatReplicate", category_Data, &Player::getSuperSafeChat, &Player::setSuperSafeChat, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::LocalUser);

Reflection::BoundProp<std::string> Player::prop_OsPlatform("OsPlatform", category_Data, &Player::osPlatform, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::RobloxScript);
Reflection::BoundProp<std::string> Player::prop_VRDevice("VRDevice", category_Data, &Player::vrDevice, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);

static Reflection::RemoteEventDesc<Player, void(float)> event_simulationRadiusChanged(&Player::simulationRadiusChangedSignal, "SimulationRadiusChanged", "radius", Security::LocalUser, Reflection::RemoteEventCommon::SCRIPTING,	Reflection::RemoteEventCommon::CLIENT_SERVER);
Reflection::PropDescriptor<Player, float> Player::prop_DeprecatedMaxSimulationRadius("MaxSimulationRadius" /*keep this name*/, category_Data, &Player::getDeprecatedMaxSimulationRadius, &Player::setDeprecatedMaxSimulationRadius, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::LocalUser);
Reflection::BoundProp<float> Player::prop_MaxSimulationRadius("MaximumSimulationRadius", category_Data, &Player::maxSimulationRadius, Reflection::PropertyDescriptor::UI, Security::TestLocalUser);
Reflection::BoundProp<float> Player::prop_SimulationRadius("SimulationRadius", category_Data, &Player::simulationRadius, Reflection::PropertyDescriptor::SCRIPTING, Security::TestLocalUser);

Reflection::EnumPropDescriptor<Player, Player::ChatMode> prop_ChatMode("ChatMode", category_Data, &Player::getChatMode, NULL, Reflection::PropertyDescriptor::UI, Security::RobloxScript);

static Reflection::PropDescriptor<Player, BrickColor/**/> prop_teamColor ("TeamColor", category_Team, &Player::getTeamColor, &Player::setTeamColor);
static Reflection::PropDescriptor<Player, bool/**/> prop_neutral("Neutral", category_Team, &Player::getNeutral, &Player::setNeutral);

static Reflection::PropDescriptor<Player, bool> prop_autoJumpEnabled("AutoJumpEnabled", category_Data, &Player::getAutoJumpEnabled, &Player::setAutoJumpEnabled);

static Reflection::PropDescriptor<Player, bool> prop_Guest("Guest", category_Data, &Player::isGuest, NULL, Reflection::PropertyDescriptor::UI, Security::RobloxScript);
static Reflection::EnumPropDescriptor<Player, Player::MembershipType> prop_membershipType("MembershipType", category_Data, &Player::getMembershipType, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::EnumPropDescriptor<Player, Player::MembershipType/**/> prop_membershipTypeReplicate("MembershipTypeReplicate", category_Data, &Player::getMembershipType, &Player::setMembershipType, Reflection::PropertyDescriptor::REPLICATE_ONLY);

static Reflection::PropDescriptor<Player, int> prop_accountAge("AccountAge", category_Data, &Player::getAccountAge, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Player, int/**/> prop_accountAgeReplicate("AccountAgeReplicate", category_Data, &Player::getAccountAge, &Player::setAccountAge, Reflection::PropertyDescriptor::REPLICATE_ONLY);

static Reflection::EventDesc<Player, void(std::string, shared_ptr<Instance>)> event_Chatted(&Player::chattedSignal, "Chatted", "message", "recipient");
static Reflection::EventDesc<Player, void(shared_ptr<Instance>)> event_CharacterAdded(&Player::characterAddedSignal, "CharacterAdded", "character");
static Reflection::RemoteEventDesc<Player, void(shared_ptr<Instance>)> event_CharacterLoaded(&Player::CharacterAppearanceLoadedSignal, "CharacterAppearanceLoaded", "character", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::EventDesc<Player, void(shared_ptr<Instance>)> event_CharacterRemoving(&Player::characterRemovingSignal, "CharacterRemoving", "character");
static Reflection::EventDesc<Player, void(double)> event_Idled(&Player::idledSignal, "Idled", "time");
static Reflection::EventDesc<Player, void(shared_ptr<Instance>, FriendService::FriendStatus)> event_FriendStatusChanged(&Player::friendStatusChangedSignal, "FriendStatusChanged", "player", "friendStatus", Security::RobloxScript);

static Reflection::RemoteEventDesc<Player, void()> desc_kill(&Player::killPlayerSignal, "Kill", Security::LocalUser, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::RemoteEventDesc<Player, void(std::string, std::string, std::string)> desc_scriptSecurityError(&Player::scriptSecurityErrorSignal, "ScriptSecurityError", "hash", "error", "stack", Security::LocalUser, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Player, void(std::string, Vector3)> desc_remoteInsert(&Player::remoteInsertSignal, "RemoteInsert", "url", "position", Security::LocalUser, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::RemoteEventDesc<Player, void(std::string)> desc_statsSignal(&Player::statsSignal, "StatsAvailable", "info", Security::LocalUser, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::PropDescriptor<Player, bool> prop_appearanceDidLoad("AppearanceDidLoad", category_Data, &Player::getAppearanceDidLoad, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(Reflection::PropertyDescriptor::UI), Security::RobloxScript);
static Reflection::BoundFuncDesc<Player, bool()> func_AppearanceDidLoad(&Player::getAppearanceDidLoadNonConst, "HasAppearanceLoaded", Security::None);

static Reflection::EnumPropDescriptor<Player, Camera::CameraMode> prop_cameraMode("CameraMode", "Camera", &Player::getCameraMode, &Player::setCameraMode);
static Reflection::PropDescriptor<Player, float> prop_cameraMaxZoomDistance("CameraMaxZoomDistance", "Camera", &Player::getCameraMaxZoomDistance, &Player::setCameraMaxZoomDistance);
static Reflection::PropDescriptor<Player, float> prop_cameraMinZoomDistance("CameraMinZoomDistance", "Camera", &Player::getCameraMinZoomDistance, &Player::setCameraMinZoomDistance);

static Reflection::EnumPropDescriptor<Player, StarterPlayerService::DeveloperTouchCameraMovementMode> prop_devTouchCameraMode("DevTouchCameraMode", "Camera", &Player::getDevTouchCameraMode, &Player::setDevTouchCameraMode);
static Reflection::EnumPropDescriptor<Player, StarterPlayerService::DeveloperComputerCameraMovementMode> prop_devComputerCameraMode("DevComputerCameraMode", "Camera", &Player::getDevComputerCameraMode, &Player::setDevComputerCameraMode);
static Reflection::PropDescriptor<Player, bool> prop_devEnableMouseLockOption("DevEnableMouseLock", "Camera", &Player::getDevEnableMouseLockOption, &Player::setDevEnableMouseLockOption);
static Reflection::EnumPropDescriptor<Player, StarterPlayerService::DeveloperCameraOcclusionMode> prop_devCameraOcclusionMode("DevCameraOcclusionMode", "Camera", &Player::getDevCameraOcclusionMode, &Player::setDevCameraOcclusionMode);

static Reflection::EnumPropDescriptor<Player, StarterPlayerService::DeveloperTouchMovementMode> prop_devTouchMovementMode("DevTouchMovementMode", category_Control, &Player::getDevTouchMovementMode, &Player::setDevTouchMovementMode);
static Reflection::EnumPropDescriptor<Player, StarterPlayerService::DeveloperComputerMovementMode> prop_devComputerMovementMode("DevComputerMovementMode", category_Control, &Player::getDevComputerMovementMode, &Player::setDevComputerMovementMode);

static Reflection::PropDescriptor<Player, float> prop_nameDisplayDistance("NameDisplayDistance", "Camera", &Player::getNameDisplayDistance, &Player::setNameDisplayDistance);
static Reflection::PropDescriptor<Player, float> prop_healthDisplayDistance("HealthDisplayDistance", "Camera", &Player::getHealthDisplayDistance, &Player::setHealthDisplayDistance);

static Reflection::RemoteEventDesc<Player, void()>	event_connectDiedSignal(&Player::connectDiedSignalBackend, "ConnectDiedSignalBackend", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::PropDescriptor<Player, int> prop_getFollowUserIdReplicated("FollowUserIdReplicated", category_Data, &Player::getFollowUserId, &Player::setFollowUserId, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::None);
static Reflection::PropDescriptor<Player, int> prop_getFollowUserId("FollowUserId", category_Data, &Player::getFollowUserId, NULL);

static Reflection::PropDescriptor<Player, CoordinateFrame> prop_CloudEditCameraCoordinateFrame("CloudEditCameraCoordinateFrame", category_Data, &Player::getCloudEditCameraCoordinateFrame, &Player::setCloudEditCameraCoordinateFrame, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::None);
Reflection::RemoteEventDesc<Player, void(shared_ptr<const Reflection::ValueArray>)> Player::event_cloudEditSelectionChanged(&Player::cloudEditSelectionChanged, "CloudEditSelectionChanged", "newSelection", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);

// Teleport related
static Reflection::RemoteEventDesc<Player, void(TeleportService::TeleportState, int, std::string)> event_OnTeleport(&Player::onTeleportSignal, "OnTeleport", "teleportState", "placeId", "spawnName", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Player, void(TeleportService::TeleportState, shared_ptr<const Reflection::ValueTable>, shared_ptr<Instance>)> event_OnTeleportInternal(&Player::onTeleportInternalSignal, "OnTeleportInternal", "teleportState", "teleportInfo", "customLoadingScreen", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::PropDescriptor<Player, bool> prop_teleported("Teleported", category_Data, &Player::getTeleported, NULL, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<Player, bool> prop_teleportedIn("TeleportedIn", category_Data, &Player::getTeleportedIn, &Player::setTeleportedIn, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
REFLECTION_END();


//#define REMOVE_PLAYER_PROTECTIONS 1
Player::Player(void)
	:userId(0)
	,superSafeChat(false)
	,under13(true)
	,simulationRadius(DistributedPhysics::MIN_CLIENT_SIMULATION_DISTANCE())
	,maxSimulationRadius(DistributedPhysics::MAX_CLIENT_SIMULATION_DISTANCE())
	,teamColor(BrickColor::brickWhite())
	,neutral(true)
	,loadAppearanceCounter(0)
	,dataReady(false)
	,membershipType(Player::MEMBERSHIP_NONE)
	,accountAge(0)
	,chatInfoHasBeenLoaded(false)
	,chatFilterType(CHAT_FILTER_WHITELIST)
	,dataComplexityLimit(45000)
	,hasGroupBuildTools(false)
	,personalServerRank(0)
	,loadingInstances(0)
	,appearanceDidLoad(false)
	,characterAppearanceLoaded(false)
	,canLoadCharacterAppearance(true)
	,cameraMode(RBX::Camera::CAMERAMODE_CLASSIC)
	,forceEarlySpawnLocationCalculation(false)
	,hasSpawnedAtLeastOnce(false)
	,teleportSpawnName()
	,nameDisplayDistance(defaultDisplayDistance)
	,healthDisplayDistance(defaultDisplayDistance)
	,cameraMaxZoomDistance(RBX::Camera::distanceMaxCharacter())
	,cameraMinZoomDistance(RBX::Camera::distanceMin())
	,copiedGuiOnce(false)
	,followUserId(0)
	,enableMouseLockOption(true)
	,touchCameraMovementMode(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER)
	,computerCameraMovementMode(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER)
	,cameraOcclusionMode(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_ZOOM)
	,touchMovementMode(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_USER)
	,computerMovementMode(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_USER)
	,loadedStarterGear(false)
	,respawnLocation(shared_ptr<SpawnLocation>())
	,teleported(false)
	,teleportedIn(false)
	,autoJumpEnabled(true)
{
#ifndef REMOVE_PLAYER_PROTECTIONS 
	RBX::Security::Context::current().requirePermission(RBX::Security::WritePlayer, "create a Player");
#endif
	setName(sPlayer);
}

void Player::reportScriptSecurityError(const std::string& hash, const std::string& error, const std::string& stack)
{
	desc_scriptSecurityError.fireAndReplicateEvent(this, hash, error, stack);
}
void Player::killPlayer()
{
	desc_kill.fireAndReplicateEvent(this);
}

Player::~Player(void)
{
	setCharacter(NULL);
}

bool Player::getSuperSafeChat() const {
    Players* players = ServiceProvider::find<Players>(this);
    if (players && players->getNonSuperSafeChatForAllPlayersEnabled())
    {
        return false;
    }

	// Only guests should be forced into "super safe chat".
	return getUserID() < 0;
}

void Player::setSuperSafeChat(bool value)
{
	if(value != superSafeChat)
	{
		superSafeChat = value;
		raisePropertyChanged(prop_SuperSafeChat);
		raisePropertyChanged(prop_ChatMode);
	}
}

Player::ChatMode Player::getChatMode() const
{
    // Guests should still be forced to use bubble chat.
    return getUserID() < 0 ? CHAT_MODE_MENU : CHAT_MODE_TEXT_AND_MENU;
}


/*
	Grow slowly if more than 2x real time.
	Shrink quickly is less than 1x real time.
*/

bool Player::physicsOutBandwidthExceeded(const RBX::Instance* context)
{
	return Client::physicsOutBandwidthExceeded(context);
}

double Player::getNetworkBufferHealth(const RBX::Instance* context)
{
	return Client::getNetworkBufferHealth(context);
}

void Player::reportStat(std::string stat)
{
	desc_statsSignal.fireAndReplicateEvent(this, stat);
}

void Player::LoadDataResultHelper(boost::weak_ptr<Player> weakPlayer, shared_ptr<const Reflection::ValueMap> result)
{
	if(shared_ptr<Player> player = weakPlayer.lock())
	{
		player->loadDataResult(result);
	}
}

void Player::getWebPersonalServerRank(boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(PersonalServerService* personalServerService = ServiceProvider::find<PersonalServerService>(this))
		if(Network::Players::backendProcessing(this))
			if(DataModel* dataModel = DataModel::get(this))
				personalServerService->getRank(this, dataModel->getPlaceID(), resumeFunction,errorFunction);
			else
				errorFunction("getWebPersonalServerRank dataModel is nil");
		else
			errorFunction("getWebPersonalServerRank should be called from server");
	else
		errorFunction("Player is not in Workspace");
}

void Player::setWebPersonalServerRank(int newRank, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(PersonalServerService* personalServerService = ServiceProvider::find<PersonalServerService>(this))
		if(Network::Players::backendProcessing(this))
			if(DataModel* dataModel = DataModel::get(this))
				personalServerService->setRank(this, dataModel->getPlaceID(), newRank, resumeFunction, errorFunction);
			else
				errorFunction("setWebPersonalServerRank dataModel is nil");
		else
			errorFunction("setWebPersonalServerRank should be called from server");
	else
		errorFunction("Player is not in Workspace");
}

void Player::waitForDataReady(boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(dataReady)
		resumeFunction(true);
	else
		waitForDataReadyWaiters.push_back(resumeFunction);
}

static void FireWaitForDataReady(boost::function<void(bool)> resumeFunction)
{
	resumeFunction(true);
}
void Player::fireWaitForDataReady()
{
	std::for_each(waitForDataReadyWaiters.begin(), waitForDataReadyWaiters.end(), &FireWaitForDataReady);
	waitForDataReadyWaiters.clear();
}

void Player::loadDataResult(shared_ptr<const Reflection::ValueMap> result)
{
	persistentData.reset(new PersistentDataStore(result.get(), ServiceProvider::find<Players>(this), dataComplexityLimit));
		
	dataReady = true;
	raisePropertyChanged(prop_dataReady);
	fireWaitForDataReady();
}

void Player::setDataComplexityLimit(int value)
{
	if(dataComplexityLimit != value){
		dataComplexityLimit = value;
		if(persistentData){
			//Note this won't stop data being loaded, or data that has already been put in, only future data
			persistentData->setComplexityLimit(value);
		}

		raisePropertyChanged(prop_dataComplexityLimit);
	}
}

int Player::getDataComplexity() const
{
	if(persistentData){
		return persistentData->getComplexity();
	}
	return 0;
}
void Player::loadData()
{
	Network::Players* players = ServiceProvider::find<Players>(this);
	if(!players) 
		throw std::runtime_error("Cannot load data from Player that is not in DataModel");
	if(userId < 0)
	{
		//We're a guest, so we have no data to load
		loadDataResult(shared_ptr<const Reflection::ValueMap>());
		return;
	}
	
	std::string url = players->getLoadDataUrl(userId);

	LuaWebService* luaWebService = ServiceProvider::create<LuaWebService>(this);
	if(!luaWebService)
		throw std::runtime_error("Can't find LuaWebService, something is very wrong");

	luaWebService->asyncRequestNoCache(url, 0, boost::bind(&Player::LoadDataResultHelper, weak_from(this), _1), AsyncHttpQueue::AsyncWrite);
}
static void PlayerDontCareResponse(std::string* response, std::exception* exception)
{}

void Player::saveData()
{
	Network::Players* players = ServiceProvider::find<Players>(this);
	if(!players) 
		throw std::runtime_error("Cannot save data for Player that is not in DataModel");
	
	if(userId < 0)
	{
		//We're a guest, so we have no data to save
		return;
	}

	if(!persistentData)
		return;

	if(persistentData->empty())
		return;

	std::string url = players->getSaveDataUrl(userId);

	std::string dataString;
	if(persistentData->save(dataString)){
		Http http(url);
		http.post(dataString, Http::kContentTypeDefaultUnspecified, true, &PlayerDontCareResponse);
	}
}

void Player::saveLeaderboardData()
{
	Network::Players* players = ServiceProvider::find<Players>(this);
	if(!players) 
		throw std::runtime_error("Cannot save leaderboard data for Player that is not in DataModel");
	
	if(userId < 0)
	{
		//We're a guest, so we have no data to save
		return;
	}

	if(!persistentData)
		return;

	if(persistentData->empty())
		return;

	if(!persistentData->isLeaderboardDirty())
		return;

	std::string url = players->getSaveLeaderboardDataUrl(userId);

	std::string dataString;
	if(persistentData->saveLeaderboard(dataString)){
		Http http(url);
		http.post(dataString, Http::kContentTypeDefaultUnspecified, true, &PlayerDontCareResponse);
	}
}

double Player::loadNumber(std::string key)
{
	if(!Players::backendProcessing(this))
		throw std::runtime_error("LocalScripts cannot use LoadNumber");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");
	
	RBXASSERT(persistentData);

	return persistentData->getNumber(key);

}
void Player::saveNumber(std::string key, double value)
{
	if(!Players::backendProcessing(this))
		throw std::runtime_error("LocalScripts cannot use SaveNumber");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");
	
	RBXASSERT(persistentData);
	if(!persistentData->setNumber(key, value)){
		raisePropertyChanged(prop_dataComplexity);
		throw RBX::runtime_error("Exceeded DataComplexity limit for Number key %s", key.c_str());
	}
	raisePropertyChanged(prop_dataComplexity);
}


bool Player::loadBoolean(std::string key)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use LoadBoolean");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	return persistentData->getBoolean(key);
}
void Player::saveBoolean(std::string key, bool value)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use SaveBoolean");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	if(!persistentData->setBoolean(key, value)){
		raisePropertyChanged(prop_dataComplexity);
		throw RBX::runtime_error("Exceeded DataComplexity limit for Boolean key %s", key.c_str());
	}
	raisePropertyChanged(prop_dataComplexity);
}


std::string Player::loadString(std::string key)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use LoadString");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	return persistentData->getString(key);
}
void Player::saveString(std::string key, std::string value)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use SaveString");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	if(!persistentData->setString(key, value)){
		raisePropertyChanged(prop_dataComplexity);
		throw RBX::runtime_error("Exceeded DataComplexity limit for String key %s", key.c_str());
	}
	raisePropertyChanged(prop_dataComplexity);
}


shared_ptr<const Reflection::ValueArray> Player::loadList(std::string key)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use LoadList");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	return persistentData->getList(key);

}
void Player::saveList(std::string key, shared_ptr<const Reflection::ValueArray> value)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use SaveList");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	if(!persistentData->setList(key, value)){
		raisePropertyChanged(prop_dataComplexity);
		throw RBX::runtime_error("Exceeded DataComplexity limit for List key %s", key.c_str());
	}
	raisePropertyChanged(prop_dataComplexity);
}

shared_ptr<const RBX::Reflection::ValueMap> Player::loadTable(std::string key)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use LoadTable");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	return persistentData->getTable(key);
}
void Player::saveTable(std::string key, shared_ptr<const RBX::Reflection::ValueMap> value)
{
	if(!Players::backendProcessing(this, false))
		throw std::runtime_error("LocalScripts cannot use SaveTable");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	if(!persistentData->setTable(key, value)){
		raisePropertyChanged(prop_dataComplexity);
		throw RBX::runtime_error("Exceeded DataComplexity limit for Table key %s", key.c_str());
	}
	raisePropertyChanged(prop_dataComplexity);
}



shared_ptr<Instance> Player::loadInstance(std::string key)
{
	if(!Players::backendProcessing(this))
		throw std::runtime_error("LocalScripts cannot use LoadInstance");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");

	RBXASSERT(persistentData);
	return persistentData->getInstance(key);
}
void Player::saveInstance(std::string key, shared_ptr<Instance> value)
{
	if(!Players::backendProcessing(this))
		throw std::runtime_error("LocalScripts cannot use SaveInstance");
	if(!dataReady)
		throw std::runtime_error("Data for player not yet loaded, wait for DataReady");
	if(!value)
		throw std::runtime_error("Cannot save a null instance");

	RBXASSERT(persistentData);
	if(!persistentData->setInstance(key, value)){
		raisePropertyChanged(prop_dataComplexity);
		throw RBX::runtime_error("Exceeded DataComplexity limit for Instance key %s", key.c_str());
	}
	raisePropertyChanged(prop_dataComplexity);
}

void Player::renderStreamedRegion(Adorn* adorn)
{
	RBX::Network::Client* client = ServiceProvider::find<RBX::Network::Client>(this);
	if (client)
	{
		if (RBX::Network::ClientReplicator *cRep = client->findFirstChildOfType<RBX::Network::ClientReplicator>())
		{
			cRep->renderStreamedRegions(adorn);
		}
	}
}

void Player::renderDPhysicsRegion(Adorn* adorn)
{
	CoordinateFrame coord;
	if (this->hasCharacterHead(coord)) {
		if (const Primitive* prim = getConstCharacterRoot()) {
			coord.translation.y -= 3.0f;
			Color3 color = RBX::Color::colorFromInt(prim->getNetworkOwner().getAddress());
			const float cylHeight = 1.0f;
			
			DrawAdorn::cylinder(adorn, coord.translation, 1, cylHeight, simulationRadius, Color4(color, 0.6f), false);
			Workspace* workspace = ServiceProvider::find<Workspace>(this);
			if (workspace && workspace->getNetworkStreamingEnabled()) {
				DrawAdorn::cylinder(adorn, coord.translation, 1, cylHeight, maxSimulationRadius, Color4(color, 0.3f), false);
			}
		}
	}
}


void Player::renderPartMovementPath(Adorn* adorn)
{
    RBX::Network::Client* client = ServiceProvider::find<RBX::Network::Client>(this);
    if (client)
    {
        if (RBX::Network::ClientReplicator *cRep = client->findFirstChildOfType<RBX::Network::ClientReplicator>())
        {
            cRep->renderPartMovementPath(adorn);
        }
    }
}

void Player::updateSimulationRadius(float value)
{
	float clampedValue = 1.0f;

	clampedValue = G3D::clamp(value, 1.0f, maxSimulationRadius);

	if (clampedValue != simulationRadius)
		event_simulationRadiusChanged.fireAndReplicateEvent(this, clampedValue);
}

void Player::setSimulationRadius(float value)
{ 
    if (simulationRadius > value)
    {
        // client is overloaded, let's invalidate the "permanent ownership" of projectiles
        RBX::Network::Server* server = ServiceProvider::find<RBX::Network::Server>(this);
        if (server && server->getNetworkOwnerJob())
        {
            server->getNetworkOwnerJob()->invalidateProjectileOwnership(getRemoteAddressAsRbxAddress());
        }
    }

	simulationRadius = value;

#ifdef RBX_TEST_BUILD
	// some unit tests sets the max sim radius in server script
	if (simulationRadius > maxSimulationRadius)
		simulationRadius = maxSimulationRadius;
#endif
}

void Player::setMaxSimulationRadius(float value)
{ 
	float clampedValue = 1.0f;
	
	clampedValue = G3D::clamp(value, 1.0f, DistributedPhysics::MAX_CLIENT_SIMULATION_DISTANCE());

	if (clampedValue != maxSimulationRadius) {
		prop_MaxSimulationRadius.setValue(this, clampedValue); 

		if (clampedValue < simulationRadius)
			setSimulationRadius(clampedValue);
	}
}

void Player::setCloudEditCameraCoordinateFrame(const CoordinateFrame& cframe)
{
	if (cframe != cloudEditCameraCFrame)
	{
		cloudEditCameraCFrame = cframe;
		raisePropertyChanged(prop_CloudEditCameraCoordinateFrame);
	}
}

const Primitive* Player::getConstCharacterRoot() const
{
	if (getConstCharacter()) {
		if (const PartInstance* head = Humanoid::getConstHeadFromCharacter(character.get())) {
			if (const Primitive* headPrim = head->getConstPartPrimitive()) {
				if (const Assembly* assembly = headPrim->getConstAssembly()) {
					return assembly->getConstAssemblyPrimitive();
				}
			}
		}
	}
	return NULL;
}

const PartInstance* Player::hasCharacterHead(CoordinateFrame& pos) const
{
	if (character.get()) {
		if (const PartInstance* head = Humanoid::getConstHeadFromCharacter(character.get())) {
			pos = head->getCoordinateFrame();
			return head;
		}
	}
	return NULL;
}


void Player::setCharacter(ModelInstance* value)
{
	if (value && 
		(value->getClassName() != ModelInstance::className()) &&
		(value->getClassName() != PartInstance::className()))
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING, "can't set character to %s, only Models or Parts are allowed!", value->getName().c_str());
		return;
	}

	if (value != this->character.get())
	{
		if (character) {
			characterDiedConnection.disconnect();
			characterRemovingSignal(character);
			if (Players::backendProcessing(this, false)) {
				character->setParent(NULL);		// we delete the old character here, server side only to prevent confusion
			}
			character.reset();

			if(!RBX::GameBasicSettings::singleton().inMouseLockMode())
			if(shared_ptr<RBX::CoreGuiService> coreGuiService = shared_from(ServiceProvider::find<RBX::CoreGuiService>(this->fastDynamicCast<Instance>())))
				if(RBX::GuiTextButton* button = Instance::fastDynamicCast<RBX::GuiTextButton>(coreGuiService->findFirstChildByName2("MouseLockLabel", true).get()))
					button->setVisible(false);
		}

		if (value) {
			FASTLOG1F(DFLog::NetworkJoin, "setCharacter received character @ %f s", Time::nowFastSec());
			character = shared_from(value);
			character->lockName();

			Humanoid* humanoid = Humanoid::modelIsCharacter(value);
			if (humanoid) {
				humanoid->setCameraMode(cameraMode);
				humanoid->setNameDisplayDistance(nameDisplayDistance);
				humanoid->setHealthDisplayDistance(healthDisplayDistance);
				humanoid->setMinDistance(cameraMinZoomDistance);
				humanoid->setMaxDistance(cameraMaxZoomDistance);
			}

            Workspace* workspace = ServiceProvider::find<Workspace>(this);

			if (Players::backendProcessing(this, false)) {		// Server does it in client/server - bail on client mode.  will still execute in solo mode
                RBXASSERT(workspace);
				
				rebuildBackpack();								// every time a character is added - rebuild the backpack

				if (!workspace->getNetworkFilteringEnabled())	// this rebuilds the gui using the server
					rebuildGui();
				
				// hook up resurrection code
				if (humanoid)
					characterDiedConnection = humanoid->diedSignal.connect(boost::bind(&Player::onCharacterDied, this));
			}

			bool frontendProcessing = Players::frontendProcessing(this, false);
			if (frontendProcessing && workspace->getNetworkFilteringEnabled())
			{
				// this rebuilds the gui on the client because FilteringEnabled = true
				rebuildGui();
			}

			characterAddedSignal(character);
			if (frontendProcessing)
			{
				if(humanoid)
					event_connectDiedSignal.replicateEvent(this);
				onCharacterChangedFrontend();
			}
			else
				 onTeleportSignal.connect(boost::bind(&Player::handleTeleportSignalBackend, this, _1));
		}
		raiseChanged(prop_Character);
	}
}

void Player::onCharacterDied()
{
	RBXASSERT(Players::backendProcessing(this));

	// we don't auto respawn if we are stopping auto character load
	Players* players = ServiceProvider::find<Players>(this);
	if (players == NULL || players->getCharacterAutoSpawnProperty())
	{
		calculateNextSpawnLocation(ServiceProvider::findServiceProvider(this));
		if (TimerService* timer = ServiceProvider::create<TimerService>(this)) {
			timer->delay(boost::bind(&Player::loadCharacter, shared_from(this), true, ""), 5.0);
		}
	}
}

void Player::setTeamColor(BrickColor value)
{
	// Allow people to set a nil color - just changes player to neutral
	if (value != teamColor)
	{
		teamColor = value;
		if (calculatesSpawnLocationEarly() && teamStatusChangedCallback) {
			teamStatusChangedCallback();
		}
		raisePropertyChanged(prop_teamColor);
	}
}

void Player::setNeutral(bool value)
{
	if ( value != neutral ) 
	{
		neutral = value;
		if (calculatesSpawnLocationEarly() && teamStatusChangedCallback) {
			teamStatusChangedCallback();
		}
		raisePropertyChanged(prop_neutral);
	}
}

void Player::setUnder13(bool value)
{
	if (value != under13)
	{
		under13 = value;
	}
}

void Player::setRespawnLocation(SpawnLocation* value)
{
	if (value != getDangerousRespawnLocation())
	{
		respawnLocation = shared_from(value);
		raisePropertyChanged(prop_RespawnLocation);
	}
}

bool parse(const std::string& url, const std::string& arg, std::string& out)
{
	std::string value;
	std::string lower = url;
	std::string lowerArg = arg; 
	std::transform(lower.begin(), lower.end(), lower.begin(), tolower);
	std::transform(lowerArg.begin(), lowerArg.end(), lowerArg.begin(), tolower);
	size_t pos = lower.find(lowerArg);

	if (pos != std::string::npos)
	{
		pos += lowerArg.size();
		for (; pos < lower.size(); pos++)
		{
			if (std::isdigit(lower[pos]))
				value += lower[pos];
			else
				break;
		}

		if (value.size() > 0)
		{
			out = value;
			return true;
		}
	}

	return false;
}

void Player::setCharacterAppearance(const std::string& value)
{
	if (value != this->characterAppearance)
	{
		characterAppearance = value;
		raisePropertyChanged(prop_characterAppearance);
		if (DFFlag::LoadStarterGearWithoutLoadCharacter && !loadedStarterGear)
		{
			loadStarterGear();
		}
	}
}

void Player::onLocalPlayerNotIdle(RBX::ServiceProvider* serviceProvider)
{
	if ( Network::Players* players = serviceProvider->find<Network::Players>() )
		if (Network::Player* player = players->getLocalPlayer())
			if (ServiceProvider::findServiceProvider(player))
				player->registerLocalPlayerNotIdle();
}


void Player::registerLocalPlayerNotIdle()
{
	RBXASSERT(Players::frontendProcessing(this));
	lastActivityTime = Time::now<Time::Fast>();
}



void Player::doPeriodicIdleCheck()
{
	// this code is here for the case where the player is pulled out of the datamodel but the timer is still
	// held on to by the idle timer

	if (ServiceProvider::findServiceProvider(this))
	{
		RBXASSERT(Players::frontendProcessing(this));

		if (!lastActivityTime.isZero())
		{
			Time::Interval idleTime = Time::now<Time::Fast>() - lastActivityTime;
			const bool idle = idleTime > idlePeriod;

			// only kick in multiplayer visit mode
			if (idle) 
			{
				Players *players = ServiceProvider::find<Players>(this);
				if (players && players->getLocalPlayer() && Players::clientIsPresent(this))
				{
					idledSignal(idleTime.seconds());
				}
				else
				{
					if (!players->getLocalPlayer())
						FASTLOG1(FLog::Warning, "Player Idle Check no local player, id = %d", this->userId);

					if (!Players::clientIsPresent(this))
						FASTLOG1(FLog::Warning, "Player Idle Check client not present, id = %d", this->userId);
				}
			}
		}

		TimerService* idleTimer = ServiceProvider::create<TimerService>(this);
		if (idleTimer)
			idleTimer->delay(boost::bind(&Player::doPeriodicIdleCheck, shared_from(this)), idleCheckFrequency);
	}
}

void Player::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider && Players::backendProcessing(oldProvider))
	{
		setCharacter(NULL);
		backendDiedSignalConnection.disconnect();
	}
	
	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider && !Players::isCloudEdit(newProvider))
	{
		if (Players::frontendProcessing(newProvider))
		{
			onCharacterChangedFrontend();
			RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(newProvider);
			if (localPlayer && this == localPlayer)
				rebuildPlayerScripts();
		}

		if (Players::backendProcessing(newProvider))
			backendDiedSignalConnection = connectDiedSignalBackend.connect(boost::bind(&Player::onCharacterDied, this));

		if (Players::backendProcessing(newProvider) && oldProvider == NULL) {
			if(StarterPlayerService* starterPlayerService = ServiceProvider::create<StarterPlayerService>(this)) {
				setCameraMode(starterPlayerService->getCameraMode());
				setCameraMaxZoomDistance(starterPlayerService->getCameraMaxZoomDistance());
				setCameraMinZoomDistance(starterPlayerService->getCameraMinZoomDistance());
				setHealthDisplayDistance(starterPlayerService->getHealthDisplayDistance());
				setNameDisplayDistance(starterPlayerService->getNameDisplayDistance());
				setDevEnableMouseLockOption(starterPlayerService->getEnableMouseLockOption());
				setDevTouchCameraMode(starterPlayerService->getDevTouchCameraMovementMode());
				setDevComputerCameraMode(starterPlayerService->getDevComputerCameraMovementMode());
				setDevCameraOcclusionMode(starterPlayerService->getDevCameraOcclusionMode());
				setDevTouchMovementMode(starterPlayerService->getDevTouchMovementMode());
				setDevComputerMovementMode(starterPlayerService->getDevComputerMovementMode());
				setAutoJumpEnabled(starterPlayerService->getAutoJumpEnabled());
			}
		}


		simulationRadiusChangedConnection = simulationRadiusChangedSignal.connect(boost::bind(&Player::setSimulationRadius, this, _1));
		if(GuiService* gs = ServiceProvider::find<GuiService>(this)) {
			setShutdownMessageConnection = setShutdownMessageSignal.connect(boost::bind(&GuiService::setErrorMessage, gs, _1)); 
		}

	}

	// Player idle-check is only made client-side in multiplayer visit mode!
	if (oldProvider == NULL && Players::frontendProcessing(newProvider))
	{
		// player object is entering datamodel
		registerLocalPlayerNotIdle();
		doPeriodicIdleCheck();
	}
}

void Player::setAppearanceDidLoad(bool value)
{
	if(value != appearanceDidLoad)
	{
		appearanceDidLoad = value;
		raisePropertyChanged(prop_appearanceDidLoad);
	}
}

void Player::setCharacterAppearanceLoaded(bool value)
{
	characterAppearanceLoaded = value;
}

// Utility function for binds:: below
static void addChild(const shared_ptr<ModelInstance>& parent, const shared_ptr<Instance>& child)
{
	child->setParent(parent.get());
}

// Utility function for binds:: below
static void addChildToInstance(const weak_ptr<Instance>& parent, const shared_ptr<Instance>& child)
{
	shared_ptr<Instance> p(parent.lock());
	if (!p)
		return;

	child->setParent(p.get());
}


static void deleteAppearance(Instance* instance)
{
	if (Instance::fastDynamicCast<CharacterAppearance>(instance))
		instance->setParent(NULL);
}


///////////////////////////////////////////////////////////////////////////////////
// StarterGear Logic Begin
///////////////////////////////////////////////////////////////////////////////////

void Player::setGearParent(weak_ptr<Player> player, weak_ptr<Instance> instance, bool equipped)
{
	shared_ptr<Player> p(player.lock());
	if (!p)
	{
		return;
	}

	shared_ptr<Instance> i(instance.lock());
	if (!i)
	{
		return;
	}

	ModelInstance *character = p->getCharacter();

	//TODO remove HopperBin once we have proper Drag/Delete/Clone tools
	if (i->fastDynamicCast<Tool>() || i->fastDynamicCast<HopperBin>())
	{
		if (p->loadAppearanceCounter < 2)
		{
			// try to stick this stuff in the StarterGear
			if (StarterGear* starterGear = p->findFirstChildOfType<StarterGear>()) {
				i->setParent(starterGear);
			}	
			// also stick it in the backpack, so it's right the first time you spawn
			if (equipped && character)
			{
				// this is the gear we have equipped on the website, equip it here so that thumbnailing works ok
				i->clone(EngineCreator)->setParent(character);
			}
			else
			{
				if (Backpack* backpack = p->findFirstChildOfType<Backpack>()) {
					i->clone(EngineCreator)->setParent(backpack);
				}	
			}
		}
		else
		{
			// do nothing because the gear is already loaded into the startergear and that will be copied into the backpack on respawn.
		}	
	}
}

static void doLoadGear(weak_ptr<Player> player, shared_ptr<Instances> instances, std::string contentDescription, bool equipped)
{
	shared_ptr<Player> p(player.lock());
	if (!p)
		return;
	try
	{
		std::for_each(instances->begin(), instances->end(), boost::bind(&Player::setGearParent, player, _1, equipped));
	}
	catch (RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error loading character gear %s: %s", contentDescription.c_str(), e.what());
	}
}


void static doMakeStarterGearRequests(std::string response, weak_ptr<Player> player, weak_ptr<DataModel> dataModel)
{
	std::vector<std::string> strings;
	boost::split(strings, response, boost::is_any_of(";"));
	shared_ptr<DataModel> d(dataModel.lock());
	shared_ptr<Player> p(player.lock());

	if (!DFFlag::LoadStarterGearWithoutLoadCharacter)
	{
		if (p)
		{
			p->addToLoadingInstances((int)strings.size());
		}
	}

	if (d)
	{
		std::string placeIdStr;
		placeIdStr = format_string(kPlaceIdUrlParamFormat, d->getPlaceID());

		for (size_t i = 0; i < strings.size(); i++)
		{
			std::string contentIdString = strings[i];
			contentIdString += placeIdStr;

			ServiceProvider::create<ContentProvider>(d.get())->loadContent(
				ContentId(contentIdString), ContentProvider::PRIORITY_CHARACTER,
				boost::bind(&doLoadGear, player, _2, contentIdString, (contentIdString.find("equipped=1") != std::string::npos)),  // equipped sentinel check
				AsyncHttpQueue::AsyncWrite
				);
		}	
	}
}

void static makeStarterGearRequests(std::string *response, std::exception *err, weak_ptr<Player> player, weak_ptr<DataModel> dataModel)
{
	if (!response) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Make Starter Gear Request exception: %s", err->what());
		return;
	}

	shared_ptr<DataModel> d(dataModel.lock());
	if (d)
	{
		d->submitTask(boost::bind(&doMakeStarterGearRequests, *response, player, dataModel), DataModelJob::Write);
	}
}

void Player::loadStarterGear()
{
	if (!loadedStarterGear && !characterAppearance.empty())
	{
		loadedStarterGear = true;

		weak_ptr<DataModel> dm(shared_from(Instance::fastDynamicCast<DataModel>(this->getRootAncestor())));
		RBX::Http(characterAppearance).get(boost::bind(&makeStarterGearRequests, _1, _2, weak_from(this), boost::weak_ptr<DataModel>(dm)));	
	}
}

///////////////////////////////////////////////////////////////////////////////////
// StarterGear Logic Begin
///////////////////////////////////////////////////////////////////////////////////



void Player::setAppearanceParent(weak_ptr<Player> player, weak_ptr<Instance> instance, bool equipped)
{	
	shared_ptr<Player> p(player.lock());
	if (!p)
		return;

	shared_ptr<Instance> i(instance.lock());
	if (!i)
		return;

	// This is a hack that works as follows:  if they receive an asset with type DataModelMesh, then it must be for a head.  This is b/c we currently dont allow
	// a mesh to be used on any other part except for in accoutrements, so this shouldnt be an issue.  In the future, we may want to pass another value
	// which indicates where the mesh is being applied to.

	ModelInstance *character = p->getCharacter();

	if (character)
	{
		if (RBX::DataModelMesh* headMesh = i->fastDynamicCast<RBX::DataModelMesh>())
		{
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setHeadMesh(headMesh);
		}
		else if (RBX::Decal* decal = i->fastDynamicCast<RBX::Decal>())
		{
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setHeadDecal(decal);
		}
		else if (i->fastDynamicCast<CharacterAppearance>())
		{
			i->setParent(character);
		}
		else if (dynamic_cast<IEquipable*>(i.get()))
		{
			// hats
			if (!DFFlag::UseR15Character)
			{
				if (!i->fastDynamicCast<Tool>())
				{
					i->setParent(character);
				}
			} else {
				if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				{
					if (!humanoid->getUseR15())
					{
						if (!i->fastDynamicCast<Tool>())
						{
							i->setParent(character);
						}
					}
				}

			}
		}
	}

	//TODO remove HopperBin once we have proper Drag/Delete/Clone tools
	if (i->fastDynamicCast<Tool>() || i->fastDynamicCast<HopperBin>())
	{
		if (p->loadAppearanceCounter < 2)
		{
			// try to stick this stuff in the StarterGear
			if (!DFFlag::LoadStarterGearWithoutLoadCharacter)
			{
				if (StarterGear* starterGear = p->findFirstChildOfType<StarterGear>()) {
					i->setParent(starterGear);
				}	
			}
			// also stick it in the backpack, so it's right the first time you spawn
			if (equipped && character)
			{
				// this is the gear we have equipped on the website, equip it here so that thumbnailing works ok
				i->clone(EngineCreator)->setParent(character);
			}
			else if (!DFFlag::LoadStarterGearWithoutLoadCharacter)
			{
				if (Backpack* backpack = p->findFirstChildOfType<Backpack>()) {
					i->clone(EngineCreator)->setParent(backpack);
				}	
			}
		}
		else
		{
			// do nothing because the gear is already loaded into the startergear and that will be copied into the backpack on respawn.
		}	
	}

	p->removeFromLoadingInstances(1);

	if(!p->getAppearanceDidLoad())
	{
		if(p->getLoadingInstances() <= 0) 
		{
			p->setAppearanceDidLoad(true);
		}
	}

	if (!p->getCharacterAppearanceLoaded())
	{
		if (p->getLoadingInstances() <= 0) 
		{
			p->setCharacterAppearanceLoaded(true);
			event_CharacterLoaded.fireAndReplicateEvent(p.get(), p->character);
		}
	}
}


static void doLoadAppearance(weak_ptr<Player> player, AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances, 
							 std::string contentDescription, bool equipped, double requestTime)
{
	shared_ptr<Player> p(player.lock());
	if (!p)
		return;
	try
	{
		// debug timer
		double loadTime = Time::nowFastSec() - requestTime;
		if (loadTime > 5.0)
			StandardOut::singleton()->printf(MESSAGE_WARNING, "doLoadAppearance %s time: %f", contentDescription.c_str(), loadTime);

		std::for_each(instances->begin(), instances->end(), boost::bind(&Player::setAppearanceParent, player, _1, equipped));
	}
	catch (RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error loading character appearance %s: %s", contentDescription.c_str(), e.what());
	}
}

static void setAppearanceParentNull(shared_ptr<Instance> instance)
{
	if (Instance::fastDynamicCast<CharacterAppearance>(instance.get()))
		instance->setParent(NULL);
}

void Player::removeCharacterAppearance()
{
	if (!Players::backendProcessing(this))
		throw std::runtime_error("RemoveCharacterAppearance can only be called by the backend server");

	if (!character)
		return;

	character->visitChildren(&setAppearanceParentNull);
}

static void setAppearanceParentNullScript(shared_ptr<Instance> instance)
{
	if (Instance::fastDynamicCast<CharacterAppearance>(instance.get()))
		instance->setParent(NULL);
	else if (dynamic_cast<IEquipable*>(instance.get()))
		// hats
		if (!instance->fastDynamicCast<Tool>())
			instance->setParent(NULL);
}

void Player::removeCharacterAppearanceScript()
{
	if (!character)
		return;

	character->visitChildren(&setAppearanceParentNullScript);
}


void static doMakeAccoutrementRequests(std::string response, weak_ptr<Player> player, weak_ptr<DataModel> dataModel)
{
	std::vector<std::string> strings;
	boost::split(strings, response, boost::is_any_of(";"));
	shared_ptr<DataModel> d(dataModel.lock());
	shared_ptr<Player> p(player.lock());

	if(p)
		p->addToLoadingInstances((int)strings.size());
	if (d)
	{
		// check debug timer
		shared_ptr<Player> p = player.lock();
		if (p)
		{
			Time::Interval dt = p->appearanceFetchTimer.delta();
			if (dt.seconds() > 5.0f)
				StandardOut::singleton()->printf(MESSAGE_WARNING, "Player appearance took %f seconds to load", dt.seconds());
		}

		std::string placeIdStr;
		placeIdStr = format_string(kPlaceIdUrlParamFormat, d->getPlaceID());

		for (size_t i = 0; i < strings.size(); i++)
		{
			std::string contentIdString = strings[i];
			contentIdString += placeIdStr;

			ServiceProvider::create<ContentProvider>(d.get())->loadContent(
				ContentId(contentIdString), ContentProvider::PRIORITY_CHARACTER,
				boost::bind(&doLoadAppearance, player, _1, _2, contentIdString, (contentIdString.find("equipped=1") != std::string::npos), Time::nowFastSec()),  // equipped sentinel check
				AsyncHttpQueue::AsyncWrite
			);
		}	
	}
}

void static makeAccoutrementRequests(std::string *response, std::exception *err, weak_ptr<Player> player, weak_ptr<DataModel> dataModel)
{
	if (!response) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Make Accoutrement Request exception: %s", err->what());
		return;
	}

	shared_ptr<DataModel> d(dataModel.lock());
	if (d)
	{
		try
		{
			d->submitTask(boost::bind(&doMakeAccoutrementRequests, *response, player, dataModel), DataModelJob::Write);
		}
		catch (const RBX::base_exception&)
		{
		}
	}
}

void Player::setHasGroupBuildTools(bool value)
{
	if(value != hasGroupBuildTools)
	{
		hasGroupBuildTools = value;
		raisePropertyChanged(prop_groupBuildTools);
	}
}

void Player::setCanLoadCharacterAppearance(bool value)
{
	if(value != canLoadCharacterAppearance)
	{
		canLoadCharacterAppearance = value;
		raisePropertyChanged(prop_canLoadCharacterAppearance);
	}
}

void Player::setAutoJumpEnabled(bool value)
{
	if (value != autoJumpEnabled)
	{
		autoJumpEnabled = value;
		raisePropertyChanged(prop_autoJumpEnabled);
	}
}

void Player::setPersonalServerRank(int value)
{
	if(value != personalServerRank && value >= 0 && value <= 255) // 255 also check in getter in case of exploits
	{
		personalServerRank = value;
		if(PersonalServerService* personalServerService = ServiceProvider::find<PersonalServerService>(this))
			personalServerRank = personalServerService->getCurrentPrivilege(personalServerRank);

		if(personalServerRank >= PersonalServerService::PRIVILEGE_MEMBER)
			giveBuildTools(); // if player is promoted to at least member, then set the build tools bool (handled in groupbuild script)
		else
			removeBuildTools(); // if player is demoted below member, then unset the build tools bool (handled in groupbuild script)
		raisePropertyChanged(prop_personalServerRank);
	}
}

void Player::giveBuildTools()
{
	if (ScriptContext* scriptContext = ServiceProvider::find<ScriptContext>(this))
	{
		scriptContext->addCoreScriptLocal("CoreScripts/BuildToolsScripts/BuildToolManager", shared_from(this));
	}
	setHasGroupBuildTools(true);
}

void Player::removeBuildTools()
{
	setHasGroupBuildTools(false);
}

float Player::distanceFromCharacter(Vector3 point)
{
	if(character){
		if(const Instance* head = character->findConstFirstChildByName("Head")){
			if(const PartInstance* partHead = head->fastDynamicCast<PartInstance>()){
				return (point - partHead->getTranslationUi()).magnitude();
			}
		}
	}
	return 0.0f;
}
void Player::loadCharacterAppearance(bool blockingCall)
{
	setCharacterAppearanceLoaded(false);
    if (!Players::backendProcessing(this))
		throw std::runtime_error("LoadCharacterAppearance can only be called by the backend server");
	if(!canLoadCharacterAppearance)
		return;

	loadAppearanceCounter++; // used to track whether this is the first time we've done this or not.

	// Strip any existing appearance data
	removeCharacterAppearance();

	if (!character)
		return;
	
	//characterAppearance.append(";c:/cylinderblockmesh.rbxm");
	//characterAppearance.append(";c:/testblockmesh.rbxm");
	//characterAppearance.append(";c:/testskullmesh.rbxm");
	// load new appearance

	std::string response;
	if (!characterAppearance.empty())
	{

		//characterAppearance is the url of a handler that returns a list of asset urls to hit.

		//these urls are shirts, pants, gear, hats, etc.

		std::string sanitizedCharacterAppearance = characterAppearance;
		if (DFFlag::ValidateCharacterAppearanceUrl)
		{
			ContentId characterAppearanceContent(characterAppearance);
			if (!characterAppearanceContent.reconstructUrl(GetBaseURL(), kValidCharacterAppearancePaths, ARRAYSIZE(kValidCharacterAppearancePaths))) {
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Invalid CharacterAppearance for %s (%s)", getName().c_str(), characterAppearance.c_str());
				return;
			}
			sanitizedCharacterAppearance = characterAppearanceContent.toString();
		}

		if (blockingCall)
		{
			RBX::Http(sanitizedCharacterAppearance).get(response);
			std::vector<std::string> strings;
			boost::split(strings, response, boost::is_any_of(";"));

			Instances characterAppearanceInstances;
			Instances characterAppearanceInstancesEquipped;

			std::string placeIdStr;
			placeIdStr = format_string(kPlaceIdUrlParamFormat, DataModel::get(this)->getPlaceID());
			
            for (size_t i = 0; i < strings.size(); i++)
            {
                std::string contentIdString = strings[i];
                contentIdString += placeIdStr;
                
                ServiceProvider::create<ContentProvider>(this)->requestContentString(ContentId(contentIdString), ContentProvider::PRIORITY_SCRIPT);
            }

			for (size_t i = 0; i < strings.size(); i++)
			{
				try
				{
					std::string contentIdString = strings[i];
					contentIdString += placeIdStr;
					if (contentIdString.find("equipped=1") != std::string::npos) {
						ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId(contentIdString), characterAppearanceInstancesEquipped);
					} else {
						ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId(contentIdString), characterAppearanceInstances);
					}
				}
				catch(RBX::base_exception&)
				{
					//Quash errors related to Character Appearance loading
				}
			}


			addToLoadingInstances((int)characterAppearanceInstancesEquipped.size());
			addToLoadingInstances((int)characterAppearanceInstances.size());

			std::for_each(characterAppearanceInstancesEquipped.begin(), characterAppearanceInstancesEquipped.end(), boost::bind(&setAppearanceParent, weak_from(this), _1, true));
			std::for_each(characterAppearanceInstances.begin(), characterAppearanceInstances.end(), boost::bind(&setAppearanceParent, weak_from(this), _1, false));

		}
		else
		{
			// start debug timer
			appearanceFetchTimer.reset();

			//void get(boost::function<void(std::string*, std::exception*)> handler);
			weak_ptr<DataModel> dm(shared_from(Instance::fastDynamicCast<DataModel>(this->getRootAncestor())));
			RBX::Http(sanitizedCharacterAppearance).get(boost::bind(&makeAccoutrementRequests, _1, _2, weak_from(this), boost::weak_ptr<DataModel>(dm)));	
		}
	}
	else
	{
		event_CharacterLoaded.fireAndReplicateEvent(this, character);
	}
}

void Player::loadCharacterAppearanceScript(shared_ptr<Instance> asset)
{
	if(!asset)
		return;

	if(Instance* instance = asset.get())
		setAppearanceParent(weak_from(this),weak_from(instance),false);
}

void Player::removeCharacter()
{
	if (!Players::backendProcessing(this))
		throw std::runtime_error("RemoveCharacter can only be called by the backend server");

	FASTLOG(FLog::Network, "Player:removeCharacter");

	setCharacter(NULL);		// old character will be deleted here
}

void Player::doFirstSpawnLocationCalculation(
		const ServiceProvider* serviceProvider, const std::string& preferedSpawnName) {

	RBXASSERT(nextSpawnLocation == NULL);
	RBXASSERT(!hasSpawnedAtLeastOnce);

	teleportSpawnName = preferedSpawnName;

	if (Teams* ts = serviceProvider->find<Teams>()) {
		ts->assignNewPlayerToTeam(this);
	}
	teamStatusChangedCallback = boost::bind(&calculateNextSpawnLocationHelper, weak_from(this), serviceProvider);

	calculateNextSpawnLocation(serviceProvider);
}

void Player::calculateNextSpawnLocationHelper(weak_ptr<Player>& weakPlayer, const ServiceProvider* serviceProvider) {
	if (shared_ptr<Player> player = weakPlayer.lock()) {
		player->calculateNextSpawnLocation(serviceProvider);
	}
}

void Player::luaJumpCharacter()
{
    if(UserInputService *userInputService = ServiceProvider::create<UserInputService>(this))
        userInputService->jumpLocalCharacterLua();
}

void Player::move(Vector3 walkVector, bool relativeToCamera)
{
	if (ModelInstance* character = getCharacter())
	{
		if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
		{
			humanoid->move(walkVector, relativeToCamera);
		}
		else
		{
			RBX::StandardOut::singleton()->print(MESSAGE_WARNING,"Player:Move called, but player currently has no humanoid.");
		}
	}
	else
	{
		RBX::StandardOut::singleton()->print(MESSAGE_WARNING,"Player:Move called, but player currently has no character.");
	}
}

void Player::luaMoveCharacter(Vector2 walkDirection, float maxWalkDelta)
{
    if(UserInputService *userInputService = ServiceProvider::create<UserInputService>(this))
        userInputService->moveLocalCharacterLua(walkDirection, maxWalkDelta);
}

void Player::luaLoadCharacter(bool inGame)
{
	FASTLOG1(FLog::Network, "Player:luaLoadCharacter inGame(%d)", inGame);

	std::string preferredSpawn = "";
	// special logic for play solo
	if (Players::backendProcessing(this) && Players::frontendProcessing(this))
	{
		preferredSpawn = TeleportService::GetSpawnName();
		TeleportService::ResetSpawnName();
	}

	loadCharacter(inGame, preferredSpawn);

	// loadCharacter will throw an exception if we aren't backend processing
	RBXASSERT(Players::backendProcessing(this));

	// used in play solo to make game loaded signal fire
	if (Players::frontendProcessing(this)) {
		if(DataModel* dataModel = DataModel::get(this)) {
			dataModel->gameLoaded();
		}
	}
}

void Player::calculateNextSpawnLocation(const ServiceProvider *serviceProvider)
{
	Vector3 location;
	int forcefieldDuration = 0;
	CoordinateFrame cf;

	Workspace* workspace = serviceProvider->find<Workspace>();
	if (!workspace)
	{
		FASTLOG(FLog::Error, "Player:calculateNextSpawnLocation - ServiceProvider didn't have a Workspace");

		// should only be called when Player is in Datamodel
		throw std::runtime_error("calculateNextSpawnLocation can only be called when Player is in the world");
	}

	bool locationFound = false;
	if (SpawnerService* ss = serviceProvider->find<SpawnerService>())
	{
		if (SpawnLocation* sl = ss->GetSpawnLocation(this, hasSpawnedAtLeastOnce ? "" : teleportSpawnName))
		{
			location = sl->getCoordinateFrame().translation;
			forcefieldDuration = sl->forcefieldDuration;
			cf = sl->getCoordinateFrame();

			spawnLocationChangedConnection = sl->propertyChangedSignal.connect(
				boost::bind(&Player::calculateNextSpawnLocationHelper,
				weak_from(this), serviceProvider));
			locationFound = true;
		}
	}

	if (!locationFound)
	{
		Extents worldExtents = workspace->computeExtentsWorldSlow();
		location = Vector3(0, 100, 0);

		if (!worldExtents.contains(Vector3::zero()))
		{	
			location += worldExtents.bottomCenter();
		}
		forcefieldDuration = 0;
	}

	// This calculates our exact spawn location, taking into account there may be a "tower" of parts where we spawn.
	ContactManager *cm = workspace->getWorld()->getContactManager();
	const float kMaxSearchDepth = workspace->computeExtentsWorldFast().max().y;
	location = cm->findUpNearestLocationWithSpaceNeeded(kMaxSearchDepth, location, Humanoid::defaultCharacterCorner());
	cf.translation = location;

	FASTLOG3F(DFLog::PartStreamingRequests, "next spawn location = (%f,%f,%f)", location.x, location.y, location.z);

	nextSpawnLocation.reset(new SpawnData(location, forcefieldDuration, cf));
	nextSpawnLocationChangedSignal(nextSpawnLocation->position);
}

Player::SpawnData Player::calculateSpawnLocation(const std::string& preferedSpawnName)
{
	Vector3 location;
	int forcefieldDuration = 0;
	CoordinateFrame cf;

	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	if (!workspace)
	{
		FASTLOG(FLog::Error, "Player:calculateNextSpawnLocation - Player not in the world");

		// should only be called when Player is in Datamodel
		throw std::runtime_error("calculateNextSpawnLocation can only be called when Player is in the world");
	}

	bool locationFound = false;
	if (SpawnerService* ss = ServiceProvider::find<SpawnerService>(this))
	{
		if (SpawnLocation * sl = ss->GetSpawnLocation(this, preferedSpawnName))
		{
			location = sl->getCoordinateFrame().translation;
			cf = sl->getCoordinateFrame();
			forcefieldDuration = sl->forcefieldDuration;
			locationFound = true;
		}
	}

	if (!locationFound)
	{
		Extents worldExtents = workspace->computeExtentsWorldSlow();
		location = Vector3(0, 100, 0);

		if (!worldExtents.contains(Vector3::zero()))
		{	
			location += worldExtents.bottomCenter();
		}
		forcefieldDuration = 0;
	}

	// This calculates our exact spawn location, taking into account there may be a "tower" of parts where we spawn.
	ContactManager *cm = workspace->getWorld()->getContactManager();
	location = cm->findUpNearestLocationWithSpaceNeeded(workspace->computeExtentsWorldSlow().max().y, location, Humanoid::defaultCharacterCorner());

	FASTLOG3F(DFLog::PartStreamingRequests, "next spawn location = (%f,%f,%f)", location.x, location.y, location.z);
	cf.translation = location;

	return SpawnData(location, forcefieldDuration, cf);
}

static void CharacterLoadHelper(shared_ptr<ModelInstance> model, AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances)
{
	if(result == AsyncHttpQueue::Succeeded){
		std::for_each(instances->begin(), instances->end(), boost::bind(&addChild, model, _1));	
	}
}

static void PlayerScriptsLoadHelper(weak_ptr<PlayerScripts> playerScripts, AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances)
{
	if(result == AsyncHttpQueue::Succeeded){
		std::for_each(instances->begin(), instances->end(), boost::bind(&addChildToInstance, playerScripts, _1));	
	}
}

void Player::checkContextReadyToSpawnCharacter() {
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	DataModel* dataModel = DataModel::get(this);

	if (!workspace || !dataModel){	
		FASTLOG(FLog::Error, "Player:LoadCharacter - Player not in the world");

		// should only be called when Player is in Datamodel
		throw std::runtime_error("LoadCharacter can only be called when Player is in the world");
	}

	if (!Players::backendProcessing(this)){
		FASTLOG(FLog::Error, "Player:LoadCharacter - Backend server required");

		throw std::runtime_error("LoadCharacter can only be called by the backend server");
	}
}

void Player::loadCharacter(bool inGame, std::string preferedSpawnName)
{
	checkContextReadyToSpawnCharacter();

	shared_ptr<ModelInstance> model;
	ModelInstance *pModel = NULL;
	StarterPlayerService* starterPlayerService = ServiceProvider::create<StarterPlayerService>(this) ;
	bool doLoadCharacterAppearance = true;

    bool useR15 = false;
    if (DFFlag::UseR15Character) 
    {
        if(DataModel* dataModel = DataModel::get(this))
        {
            useR15 = dataModel->getForceR15();
        }
    }

	if (DFFlag::UseStarterPlayerCharacter && starterPlayerService != NULL) 
	{
		Instance *pStarterCharacter = starterPlayerService->findFirstChildByName("StarterCharacter");
		if (pStarterCharacter && RBX::Instance::fastDynamicCast<ModelInstance>(pStarterCharacter) != NULL)
		{
			if (shared_ptr<Instance> copy = pStarterCharacter->clone(EngineCreator)) 
			{
				model = RBX::Instance::fastSharedDynamicCast<ModelInstance>(copy);
				pModel = model.get();
			}
		}
		doLoadCharacterAppearance = starterPlayerService->getLoadCharacterAppearance();
	}

	if (model == NULL)
	{
		Instances instances;

		if (useR15) 
		{
			ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/characterR15.rbxm"), instances);
		} else {
			ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/character3.rbxm"), instances);
		}

		if (instances.size() != 1){
			FASTLOG(FLog::Error, "Player:LoadCharacter - Could not load character3.rbxm");
			throw std::runtime_error("character3.rbxm should contain a Character model");
		}

		model = Instance::fastSharedDynamicCast<ModelInstance>(instances.front());
		pModel = model.get();
		if (!pModel){
			throw std::runtime_error("character3.rbxm should contain a Character model");
		}
	}


	Humanoid* humanoid = Humanoid::modelIsCharacter(pModel);
	if (!DFFlag::UseStarterPlayerCharacter && !humanoid){
		throw std::runtime_error("character3.rbxm should contain a humanoid");
	} else {
		if (DFFlag::UseStarterPlayerHumanoid) {
			if(StarterPlayerService* starterPlayerService = ServiceProvider::create<StarterPlayerService>(this)) {
				if(Humanoid* defaultHumanoid = starterPlayerService->findFirstChildOfType<Humanoid>()) {
					if(shared_ptr<Instance> copy = defaultHumanoid->clone(EngineCreator)) {
						if (humanoid)
						{
							humanoid->destroy();
						}
						humanoid = Instance::fastDynamicCast<Humanoid>(copy.get());
						humanoid->setParent(pModel);
						copy->setName("Humanoid");
					}
				}
			}
		}
	}

	// force create the animator object 
	if (humanoid)
		humanoid->setupAnimator(); // TODO: add this to character3.rbxm?

	if (DFFlag::UseStarterPlayerCharacterScripts) 
	{
		bool overrideSoundScript = false;
		bool overrideHealthScript = false;
		bool overrideAnimateScript = false;

		if (starterPlayerService != NULL) 
		{
			RBX::StarterPlayerScripts* pStarterCharacterScripts = starterPlayerService->findFirstChildOfType<RBX::StarterCharacterScripts>();
			if (pStarterCharacterScripts)
			{

				for(size_t n = 0; n < pStarterCharacterScripts->numChildren(); n++) {
					Instance* c = pStarterCharacterScripts->getChild(n);
					// Might be null if Instance was non-archivable
					if(shared_ptr<Instance> copy = c->clone(EngineCreator))
						copy.get()->setParent(pModel);
				}

				Instance *pSoundScript = pStarterCharacterScripts->findFirstChildByName("Sound");
				if (pSoundScript)
				{
					overrideSoundScript = true;
				}

				Instance *pHealthScript = pStarterCharacterScripts->findFirstChildByName("Health");
				if (pHealthScript)
				{
					overrideHealthScript = true;
				}

				Instance *pAnimateScript = pStarterCharacterScripts->findFirstChildByName("Animate");
				if (pAnimateScript)
				{
					overrideAnimateScript = true;
				}
			}

			if (!overrideSoundScript)
			{
				// Load up the special script that adds sounds to the character
				Instances extraSound;
				ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/humanoidSoundNewLocal.rbxmx"), extraSound);
				std::for_each(extraSound.begin(), extraSound.end(), boost::bind(&addChild, model, _1));
			}

			if (inGame && !overrideHealthScript)
			{
				// new rbxm (storing in humanoidHealthRegenScript.rbxm to make turning flag on easy) only contains server script that does humanoid health regen (gui script is now core script)
				ServiceProvider::create<ContentProvider>(this)->loadContent(ContentId::fromAssets("fonts/humanoidHealthRegenScript.rbxm"), ContentProvider::PRIORITY_SCRIPT, boost::bind(&CharacterLoadHelper, model, _1, _2), AsyncHttpQueue::AsyncWrite);
			}

			if (!overrideAnimateScript)
			{
				Instances extraAnimate;
				if (useR15)
				{
					ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/humanoidAnimateR15.rbxm"), extraAnimate);
				} else {
					ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/humanoidAnimateLocalKeyframe2.rbxm"), extraAnimate);
				}
				std::for_each(extraAnimate.begin(), extraAnimate.end(), boost::bind(&addChild, model, _1));
			}

		}
	} else {
		// Load up the special script that adds sounds to the character
		{
			Instances extraSound;
			ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/humanoidSoundNewLocal.rbxmx"), extraSound);
			std::for_each(extraSound.begin(), extraSound.end(), boost::bind(&addChild, model, _1));
		}

		if(inGame)
		{
			// new rbxm (storing in humanoidHealthRegenScript.rbxm to make turning flag on easy) only contains server script that does humanoid health regen (gui script is now core script)
			ServiceProvider::create<ContentProvider>(this)->loadContent(ContentId::fromAssets("fonts/humanoidHealthRegenScript.rbxm"), ContentProvider::PRIORITY_SCRIPT, boost::bind(&CharacterLoadHelper, model, _1, _2), AsyncHttpQueue::AsyncWrite);
		}

		Instances extraAnimate;
		if (useR15)
		{
			ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/humanoidAnimateR15.rbxm"), extraAnimate);
		} else {
			ServiceProvider::create<ContentProvider>(this)->blockingLoadInstances(ContentId::fromAssets("fonts/humanoidAnimateLocalKeyframe2.rbxm"), extraAnimate);
		}
		std::for_each(extraAnimate.begin(), extraAnimate.end(), boost::bind(&addChild, model, _1));
	}

	//entry.addValue("model->setName");
	model->setName(this->getName());

	//entry.addValue("buildJoints");
	// Build the joints in the character
	if (humanoid)
	{
		humanoid->buildJoints(DataModel::get(this));	
	}

	//entry.addValue("propArchivable");
	// Characters don't get archived with the world	
	Instance::propArchivable.setValue(model.get(), false);

	//entry.addValue("setCharacter");
	setCharacter(model.get());						// old character will be deleted here

	//entry.addValue("setParent");
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	model->setParent(workspace);					// scripts start running

	// Note: We load character appearance *after* adding to the DataModel so that CharacterAppearance::apply 
	//       knows that we are backend processing.
	if (humanoid)
	{
		if (doLoadCharacterAppearance)
			loadCharacterAppearance(!inGame);

		if (!inGame) 
		{
			humanoid->buildJoints(DataModel::get(this));
		}
	}

	if (calculatesSpawnLocationEarly()) {
		RBXASSERT(nextSpawnLocation);
		SpawnerService::SpawnPlayer(workspace, model, nextSpawnLocation->cf, nextSpawnLocation->forceFieldDuration);
		hasSpawnedAtLeastOnce = true;
	} else {
		SpawnData spawnData = calculateSpawnLocation(preferedSpawnName);
		nextSpawnLocationChangedSignal(spawnData.position);
		SpawnerService::SpawnPlayer(workspace, model, spawnData.cf, spawnData.forceFieldDuration);
	}
}

void Player::setupHumanoid(shared_ptr<Humanoid> humanoid)
{
	if(UserInputService *userInputService = ServiceProvider::create<UserInputService>(this))
		if(userInputService->getTouchEnabled())
			humanoid->setAutoJump(true);

	humanoid->setAutoJumpEnabled(getAutoJumpEnabled());

	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	RBXASSERT(workspace);

	if (workspace && workspace->getCamera() && workspace->getCamera()->getCameraSubject() == NULL)
	{
		workspace->getCamera()->setCameraSubject(humanoid.get());
		workspace->getCamera()->setCameraType(Camera::CUSTOM_CAMERA);

		workspace->getCamera()->setDistanceFromTarget(13.0);
		workspace->getCamera()->zoom(-1);
	}
}

void Player::characterChildAdded(shared_ptr<Instance> child)
{
	if(Humanoid* humanoid = RBX::Instance::fastDynamicCast<Humanoid>(child.get()))
	{
		setupHumanoid(shared_from(humanoid));
		charChildAddedConnection.disconnect(); // only listen to the first humanoid added to character
	}
}

// only do this if it's my local Player
void Player::onCharacterChangedFrontend()
{
	RBXASSERT(Players::frontendProcessing(this));

	if (this == Players::findLocalPlayer(this))
	{
		Workspace* workspace = ServiceProvider::find<Workspace>(this);
		RBXASSERT(workspace);

		if (character == NULL)
		{
			workspace->getCamera()->setCameraType(Camera::FIXED_CAMERA);
			workspace->getCamera()->setDistanceFromTarget(0.0);
			workspace->getCamera()->zoom(-1);
		}
		else
		{
			if (Humanoid* humanoid = Humanoid::modelIsCharacter(character.get()))
				setupHumanoid(shared_from(humanoid));
			else  // in case humanoid is null at this point, do a check on childAdded so we can setup later
				charChildAddedConnection = character->onDemandWrite()->childAddedSignal.connect(boost::bind(&Player::characterChildAdded, this, _1));

			workspace->setDefaultMouseCommand();

            onTeleportInternalSignal.connect(boost::bind(&Player::handleTeleportInternalSignal, this, _1, _2, _3));
		}
	}
}

void copyChildrenToBackpack(Instance* starterItems, Instance* backpack)
{
	for(size_t n = 0; n < starterItems->numChildren(); n++) {
		Instance* c = starterItems->getChild(n);
		// Might be null if Instance was non-archivable
		if(shared_ptr<Instance> copy = c->clone(EngineCreator))
			copy.get()->setParent(backpack);
	}
}

void copyChildrenToGui(Instance* starterItems, Instance* playerGui)
{
	copyChildrenToBackpack(starterItems, playerGui);
}

void Player::createPlayerGui()
{
	RBX::PlayerGui* currPlayerGui = findFirstChildOfType<RBX::PlayerGui>();
	if (!currPlayerGui)
	{
		shared_ptr<RBX::PlayerGui> playerGui = Creatable<Instance>::create<RBX::PlayerGui>();
		playerGui.get()->setParent(this);
		currPlayerGui = playerGui.get();
	}
}


void Player::rebuildGui()
{
	// If network filter is disabled, this should only be called by server
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	if (!Players::backendProcessing(this) && !workspace->getNetworkFilteringEnabled())
		throw std::runtime_error("rebuildBackpack can only be called by the backend server");

	// Make sure we have a clean slate player gui, then populate it with starter gui elements
	StarterGuiService* starterGui = ServiceProvider::find<StarterGuiService>(this);

	RBX::PlayerGui* currPlayerGui = findFirstChildOfType<RBX::PlayerGui>();	
	if(currPlayerGui) 
	{	
		if (!copiedGuiOnce)
		{
			copyChildrenToGui(starterGui, currPlayerGui);
			copiedGuiOnce = true;
		}
		else if(starterGui && starterGui->getResetPlayerGui()) 
		{
			currPlayerGui->removeAllChildren();
			copyChildrenToGui(starterGui, currPlayerGui);
		}	
	}
}


void Player::rebuildBackpack()
{
	if (!Players::backendProcessing(this))
		throw std::runtime_error("rebuildBackpack can only be called by the backend server");

	while (Backpack* oldBackpack = this->findFirstChildOfType<Backpack>()) {
		oldBackpack->setParent(NULL);
	}

	// make a backpack under this player
	shared_ptr<Instance> backpack = Creatable<Instance>::create<Backpack>();
	backpack.get()->setParent(this);

	if (StarterPackService* starterPack = ServiceProvider::find<StarterPackService>(this)) {
		copyChildrenToBackpack(starterPack, backpack.get());
	}

	if (StarterGear* starterGear = this->findFirstChildOfType<StarterGear>()) {
		copyChildrenToBackpack(starterGear, backpack.get());
	}
}

void Player::rebuildPlayerScripts()
{
	if (!Players::frontendProcessing(this))
		throw std::runtime_error("rebuildPlayerScripts can only be called by the client");

	RBX::PlayerScripts* currPlayerScripts= findFirstChildOfType<RBX::PlayerScripts>();
	if (!currPlayerScripts)
	{
		shared_ptr<RBX::PlayerScripts> playerScripts = Creatable<Instance>::create<RBX::PlayerScripts>();
		if (playerScripts.get()) 
		{
			playerScripts->setParent(this);
			currPlayerScripts = playerScripts.get();
		}
	}
}

void Player::setName(const std::string& value)
{
#ifndef REMOVE_PLAYER_PROTECTIONS 
	RBX::Security::Context::current().requirePermission(RBX::Security::WritePlayer, "set a Player's name");
#endif
	Super::setName(value);
}

void Player::setUserId(int value)
{
	if (value!=userId)
	{
#ifdef REMOVE_PLAYER_PROTECTIONS 
		if(FriendService* friendService = ServiceProvider::find<FriendService>(this)){
			if(!isGuest()){
				friendService->playerRemoving(userId);
			}
		}	
#endif
#ifndef REMOVE_PLAYER_PROTECTIONS 
		RBX::Security::Context::current().requirePermission(RBX::Security::WritePlayer, "set a Player's ID");
#endif
		userId = value;
		raisePropertyChanged(prop_userId);
		raisePropertyChanged(prop_Guest);

#ifdef REMOVE_PLAYER_PROTECTIONS
		if(FriendService* friendService = ServiceProvider::find<FriendService>(this)){
			if(!isGuest()){
				friendService->playerAdded(userId);
			}
		}
#endif
	}
}

void Player::setGameSessionID(std::string value)
{
	if (value != gameSessionID)
	{
		gameSessionID = value;
	}
}

void Player::setMembershipType(Player::MembershipType value)
{
    if (membershipType != value)
    {
        membershipType = value;
        raisePropertyChanged(prop_membershipTypeReplicate);
        raisePropertyChanged(prop_membershipType);
    }
}

void Player::setAccountAge(int value)
{
    if (accountAge != value)
    {
        accountAge = value;
        raisePropertyChanged(prop_accountAgeReplicate);
        raisePropertyChanged(prop_accountAge);
    }
}

void Player::setCameraMode(RBX::Camera::CameraMode value)
{
	if(cameraMode != value)
	{
		cameraMode = value;
		if(ModelInstance* character = getCharacter())
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setCameraMode(cameraMode); // signal to the camera what camera mode we are in
		raisePropertyChanged(prop_cameraMode);
	}
}


void Player::setNameDisplayDistance(float value)
{
	float legalValue = G3D::max(value, 0.0f);
	if(nameDisplayDistance != legalValue)
	{
		nameDisplayDistance = legalValue;
		if(ModelInstance* character = getCharacter())
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setNameDisplayDistance(nameDisplayDistance);			
		raisePropertyChanged(prop_nameDisplayDistance);
	}
}

void Player::setHealthDisplayDistance(float value)
{
	float legalValue = G3D::max(value, 0.0f);
	if(healthDisplayDistance != legalValue)
	{
		healthDisplayDistance = legalValue;
		if(ModelInstance* character = getCharacter())
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setHealthDisplayDistance(healthDisplayDistance);			
		raisePropertyChanged(prop_healthDisplayDistance);
	}
}



void Player::setCameraMaxZoomDistance(float value)
{
	if(cameraMaxZoomDistance != value)
	{
		if (value < cameraMinZoomDistance)
			value = cameraMinZoomDistance;

		if (value > Camera::distanceMaxCharacter())
			value = Camera::distanceMaxCharacter();

		cameraMaxZoomDistance = value;
		if(ModelInstance* character = getCharacter())
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setMaxDistance(cameraMaxZoomDistance);			
		raisePropertyChanged(prop_cameraMaxZoomDistance);
	}
}

void Player::setCameraMinZoomDistance(float value)
{
	if(cameraMinZoomDistance != value)
	{
		if (value > cameraMaxZoomDistance)
			value = cameraMaxZoomDistance;

		if (value < Camera::distanceMin())
			value = Camera::distanceMin();

		cameraMinZoomDistance = value;
		if(ModelInstance* character = getCharacter())
			if (Humanoid* humanoid = character->findFirstChildOfType<Humanoid>())
				humanoid->setMinDistance(cameraMinZoomDistance);			
		raisePropertyChanged(prop_cameraMinZoomDistance);
	}
}

void Player::setDevEnableMouseLockOption(bool setting)
{
	if (!Players::backendProcessing(this, false) && Players::frontendProcessing(this, false))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setDevEnableMouseLockOption");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficent permissions to set DevEnableMouseLockOption");
			throw e;
		}
	}

	if(enableMouseLockOption != setting)
	{
		enableMouseLockOption = setting;
		raisePropertyChanged(prop_devEnableMouseLockOption);
	}
}

void Player::setDevTouchCameraMode(StarterPlayerService::DeveloperTouchCameraMovementMode setting)
{
	if (!Players::backendProcessing(this, false) && Players::frontendProcessing(this, false))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setDevTouchCameraMode");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficent permissions to set DevTouchCameraMode");
			throw e;
		}
	}

	if(touchCameraMovementMode != setting)
	{
		touchCameraMovementMode = setting;
		raisePropertyChanged(prop_devTouchCameraMode);
	}
}

void Player::setDevComputerCameraMode(StarterPlayerService::DeveloperComputerCameraMovementMode setting)
{
	if (!Players::backendProcessing(this, false) && Players::frontendProcessing(this, false))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setDevComputerCameraMode");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficent permissions to set DevComputerCameraMode");
			throw e;
		}
	}

	if(computerCameraMovementMode != setting)
	{
		computerCameraMovementMode = setting;
		raisePropertyChanged(prop_devComputerCameraMode);
	}
}

void Player::setDevCameraOcclusionMode(StarterPlayerService::DeveloperCameraOcclusionMode setting)
{
	if (!Players::backendProcessing(this, false) && Players::frontendProcessing(this, false))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setDevCameraOcclusionMode");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficent permissions to set DevCameraOcclusionMode");
			throw e;
		}
	}

	if(cameraOcclusionMode != setting)
	{
		cameraOcclusionMode = setting;
		raisePropertyChanged(prop_devCameraOcclusionMode);
	}
}

void Player::setDevTouchMovementMode(StarterPlayerService::DeveloperTouchMovementMode setting)
{
	if (!Players::backendProcessing(this, false) && Players::frontendProcessing(this, false))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setDevTouchMovementMode");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficent permissions to set DevTouchMovementMode");
			throw e;
		}
	}

	if(touchMovementMode != setting)
	{
		touchMovementMode = setting;
		raisePropertyChanged(prop_devTouchMovementMode);
	}
}

void Player::setDevComputerMovementMode(StarterPlayerService::DeveloperComputerMovementMode setting)
{
	if (!Players::backendProcessing(this, false) && Players::frontendProcessing(this, false))
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "setDevComputerMovementMode");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficent permissions to set DevComputerMovementMode");
			throw e;
		}
	}

	if(computerMovementMode != setting)
	{
		computerMovementMode = setting;
		raisePropertyChanged(prop_devComputerMovementMode);
	}
}

void Player::setTeleportedIn(bool value)
{
	if (value != teleportedIn)
	{
		teleportedIn = value;
		raisePropertyChanged(prop_teleportedIn);
	}
}

const Backpack* Player::getConstPlayerBackpack() const
{
	const RBX::Backpack* backpack = findConstFirstChildOfType<RBX::Backpack>();
	return backpack;
}

Backpack* Player::getPlayerBackpack()
{
	return const_cast<Backpack*>(getConstPlayerBackpack());
}

void Player::verifySetParent(const Instance* instance) const
{
	if((instance !=NULL) && (instance->fastDynamicCast<Players>() == NULL)){
		throw RBX::runtime_error("Parent of Player can not be changed");
	}
	Super::verifySetParent(instance);
}

static void dontCareResponse()
{

}

void Player::requestFriendship(shared_ptr<Instance> instance)
{
	shared_ptr<Player> player;
	if(!(player = Instance::fastSharedDynamicCast<Player>(instance)))
	{	
		throw RBX::runtime_error("RequestFriendship should be passed a Player");
	}

	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		const std::string postData = "recipientUserId=" + boost::lexical_cast<std::string>(player->getUserID());
		apiService->postAsync("user/request-friendship?" + postData, postData, true, RBX::PRIORITY_DEFAULT, HttpService::TEXT_PLAIN, boost::bind(&dontCareResponse), boost::bind(&dontCareResponse));
	}

	desc_remoteFriendServiceSignal.fireAndReplicateEvent(this, true, player->getUserID());
}

void Player::revokeFriendship(shared_ptr<Instance> instance)
{
	shared_ptr<Player> player;
	if(!(player = Instance::fastSharedDynamicCast<Player>(instance)))
	{	
		throw RBX::runtime_error("RevokeFriendship should be passed a Player");
	}

	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		const std::string postData =  "requesterUserId=" + boost::lexical_cast<std::string>(player->getUserID());
		apiService->postAsync("user/decline-friend-request?" + postData, postData, true, RBX::PRIORITY_DEFAULT, HttpService::TEXT_PLAIN, boost::bind(&dontCareResponse), boost::bind(&dontCareResponse));
	}

	desc_remoteFriendServiceSignal.fireAndReplicateEvent(this, false, player->getUserID());
	
}
void Player::onFriendStatusChanged(shared_ptr<Instance> player, FriendService::FriendStatus friendStatus)
{
	friendStatusChangedSignal(player, friendStatus);
}

shared_ptr<Instance> Player::getMouseInstance()
{
	if (!mouse)
	{
		// create an Mouse object for this player, if this is the local player
		if (this == Players::findLocalPlayer(this))
		{
			mouse = Creatable<Instance>::create<PlayerMouse>();
			mouse->lockParent();

			// and set its command to whatever the current workspace command is
			if (Workspace* workspaceForMouseCommand = ServiceProvider::find<Workspace>(this))
			{
				mouse->setWorkspace(workspaceForMouseCommand);
			}
		}
	}
	return mouse;
}

FriendService::FriendStatus Player::getFriendStatus(shared_ptr<Instance> playerInstance)
{
	shared_ptr<Player> player = Instance::fastSharedDynamicCast<Player>(playerInstance);
	if(!player)
		throw std::runtime_error("player argument must be of type Player");
	Players* players = ServiceProvider::find<Players>(this);
	if(!players || this->getParent() != players || player->getParent() != players)
		throw std::runtime_error("Player objects must be children of 'Players'");

	if(FriendService* friendService = ServiceProvider::find<FriendService>(this)){
		return friendService->getFriendStatus(this->getUserID(), player->getUserID());
	}

	return FriendService::FRIEND_STATUS_UNKNOWN;
}
void Player::isFriendsWith(int otherUserId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(Players* players = ServiceProvider::find<Players>(this)){
		if(shared_ptr<Player> playerInstance = players->getPlayerByID(otherUserId)){
			if(FriendService* friendService = ServiceProvider::find<FriendService>(this)){
				if(friendService->getEnable()){
					//Both players are in the game, try to use getFriendStatus
					try
					{
						FriendService::FriendStatus status = getFriendStatus(playerInstance);
						switch(status)
						{
							case FriendService::FRIEND_STATUS_NOT_FRIEND:
							case FriendService::FRIEND_STATUS_FRIEND_REQUEST_SENT:
							case FriendService::FRIEND_STATUS_FRIEND_REQUEST_RECEIVED:
							{
								//Not friends, or not friends yet
								resumeFunction(false);
								return;
							}
							case FriendService::FRIEND_STATUS_FRIEND:
							{
								resumeFunction(true);
								return;
							}
							case FriendService::FRIEND_STATUS_UNKNOWN:
							default:
							{
								//Fallback to old way of quering if we are in a "Waiting" state still
								break;
							}
						}
					}
					catch(RBX::base_exception&)
					{
						//Quash exception and fallback
					}
				}
			}
		}
	}

    if(RBX::SocialService* socialService = ServiceProvider::create<RBX::SocialService>(this))
        socialService->isFriendsWith(getUserID(), otherUserId, resumeFunction, errorFunction);
    else
        errorFunction("Player not in Workspace");
}
void Player::isBestFriendsWith(int otherUserId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	isFriendsWith(otherUserId, resumeFunction, errorFunction);
}
void Player::isInGroup(int groupId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
    if(RBX::SocialService* socialService = ServiceProvider::create<RBX::SocialService>(this))
        socialService->isInGroup(getUserID(), groupId, resumeFunction, errorFunction);
    else
        errorFunction("Player not in Workspace");
}

void Player::getRankInGroup(int groupId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(RBX::SocialService* socialService = ServiceProvider::create<RBX::SocialService>(this))
	{
		socialService->getRankInGroup(getUserID(), groupId, resumeFunction, errorFunction);
	}
	else
		errorFunction("Player not in Workspace");
}

void Player::getRoleInGroup(int groupId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(RBX::SocialService* socialService = ServiceProvider::create<RBX::SocialService>(this))
	{
		socialService->getRoleInGroup(getUserID(), groupId, resumeFunction, errorFunction);
	}
	else
		errorFunction("Player not in Workspace");
}

void Player::getFriendsOnline(int maxFriends, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction) 
{
	if(maxFriends > 200)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "GetFriendsOnline only returns a maximum of 200 friends");
	}
	if(Players* players = ServiceProvider::find<Players>(this))
	{
		if(this != players->getLocalPlayer())
		{
			errorFunction("GetFriendsOnline must be invoked on the Local Player");
		}
	}
	if(RBX::FriendService* friendService = ServiceProvider::create<RBX::FriendService>(this)) 
	{
		friendService->getFriendsOnline(maxFriends, resumeFunction, errorFunction);
	}
	else
	{
		errorFunction("Player not in Workspace"); 
	}
}

const RakNet::SystemAddress& Player::getRemoteAddress() const {
	if (remoteAddress) {
		return *remoteAddress;
	} else {
		static RakNet::SystemAddress addr;
		return addr;
	}
}

const SystemAddress Player::getRemoteAddressAsRbxAddress() const {
	return RakNetToRbxAddress(getRemoteAddress());
}

void Player::setRemoteAddress(const RakNet::SystemAddress& address) {
	remoteAddress.reset(new RakNet::SystemAddress(address));
}

void Player::destroy()
{
	if (DFFlag::CloudEditDisablePlayerDestroy && Players::isCloudEdit(this))
		throw std::runtime_error("Cannot Destroy Player");

	Super::destroy();
}

void Player::kick(std::string msg)
{
	bool delay = false;

	if (DFFlag::FilterKickMessage)
	{
		if(ProfanityFilter::ContainsProfanity(msg))
		{
			msg = "";
		}
	}

	if (msg != "")
	{
		// set up message
		if (Network::Players::serverIsPresent(this)) // check for play solo
		{
			const RBX::SystemAddress& target = getRemoteAddressAsRbxAddress();

			// respond to client
			Reflection::EventArguments args(1);
			args[0] = msg;

			raiseEventInvocation(event_setShutdownMessage, args, &target);
		} 
		else 
		{
			if(GuiService* gs = ServiceProvider::find<GuiService>(this)) {
				gs->setErrorMessage(msg);
			}
		}
		delay = true;
	}

	try
	{
		if(character)
			character->destroy();
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_WARNING, "Player:Kick() failed to destroy character: %s", e.what());
	}

	if(Network::Players* players = ServiceProvider::find<Players>(this))
	{
		if (Players::frontendProcessing(this, false))
		{
			if(this != players->getLocalPlayer())
				throw std::runtime_error("Player:Kick() called from local script, but not on local player!");
			else
				players->disconnectPlayerLocal(userId, Replicator::DisconnectReason_LuaKick);
		}
		else
		{
			TimerService* ts = RBX::ServiceProvider::find<TimerService>(this);
			if (ts && delay) 
			{
				ts->delay(boost::bind(&Players::disconnectPlayer, players, userId, Replicator::DisconnectReason_LuaKick), 1.0f);
			} else {
				players->disconnectPlayer(userId, Replicator::DisconnectReason_LuaKick);
			}
		}
	}

	if (!delay)
	{
		unlockParent();
		destroy();
	}
}


void Player::handleTeleportInternalSignal(TeleportService::TeleportState teleportState, shared_ptr<const Reflection::ValueTable> teleportInfo, shared_ptr<Instance> CustomLoadingGUI)
{
    switch (teleportState)
    {
    case TeleportService::TeleportState_RequestedFromServer:
        {
            if (TeleportService* teleportService = ServiceProvider::find<TeleportService>(this))
            {
                teleportService->TeleportImpl(teleportInfo, CustomLoadingGUI);
            }
            break;
        }
    default:
        // do nothing
        break;
    }
}

void Player::handleTeleportSignalBackend(TeleportService::TeleportState teleportState)
{
	RBXASSERT(Players::backendProcessing(this));

	if (teleportState == TeleportService::TeleportState_InProgress || 
		teleportState == TeleportService::TeleportState_Started ||
		teleportState == TeleportService::TeleportState_WaitingForServer)
		teleported = true;
	else
		teleported = false;
}

void Player::onTeleport(TeleportService::TeleportState teleportState, int placeId, std::string instanceIdOrSpawnName)
{
    event_OnTeleport.fireAndReplicateEvent(this, teleportState, placeId, instanceIdOrSpawnName);
}

void Player::onTeleportInternal(TeleportService::TeleportState teleportState, shared_ptr<const Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI)
{
	event_OnTeleport.fireAndReplicateEvent(this, teleportState, teleportInfo->at("placeId").get<int>(), teleportInfo->at("spawnName").get<std::string>());
	event_OnTeleportInternal.fireAndReplicateEvent(this, teleportState, teleportInfo, customLoadingGUI);
}

void Player::loadChatInfo() {
	boost::function0<void> f = boost::bind(&Player::loadChatInfoInternal, weak_from(this));
	boost::function0<void> g = boost::bind(&StandardOut::print_exception, f, RBX::MESSAGE_ERROR, false);
	boost::thread(thread_wrapper(g, "rbx_getPlayerMetaData"));
}

void Player::loadChatInfoInternal(weak_ptr<Player> weakPlayer) {
	unsigned userIdBackoffExponent = 0;
	shared_ptr<Player> player = weakPlayer.lock();
	bool trying = NULL != player;
	while (trying) {
		try {
			std::string response;
			RBX::Http(GetPlayerGameDataUrl(GetBaseURL(), player->getUserID())).get(response);
            
            static const char* kChat = "ChatFilter";
			std::string chatFilterTypeString;

        	rapidjson::Document doc;
        	doc.Parse<rapidjson::kParseDefaultFlags>(response.c_str());
        	FASTLOG(FLog::Network, "Completed Player's JSON parsing...");
        	RBXASSERT(doc.HasMember(kChat));
        
        	chatFilterTypeString = doc[kChat].GetString();
        	FASTLOGS(FLog::Network, "loadChatInfoInternal.chatFilterType: %s", chatFilterTypeString);
			            
			Player::ChatFilterType chatFilterType;
			bool chatFilterTypeValid = false;
			if (boost::iequals("whitelist", chatFilterTypeString)) {
				chatFilterType = Player::CHAT_FILTER_WHITELIST;
				chatFilterTypeValid = true;
			} else if (boost::iequals("blacklist", chatFilterTypeString)) {
				chatFilterType = Player::CHAT_FILTER_BLACKLIST;
				chatFilterTypeValid = true;
			}

			if (chatFilterTypeValid) {
				DataModel::LegacyLock(DataModel::get(player.get()), DataModelJob::Write);
				player->chatFilterType = chatFilterType;
				player->chatInfoHasBeenLoaded = true;
				trying = false;
			}
		} catch (std::exception &e) {
			StandardOut::singleton()->printf(MESSAGE_ERROR, "loadChatInfoInternal had an error: %s", e.what());
			StandardOut::singleton()->printf(MESSAGE_INFO, "loadChatInfoInternal retrying...");
		}

		if (trying) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(1000 * (1 << userIdBackoffExponent)));
			if (userIdBackoffExponent < DFLog::PlayerChatInfoExponentialBackoffLimitMultiplier) {
				++userIdBackoffExponent;
			}
		}
	} // while
}

void Player::setChatInfo(ChatFilterType filterType)
{
	chatInfoHasBeenLoaded = true;
	chatFilterType = filterType;
}

bool Player::isChatInfoValid() const
{
	return chatInfoHasBeenLoaded;
}

Player::ChatFilterType Player::getChatFilterType() const
{
	return chatFilterType;
}

void Player::setForceEarlySpawnLocationCalculation() {
	forceEarlySpawnLocationCalculation = true;
}

bool Player::calculatesSpawnLocationEarly() const {
	return forceEarlySpawnLocationCalculation;
}

namespace RBX {
namespace Reflection {
template<>
EnumDesc<Player::MembershipType>::EnumDesc()
:EnumDescriptor("MembershipType")
{
	addPair(Player::MEMBERSHIP_NONE,                     "None");
	addPair(Player::MEMBERSHIP_BUILDERS_CLUB,            "BuildersClub");
	addPair(Player::MEMBERSHIP_TURBO_BUILDERS_CLUB,      "TurboBuildersClub");
	addPair(Player::MEMBERSHIP_OUTRAGEOUS_BUILDERS_CLUB, "OutrageousBuildersClub");
}


template<>
Player::MembershipType& Variant::convert<Player::MembershipType>(void)
{
	return genericConvert<Player::MembershipType>();
}

template<>
EnumDesc<Player::ChatMode>::EnumDesc()
:EnumDescriptor("ChatMode")
{
	addPair(Player::CHAT_MODE_MENU, "Menu");
	addPair(Player::CHAT_MODE_TEXT_AND_MENU, "TextAndMenu");
}

template<>
Player::ChatMode& Variant::convert<Player::ChatMode>(void)
{
	return genericConvert<Player::ChatMode>();
}

}//namespace Reflection

template<>
bool StringConverter<Player::MembershipType>::convertToValue(const std::string& text, Player::MembershipType& value)
{
	return Reflection::EnumDesc<Player::MembershipType>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool StringConverter<Player::ChatMode>::convertToValue(const std::string& text, Player::ChatMode& value)
{
	return Reflection::EnumDesc<Player::ChatMode>::singleton().convertToValue(text.c_str(),value);
}

}//namespace RBX

