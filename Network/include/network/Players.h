/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Network/Player.h"
#include "Network/ChatFilter.h"
#include "Util/SystemAddress.h"
#include "Util/GameMode.h"
#include "V8DataModel/FriendService.h"
#include "V8DataModel/ContentProvider.h"
#include "boost/thread/thread.hpp"
#include <boost/thread/condition.hpp>
#include <boost/unordered_set.hpp>
#include <queue>

namespace RakNet {
	class RakPeerInterface;
	struct Packet;
	class BitStream;
	struct SystemAddress;
}

namespace RBX { 
	class PartInstance;
	class ModelInstance;
	class Region2;
	class Adorn;
	class ScriptInformationProvider;
	class DataModel;

namespace Network {

    struct MemHash
    {
        size_t checkIdx;
        unsigned int value;
        unsigned int failMask;
    };

    typedef std::vector<MemHash> MemHashVector;
    typedef std::vector<MemHashVector> MemHashConfigs;

	class ConcurrentRakPeer;

	class ChatMessage
	{
	public:
		enum ChatType
		{
			CHAT_TYPE_ALL,
			CHAT_TYPE_TEAM,
			CHAT_TYPE_WHISPER,
			CHAT_TYPE_GAME,
			//CHAT_TYPE_PARTY
		};

		std::string guid;
		std::string message;
		ChatType chatType;
		shared_ptr<Player> const source;
		shared_ptr<Player> const destination;
		ChatMessage(const ChatMessage& other, const std::string& message);
		ChatMessage(const char* message, ChatType chatType, shared_ptr<Player> source);
		ChatMessage(const char* message, ChatType chatType, shared_ptr<Player> source, shared_ptr<Player> destination);

		bool isVisibleToPlayer(shared_ptr<Player> player) const;
		static bool isVisibleToPlayer(shared_ptr<Player> const player, shared_ptr<Player> const source, shared_ptr<Player> const destination, const ChatType chatType);
		std::string getReportAbuseMessage() const;
	};

	struct AbuseReport
	{
		struct Message 
		{ 
			int userID; 
			std::string text; 
			std::string guid; 
		};

		int placeID;
		std::string gameJobID;

		int submitterID;
		int allegedAbuserID;
		std::string comment;
		std::list< Message > messages;
		void addMessage(shared_ptr<Player> reportingPlayer, const ChatMessage& cm);
	};

	class AbuseReporter
	{
		struct data
		{
			std::queue<AbuseReport> queue;
			boost::mutex requestSync;		// synchronizes the request queue
		};
		shared_ptr<data> _data;
		scoped_ptr<worker_thread> requestProcessor;
	public:
		AbuseReporter(std::string abuseUrl);
		void add(AbuseReport& r, shared_ptr<Player> reportingPlayer, const std::list<ChatMessage>& chatHistory);
	private:
		static worker_thread::work_result processRequests(shared_ptr<data> _data, std::string abuseUrl);
	};

	extern const char* const sPlayers;

