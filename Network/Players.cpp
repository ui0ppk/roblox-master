/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#include "Network/Players.h"
#include "Network/Player.h"
#include "V8DataModel/FriendService.h"
#include "Replicator.h"
#include "GuidRegistryService.h"
#include "Client.h"
#include "Server.h"
#include "Streaming.h"
#include "Util.h"
#include "util/RobloxGoogleAnalytics.h"

#include "WebChatFilter.h"
#include "RakPeerInterface.h"
#include "PacketIds.h"
#include "BitStream.h"
#include "RakNet/Source/PluginInterface2.h"
#include "ConcurrentRakPeer.h"
#include "PhysicsSender.h"
#include "PhysicsReceiver.h"

#include "Humanoid/Humanoid.h"
#include "V8DataModel/CharacterAppearance.h"
#include "V8DataModel/DataModelMesh.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/InsertService.h"
#include "V8DataModel/Modelinstance.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/SafeChat.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/ReplicatedFirst.h"
#include "v8datamodel/HttpRbxApiService.h"
#include "Util/LuaWebService.h"
#include "Util/SystemAddress.h"

#include "V8Xml/WebParser.h"

#include "V8World/DistributedPhysics.h"
#include "Util/Statistics.h"
#include "Script/ScriptContext.h"
#include "Script/Script.h"

#include "V8Xml/XmlSerializer.h"
#include "Util/SafeToLower.h"

#include "rbx/RbxDbgInfo.h"

#include "Util/http.h"
#include "Util/Region2.h"
#include "Network/NetworkOwner.h"
#include <boost/algorithm/string.hpp>
#include <sstream>

const char* const RBX::Network::sPlayers = "Players";
static const char* const kPlaceIdUrlParamFormat = "&serverplaceid=%d";

using namespace RBX;
using namespace RBX::Network;
using namespace RakNet;

LOGGROUP(GoldenHashes)
LOGGROUP(Network)
LOGGROUP(US14116)

FASTFLAG(DebugLocalRccServerConnection)
DYNAMIC_FASTFLAG(RCCSupportCloudEdit)

DYNAMIC_LOGGROUP(WebChatFiltering)

DYNAMIC_FASTFLAGVARIABLE(GetCharacterAppearanceEnabled, false)

DYNAMIC_FASTFLAGVARIABLE(CreatePlayerGuiLocal, false)

DYNAMIC_FASTFLAGVARIABLE(FirePlayerAddedAndPlayerRemovingOnClient, false)

FASTSTRINGVARIABLE(GetUserIdUrl, "users/get-by-username?username=%s")
FASTSTRINGVARIABLE(GetUserNameUrl, "users/%i")
FASTSTRINGVARIABLE(GetFriendsUrl, "%susers/%i/friends")
DYNAMIC_FASTFLAGVARIABLE(LoadGuisWithoutChar, false)

DYNAMIC_FASTFLAGVARIABLE(FilterInvalidWhisper, true)
DYNAMIC_FASTFLAGVARIABLE(CloudEditSupportPlayersKickAndShutdown, true)

REFLECTION_BEGIN();
static Reflection::PropDescriptor<Players, int> propPlayerCount("NumPlayers", category_Data, &Players::getNumPlayers, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Players, int> depPlayerCount("numPlayers", category_Data, &Players::getNumPlayers, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(propPlayerCount, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
static Reflection::PropDescriptor<Players, int> propPlayerMaxCount("MaxPlayers", category_Data, &Players::getMaxPlayers, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Players, int> propPlayerMaxCountInternal("MaxPlayersInternal", category_Data, &Players::getMaxPlayers, &Players::setMaxPlayers, Reflection::PropertyDescriptor::STANDARD, Security::LocalUser);
static Reflection::PropDescriptor<Players, int> propPlayerPreferredCount("PreferredPlayers", category_Data, &Players::getPreferredPlayers, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<Players, int> propPlayerPreferredCountInternal("PreferredPlayersInternal", category_Data, &Players::getPreferredPlayers, &Players::setPreferredPlayers, Reflection::PropertyDescriptor::STANDARD, Security::LocalUser);
Reflection::RefPropDescriptor<Players, Instance> Players::propLocalPlayer("LocalPlayer", category_Data, &Players::getLocalPlayerDangerous, NULL, Reflection::PropertyDescriptor::UI);
Reflection::RefPropDescriptor<Players, Instance> dep_propLocalPlayer("localPlayer", category_Data, &Players::getLocalPlayerDangerous, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(Players::propLocalPlayer, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(int)> func_GetPlayerByID(&Players::getPlayerInstanceByID, "GetPlayerById", "userId", Security::LocalUser);
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(int)> func_GetPlayerByIdDeprecated(&Players::getPlayerInstanceByID, "GetPlayerByID", "userID", Security::LocalUser, Reflection::PropertyDescriptor::Attributes::deprecated(func_GetPlayerByID));

Reflection::PropDescriptor<Players, bool> Players::propCharacterAutoSpawn("CharacterAutoLoads", category_Behavior, &Players::getCharacterAutoSpawnProperty, &Players::setCharacterAutoSpawnProperty, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);

static Reflection::BoundFuncDesc<Players, void(std::string)> funcChat(&Players::chat, "Chat", "message", Security::Plugin);
static Reflection::BoundFuncDesc<Players, void(std::string)> funcTeamChat(&Players::teamChat, "TeamChat", "message", Security::Plugin);
static Reflection::BoundFuncDesc<Players, void(std::string, shared_ptr<Instance>)> funcPlayerChat(&Players::whisperChat, "WhisperChat", "message", "player", Security::LocalUser);
static Reflection::EventDesc<Players, void(Players::PlayerChatType, shared_ptr<Instance>, std::string, shared_ptr<Instance>)> event_Chatted(&Players::playerChattedSignal, "PlayerChatted", "chatType", "player", "message", "targetPlayer", Security::LocalUser);
static Reflection::EventDesc<Players, void(std::string)> event_GameAnnounce(&Players::gameAnnounceSignal, "GameAnnounce", "message", Security::RobloxScript);

static Reflection::BoundFuncDesc<Players, void(shared_ptr<Instance>, std::string, std::string)> funcReportAbuse(&Players::reportAbuseLua, "ReportAbuse", "player", "reason", "optionalMessage", Security::LocalUser);
static Reflection::BoundFuncDesc<Players, shared_ptr<const Instances>()> func_GetPlayers(&Players::getPlayers, "GetPlayers", Security::None);
static Reflection::BoundFuncDesc<Players, shared_ptr<const Instances>()> dep_players(&Players::getPlayers, "players", Security::None, Reflection::Descriptor::Attributes::deprecated(func_GetPlayers));
static Reflection::BoundFuncDesc<Players, shared_ptr<const Instances>()> dep_getPlayers(&Players::getPlayers, "getPlayers", Security::None, Reflection::Descriptor::Attributes::deprecated(func_GetPlayers));
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(int, bool)> func_createLocalPlayer(&Players::createLocalPlayer, "CreateLocalPlayer", "userId", "isTeleport", false, Security::Plugin);
static Reflection::BoundFuncDesc<Players, void(std::string)> func_setAbuseReportUrl( &Players::setAbuseReportUrl, "SetAbuseReportUrl", "url", Security::Roblox);
static Reflection::BoundFuncDesc<Players, void(std::string)> func_setChatFilterUrl( &Players::setChatFilterUrl, "SetChatFilterUrl", "url", Security::Roblox);

static Reflection::BoundFuncDesc<Players, void(std::string)> func_setBuildUserPermissionsUrl(&Players::setBuildUserPermissionsUrl, "SetBuildUserPermissionsUrl", "url", Security::Roblox);
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(shared_ptr<Instance>)> func_playerFromCharacter(&Players::playerFromCharacter, "GetPlayerFromCharacter", "character", Security::None);
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(shared_ptr<Instance>)> func_playerFromCharacterOld(&Players::playerFromCharacter, "playerFromCharacter", "character", Security::None, Reflection::Descriptor::Attributes::deprecated(func_playerFromCharacter));
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(shared_ptr<Instance>)> dep_playerFromCharacter(&Players::playerFromCharacter, "getPlayerFromCharacter", "character", Security::None, Reflection::Descriptor::Attributes::deprecated(func_playerFromCharacter));

static Reflection::EventDesc<Players, void(shared_ptr<Instance>, shared_ptr<Instance>, FriendService::FriendEventType)> event_FriendRequestEvent(&Players::friendRequestEvent, "FriendRequestEvent", "player", "player", "friendRequestEvent", Security::RobloxScript);
static Reflection::EventDesc<Players, void(shared_ptr<Instance>)> event_PlayerAddedEarly(&Players::playerAddedEarlySignal, "PlayerAddedEarly", "player", Security::LocalUser);
static Reflection::EventDesc<Players, void(shared_ptr<Instance>)> event_PlayerAdded(&Players::playerAddedSignal, "PlayerAdded", "player");
static Reflection::EventDesc<Players, void(shared_ptr<Instance>)> event_PlayerRemoving(&Players::playerRemovingSignal, "PlayerRemoving", "player");
static Reflection::EventDesc<Players, void(shared_ptr<Instance>)> event_PlayerRemovingLate(&Players::playerRemovingLateSignal, "PlayerRemovingLate", "player", Security::LocalUser);

static Reflection::BoundFuncDesc<Players, void(std::string)> func_setSysStatsUrl( &Players::setSysStatsUrl, "SetSysStatsUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<Players, void(std::string)> func_setSysHash( &Players::setSysHash, "SetSysStatsUrlId", "urlId", Security::LocalUser);

static Reflection::BoundFuncDesc<Players, void(std::string)> func_setLoadDataUrl( &Players::setLoadDataUrl, "SetLoadDataUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<Players, void(std::string)> func_setSaveDataUrl( &Players::setSaveDataUrl, "SetSaveDataUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<Players, void(std::string)> func_setSaveLeaderboardDataUrl( &Players::setSaveLeaderboardDataUrl, "SetSaveLeaderboardDataUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<Players, void(std::string)> func_addLeaderboardKey( &Players::addLeaderboardKey, "AddLeaderboardKey", "key", Security::LocalUser);

static Reflection::BoundFuncDesc<Players, void(Players::ChatOption)> func_setChatOption(&Players::setChatOption, "SetChatStyle", "style", Players::CLASSIC_CHAT, Security::Plugin);
Reflection::PropDescriptor<Players, bool> propClassicChat("ClassicChat", category_Data, &Players::getClassicChat, NULL, Reflection::PropertyDescriptor::UI);
Reflection::PropDescriptor<Players, bool> propBubbleChat("BubbleChat", category_Data, &Players::getBubbleChat, NULL, Reflection::PropertyDescriptor::UI);

static Reflection::BoundFuncDesc<Players, bool()> func_coreScriptHealthBar(&Players::getUseCoreScriptHealthBar, "GetUseCoreScriptHealthBar", Security::RobloxScript);

// Chat Ignore Support
static Reflection::RemoteEventDesc<Players, void(int, int, std::string)> event_serverFinishedBlockUser(&Players::blockUserFinishedFromServerSignal, "BlockedRequestFinishedSignal", "blockerUserId", "blockeeUserId", "didDoRequest", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<Players, void(int, int, bool)> event_blockUserFromClient(&Players::blockUserRequestFromClientSignal, "BlockUserSignal", "blockerUserId", "blockeeUserId", "blocking", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::BoundYieldFuncDesc<Players, std::string(int, int)> func_blockUser(&Players::blockUser, "BlockUser", "blockerUserId", "blockeeUserId", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<Players, std::string(int, int)> func_unblockUser(&Players::unblockUser, "UnblockUser", "exblockerUserId", "exblockeeUserId", Security::RobloxScript);

//UserName to userId, userId to UserName
static Reflection::BoundYieldFuncDesc<Players, int(std::string)> func_getUserIdFromName(&Players::getUserIdFromName, "GetUserIdFromNameAsync", "userName", Security::None);
static Reflection::BoundYieldFuncDesc<Players, std::string(int)> func_getNameFromUserId(&Players::getNameFromUserId, "GetNameFromUserIdAsync", "userId", Security::None);
static Reflection::BoundYieldFuncDesc<Players, shared_ptr<Instance>(int)> func_getFriends(&Players::getFriends, "GetFriendsAsync", "userId", Security::None);
static Reflection::BoundYieldFuncDesc<Players, shared_ptr<Instance>(int)> func_getCharacterAppearance(&Players::getCharacterAppearance, "GetCharacterAppearanceAsync", "userId", Security::None);
static Reflection::BoundFuncDesc<Players, shared_ptr<Instance>(int)> func_getPlayerByUserId(&Players::getPlayerInstanceByID, "GetPlayerByUserId", "userId", Security::None);

Reflection::RemoteEventDesc<Players, void(int)> Players::event_requestCloudEditKick(
		&Players::requestCloudEditKick, "RequestCloudEditKick", "playerId",
		Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY,
		Reflection::RemoteEventCommon::CLIENT_SERVER);
Reflection::RemoteEventDesc<Players, void()> Players::event_requestCloudEditShutdown(
		&Players::requestCloudEditShutdown, "RequestCloudEditShutdown",
		Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY,
		Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

std::set<std::string> Players::goldenHashes;
boost::mutex Players::goldenHashesMutex;
bool Players::canKickBecauseRunningInRealGameServer = false;

MemHashConfigs Players::goldMemHashes;
boost::mutex Players::goldMemHashesMutex;

namespace {
	shared_ptr<RakNet::BitStream> buildBasicChatBitStream(
			const Instance* sender, const Instance* receiver, unsigned char chatType) {
		// serialize our data, changing the message to the filtered message
		shared_ptr<RakNet::BitStream> data(new RakNet::BitStream());
		*data << chatType;
		if (sender) {
			FASTLOGS(DFLog::WebChatFiltering, "senderGUID: %s", sender->getGuid().readableString());
			RBX::Guid::Data guid;
			sender->getGuid().extract(guid);
			*data << guid.scope;
			*data << guid.index;
		}
		if (receiver) {
			FASTLOGS(DFLog::WebChatFiltering, "senderGUID: %s", receiver->getGuid().readableString());
			RBX::Guid::Data guid;
			receiver->getGuid().extract(guid);
			*data << guid.scope;
			*data << guid.index;
		}
		return data;
	}
}

bool Players::isNetworkClient(Instance* instance)
{
	return Instance::fastDynamicCast<RBX::Network::Client>(instance) != NULL;
}
Players::Players()
	:rakPeer(NULL)
	,maxPlayers(12)
	,sysStatsUrl("")
	,players(Instances()) // Initialize to a non-NULL value
	,chatOption(CLASSIC_CHAT)
	,characterAutoSpawn(true)
	,testPlayerNameId(0)
	,testPlayerUserId(0)
{
	this->setName("Players");
}

template<class T> bool valueTableGet(shared_ptr<const Reflection::ValueTable> valueTable, const std::string& key, T* outValue)
{
    Reflection::ValueTable::const_iterator it = valueTable->find(key);
    if(it == valueTable->end() || !it->second.isType<T>()) {
        return false;
    }
        
    *outValue = it->second.cast<T>();
        
    return true;
}

void Players::getUserIdFromName(std::string userName, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		apiService->getAsync(format(FString::GetUserIdUrl.c_str(), userName.c_str()), true, RBX::PRIORITY_DEFAULT,
			boost::bind(&Players::onReceivedRawGetUserIdSuccess, weak_from(DataModel::get(this)), _1, resumeFunction, errorFunction), 
			boost::bind(&Players::onReceivedRawGetUserIdError, weak_from(DataModel::get(this)), _1, errorFunction) );
	}
}


void Players::onReceivedRawGetUserIdSuccess(weak_ptr<DataModel> weakDataModel, std::string response, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
	{
		shared_ptr<const Reflection::ValueTable> table;
		std::string errorMessage;

		if (LuaWebService::parseWebJSONResponseHelper(&response, NULL, table, errorMessage))
		{
			int id;
			if (!valueTableGet(table, "Id", &id)) 
			{
				errorFunction("Players:GetUserIdFromName() failed because the user does not exist");
			} 
			else 
			{
				resumeFunction(id);
			}
		}
		else
		{
			errorFunction(format("Players:GetUserIdFromName() failed because %s", errorMessage.c_str()));
		}
	}
	else 
	{
		errorFunction("Players:GetUserIdFromName() failed because of an unknown error.");
	}
}

void Players::onReceivedRawGetUserIdError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction)
{
	if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
	{
		if (!error.empty())
		{
			errorFunction(format("Players:GetUserIdFromName() failed because %s", error.c_str()));
		}
		else
		{
			errorFunction("Players:GetUserIdFromName() failed because of an unknown error.");
		}
	}
	else
	{
		errorFunction("Players:GetUserIdFromName() failed because of an unknown error.");
	}
}


void Players::getNameFromUserId(int userId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		apiService->getAsync(format(FString::GetUserNameUrl.c_str(), userId), true, RBX::PRIORITY_DEFAULT,
			boost::bind(&Players::onReceivedRawGetUserNameSuccess, weak_from(DataModel::get(this)), _1, resumeFunction, errorFunction), 
			boost::bind(&Players::onReceivedRawGetUserNameError, weak_from(DataModel::get(this)), _1, errorFunction) );
	}
}

void Players::getFriends(int userId, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
{
    std::string fetchUrl;
    ContentProvider* cp = DataModel::get(this)->find<ContentProvider>();
    if (cp) 
	{
        fetchUrl = format(FString::GetFriendsUrl.c_str(), cp->getApiBaseUrl().c_str(), userId);
		shared_ptr<Pages> pagination = Creatable<Instance>::create<FriendPages>(weak_from(DataModel::get(this)), fetchUrl);
		pagination->fetchNextChunk(boost::bind(resumeFunction, pagination), errorFunction);
	}
	else
	{
		errorFunction("Content Provider not found");
	}
}

void Players::setAppearanceParent(shared_ptr<Instance> model, weak_ptr<Instance> instance)
{	
	shared_ptr<Instance> i(instance.lock());
	if (!i)
		return;

	if (Instance::fastDynamicCast<RBX::DataModelMesh>(i.get()) || Instance::fastDynamicCast<RBX::Decal>(i.get()) || Instance::fastDynamicCast<RBX::CharacterAppearance>(i.get()))
	{
		i->setParent(model.get());
	}
	else if (dynamic_cast<IEquipable*>(i.get()))
	{
		if (!i->fastDynamicCast<Tool>())
		{
			i->setParent(model.get());
		}
	}
}

void Players::doLoadAppearance(AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances, std::string contentDescription, shared_ptr<ModelInstance> model
							 , shared_ptr<int> amountToLoad, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
{
	if (amountToLoad.get())
	{
		try
		{
			std::for_each(instances->begin(), instances->end(), boost::bind(&Players::setAppearanceParent, model, _1));
			*amountToLoad.get() = *amountToLoad.get() - 1;
			if (*amountToLoad.get() == 0)
			{
				amountToLoad.reset();
				resumeFunction(model);
			}
		}
		catch (RBX::base_exception& e)
		{
			amountToLoad.reset();
			errorFunction(format("Players:GetCharacterAppearanceAsync() Error loading character appearance %s: %s", contentDescription.c_str(), e.what()));
		}
	}
}

void Players::doMakeAccoutrementRequests(std::string response, weak_ptr<DataModel> dataModel, shared_ptr<ModelInstance> model
									   , boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
{
	std::vector<std::string> strings;
	boost::split(strings, response, boost::is_any_of(";"));
	shared_ptr<DataModel> d(dataModel.lock());
	shared_ptr<int> amountToLoad = shared_ptr<int>(new int);
	*amountToLoad.get() = (int)strings.size();

	if (d)
	{
		std::string placeIdStr;
		placeIdStr = format(kPlaceIdUrlParamFormat, d->getPlaceID());

		for (size_t i = 0; i < strings.size(); i++)
		{
			std::string contentIdString = strings[i];
			contentIdString += placeIdStr;

			ServiceProvider::create<ContentProvider>(d.get())->loadContent(
				ContentId(contentIdString), ContentProvider::PRIORITY_DEFAULT,
				boost::bind(&Players::doLoadAppearance, _1, _2, contentIdString, model, amountToLoad, resumeFunction, errorFunction), 
				AsyncHttpQueue::AsyncWrite
			);
		}	
	}
}

void Players::makeAccoutrementRequests(std::string *response, std::exception *err, weak_ptr<DataModel> dataModel, shared_ptr<ModelInstance> model,
									 boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
{
	if (!response) 
	{
		if (err)
		{
			errorFunction(format("Players:GetCharacterAppearanceAsync() Request exception: %s", err->what()));
		}
		else
		{
			errorFunction(format("Players:GetCharacterAppearanceAsync() Request exception"));
		}
		return;
	}

	shared_ptr<DataModel> d(dataModel.lock());
	if (d)
	{
		try
		{
			d->submitTask(boost::bind(&Players::doMakeAccoutrementRequests, *response, dataModel, model, resumeFunction, errorFunction), DataModelJob::Write);
		}
		catch (const RBX::base_exception&)
		{
			errorFunction("Players:GetCharacterAppearanceAsync() unexpected error");
		}
	}
}


void Players::getCharacterAppearance(int userId, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction)
{
	if (!DFFlag::GetCharacterAppearanceEnabled)
	{
		errorFunction("Players:GetCharacterAppearanceAsync() is not yet enabled!");
		return;
	}

	if (userId <= 0)
	{
		errorFunction("Players:GetCharacterAppearanceAsync() got negative userId");
		return;
	}
	std::string characterAppearance = format("%sAsset/CharacterFetch.ashx?userId=%d", ServiceProvider::create<ContentProvider>(this)->getBaseUrl().c_str(), userId);

	shared_ptr<ModelInstance> model = ModelInstance::createInstance();
	weak_ptr<DataModel> dm(shared_from(Instance::fastDynamicCast<DataModel>(this->getRootAncestor())));
	RBX::Http(characterAppearance).get(boost::bind(&makeAccoutrementRequests, _1, _2, boost::weak_ptr<DataModel>(dm), model, resumeFunction, errorFunction));	
}

void Players::onReceivedRawGetUserNameSuccess(weak_ptr<DataModel> weakDataModel, std::string response, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
	{
		shared_ptr<const Reflection::ValueTable> table;
		std::string errorMessage;

		if (LuaWebService::parseWebJSONResponseHelper(&response, NULL, table, errorMessage))
		{
			std::string userName;
			if (!valueTableGet(table, "Username", &userName)) 
			{
				errorFunction("Players:GetNameFromUserId() failed because the user does not exist");
			} else 
			{
				resumeFunction(userName);
			}
		}
		else
		{
			errorFunction(format("Players:GetNameFromUserId() failed because %s", errorMessage.c_str()));
		}
	}
	else
	{
		errorFunction("Players:GetNameFromUserId() failed because of an unknown error.");
	}
}

void Players::onReceivedRawGetUserNameError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction)
{
	if (shared_ptr<DataModel> sharedDataModel = weakDataModel.lock()) 
	{
		if (!error.empty())
		{
			errorFunction(format("Players:GetNameFromUserId() failed because %s", error.c_str()));
		}
		else
		{
			errorFunction("Players:GetNameFromUserId() failed because of an unknown error.");
		}
	}
	else
	{
		errorFunction("Players:GetNameFromUserId() failed because of an unknown error.");
	}
}

bool Players::clientIsPresent(const RBX::Instance* context, bool testInDatamodel)
{
	return Client::clientIsPresent(context, testInDatamodel);
}

bool Players::serverIsPresent(const RBX::Instance* context, bool testInDatamodel)
{
	return Server::serverIsPresent(context, testInDatamodel);
}

bool Players::getDistributedPhysicsEnabled()
{
	return NetworkSettings::singleton().distributedPhysicsEnabled;
}

bool Players::isCloudEdit(const RBX::Instance* context)
{
	if (Client* c = ServiceProvider::find<Client>(context))
	{
		return c->isCloudEdit();
	}
	else if (Server* s = ServiceProvider::find<Server>(context))
	{
		return s->isCloudEdit();
	}
	else
	{
		return false;
	}
}

bool Players::frontendProcessing(const RBX::Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
	RBXASSERT(!testInDatamodel || serviceProvider!=NULL);
	return serviceProvider && !serverIsPresent(context, testInDatamodel);
}

bool Players::backendProcessing(const RBX::Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
	RBXASSERT(!testInDatamodel || serviceProvider!=NULL);
	return serviceProvider && !clientIsPresent(context, testInDatamodel);
}

int Players::getPlayerCount(const RBX::Instance* context)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
	if(serviceProvider == NULL)
		return 0;
	Players* players = serviceProvider->find<Network::Players>();
	return players ? players->getNumPlayers() : 0;
}

RBX::SystemAddress Players::findLocalSimulatorAddress(const RBX::Instance* context)
{
	if (!getDistributedPhysicsEnabled()) {
		return NetworkOwner::Unassigned();
	}

	if (serverIsPresent(context, false)) {
		return NetworkOwner::Server();
	}

	return Client::findLocalSimulatorAddress(context);
}

int Players::getMaxPlayers() const
{
	return maxPlayers;
}

int Players::getPreferredPlayers() const
{
	return preferredPlayers;
}

void Players::setMaxPlayers(int value)
{
	if (value < 0)
		value = 1;
	if (value != maxPlayers)
	{
		maxPlayers = value;
		raisePropertyChanged(propPlayerMaxCount);
		raisePropertyChanged(propPlayerMaxCountInternal);
	}
}


void Players::setPreferredPlayers(int value)
{
	if (value < 0)
		value = 1;
	if (value != preferredPlayers)
	{
		preferredPlayers = value;
		raisePropertyChanged(propPlayerPreferredCount);
		raisePropertyChanged(propPlayerPreferredCountInternal);
	}
}

/*
									id == 0
	Local Player				yes				no
	Already Exists			
			YES					ignore			reset(id)

			NO					create(id)		create(id)
*/

// only for reflection
Instance* Players::getLocalPlayerDangerous() const
{
	return localPlayer.get();
}
shared_ptr<Instance> Players::createLocalPlayer(int userId, bool teleportedIn)
{
	if (localPlayer)
		throw std::runtime_error("Local player already exists");
		
	{
		RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);
		localPlayer = Creatable<Instance>::create<Player>();
	}

    if(RBX::DataModel* dataModel = (RBX::DataModel*)this->getParent())
        dataModel->getGuiBuilder().removeSafeChatMenu();

	localPlayer->setTeleportedIn(teleportedIn);

	{
		RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);
		localPlayer->setUserId(userId);
	}
	RBX::RbxDbgInfo::s_instance.PlayerID = userId;
	Player::prop_OsPlatform.setValue(localPlayer.get(), DebugSettings::singleton().osPlatform());
	
	localPlayer->setParent(this);
	raiseChanged(propLocalPlayer);

	shared_ptr<Instance> starterGear = Creatable<Instance>::create<StarterGear>();
	starterGear.get()->setParent(localPlayer.get());

	if (localPlayer->calculatesSpawnLocationEarly()) {
		if (backendProcessing(this) && frontendProcessing(this)) { // play solo mode
			localPlayer->doFirstSpawnLocationCalculation(ServiceProvider::findServiceProvider(this), "");
		}
	}

	//Visit GUI Fixes
	if (DFFlag::LoadGuisWithoutChar && Network::Players::frontendProcessing(this) && Network::Players::backendProcessing(this))
		RBX::DataModel::get(this)->setIsGameLoaded(true);

	return localPlayer;
}


void Players::resetLocalPlayer(){
	if(localPlayer)
	{
		localPlayer->unlockParent();
		localPlayer->destroy();
		localPlayer.reset();	
	}
}

bool Players::superSafeOn() const
{
	return (localPlayer) ? Player::prop_SuperSafeChat.getValue(localPlayer.get()) : false;
}


Players::~Players(void)
{
	setConnection(NULL);
}

static void writeMessage(const AbuseReport::Message& msg, XmlElement* messages)
{
	XmlElement* element = messages->addChild(RBX::Name::declare("message"));
	element->setValue(msg.text);
	element->addAttribute(RBX::Name::declare("userID"), msg.userID);
	element->addAttribute(RBX::Name::declare("guid"), msg.guid);
}

ChatMessage::ChatMessage(const char* message, ChatType chatType, shared_ptr<Player> source)
	:message(message)
	,chatType(chatType)
	,source(source)
{
	Guid::generateStandardGUID(guid);
}
ChatMessage::ChatMessage(const ChatMessage& other, const std::string& newMessage)
	:message(newMessage)
	,guid(other.guid)
	,chatType(other.chatType)
	,source(other.source)
	,destination(other.destination)
{}


ChatMessage::ChatMessage(const char* message, ChatType chatType, shared_ptr<Player> source, shared_ptr<Player> destination)
	:message(message)
	,chatType(chatType)
	,source(source)
	,destination(destination)
{
	Guid::generateStandardGUID(guid);
}

bool ChatMessage::isVisibleToPlayer(shared_ptr<Player> player) const
{
	return isVisibleToPlayer(player, source, destination, chatType);
}
bool ChatMessage::isVisibleToPlayer(shared_ptr<Player> const player, shared_ptr<Player> const source, shared_ptr<Player> const destination, const ChatMessage::ChatType chatType)
{
	switch(chatType)
	{
	case CHAT_TYPE_ALL:
	case CHAT_TYPE_GAME:
		return true;
	case CHAT_TYPE_TEAM:
		return (player && source && !source->getNeutral() && !player->getNeutral() && source->getTeamColor() == player->getTeamColor());
	case CHAT_TYPE_WHISPER:
		return (player && (source == player || destination == player));
	default:
		return false;
	}
}
std::string ChatMessage::getReportAbuseMessage() const
{
	switch(chatType)
	{
		case CHAT_TYPE_ALL:
			return message;
		case CHAT_TYPE_GAME:
			return "[[game]]" + message;
		case CHAT_TYPE_TEAM:
			return "[[team]]" + message;
		case CHAT_TYPE_WHISPER:
		{
			std::ostringstream str;
			str << "[[to ";
			if(destination){
				str << destination->getName();
			}
			else {
				str << "???";
			}
			str << "]]" << message;
			return str.str();
		}
		default:
			return message;
	}
}

void AbuseReport::addMessage(shared_ptr<Player> reportingPlayer, const ChatMessage& cm) 
{
	if(messages.size()>=(size_t)GameSettings::singleton().reportAbuseChatHistory)
		return;

	if(!reportingPlayer || cm.isVisibleToPlayer(reportingPlayer)) {
		int userID = cm.source ? cm.source->getUserID() : 0;
		Message m = { userID, cm.getReportAbuseMessage(), cm.guid };
		messages.push_back(m);
	}
}

AbuseReporter::AbuseReporter(std::string abuseUrl)
	:_data(new data())
{
	requestProcessor.reset(new worker_thread(boost::bind(&AbuseReporter::processRequests, _data, abuseUrl), "rbx_abusereporter"));
}


void AbuseReporter::add(AbuseReport& r, shared_ptr<Player> reportingPlayer, const std::list<ChatMessage>& chatHistory)
{
	// Copy all the messages from the chat history
	std::for_each(
		chatHistory.begin(), 
		chatHistory.end(),
		boost::bind(&AbuseReport::addMessage, boost::ref(r), reportingPlayer, _1)
	);
	
	// Push a new request into the queue
	{
		boost::mutex::scoped_lock lock(_data->requestSync);
		_data->queue.push(r);
	}
	requestProcessor->wake();
}


worker_thread::work_result AbuseReporter::processRequests(shared_ptr<data> _data, std::string abuseUrl)
{
	try
	{
		AbuseReport r;
		
		{
			boost::mutex::scoped_lock lock(_data->requestSync);
			if (_data->queue.empty())
				return worker_thread::done;
			r = _data->queue.front();
		}

		std::stringstream body;
		{
			TextXmlWriter writer(body);
			XmlElement root(RBX::Name::declare("report"));
			root.addAttribute(RBX::Name::declare("userID"), r.submitterID);
			root.addAttribute(RBX::Name::declare("placeID"), r.placeID);
			root.addAttribute(RBX::Name::declare("gameJobID"), r.gameJobID);

			root.addChild(new XmlElement(RBX::Name::declare("comment"), r.comment));
			XmlElement* messages = root.addChild(new XmlElement(RBX::Name::declare("messages")));
			std::for_each(r.messages.begin(), r.messages.end(), boost::bind(&writeMessage, _1, messages));
			writer.serialize(&root);

			body.flush();
		}

		std::string response;

		Http(abuseUrl).post(body, Http::kContentTypeDefaultUnspecified, false, response);
		StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Posted abuse report to %s", abuseUrl.c_str());
		StandardOut::singleton()->print(MESSAGE_INFO, "Posted abuse report");
	}
	catch (RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Unable to post abuse report to %s. %s", abuseUrl.c_str(), e.what());
		StandardOut::singleton()->printf(MESSAGE_WARNING, "Unable to post abuse report. %s", e.what());
	}

	{
		boost::mutex::scoped_lock lock(_data->requestSync);
		_data->queue.pop();
	}
	return worker_thread::more;
}


bool Players::canReportAbuse() const
{
	return (abuseReporter || clientIsPresent(this));
}

void Players::reportAbuse(Player* player, const std::string& comment)
{
	if (abuseReporter)
	{
		if (player)
		{
			StandardOut::singleton()->printf(MESSAGE_INFO, "Submitting abuse report on %s", player ? player->getName().c_str() : "[[[anonymous]]]");
		}
		else
		{
			StandardOut::singleton()->printf(MESSAGE_INFO, "Submitting abuse report on game");
		}

		AbuseReport r;
		r.submitterID = localPlayer ? localPlayer->getUserID() : 0;
		r.allegedAbuserID = player ? player->getUserID() : 0;
		r.comment = comment;

		if(RBX::DataModel* dataModel = RBX::DataModel::get(this))
		{
			r.placeID = dataModel->getPlaceID();
			r.gameJobID = dataModel->getJobId();
		}


		abuseReporter->add(r, localPlayer, chatHistory);
	}
	else
	{
		// Send the abuse report to somebody else for processing
		StandardOut::singleton()->print(MESSAGE_INFO, "Sending abuse report to game server via Raknet");

		if (!rakPeer)
			throw std::runtime_error("Can't report abuse: Not in a networked game");

		shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());

		*bitStream << (unsigned char) ID_REPORT_ABUSE;

		// Write the alleged abuser (or null)
		*bitStream << (player ? player->getUserID() : (int)0);

		// Write the sender's comment
		*bitStream << comment;

		// Send ID_REPORT_ABUSE (this packet is received by the Server)
		rakPeer->Send(bitStream, HIGH_PRIORITY, RELIABLE, CHAT_CHANNEL, UNASSIGNED_SYSTEM_ADDRESS, true);
	}
}


void Players::reportAbuseLua(shared_ptr<Instance> instance, std::string reason, std::string comment)
{
	shared_ptr<Player> player;
	if (instance)
	{
		player = Instance::fastSharedDynamicCast<Player>(instance);
		if(!player)
			throw RBX::runtime_error("player must be a Player object or null");
	}

	if(!localPlayer)
		throw RBX::runtime_error("You can only report-abuse from a client machine");

	if(localPlayer->getUserID() > 0)
	{
		std::ostringstream strstream;
		strstream << reason << ";" << comment;
		reportAbuse(player ? player.get() : NULL, strstream.str());
	}
}

void Players::gamechat(const std::string& message)
{
	if (localPlayer)
		throw std::runtime_error("Only servers can produce game chat");

	if (!rakPeer)
		return;

	shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
	*bitStream << (unsigned char) ID_CHAT_GAME;
	*bitStream << message;

	rakPeer->Send(bitStream, CHAT_PRIORITY, CHAT_RELIABILITY, CHAT_CHANNEL, UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Players::checkChat(const std::string& message)
{
	if (!localPlayer)
		throw std::runtime_error("No local Player to chat from");

	if (superSafeOn())
	{
		if (message.find("/sc ") != 0)
			throw std::runtime_error("SuperSafe chat is on");
	}
}

static RBX::Guid::Data readPlayerIdentifier(RakNet::BitStream &inBitStream)
{
	RBX::Guid::Data id;
	inBitStream >> id.scope;
	inBitStream >> id.index;
	return id;
}

static void writePlayerIdentifier(RakNet::BitStream& bitStream, const boost::intrusive_ptr<GuidItem<Instance>::Registry>& guidRegistery, shared_ptr<Player> player)
{
	// Write the sender
	guidRegistery->registerGuid(player.get());
	RBX::Guid::Data id;
	player->getGuid().extract(id);

	// This is very inefficient without a dictionary, but we can't
	// use the dictionary here - these packets go out-of-order
	bitStream << id.scope;
	bitStream << id.index;
}

void Players::whisperChat(std::string message, shared_ptr<Instance> instance)
{
	checkChat(message);

	shared_ptr<Player> player = Instance::fastSharedDynamicCast<Player>(instance);
	if(!player || player->getParent() != this)
		throw std::runtime_error("Player object is not a player to chat to");

	
	shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
	*bitStream << (unsigned char) ID_CHAT_PLAYER;

	// Write the sender
	writePlayerIdentifier(*bitStream, getGuidRegistry(), localPlayer);

	// Write the receiver
	writePlayerIdentifier(*bitStream, getGuidRegistry(), player);

	// Write the text
	*bitStream << message;

	// Send ID_CHAT_PLAYER
	rakPeer->Send(bitStream, CHAT_PRIORITY, CHAT_RELIABILITY, CHAT_CHANNEL, UNASSIGNED_SYSTEM_ADDRESS, true);

	ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_WHISPER, localPlayer, player);
	raiseChatMessageSignal(event);
}

void Players::teamChat(std::string message)
{
	checkChat(message);

	shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
	*bitStream << (unsigned char) ID_CHAT_TEAM;

	// Write the sender
	writePlayerIdentifier(*bitStream, getGuidRegistry(), localPlayer);

	// Write the text
	*bitStream << message;
	
	// Send ID_CHAT_TEAM
	rakPeer->Send(bitStream, CHAT_PRIORITY, CHAT_RELIABILITY, CHAT_CHANNEL, UNASSIGNED_SYSTEM_ADDRESS, true);

	ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_TEAM, localPlayer);
	raiseChatMessageSignal(event);
}

void Players::chat(std::string message)
{
	checkChat(message);

	if (rakPeer)
	{
		shared_ptr<RakNet::BitStream> bitStream(new RakNet::BitStream());
		*bitStream << (unsigned char) ID_CHAT_ALL;

		// Write the sender
		writePlayerIdentifier(*bitStream, getGuidRegistry(), localPlayer);

		// Write the text
		*bitStream << message;

		// Send ID_CHAT_ALL
		rakPeer->Send(bitStream, CHAT_PRIORITY, CHAT_RELIABILITY, CHAT_CHANNEL, UNASSIGNED_SYSTEM_ADDRESS, true);
	}

	ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_ALL, localPlayer);
	raiseChatMessageSignal(event);
}

// returns true if we were able to parse an intelligible command
static bool ParseChatCommand(const ChatMessage& msg, bool isSelf, std::string* parsed_msg) 	
{
	if (msg.message.size() <= 0 || msg.message[0] != '/')
		return false;

	std::vector<std::string> v;
	
	boost::char_separator<char> sep(" -_/,");
	boost::tokenizer<boost::char_separator<char> >  tokens(msg.message, sep);
	for (boost::tokenizer<boost::char_separator<char> > ::iterator tok_iter = tokens.begin();
		tok_iter != tokens.end(); ++tok_iter)
	{
		v.push_back(*tok_iter);
	}
	
	if (v.size()==0)
		return false;

	// Now have tokenized version of command
	// lowercase the actual command
	safeToLower(v[0]);
	
	// SafeChat Speak
	if (v[0] == "sc") {
		if (parsed_msg)
			*parsed_msg = SafeChat::singleton().getMessage(v);
		return true;
	}

	return false;
}
void Players::raisePlayerChattedSignal(const ChatMessage& message)
{
	if(message.chatType == ChatMessage::CHAT_TYPE_ALL)
	{
		if(message.source)
			message.source->chattedSignal(message.message, message.destination);
	}

	if(localPlayer)
	{
		switch(message.chatType)
		{
		case ChatMessage::CHAT_TYPE_ALL:
			playerChattedSignal(PLAYER_CHAT_TYPE_ALL, message.source, message.message, shared_ptr<Instance>());
			break;
		case ChatMessage::CHAT_TYPE_TEAM:
			playerChattedSignal(PLAYER_CHAT_TYPE_TEAM, message.source, message.message, shared_ptr<Instance>());
			break;
		case ChatMessage::CHAT_TYPE_WHISPER:
			playerChattedSignal(PLAYER_CHAT_TYPE_WHISPER, message.source, message.message, message.destination);
			break;
		case ChatMessage::CHAT_TYPE_GAME:
			gameAnnounceSignal(message.message);
			break;
		}
	}
}
void Players::raiseChatMessageSignal(const ChatMessage& message)
{
	bool fromOthers = (localPlayer == NULL) || (message.source != localPlayer);

	std::string msg;
	if (ParseChatCommand(message, !fromOthers, &msg))
	{
		ChatMessage parsedMessage(message, msg);
		chatMessageSignal(parsedMessage);

		raisePlayerChattedSignal(parsedMessage);
	}
	else
	{
		// Check if we are using super safe chat - if so, forget about it
		if (localPlayer && localPlayer->getSuperSafeChat()) 
			return;

		chatMessageSignal(message);
		raisePlayerChattedSignal(message);
	}
}

void Players::addChatMessage(const ChatMessage& message)
{
	chatHistory.push_front(message);
	while (chatHistory.size()>(size_t)GameSettings::singleton().chatHistory)
		chatHistory.pop_back();

	raiseChatMessageSignal(message);
}

const boost::intrusive_ptr<GuidItem<Instance>::Registry>& Players::getGuidRegistry()
{
	if (!guidRegistry)
		guidRegistry = ServiceProvider::create<GuidRegistryService>(this)->registry;
	return guidRegistry;
}

Players::ReceiveResult Players::OnReceiveChat(Player* sourceValidation, RakNet::RakPeerInterface *peer, RakNet::Packet *packet, unsigned char chatType)
{
	RakNet::BitStream inBitstream(packet->data, packet->length, false);

	inBitstream.IgnoreBits(8); // Ignore the packet id

	RBX::Guid::Data sourceGuid;
	RBX::Guid::Data destinationGuid;
	shared_ptr<Instance> sourceInstance;
	shared_ptr<Instance> destinationInstance;
	if(chatType != ID_CHAT_GAME){
		sourceGuid = readPlayerIdentifier(inBitstream);
		sourceInstance = getGuidRegistry()->getByGuid(sourceGuid);
		if (sourceValidation)
		{
			if (sourceInstance.get() != sourceValidation)
			{
				// bad data. Hacker?
				return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;
			}
		}
	}
	if(chatType == ID_CHAT_PLAYER){
		destinationGuid = readPlayerIdentifier(inBitstream);
		destinationInstance = getGuidRegistry()->getByGuid(destinationGuid);
		if (DFFlag::FilterInvalidWhisper && !destinationInstance.get())
		{
			// bad data. Hacker?
			return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;
		}
	}

	// Read message
	std::string message;
	inBitstream >> message;

	if (!sourceInstance)
	{
		if(chatType == ID_CHAT_GAME){
			ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_GAME, shared_from<Player>(NULL));
			addChatMessage(event);
		}
		else{
			StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Chat message from unknown Player %s %s", RakNetAddressToString(packet->systemAddress).c_str(), message.c_str());
		}
	}
	else if (sourceInstance->getParent()==this)		// confirm the player hasn't been bumped
	{
		shared_ptr<Player> sourcePlayer = shared_polymorphic_downcast<Player>(sourceInstance);

		const bool kIsBackendProcessing = backendProcessing(this);

		// Check on the client (game client, not game server) to see if we should display the chat based on whether we're in the right team
		// NOTE:  Everyone RECEIVES all chat!
		switch(chatType)
		{
		case ID_CHAT_ALL:
			{
				ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_ALL, sourcePlayer);
				if(kIsBackendProcessing || event.isVisibleToPlayer(localPlayer))
					addChatMessage(event);
				break;
			}
		case ID_CHAT_TEAM:
			{
				ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_TEAM, sourcePlayer);
				if(kIsBackendProcessing || event.isVisibleToPlayer(localPlayer))
					addChatMessage(event);
				break;
			}
		case ID_CHAT_PLAYER:
			{
                shared_ptr<Player> destinationPlayer;
				if (DFFlag::FilterInvalidWhisper)
                {
				    destinationPlayer = shared_dynamic_cast<Player>(destinationInstance);
				    if (!destinationPlayer.get())
				    {
				    	// bad data. Hacker?
				    	 return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;
				    }
                }
                else
                {
                    destinationPlayer = shared_polymorphic_downcast<Player>(destinationInstance);
                }
				ChatMessage event(message.c_str(), ChatMessage::CHAT_TYPE_WHISPER, sourcePlayer, destinationPlayer);
				if(kIsBackendProcessing || event.isVisibleToPlayer(localPlayer))
					addChatMessage(event);
				break;
			}
		}

		// Note: This only happens on the game server
		// Check the message through the content filter for abusive content
		if (kIsBackendProcessing)
		{
			if (0 == message.length())
				return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;

			// This is our switch to allow us to turn off content filtering and check it against the local Diogenes (player's client)
			// TODO:  The local diogenes code is ugly, we send messages to everyone and then check to see if it's not in diogenes before displaying.
			//		  I'd like to change this to the game server to avoid player's hacking their diogenes file and seeing bad text, but
			//		  the player who actually sends the message needs to be displayed it regardless (so they dont know we're filtering), and this
			//		  is done when he receives the message.  So I would have to have him display it on his screen manually instead of on receive.
			//		  But then I'd have to check to see if he's received his own message to make sure he doesn't see his msgs twice... - Tim
			if (chatFilterUrl == "")
			{
				// Forward message to other peers
				peer->Send((const char*)packet->data, packet->length, CHAT_PRIORITY, CHAT_RELIABILITY, CHAT_CHANNEL, packet->systemAddress, true);
				return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;
			}

            ChatFilter::FilteredChatMessageCallback callback = boost::bind(
                &Players::sendFilteredChatMessageSignalHelper, shared_from(this),
                packet->systemAddress,
                buildBasicChatBitStream(sourceInstance.get(), destinationInstance.get(), chatType),
				sourceInstance,
                shared_ptr<ChatMessage>(),
                _1);

            ChatFilter* chatFilter = ServiceProvider::create<WebChatFilter>(this);
            chatFilter->filterMessage(sourcePlayer, destinationInstance, message, callback);

            return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;
		}
	}
	return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;
}

void Players::sendFilteredChatMessageSignalHelper(const RakNet::SystemAddress& systemAddress,
		const shared_ptr<RakNet::BitStream>& baseData, const shared_ptr<Instance> sourceInstance, shared_ptr<ChatMessage> chatEvent, const ChatFilter::Result& result) {
	DataModel::LegacyLock l(DataModel::get(this), DataModelJob::Write);

	sendFilteredChatMessageSignal(systemAddress, baseData, sourceInstance,
		result.whitelistFilteredMessage, result.blacklistFilteredMessage);
}

Players::ReceiveResult Players::OnReceiveReportAbuse(Player* player, RakNet::RakPeerInterface *peer, RakNet::Packet *packet)
{
	// Read all the info
	RakNet::BitStream inBitstream(packet->data, packet->length, false);

	inBitstream.IgnoreBits(8); // Ignore the packet id

	AbuseReport r;

	r.submitterID = player ? player->getUserID() : 0;
	inBitstream >> r.allegedAbuserID;
	inBitstream >> r.comment;

	if(RBX::DataModel* dataModel = RBX::DataModel::get(this))
	{
		r.placeID = dataModel->getPlaceID();
		r.gameJobID = dataModel->getJobId();
	}

	{
		std::ostringstream str;
		str << "AbuserID:" << r.allegedAbuserID << ";" << r.comment;
		r.comment = str.str();
	}

	shared_ptr<Player> reportingPlayer = shared_from(player);

	if (abuseReporter)
		abuseReporter->add(r, reportingPlayer, chatHistory);

	abuseReportedReceived(r);

	return Players::PLAYERS_STOP_PROCESSING_AND_DEALLOCATE;	// We handled the message. Nobody else needs to
}
				
void Players::setAbuseReportUrl(std::string value)
{
	if (value.empty())
		abuseReporter.reset();
	else
		abuseReporter.reset(new AbuseReporter(value));
}

void Players::setChatFilterUrl(std::string value)
{
	chatFilterUrl = value;
}
std::string Players::getLoadDataUrl(int userId) const
{
	if(loadDataUrl.empty())
		throw std::runtime_error("No LoadData url set");
	return RBX::format(loadDataUrl.c_str(), userId);
}
void Players::setLoadDataUrl(std::string value)
{
	loadDataUrl = value;
}

std::string Players::getSaveDataUrl(int userId) const
{
	if(saveDataUrl.empty())
		throw std::runtime_error("No SaveData url set");
	return RBX::format(saveDataUrl.c_str(), userId);
}
void Players::setSaveDataUrl(std::string value)
{
	saveDataUrl = value;
}
std::string Players::getSaveLeaderboardDataUrl(int userId) const
{
	if(saveLeaderboardDataUrl.empty())
		throw std::runtime_error("No SaveLeaderboardData url set");
	return RBX::format(saveLeaderboardDataUrl.c_str(), userId);	
}
void Players::setSaveLeaderboardDataUrl(std::string value)
{
	saveLeaderboardDataUrl = value;
}

void Players::addLeaderboardKey(std::string key)
{
	if(!hasLeaderboardKey(key))
		leaderboardKeys.insert(key);
}
bool Players::hasLeaderboardKey(const std::string& key) const
{
	return leaderboardKeys.find(key) != leaderboardKeys.end();
}
boost::unordered_set<std::string>::const_iterator Players::beginLeaderboardKey() const
{
	return leaderboardKeys.begin();
}
boost::unordered_set<std::string>::const_iterator Players::endLeaderboardKey() const
{
	return leaderboardKeys.end();
}

void Players::setSysHash(std::string hash)
{
	//Check for new Mac hash embedded into the goldenHash
	size_t pos = hash.find(":");
	if(pos != std::string::npos){
		//The mac client has a different hash
		goldenHash = hash.substr(0, pos);
		goldenHash2 = hash.substr(pos + 1);
	}
	else{
		goldenHash2 = hash;
		goldenHash = hash;
	}

	FASTLOGS(FLog::GoldenHashes, "Hashses set through script, Windows hash: %s", goldenHash);
	FASTLOGS(FLog::GoldenHashes, "Mac hash: %s", goldenHash2);
}

void Players::setSysStatsUrl(std::string value)
{
	sysStatsUrl = value;
}

void Players::setGoldenHashes(const std::string& windowsHash, const std::string& macHash, const std::string& windowsPlayerBetaHash)
{
	if(windowsHash.length() > 0)
		goldenHash = windowsHash;

	if(macHash.length() > 0)
		goldenHash2 = macHash;

	if (windowsPlayerBetaHash.length() > 0)
		goldenHash3 = windowsPlayerBetaHash;

	if(goldenHash.size() > 0 || goldenHash2.size() > 0 || goldenHash3.size() > 0)
		canKickBecauseRunningInRealGameServer = true;

	FASTLOG1(FLog::GoldenHashes, "Golden hashes set, should kick: %u", canKickBecauseRunningInRealGameServer);
	FASTLOGS(FLog::GoldenHashes, "Windows hash: %s", goldenHash);
	FASTLOGS(FLog::GoldenHashes, "Mac hash: %s", goldenHash2);
	FASTLOGS(FLog::GoldenHashes, "WP Beta: %s ", goldenHash3);
}

void Players::setGoldenHashes2(const std::set<std::string>& hashes)
{
	boost::mutex::scoped_lock lock(goldenHashesMutex);
	goldenHashes = hashes;

	if (goldenHashes.size() > 0)
		canKickBecauseRunningInRealGameServer = true;

	FASTLOG1(FLog::GoldenHashes, "Golden hashes set, should kick: %u", canKickBecauseRunningInRealGameServer);

	for (std::set<std::string>::iterator i = goldenHashes.begin(); i != goldenHashes.end(); i++)
	{
		FASTLOGS(FLog::GoldenHashes, "hash: %s", *i);
	}
}

void Players::setGoldMemHashes(const MemHashConfigs& hashes)
{
	boost::mutex::scoped_lock lock(goldMemHashesMutex);
	goldMemHashes = hashes;
}

void Players::setBuildUserPermissionsUrl(std::string value)
{
	buildUserPermissionsUrl = value;
}

bool Players::hasBuildUserPermissionsUrl() const
{
	return !buildUserPermissionsUrl.empty();
}

std::string Players::getBuildUserPermissionsUrl(int playerId) const
{
	return format(buildUserPermissionsUrl.c_str(), playerId);
}


void Players::friendEventFired(int userId, int otherUserId, FriendService::FriendEventType friendEvent)
{
	//Frontend and backend
	shared_ptr<Player> playerA = getPlayerByID(userId);
	shared_ptr<Player> playerB = getPlayerByID(otherUserId);
	if(playerA && playerB){
		friendRequestEvent(playerA, playerB, friendEvent);
	}
}
void Players::friendStatusChanged(int userId, int otherUserId, FriendService::FriendStatus friendStatus)
{
	shared_ptr<Player> playerA = getPlayerByID(userId);
	shared_ptr<Player> playerB = getPlayerByID(otherUserId);
	if(playerA && playerB)
	{
		playerA->onFriendStatusChanged(playerB, friendStatus);
	}
}
void Players::friendServiceRequest(bool makeFriends, weak_ptr<Player> sourcePlayer, int otherUserId)
{
	shared_ptr<Player> playerA = sourcePlayer.lock();
	shared_ptr<Player> playerB = getPlayerByID(otherUserId);
	if(!playerA || !playerB)
		return;
	
	if(RBX::FriendService* fs = ServiceProvider::find<FriendService>(this)){
		if(!makeFriends){
			fs->rejectFriendRequestOrBreakFriendship(playerA->getUserID(), playerB->getUserID());	
		}
		else{
			fs->issueFriendRequestOrMakeFriendship(playerA->getUserID(), playerB->getUserID());
		}
	}
}

void Players::setCharacterAutoSpawnProperty(bool value)
{
	if(value != characterAutoSpawn)
	{
		characterAutoSpawn = value;
		raisePropertyChanged(propCharacterAutoSpawn);
	}
}

bool Players::getShouldAutoSpawnCharacter() const
{
	return !isCloudEdit(this) && characterAutoSpawn;
}

void Players::loadLocalPlayerGuis()
{
	//Fixes issues with characterAutoSpawn not giving guis.
	//If characterAutoSpawn is false, and we're Filtering or Visit mode
	bool isVisit = (Network::Players::frontendProcessing(this) && Network::Players::backendProcessing(this));
	bool isFiltering = ServiceProvider::find<Workspace>(this)->getNetworkFilteringEnabled();
	if ((isVisit || isFiltering) && !characterAutoSpawn)
	{
		localPlayer->rebuildGui();
	}
}

void Players::gotBlockUserSuccess(std::string response, bool blockUser, int blockerUserId, int blockeeUserId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	std::string methodName = (blockUser ? "Players:BlockUser" : "Players:UnblockUser");
	if (!response.empty())
	{
		shared_ptr<const Reflection::ValueTable> jsonResult(rbx::make_shared<const Reflection::ValueTable>());
		bool parseResult = WebParser::parseJSONTable(response, jsonResult);

		if (!parseResult)
		{
			std::string errorString = methodName + " could not parse JSON response from web.";
			if (!errorFunction.empty())
			{
				errorFunction(errorString);
			}
			else
			{
				event_serverFinishedBlockUser.fireAndReplicateEvent(this, blockerUserId, blockeeUserId, errorString);
			}
			return;
		}

		Reflection::Variant successVar = jsonResult->at("success");

		bool success = successVar.get<bool>();
		if (!resumeFunction.empty())
		{
			if (success)
			{
				resumeFunction("");
			}
			else
			{
				errorFunction(methodName + " response indicating block request failed");
			}
			return;
		}
		else
		{
			if (success)
			{
				event_serverFinishedBlockUser.fireAndReplicateEvent(this, blockerUserId, blockeeUserId, "");
			}
			else
			{
				event_serverFinishedBlockUser.fireAndReplicateEvent(this, blockerUserId, blockeeUserId, methodName + " response indicating block request failed");
			}
		}
	}
	else
	{
		std::string errorString = methodName + " got empty response from web.";
		if (!errorFunction.empty())
		{
			errorFunction(errorString);
		}
		else
		{
			event_serverFinishedBlockUser.fireAndReplicateEvent(this, blockerUserId, blockeeUserId, errorString);
		}
	}
}

void Players::gotBlockUserError(std::string error, bool blockUser, int blockerUserId, int blockeeUserId, boost::function<void(std::string)> errorFunction)
{
	const std::string methodName = (blockUser ? "Players:BlockUser" : "Players:UnblockUser");
	const std::string errorString = methodName + " failed because " + error;

	if (!errorFunction.empty())
	{
		errorFunction(errorString);
	}
	else
	{
		event_serverFinishedBlockUser.fireAndReplicateEvent(this, blockerUserId, blockeeUserId, errorString);
	}
}

void Players::clientReceiveBlockUserFinished(int blockerUserId, int blockeeUserId, std::string errorString)
{
	std::pair<int,int> blockerPair(blockerUserId, blockeeUserId);

	if ( clientBlockUserMap.find(blockerPair) != clientBlockUserMap.end() )
	{
		if (errorString.empty())
		{
			clientBlockUserMap[blockerPair].first(errorString);
		}
		else
		{
			clientBlockUserMap[blockerPair].second(errorString);
		}

		clientBlockUserMap.erase(blockerPair);
		return;
	}

	std::pair<int,int> reverseBlockerPair(blockeeUserId, blockerUserId);

	if ( clientBlockUserMap.find(reverseBlockerPair) != clientBlockUserMap.end() )
	{
		if (errorString.empty())
		{
			clientBlockUserMap[blockerPair].first(errorString);
		}
		else
		{
			clientBlockUserMap[blockerPair].second(errorString);
		}
		clientBlockUserMap.erase(reverseBlockerPair);
		return;
	}
}

void Players::serverMakeBlockUserRequest(bool blockUser, int blockerUserId, int blockeeUserId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if(RBX::HttpRbxApiService* apiService = RBX::ServiceProvider::find<RBX::HttpRbxApiService>(this))
	{
		std::stringstream params;
		params << "blockerId=" << blockerUserId << "&blockeeId=" << blockeeUserId;

		std::string apiPath = blockUser ? "userblock/blockuser" : "userblock/unblockuser";

		apiService->postAsync(apiPath, params.str(), true, RBX::PRIORITY_DEFAULT, RBX::HttpService::APPLICATION_URLENCODED,
			boost::bind(&Players::gotBlockUserSuccess, this, _1, blockUser, blockerUserId, blockeeUserId, resumeFunction, errorFunction),
			boost::bind(&Players::gotBlockUserError, this, _1, blockUser, blockerUserId, blockeeUserId, errorFunction) );
	}
	else
	{
		errorFunction("Players:BlockUser could not find ApiService.");
		return;
	}
}

void Players::internalBlockUser(int blockerUserId, int blockeeUserId, bool isBlocking, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	if (blockerUserId <= 0 || blockeeUserId <= 0)
	{
		errorFunction("Players:BlockUser or Player:UnblockUser failed because the blocker and/or blockee userId is <= 0");
		return;
	}

	if (clientIsPresent(this))
	{
		std::pair<int,int> blockerPair(blockerUserId, blockeeUserId);
		std::pair<int,int> reverseBlockerPair(blockeeUserId, blockerUserId);

		if ( clientBlockUserMap.find(blockerPair) != clientBlockUserMap.end() ||
			clientBlockUserMap.find(reverseBlockerPair) != clientBlockUserMap.end())
		{
			errorFunction( RBX::format("Currently processing block request for userIds %i and %i, please wait until these requests return a value before submitting a new request.", blockerUserId, blockeeUserId) );
			return;
		}

		clientBlockUserMap[blockerPair] = std::pair< boost::function<void(std::string)>, boost::function<void(std::string)> >(resumeFunction, errorFunction);
		event_blockUserFromClient.fireAndReplicateEvent(this, blockerUserId, blockeeUserId, isBlocking);
	}
	else
	{
		serverMakeBlockUserRequest(isBlocking, blockerUserId, blockeeUserId, resumeFunction, errorFunction);
	}
}

void Players::blockUser(int blockerUserId, int blockeeUserId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	internalBlockUser(blockerUserId, blockeeUserId, true, resumeFunction, errorFunction);
}

void Players::unblockUser(int blockerUserId, int blockeeUserId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	internalBlockUser(blockerUserId, blockeeUserId, false, resumeFunction, errorFunction);
}

void Players::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		blockUserClientSignalConnection.disconnect();
		blockUserFinishedFromServerConnection.disconnect();
		if (DFFlag::LoadGuisWithoutChar)
			loadLocalPlayerGuisConnection.disconnect();
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		if (backendProcessing(newProvider))
		{
			blockUserClientSignalConnection = blockUserRequestFromClientSignal.connect(boost::bind(&Players::serverMakeBlockUserRequest, this, _3, _1, _2, boost::function<void(std::string)>(), boost::function<void(std::string)>()));
		}

		if (frontendProcessing(newProvider))
		{
			blockUserFinishedFromServerConnection = blockUserFinishedFromServerSignal.connect(boost::bind(&Players::clientReceiveBlockUserFinished, this, _1, _2, _3));
			if (DFFlag::LoadGuisWithoutChar)
			{
				if (ServiceProvider::create<ReplicatedFirst>(newProvider))
					loadLocalPlayerGuisConnection = RBX::DataModel::get(newProvider)->gameLoadedSignal.connect(boost::bind(&Players::loadLocalPlayerGuis, this));
			}
		}
	}
}

void Players::setConnection(ConcurrentRakPeer* rakPeer)
{
	this->rakPeer = rakPeer;
}

bool Players::askAddChild(const Instance* instance) const
{
	return Instance::fastDynamicCast<Player>(instance)!=NULL;
}

ModelInstance* Players::findLocalCharacter(RBX::Instance* context)
{
	if (Player* player = findLocalPlayer(context)) {
		return player->getCharacter();
	}
	else {
		return NULL;
	}
} 



const ModelInstance* Players::findConstLocalCharacter(const RBX::Instance* context)
{
	if (const Player* player = findConstLocalPlayer(context)) {
		return player->getConstCharacter();
	}
	else {
		return NULL;
	}
}

Player* Players::findLocalPlayer(Instance* context)
{
	if (Players* players = ServiceProvider::find<Players>(context))	{
		return players->getLocalPlayer();
	}
	else {
		return NULL;
	}
} 

const Player* Players::findConstLocalPlayer(const Instance* context)
{
	if (const Players* players = ServiceProvider::find<Players>(context))	{
		return players->getConstLocalPlayer();
	}
	else {
		return NULL;
	}
} 

shared_ptr<Player> Players::findAncestorPlayer(const RBX::Instance* descendent)
{
	if (Players* players = ServiceProvider::find<Players>(descendent))	{
		//First check is the descendent is from one of our Players
		//Must use dynamic_cast since non-Players can be added to the Players object
		Player* player = Instance::fastDynamicCast<Player>(players->findFirstAncestorOf(descendent).get());
		if(player) return shared_from<Player>(player);

		//Then check if this is an ancestor of one of our Players "Character" object (the ModelInstance in the Workspace)
		shared_ptr<const Instances> playersVector = players->getPlayers();
		for(Instances::const_iterator iter = playersVector->begin(); iter != playersVector->end(); ++iter){
			if((player = boost::polymorphic_downcast<Player*>(iter->get()))){
				if(player->getCharacter()->isAncestorOf(descendent)){
					return shared_from<Player>(player);
				}
			}
		}
	}
	return shared_ptr<Player>();
}

shared_ptr<Player> Players::findPlayerWithAddress(const RBX::SystemAddress& playerAddress, const RBX::Instance* context)
{
    if (Players* players = ServiceProvider::find<Players>(context))	
	{
        //First check is the descendent is from one of our Players
        //Must use dynamic_cast since non-Players can be added to the Players object
        
        shared_ptr<const Instances> playersVector = players->getPlayers();
        
        for(Instances::const_iterator iter = playersVector->begin(); iter != playersVector->end(); ++iter)
		{
            if(shared_ptr<Player> player = Instance::fastSharedDynamicCast<Player>(*iter))
			{
                if( player->getRemoteAddressAsRbxAddress() == playerAddress )
				{
                    return player;
                }
            }
        }
    }
    return shared_ptr<Player>();
}

shared_ptr<Player> Players::getPlayerByID(int userID)
{
	Instances::const_iterator iter = players->begin();
	Instances::const_iterator end = players->end();
	for (; iter!=end; ++iter)
	{
		shared_ptr<Player> player = shared_polymorphic_downcast<Player>(*iter);
		if (player->getUserID() == userID)
			return player;
	}

	return shared_ptr<Player>();
}

shared_ptr<Instance> Players::getPlayerInstanceByID(int userID)
{
	return getPlayerByID(userID);
}



shared_ptr<Instance> Players::playerFromCharacter(shared_ptr<Instance> character)
{
    if (!character)
        return shared_ptr<Instance>();

	shared_ptr<const Instances> myPlayers = getPlayers();

	Instances::const_iterator iter = myPlayers->begin();
	Instances::const_iterator end = myPlayers->end();
	for (; iter!=end; ++iter)
	{
		Player* player = boost::polymorphic_downcast<Player*>(iter->get());
		if (player->getCharacter() == character.get())
			return *iter;
	}
	return shared_ptr<Instance>();
}

Player* Players::getPlayerFromCharacter(RBX::Instance* character)
{
	if (Players* players = ServiceProvider::find<Network::Players>(character))
	{
		shared_ptr<Instance> player = players->playerFromCharacter(shared_from<Instance>(character));
		return boost::polymorphic_downcast<Player*>(player.get());
	}
	return NULL;
}

void Players::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if (Players::backendProcessing(this, false))
		if (Player* player = Instance::fastDynamicCast<Player>(instance.get()))
			player->lockParent();	// prevent the player from coming back in (this is an exploit protection)
	Super::onDescendantRemoving(instance);
}

void Players::onChildRemoving(Instance* child)
{
	if (Player* player = Instance::fastDynamicCast<Player>(child))
	{
		if(FriendService* friendService = ServiceProvider::find<FriendService>(this)){
			friendService->playerRemoving(player->getUserID());
		}

		{
			boost::shared_ptr<Instances>& c = players.write();
			c->erase(std::find(c->begin(), c->end(), shared_from(child)));
		}

		if (DFFlag::FirePlayerAddedAndPlayerRemovingOnClient || Players::backendProcessing(this, false))
		{
			playerRemovingSignal(shared_from(player));
			playerRemovingLateSignal(shared_from(player));
		}

		Http::playerCount = getNumPlayers();
	}
}

static void callServerStop(DataModel* dm)
{
	if (Server* server = dm->find<Server>())
	{
		server->stop();
	}
}

void Players::processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const RBX::SystemAddress& source)
{
	if (DFFlag::CloudEditSupportPlayersKickAndShutdown &&
		descriptor == event_requestCloudEditKick && isCloudEdit(this))
	{
		if (args.size() == 1 && args[0].isNumber())
		{
			int playerId = args[0].cast<int>();
			disconnectPlayer(playerId, Replicator::DisconnectReason::DisconnectReason_CloudEditKick);
		}
	}
	else if (DFFlag::CloudEditSupportPlayersKickAndShutdown &&
		descriptor == event_requestCloudEditShutdown && isCloudEdit(this))
	{
		if (DataModel* dm = DataModel::get(this))
		{
			dm->submitTask(&callServerStop, DataModelJob::Write);
		}
	}
	else
	{
		Super::processRemoteEvent(descriptor, args, source);
	}
}

void Players::reportScriptSecurityError(int userId, std::string hash, std::string error, std::string stack)
{
}

void Players::killPlayer(int userId)
{
	Player* player = Instance::fastDynamicCast<Player>(getPlayerByID(userId).get());
	if(player){
		// TODO: Null check???
		player->getCharacter()->findFirstChildOfType<Humanoid>()->setHealth(0.0f);
	}
}

static void whoCaresResponse(std::string* result, std::exception* exception)
{

}

void Players::disconnectPlayer(Instance& instance, int userId, int reason)
{
	for (size_t i = 0; i < instance.numChildren(); ++i) 
	{
		if(Replicator* proxy = Instance::fastDynamicCast<Replicator>(instance.getChild(i)))
			if(Player* player = proxy->findTargetPlayer())
				if(player->getUserID() == userId)
				{
					FASTLOG(FLog::Network, "Players service requests player disconnect");
					if (frontendProcessing(player))
						proxy->sendDisconnectionSignal("server", false);

					proxy->requestDisconnect(Replicator::DisconnectReason(reason));
				}
	}
}

void Players::disconnectPlayer(int userId, int reason)
{
	if (Server* server = ServiceProvider::find<Server>(this))
	{
		if (DataModel* dm = DataModel::get(this))
		{
			if (reason == Replicator::DisconnectReason_LuaKick)
			{
				int placeID = dm->getPlaceID();
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ServerLuaKick", boost::lexical_cast<std::string>(placeID).c_str());
			}
		}
		disconnectPlayer(*server, userId, reason);
	}
}

void Players::disconnectPlayerLocal(int userId, int reason)
{
	if (Client* client = ServiceProvider::find<Client>(this))
	{
		if (DataModel* dm = DataModel::get(this))
		{
			if (reason == Replicator::DisconnectReason_LuaKick)
			{
				int placeID = dm->getPlaceID();
				RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "LocalLuaKick", boost::lexical_cast<std::string>(placeID).c_str());
			}
		}
		disconnectPlayer(*client, userId, reason);
	}
}

bool Players::hashMatches(const std::string& hash)
{
    if (FFlag::DebugLocalRccServerConnection)
    {
        return true;
    }

	if (DFFlag::RCCSupportCloudEdit && isCloudEdit(this))
	{
		return true;
	}

	FASTLOGS(FLog::GoldenHashes, "Checking hash: %s", hash);

	boost::mutex::scoped_lock lock(goldenHashesMutex);
	if (goldenHashes.find(hash) != goldenHashes.end())
		return true;
	
	if (hash.empty()) return false; // TODO: we might be able to remove this now
	if (goldenHashes.size() <= 0) return true;

	std::string ios = "i"; ios += "o"; ios += "s"; ios = ios + "," + ios; // slight obfuscation
	if (hash == ios) return true; // what we expect from ios clients

	if(hash != goldenHash && hash != goldenHash2 && hash != goldenHash3)
	{
		FASTLOG(FLog::GoldenHashes, "Hash is no good");
		//Their hash is no good, they failed the test
		return false;
	}

	return true;
}

unsigned int Players::checkGoldMemHashes(const std::vector<unsigned int>& hashes)
{
    boost::mutex::scoped_lock mtx(RBX::Network::Players::goldMemHashesMutex);
    unsigned int bestResult = 0;
    unsigned int bestErrors = 0xFFFFFFFF;
    for (MemHashConfigs::const_iterator configIt = goldMemHashes.begin(); configIt < goldMemHashes.end(); ++configIt)
    {
        // skip empty configs to avoid "always pass"
        if (configIt->empty())
        {
            continue;
        }

        // try to find the config with the fewest errors
        unsigned int numErrors = 0;
        unsigned int result = 0;
        for(MemHashVector::const_iterator vecIt = configIt->begin(); vecIt < configIt->end(); ++vecIt)
        {
            // report error if out-of-bounds or different
            if (hashes.size() <= vecIt->checkIdx || hashes[vecIt->checkIdx] != vecIt->value)
            {
                result |= vecIt->failMask;
                ++numErrors;
            }
        }
        if (numErrors < bestErrors)
        {
            bestResult = result;
            bestErrors = numErrors;
        }
        // stop when something matches
        if (numErrors == 0)
        {
            break;
        }
    }
    return bestResult;
}

bool Players::getUseCoreScriptHealthBar()
{
	return true;
}

// used to report hackers
void Players::onRemoteSysStats(int userId, const std::string& stat, const std::string& message, bool desireKick)
{
	bool willKick = desireKick && canKickBecauseRunningInRealGameServer;
	// make call to handler
	if(cheatingPlayers[userId].find(stat+message) != cheatingPlayers[userId].end()) {
		if (FFlag::DebugLocalRccServerConnection)
		{
			// skip the security check
		}
		else
		{
			if (willKick) {
				//Shut. It. Down.
				StandardOut::singleton()->printf(MESSAGE_INFO, "Players::onRemoteSysStats disconnect not in the clist");
				disconnectPlayer(userId, Replicator::DisconnectReason_OnRemoteSysStats);
			}
		}
		return;
	}
	else{
		FASTLOGS(FLog::US14116, "onRemoteSysStats: %s",
			RBX::format("%d | %s | %s", userId, stat.c_str(), message.c_str()));
		cheatingPlayers[userId].insert(stat+message);
	}

	if(!serverIsPresent(this)){
		// only report from server
		return;
	}

	static const std::string kStatsCategory("RemoteSysStats");

	bool isStudioServer = true;
	int placeId = 0;
	if (DataModel* dm = DataModel::get(this)) {
		placeId = dm->getPlaceID();
		isStudioServer = placeId <= 0;
	}

	if (isStudioServer) {

		FASTLOG3(FLog::US14116, "Sending display stats: %d | %s | %s", userId, stat.c_str(), message.c_str());

	}

	if (sysStatsUrl.empty()) return;

	//They are using CheatEngine or Fidler or have a non-signed EXE
	//Make the request and send it out
	std::stringstream requestUrl;
	requestUrl << sysStatsUrl << "&UserID=" << userId << "&Resolution=" << stat << "&Message=" << message;
	requestUrl << char(0);

	Http http(requestUrl.str());
	
	std::string response;
	try
	{
		http.get(boost::bind(&whoCaresResponse, _1, _2));
	}
	catch(std::bad_alloc&)
	{
		throw;
	}
	catch(RBX::base_exception&)
	{
		//Something went bad with our request
		if (willKick) {
			StandardOut::singleton()->printf(MESSAGE_INFO, "Players::onRemoteSysStats disconnect send failed");
			disconnectPlayer(userId, Replicator::DisconnectReason_OnRemoteSysStats);
		}
		return;
	}

	if (willKick) {
		StandardOut::singleton()->printf(MESSAGE_INFO, "Players::onRemoteSysStats disconnect");
		//Shut. It. Down.
		disconnectPlayer(userId, Replicator::DisconnectReason_OnRemoteSysStats);
	}
}

void Players::onChildAdded(Instance* child)
{
	if (Player* player = Instance::fastDynamicCast<Player>(child))
	{
		int userId = player->getUserID();
		if (userId>0)
		{
			shared_ptr<Instance> otherPlayer = getPlayerByID(userId);
			if (otherPlayer)
			{
				// TODO: Wrap this into our security framework. This looks like just a hack
				otherPlayer->setParent(NULL);
				//throw std::runtime_error(format("Player %d attempted to join twice. Booting both players", player->getUserID()));
			}
		}

		{
			boost::shared_ptr<Instances>& c = players.write();
			c->push_back(shared_from(child));
		}
		
		if(FriendService* friendService = ServiceProvider::find<FriendService>(this)){
			friendService->playerAdded(userId);
		}
		
		if (Players::backendProcessing(this))		// Server does it in client/server - bail on client mode.  will still execute in solo mode
		{
			if(userId == 0 && player->getName() == "Player")
			{
				// we are in a test mode, give every player a unique name and userId
				// userIds are negative for an extra security measure for uploading to website, etc.
				RBX::Security::Impersonator impersonate(RBX::Security::Replicator_);
				player->setName(RBX::format("Player%d", ++testPlayerNameId));
				player->setUserId(--testPlayerUserId);
			}

			Workspace* workspace = ServiceProvider::find<Workspace>(this);
			if (!DFFlag::LoadGuisWithoutChar || !workspace->getNetworkFilteringEnabled() || (Network::Players::frontendProcessing(this) && Network::Players::backendProcessing(this)))
			{
				// this allows guis to be created if you load a character manually later
				player->createPlayerGui();
			}

			player->scriptSecurityErrorSignal.connect(boost::bind(&Players::reportScriptSecurityError, this, userId, _1, _2, _3));
			player->killPlayerSignal.connect(boost::bind(&Players::killPlayer, this, userId));

			player->statsSignal.connect(boost::bind(&Players::onRemoteSysStats, this, userId, "1920x1200", _1, true));

			player->remoteFriendServiceSignal.connect(boost::bind(&Players::friendServiceRequest, this, _1, weak_from(player), _2));

			if (!isCloudEdit(this) && !player->calculatesSpawnLocationEarly()) {
				// Team Service
				if (Teams* ts = ServiceProvider::find<Teams>(this))
					ts->assignNewPlayerToTeam(player);
			}

			if (!DFFlag::FirePlayerAddedAndPlayerRemovingOnClient)
			{
				playerAddedEarlySignal(shared_from(player));
				playerAddedSignal(shared_from(player));
			}
		}

		if (DFFlag::FirePlayerAddedAndPlayerRemovingOnClient)
		{
			playerAddedEarlySignal(shared_from(player));
			playerAddedSignal(shared_from(player));
		}

		Http::playerCount = getNumPlayers();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class AppendOtherCharacters {
public:
	const Player* ownerPlayer;
	Region2& region;

	AppendOtherCharacters(const Player* _main, Region2& _region) 
		: ownerPlayer(_main), region(_region)
	{}

	void operator()(shared_ptr<Instance> instance) {
		if (Player* p = Instance::fastDynamicCast<Player>(instance.get())) {
			if (p != ownerPlayer) {
				CoordinateFrame pos;
				if (p->hasCharacterHead(pos)) {
					region.appendOther( Region2::WeightedPoint(	pos.translation.xz(), 
																p->getSimulationRadius()  ) );
				}
			}
		}
	}
};


void Players::buildClientRegion(Region2& clientRegion)
{
	clientRegion.clearEmpty();

	CoordinateFrame pos;
	if (localPlayer && (localPlayer->hasCharacterHead(pos)) ) 
	{
		clientRegion.setOwner(	Region2::WeightedPoint(	pos.translation.xz(), 
														localPlayer->getSimulationRadius())	);

		AppendOtherCharacters appendOtherCharacters(localPlayer.get(), clientRegion);
		this->visitChildren(appendOtherCharacters);
	}
}

void Players::renderDPhysicsRegions(Adorn* adorn)
{
	shared_ptr<const Instances> myPlayers = getPlayers();

	Instances::const_iterator iter = myPlayers->begin();
	Instances::const_iterator end = myPlayers->end();
	for (; iter!=end; ++iter)
	{
		Player* player = boost::polymorphic_downcast<Player*>(iter->get());
		player->renderDPhysicsRegion(adorn);
	}
}


void Players::setChatOption(ChatOption value)
{
	chatOption = value;
}

void Players::setNonSuperSafeChatForAllPlayersEnabled(bool enabled)
{
	nonSuperSafeChatForAllPlayersEnabled = enabled;
}

bool Players::getNonSuperSafeChatForAllPlayersEnabled() const
{
	return nonSuperSafeChatForAllPlayersEnabled;
}

namespace RBX {
namespace Reflection {
template<>
EnumDesc<Players::ChatOption>::EnumDesc()
:EnumDescriptor("ChatStyle")
{
	addPair(Players::CLASSIC_CHAT, "Classic");
	addPair(Players::BUBBLE_CHAT, "Bubble");
	addPair(Players::CLASSIC_AND_BUBBLE_CHAT, "ClassicAndBubble");
}
template<>
Players::ChatOption& Variant::convert<Players::ChatOption>(void)
{
	return genericConvert<Players::ChatOption>();
}

template<>
EnumDesc<Players::PlayerChatType>::EnumDesc()
:EnumDescriptor("PlayerChatType")
{
	addPair(Players::PLAYER_CHAT_TYPE_ALL,  "All");
	addPair(Players::PLAYER_CHAT_TYPE_TEAM, "Team");
	addPair(Players::PLAYER_CHAT_TYPE_WHISPER, "Whisper");
}
template<>
Players::PlayerChatType& Variant::convert<Players::PlayerChatType>(void)
{
	return genericConvert<Players::PlayerChatType>();
}

}//namespace Reflection

template<>
bool StringConverter<Players::ChatOption>::convertToValue(const std::string& text, Players::ChatOption& value)
{
	return Reflection::EnumDesc<Players::ChatOption>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool StringConverter<Players::PlayerChatType>::convertToValue(const std::string& text, Players::PlayerChatType& value)
{
	return Reflection::EnumDesc<Players::PlayerChatType>::singleton().convertToValue(text.c_str(),value);
}
}//namespace RBX