	class Players
		: public DescribedNonCreatable<Players, Instance, sPlayers>
		, public Service
	{
	private:
		typedef DescribedNonCreatable<Players, Instance, sPlayers> Super;
	public:
		// Unfortunately the RakNet enums are not accessible via the Players class, so this identical enum is created
		// and mapped in PluginInterfaceAdapter::OnReceive.  - Tim
		enum ReceiveResult
		{
			// The plugin used this message and it shouldn't be given to the user.
			PLAYERS_STOP_PROCESSING_AND_DEALLOCATE=0,

			// The plugin is going to hold on to this message.  Do not deallocate it but do not pass it to other plugins either.
			PLAYERS_STOP_PROCESSING,
		};

		enum ChatOption
		{
			CLASSIC_CHAT			= 0,
			BUBBLE_CHAT				= 1,
			CLASSIC_AND_BUBBLE_CHAT	= 2
		};

		enum PlayerChatType
		{
			PLAYER_CHAT_TYPE_ALL	= 0,
			PLAYER_CHAT_TYPE_TEAM	= 1,
			PLAYER_CHAT_TYPE_WHISPER= 2
		};


		static bool isNetworkClient(Instance* instance);
		void friendEventFired(int userId, int otherUserId, FriendService::FriendEventType friendEvent);
		void friendStatusChanged(int userId, int otherUserId, FriendService::FriendStatus friendStatus);

	private:
		std::string saveDataUrl;
		std::string loadDataUrl;
		std::string saveLeaderboardDataUrl;
		boost::unordered_set<std::string> leaderboardKeys;

		std::string chatFilterUrl;
		std::string buildUserPermissionsUrl;
		std::string sysStatsUrl;
		std::string goldenHash;
		std::string goldenHash2;
		std::string goldenHash3;

		static bool canKickBecauseRunningInRealGameServer;
		static std::set<std::string> goldenHashes;
		static boost::mutex goldenHashesMutex;

		static MemHashConfigs goldMemHashes;
		static boost::mutex goldMemHashesMutex;

		bool characterAutoSpawn;

		scoped_ptr<AbuseReporter> abuseReporter;
		boost::intrusive_ptr<GuidItem<Instance>::Registry> guidRegistry;
		std::list<ChatMessage> chatHistory;

		const boost::intrusive_ptr<GuidItem<Instance>::Registry>& getGuidRegistry();

		copy_on_write_ptr<Instances> players;
		ConcurrentRakPeer* rakPeer;

		int maxPlayers;
		int preferredPlayers;
		int testPlayerNameId;
		int testPlayerUserId;
		shared_ptr<Player> localPlayer;

		ChatOption chatOption;

		bool nonSuperSafeChatForAllPlayersEnabled;

		rbx::signals::connection blockUserClientSignalConnection;
		rbx::signals::connection blockUserFinishedFromServerConnection;
		rbx::signals::connection loadLocalPlayerGuisConnection;

		boost::unordered_map<std::pair<int,int>, std::pair<boost::function<void(std::string)>, boost::function<void(std::string)> > > clientBlockUserMap;

		void raiseChatMessageSignal(const ChatMessage& message);
		void raisePlayerChattedSignal(const ChatMessage& message);

		void loadLocalPlayerGuis();
		void gotBlockUserSuccess(std::string response, bool blockUser, int blockerUserId, int blockeeUserId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void gotBlockUserError(std::string error, bool blockUser, int blockerUserId, int blockeeUserId, boost::function<void(std::string)> errorFunction);
		
		static void onReceivedRawGetUserIdSuccess(weak_ptr<DataModel> weakDataModel, std::string response, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		static void onReceivedRawGetUserIdError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction);

		static void onReceivedRawGetUserNameSuccess(weak_ptr<DataModel> weakDataModel, std::string response, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		static void onReceivedRawGetUserNameError(weak_ptr<DataModel> weakDataModel, std::string error, boost::function<void(std::string)> errorFunction);

		void serverMakeBlockUserRequest(bool blockUser, int blockerUserId, int blockeeUserId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void clientReceiveBlockUserFinished(int blockerUserId, int blockeeUserId, std::string errorString);

		void internalBlockUser(int blockerUserId, int blockeeUserId, bool isBlocking, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);

	public:
		ReceiveResult OnReceiveChat(Player* sourceValidation, RakNet::RakPeerInterface* peer, RakNet::Packet* packet, unsigned char chatType);
		ReceiveResult OnReceiveReportAbuse(Player* source, RakNet::RakPeerInterface* peer, RakNet::Packet* packet);

		rbx::signal<void(shared_ptr<Instance>,shared_ptr<Instance>,FriendService::FriendEventType)> friendRequestEvent;
		rbx::signal<void(shared_ptr<Instance>)> playerAddedEarlySignal;
		rbx::signal<void(shared_ptr<Instance>)> playerAddedSignal;
		rbx::signal<void(shared_ptr<Instance>)> playerRemovingSignal;
		rbx::signal<void(shared_ptr<Instance>)> playerRemovingLateSignal;
		rbx::signal<void(const ChatMessage&)> chatMessageSignal;
		rbx::signal<void(AbuseReport report)> abuseReportedReceived;
		rbx::signal<void(const RakNet::SystemAddress&, const shared_ptr<RakNet::BitStream>&, const shared_ptr<Instance>, const std::string&, const std::string&)> sendFilteredChatMessageSignal;

		rbx::signal<void(PlayerChatType, shared_ptr<Instance>, std::string, shared_ptr<Instance>)> playerChattedSignal;
		rbx::signal<void(std::string)> gameAnnounceSignal;

		rbx::remote_signal<void(int,int,bool)> blockUserRequestFromClientSignal;
		rbx::remote_signal<void(int,int,std::string)> blockUserFinishedFromServerSignal;
		rbx::remote_signal<void(int)> requestCloudEditKick;
		rbx::remote_signal<void()> requestCloudEditShutdown;

		static Reflection::RefPropDescriptor<Players, Instance> propLocalPlayer;
		static Reflection::PropDescriptor<Players, bool> propCharacterAutoSpawn;
		static Reflection::RemoteEventDesc<Players, void(int)> event_requestCloudEditKick;
		static Reflection::RemoteEventDesc<Players, void()> event_requestCloudEditShutdown;
		Players();
		~Players();

		bool superSafeOn() const;

		shared_ptr<Instance> createLocalPlayer(int userId, bool teleportedIn = false);
		void resetLocalPlayer();

		Player* getLocalPlayer() {return localPlayer.get();}
		const Player* getConstLocalPlayer() const {return localPlayer.get();}
		Instance* getLocalPlayerDangerous() const; // only for reflection

		int getNumPlayers() const { return (int) players->size(); }

		void getUserIdFromName(std::string userName, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getNameFromUserId(int userId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getFriends(int userId, boost::function<void(shared_ptr<Instance>) > resumeFunction, boost::function<void(std::string)> errorFunction);

		static void setAppearanceParent(shared_ptr<Instance> model, weak_ptr<Instance> instance);
		static void doLoadAppearance(AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances, std::string contentDescription, shared_ptr<ModelInstance> model, shared_ptr<int> amountToLoad, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction);
		static void doMakeAccoutrementRequests(std::string response, weak_ptr<DataModel> dataModel, shared_ptr<ModelInstance> model, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction);
		static void makeAccoutrementRequests(std::string *response, std::exception *err, weak_ptr<DataModel> dataModel, shared_ptr<ModelInstance> model, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction);
		void getCharacterAppearance(int userId, boost::function<void (shared_ptr<Instance>)> resumeFunction, boost::function<void (std::string)> errorFunction);

		int getMaxPlayers() const;
		int getPreferredPlayers() const;

		void setMaxPlayers(int value);
		void setPreferredPlayers(int value);
		void setSysStatsUrl(std::string url);
		void setSysHash(std::string hash);

		void setGoldenHashes(const std::string& windowsHash, const std::string& macHash, const std::string& windowsPlayerBetaHash);
		static void setGoldenHashes2(const std::set<std::string>& hashes);
		static void setGoldMemHashes(const MemHashConfigs& hashes);

		std::string getSaveDataUrl(int userId) const;
		void setSaveDataUrl(std::string saveDataUrl);

		std::string getLoadDataUrl(int userId) const;
		void setLoadDataUrl(std::string loadDataUrl);

		std::string getSaveLeaderboardDataUrl(int userId) const;
		void setSaveLeaderboardDataUrl(std::string saveLeaderboardDataUrl);
		void addLeaderboardKey(std::string);
		
		bool hasLeaderboardKey(const std::string& key) const;
		boost::unordered_set<std::string>::const_iterator beginLeaderboardKey() const;
		boost::unordered_set<std::string>::const_iterator endLeaderboardKey() const;

		void setChatOption(ChatOption value);
		void setNonSuperSafeChatForAllPlayersEnabled(bool enabled);
		bool getNonSuperSafeChatForAllPlayersEnabled() const;
		bool getClassicChat() const { return chatOption == CLASSIC_CHAT || chatOption == CLASSIC_AND_BUBBLE_CHAT; }
		bool getBubbleChat() const  { return chatOption == BUBBLE_CHAT || chatOption == CLASSIC_AND_BUBBLE_CHAT;  }

		shared_ptr<const Instances> getPlayers() { return players.read(); }

		// Chat-related functions
		void gamechat(const std::string& message);
		
		void chat(std::string message);
		void teamChat(std::string);
		void whisperChat(std::string message, shared_ptr<Instance> player);

		void reportAbuse(Player* player, const std::string& comment);
		void reportAbuseLua(shared_ptr<Instance> instance, std::string reason, std::string comment);
		std::list<ChatMessage>::const_iterator chatHistory_begin() { return chatHistory.begin(); }
		std::list<ChatMessage>::const_iterator chatHistory_end() { return chatHistory.end(); }
		bool canReportAbuse() const;
		void setAbuseReportUrl(std::string value);
		void setChatFilterUrl(std::string value);
		void setBuildUserPermissionsUrl(std::string value);
		bool hasBuildUserPermissionsUrl() const;
		std::string getBuildUserPermissionsUrl(int playerId) const;

		void friendServiceRequest(bool makeFriends, weak_ptr<Player> sourcePlayer, int otherUserId);

		bool getCharacterAutoSpawnProperty() const { return characterAutoSpawn; }
		void setCharacterAutoSpawnProperty(bool value);
		bool getShouldAutoSpawnCharacter() const;

		void blockUser(int blockerUserId, int blockeeUserId, 
			boost::function<void(std::string)> resumeFunction = boost::function<void(bool)>(), 
			boost::function<void(std::string)> errorFunction = boost::function<void(std::string)>());
		void unblockUser(int blockerUserId, int blockeeUserId,
			boost::function<void(std::string)> resumeFunction = boost::function<void(bool)>(), 
			boost::function<void(std::string)> errorFunction = boost::function<void(std::string)>());

		void setConnection(ConcurrentRakPeer* rakPeer);

		shared_ptr<Instance> playerFromCharacter(shared_ptr<Instance> character);
		shared_ptr<Instance> getPlayerInstanceByID(int userID);
		shared_ptr<Player> getPlayerByID(int userID);

		static Player* getPlayerFromCharacter(RBX::Instance* character);

		void buildClientRegion(Region2& clientRegion);

		void renderDPhysicsRegions(Adorn* adorn);

		///////////////////////////////////////////////////////////////////////////
		//
		// STATICS
		//
		// If Client == Client Network Address;  If Server == NetworkOwner::Server()
		static RBX::SystemAddress findLocalSimulatorAddress(const RBX::Instance* context);

		static ModelInstance* findLocalCharacter(RBX::Instance* context);
		static const ModelInstance* findConstLocalCharacter(const RBX::Instance* context);

		static Player* findLocalPlayer(RBX::Instance* context);
		static const Player* findConstLocalPlayer(const RBX::Instance* context);

		static shared_ptr<Player> findAncestorPlayer(const RBX::Instance* context);
        
        static shared_ptr<Player> findPlayerWithAddress(const RBX::SystemAddress& playerAddres, const RBX::Instance* context);

		static bool clientIsPresent(const RBX::Instance* context, bool testInDatamodel = true);

		static bool serverIsPresent(const RBX::Instance* context, bool testInDatamodel = true);

		static bool frontendProcessing(const RBX::Instance* context, bool testInDatamodel = true);

		static bool backendProcessing(const RBX::Instance* context, bool testInDatamodel = true);

		static int getPlayerCount(const RBX::Instance* context);

		// TODO: Remove this some day!
		static bool getDistributedPhysicsEnabled();

		///////////////////////////////////////////////////////////////////////////
		//
		// These are here as "official" designation of Big-Picture states
		//
		//						Frontend	Backend
		// GameServer:						x		serverIsPresent (assumes !findLocalPlayer)
		// Visit Online:		x					clientIsPresent && findLocalPlayer
		// Watch Online:		x					clientIsPresent && !findLocalPlayer	(i.e. - visit, no character)
		// Visit Solo:			x			x		!clientIsPresent && !serverIsPresent && findLocalPlayer
		// Local Play:			x					clientIsPresent && findLocalPlayer && userID == 0
		// Edit Mode:			x			x		!clientIsPresent && !serverIsPresent && !findLocalPlayer
		//
		// typedef enum {GAME_SERVER, DPHYS_GAME_SERVER, CLIENT, DPHYS_CLIENT, WATCH_ONLINE, VISIT_SOLO, EDIT} GameMode;
		
		static RBX::Network::GameMode getGameMode(const RBX::Instance* context)
		{
			bool client = clientIsPresent(context);
			bool server = serverIsPresent(context);
			bool localPlayer = (findConstLocalPlayer(context) != NULL);
			bool dPhysics = getDistributedPhysicsEnabled();

			RBXASSERT(!(server && (client || localPlayer)));

			if (server)						{return dPhysics ? DPHYS_GAME_SERVER : GAME_SERVER;}
			if (client && localPlayer)		{return dPhysics ? DPHYS_CLIENT : CLIENT;}
			if (client && !localPlayer)		{return WATCH_ONLINE;}
			if (!client && localPlayer)		{return VISIT_SOLO;}
			else							{return EDIT;}
		}

		static RBX::Network::GameMode getGameMode(const RBX::Instance* context, const int placeID)
		{
			bool client = clientIsPresent(context);
			bool server = serverIsPresent(context);
			bool localPlayer = ( findConstLocalPlayer(context) != NULL);
			bool dPhysics = getDistributedPhysicsEnabled();

			RBXASSERT(!(server && (client || localPlayer)));;

			if (placeID <= 0)							{return LOCAL_PLAY;}
			if (server)									{return dPhysics ? DPHYS_GAME_SERVER : GAME_SERVER;}
			if (client && localPlayer)					{return dPhysics ? DPHYS_CLIENT : CLIENT;}
			if (client && !localPlayer)					{return WATCH_ONLINE;}
			if (!client && localPlayer)					{return VISIT_SOLO;}
			else										{return EDIT;}
		}

		static bool isCloudEdit(const RBX::Instance* context);

		void onRemoteSysStats(int userId, const std::string& stat, const std::string& message, bool desireKick = true);
		bool hashMatches(const std::string& hash);
		static unsigned int checkGoldMemHashes(const std::vector<unsigned int>& hashes);

		void disconnectPlayer(int userId, int reason);
		void disconnectPlayerLocal(int userId, int reason);

		bool getUseCoreScriptHealthBar();

	protected:
		void disconnectPlayer(Instance& instance, int userId, int reason);

		std::map<int, std::set<std::string> > cheatingPlayers;
		bool askAddChild(const Instance* instance) const;
		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoving(Instance* child);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
		/*override*/ void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const RBX::SystemAddress& source);

	private:
		void reportScriptSecurityError(int userId, std::string hash, std::string error, std::string stack);

		void killPlayer(int userId);
		
		void addChatMessage(const ChatMessage& message);

		void checkChat(const std::string& message);

		void sendFilteredChatMessageSignalHelper(const RakNet::SystemAddress& systemAddress,
			const shared_ptr<RakNet::BitStream>& baseData, const shared_ptr<Instance> sourceInstance, shared_ptr<ChatMessage> chatEvent, const ChatFilter::Result& response);

		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	};

} }
