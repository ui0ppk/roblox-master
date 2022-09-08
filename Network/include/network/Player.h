/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/rbxTime.h"
#include "v8tree/instance.h"
#include "v8datamodel/team.h"
#include "V8Datamodel/FriendService.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/StarterPlayerService.h"
#include "Util/BrickColor.h"
#include "Util/G3DCore.h"
#include "Util/HeapValue.h"
#include "Util/Rect.h"
#include "util/RunningAverage.h"

// for player mouse
#include "V8Datamodel/Mouse.h"

#include <boost/scoped_ptr.hpp>
#include "v8datamodel/TeleportService.h"

LOGGROUP(Network)
LOGGROUP(Player)

namespace RakNet {
struct SystemAddress;
}

namespace RBX { 
	class Visit;
	class ModelInstance;
	class Backpack;
	class TimerService;
	class Region2;
	class DataModelMesh;
	class Adorn;
	class Primitive;
	class PartInstance;
	class Humanoid;
	class DataModel;
	class SpawnLocation;

namespace Network {
	class PersistentDataStore;

	extern const char* const sPlayer;

	class Player
		: public DescribedCreatable<Player, Instance, sPlayer, Reflection::ClassDescriptor::RUNTIME>
		, public Diagnostics::Countable<Player>
	{
	private:
		typedef DescribedCreatable<Player, Instance, sPlayer, Reflection::ClassDescriptor::RUNTIME> Super;
		shared_ptr<ModelInstance> character;		// The ModelInstance that this player is represented by
		BrickColor teamColor;
		bool neutral;
		int loadAppearanceCounter;
		int dataComplexityLimit;
		bool dataReady;
		weak_ptr<SpawnLocation> respawnLocation;
		CoordinateFrame cloudEditCameraCFrame;

		bool loadedStarterGear;

        // User Profile data
        bool superSafeChat;
        bool under13;
        int userId;

        // For Group Building
        bool hasGroupBuildTools;
        HeapValue<int> personalServerRank;

		rbx::signals::scoped_connection characterDiedConnection;
		rbx::signals::scoped_connection backendDiedSignalConnection;
		rbx::signals::scoped_connection spawnLocationChangedConnection;
		rbx::signals::scoped_connection simulationRadiusChangedConnection;
		rbx::signals::scoped_connection setShutdownMessageConnection;
		boost::function<void()> teamStatusChangedCallback;

		// CharacterAppearance is the url of a handler that returns a delimited list of asset ids
		std::string characterAppearance;
		bool canLoadCharacterAppearance;

		// Distributed Simulation
		float simulationRadius;
		float maxSimulationRadius;
		
		std::string osPlatform;
		std::string vrDevice;

		int loadingInstances;
		bool appearanceDidLoad;
		bool characterAppearanceLoaded;

        // Only valid on the server!
		shared_ptr<RakNet::SystemAddress> remoteAddress;

		bool forceEarlySpawnLocationCalculation;
		bool hasSpawnedAtLeastOnce;
		std::string teleportSpawnName;
		bool teleported; // describes whether player is currently in the middle of teleporting out
		bool teleportedIn; // describes whether player joined current place by teleporting

		std::string gameSessionID;

		struct SpawnData
		{
			Vector3 position;
			int forceFieldDuration;
			CoordinateFrame cf;

			SpawnData() : position(Vector3::zero()), forceFieldDuration(0) {}
			SpawnData(Vector3 position, int forceFieldDuration, CoordinateFrame frame) 
				: position(position), forceFieldDuration(forceFieldDuration), cf(frame)
			{}
		};

		boost::scoped_ptr<SpawnData> nextSpawnLocation;

		int followUserId;

		// Instance
		/*override*/ bool askAddChild(const Instance* instance) const {return true;}
		/*override*/ void verifySetParent(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		void onCharacterChangedFrontend();

		void registerLocalPlayerNotIdle();

		void loadInsertPanel(DataModel* dataModel);

		shared_ptr<PersistentDataStore> persistentData;
		std::list<boost::function<void(bool)> > waitForDataReadyWaiters;
		void fireWaitForDataReady();
		static void LoadDataResultHelper(boost::weak_ptr<Player> weakPlayer, shared_ptr<const Reflection::ValueMap> result);
		void loadDataResult(shared_ptr<const Reflection::ValueMap> result);

		bool autoJumpEnabled;

	public:
		enum MembershipType
	    {
		    MEMBERSHIP_NONE						= 0,
		    MEMBERSHIP_BUILDERS_CLUB			= 1,
		    MEMBERSHIP_TURBO_BUILDERS_CLUB		= 2,
		    MEMBERSHIP_OUTRAGEOUS_BUILDERS_CLUB	= 3
	    };
		enum ChatMode
		{
			CHAT_MODE_MENU = 0,
			CHAT_MODE_TEXT_AND_MENU = 1
		};
		enum ChatFilterType {
			CHAT_FILTER_WHITELIST,
			CHAT_FILTER_BLACKLIST
		};

		// debug timer
		RBX::Timer<RBX::Time::Fast> appearanceFetchTimer;

		static Reflection::PropDescriptor<Player, int> prop_userId;
		static Reflection::PropDescriptor<Player, int> prop_userIdDeprecated;
		static Reflection::PropDescriptor<Player, bool> prop_SuperSafeChat;
		static Reflection::BoundProp<float> prop_SimulationRadius;
		static Reflection::BoundProp<float> prop_MaxSimulationRadius;
        static Reflection::PropDescriptor<Player, float> prop_DeprecatedMaxSimulationRadius;
		static Reflection::BoundProp<std::string> prop_OsPlatform;
		static Reflection::BoundProp<std::string> prop_VRDevice;
		static Reflection::RemoteEventDesc<Player, void(shared_ptr<const Reflection::ValueArray>)> event_cloudEditSelectionChanged;

		rbx::remote_signal<void(std::string, std::string, std::string)> scriptSecurityErrorSignal;
		rbx::remote_signal<void(std::string,Vector3)> remoteInsertSignal;

		rbx::remote_signal<void()> connectDiedSignalBackend;

		rbx::remote_signal<void(bool,int)> remoteFriendServiceSignal;
		rbx::remote_signal<void()> killPlayerSignal;
		rbx::remote_signal<void(float)> simulationRadiusChangedSignal;

		rbx::remote_signal<void(std::string)> statsSignal;
		rbx::signals::scoped_connection charChildAddedConnection; 
		
		rbx::signal<void(std::string, shared_ptr<Instance>)> chattedSignal;
		rbx::signal<void(shared_ptr<Instance>)> characterAddedSignal;
		rbx::remote_signal<void(shared_ptr<Instance>)> CharacterAppearanceLoadedSignal;
		rbx::signal<void(shared_ptr<Instance>)> characterRemovingSignal;
		rbx::signal<void(Vector3)> nextSpawnLocationChangedSignal;
		rbx::signal<void(double)> idledSignal;			

		rbx::signal<void(shared_ptr<Instance>, FriendService::FriendStatus)> friendStatusChangedSignal;

        rbx::remote_signal<void(TeleportService::TeleportState, int, std::string)> onTeleportSignal;
		rbx::remote_signal<void(TeleportService::TeleportState, shared_ptr<const Reflection::ValueTable>, shared_ptr<Instance>)> onTeleportInternalSignal;

		rbx::remote_signal<void(std::string)> setShutdownMessageSignal;
		rbx::remote_signal<void(shared_ptr<const Reflection::ValueArray>)> cloudEditSelectionChanged;

		Player();
		~Player();

		void doFirstSpawnLocationCalculation(const ServiceProvider* serviceProvider, const std::string& preferedSpawnName);
		void reportScriptSecurityError(const std::string& hash, const std::string& error, const std::string& stack);
		void killPlayer();		

		void requestFriendship(shared_ptr<Instance> player);
		void revokeFriendship(shared_ptr<Instance> player);

		// Keyboard/Mouse input API
		shared_ptr<Mouse> getMouse() { return mouse; }
		shared_ptr<Instance> getMouseInstance();

		//Persistent Data API
		void loadData();
		void saveData();
		void saveLeaderboardData();
		int getDataComplexityLimit() const { return dataComplexityLimit; }
		void setDataComplexityLimit(int value);
		int getDataComplexity() const;

		bool getDataReady() const { return dataReady; }

		void waitForDataReady(boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		double loadNumber(std::string key);
		void saveNumber(std::string key, double value);
		std::string loadString(std::string key);
		void saveString(std::string key, std::string value);
		bool loadBoolean(std::string key);
		void saveBoolean(std::string key, bool value);
		shared_ptr<Instance> loadInstance(std::string key);
		void saveInstance(std::string key, shared_ptr<Instance> value);
		shared_ptr<const Reflection::ValueArray> loadList(std::string key);
		void saveList(std::string key, shared_ptr<const Reflection::ValueArray> value);
		shared_ptr<const RBX::Reflection::ValueMap> loadTable(std::string key);
		void saveTable(std::string key, shared_ptr<const RBX::Reflection::ValueMap> value);

		/*override*/ void setName(const std::string& value);
		/*override*/ bool canClientCreate() { return true; }

		ModelInstance* getDangerousCharacter() const {return character.get();}

		boost::shared_ptr<ModelInstance> getSharedCharacter() {return character; }
		ModelInstance* getCharacter() {return character.get();}
		const ModelInstance* getConstCharacter() const {return character.get();}
		void setCharacter(ModelInstance* value);

		const SpawnLocation* getConstRespawnLocation() const {
			if (shared_ptr<SpawnLocation> shared_respawnLocation = respawnLocation.lock())
				return shared_respawnLocation.get();
			else 
				return NULL;
		}
		SpawnLocation* getDangerousRespawnLocation() const {
			if (shared_ptr<SpawnLocation> shared_respawnLocation = respawnLocation.lock())
				return shared_respawnLocation.get();
			else 
				return NULL;
		}
		void setRespawnLocation(SpawnLocation* value);

		bool getCanLoadCharacterAppearance() const { return canLoadCharacterAppearance;}
		void setCanLoadCharacterAppearance(bool value);

		bool getAutoJumpEnabled() const { return autoJumpEnabled; }
		void setAutoJumpEnabled(bool value);

		const PartInstance* hasCharacterHead(CoordinateFrame& headPos) const;

		bool getHasGroupBuildTools() const { return hasGroupBuildTools; }
		void setHasGroupBuildTools(bool value);

		void giveBuildTools();
		void removeBuildTools();

		void setAppearanceLoaded();

		// Personal Server API
		void getWebPersonalServerRank(boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void setWebPersonalServerRank(int newRank, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		int getPersonalServerRank() const { return (personalServerRank > 255) ? 0 : personalServerRank; } // 255 is max rank allowed by setter.
		void setPersonalServerRank(int value);
		
		const Primitive* getConstCharacterRoot() const;

		BrickColor getTeamColor() const {return teamColor;}
		void setTeamColor(BrickColor value);

		bool getNeutral() const {return neutral;}
		void setNeutral(bool value);

		std::string getCharacterAppearance() const {return characterAppearance;}
		void setCharacterAppearance(const std::string& value);

		static void setAppearanceParent(weak_ptr<Player> player, weak_ptr<Instance> instance, bool equipped);

		static void setGearParent(weak_ptr<Player> player, weak_ptr<Instance> instance, bool equipped);
		
		bool getSuperSafeChat() const;
		void setSuperSafeChat(bool value);

        float getDeprecatedMaxSimulationRadius() const {return 0.f;}
        void setDeprecatedMaxSimulationRadius(float val){}

		ChatMode getChatMode() const;

		void setUnder13(bool value);
		bool getUnder13() {return under13;};

		void setUserId(int value);
		int getUserID() const {return userId;}

		void setGameSessionID(std::string value);
		std::string getGameSessionID() { return gameSessionID; }
		
		bool isGuest() const { return userId < 0; }
		void setMembershipType(MembershipType value);
		MembershipType getMembershipType() const {return membershipType;}

		void setCameraMode(RBX::Camera::CameraMode value);
		RBX::Camera::CameraMode getCameraMode() const {return cameraMode;}
		
		
		void setNameDisplayDistance(float value);
		float getNameDisplayDistance() const		{return nameDisplayDistance;}

		void setHealthDisplayDistance(float value);
		float getHealthDisplayDistance() const		{return healthDisplayDistance;}

		void setCameraMaxZoomDistance(float _cameraMaxZoomDistance);
		float getCameraMaxZoomDistance() const		{return cameraMaxZoomDistance;}

		void setCameraMinZoomDistance(float _cameraMinZoomDistance);
		float getCameraMinZoomDistance() const		{return cameraMinZoomDistance;}

		void setDevEnableMouseLockOption(bool setting);
		bool getDevEnableMouseLockOption() const		{return enableMouseLockOption;}
		void setDevTouchCameraMode(StarterPlayerService::DeveloperTouchCameraMovementMode setting);
		StarterPlayerService::DeveloperTouchCameraMovementMode getDevTouchCameraMode() const		{return touchCameraMovementMode;}
		void setDevComputerCameraMode(StarterPlayerService::DeveloperComputerCameraMovementMode setting);
		StarterPlayerService::DeveloperComputerCameraMovementMode getDevComputerCameraMode() const		{return computerCameraMovementMode;}
		void setDevCameraOcclusionMode(StarterPlayerService::DeveloperCameraOcclusionMode setting);
		StarterPlayerService::DeveloperCameraOcclusionMode getDevCameraOcclusionMode() const		{return cameraOcclusionMode;}

		void setDevTouchMovementMode(StarterPlayerService::DeveloperTouchMovementMode setting);
		StarterPlayerService::DeveloperTouchMovementMode getDevTouchMovementMode() const		{return touchMovementMode;}
		void setDevComputerMovementMode(StarterPlayerService::DeveloperComputerMovementMode setting);
		StarterPlayerService::DeveloperComputerMovementMode getDevComputerMovementMode() const		{return computerMovementMode;}

		void setAccountAge(int value);
		int getAccountAge() const {return accountAge;}
	
		void updateSimulationRadius(float value);
		float getSimulationRadius() const {return simulationRadius;}
		void setSimulationRadius(float value);

		float getMaxSimulationRadius() const {return maxSimulationRadius;}
		void setMaxSimulationRadius(float value);

		CoordinateFrame getCloudEditCameraCoordinateFrame() const { return cloudEditCameraCFrame; }
		void setCloudEditCameraCoordinateFrame(const CoordinateFrame& cframe);

		std::string getOsPlatform() const {return osPlatform;}

		void rebuildBackpack();			// Copy the backpack from the StartPack service
		void rebuildGui();				// Copy the gui from the StarterGui service
		void rebuildPlayerScripts();	// Copy the LocalScripts from the StarterPlayer/PlayerScripts service

		void createPlayerGui();

		Backpack* getPlayerBackpack();
		const Backpack* getConstPlayerBackpack() const;
        
        void luaJumpCharacter();
        void luaMoveCharacter(Vector2 walkDirection, float maxWalkDelta);

		void move(Vector3 walkVector, bool relativeToCamera);

		float distanceFromCharacter(Vector3 point);

		void luaLoadCharacter(bool inGame);
		void loadCharacter(bool inGame, std::string preferedSpawnName);

		void removeCharacter();
		void removeCharacterAppearance();
		void removeCharacterAppearanceScript();

		void loadCharacterAppearance(bool blockingCall);
		void loadCharacterAppearanceScript(shared_ptr<Instance> asset);

		static void onLocalPlayerNotIdle(RBX::ServiceProvider* serviceProvider);

		void renderDPhysicsRegion(Adorn* adorn);
		void renderStreamedRegion(Adorn* adorn);
        void renderPartMovementPath(Adorn* adorn);

		static bool physicsOutBandwidthExceeded(const RBX::Instance* context);
		static double getNetworkBufferHealth(const RBX::Instance* context);
		void reportStat(std::string stat);

		void addToLoadingInstances(int newInstances) { loadingInstances += newInstances; }
		void removeFromLoadingInstances(int oldInstances) { loadingInstances -= oldInstances; }
		int getLoadingInstances() { return loadingInstances; }

		bool getAppearanceDidLoad() const { return appearanceDidLoad; }
		bool getAppearanceDidLoadNonConst() { return appearanceDidLoad; }
		void setAppearanceDidLoad(bool value);
		bool getCharacterAppearanceLoaded() const { return characterAppearanceLoaded; }
		void setCharacterAppearanceLoaded(bool value);
		
		void onFriendStatusChanged(shared_ptr<Instance> player, FriendService::FriendStatus friendStatus);
		FriendService::FriendStatus getFriendStatus(shared_ptr<Instance> player);
		void isFriendsWith(int otherUserId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isBestFriendsWith(int otherUserId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void isInGroup(int groupId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getRankInGroup(int groupId, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getRoleInGroup(int groupId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void getFriendsOnline(int maxFriends, boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction, boost::function<void(std::string)> errorFunction); 

		void loadChatInfo();
		
		// should be called with at least data model read lock
		bool isChatInfoValid() const;
		ChatFilterType getChatFilterType() const;

		// for testing only
		void setChatInfo(ChatFilterType filterType);

		void kick(std::string msg);
		void handleTeleportInternalSignal(TeleportService::TeleportState teleportState, shared_ptr<const Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI);
		void handleTeleportSignalBackend(TeleportService::TeleportState teleportState);
		void setForceEarlySpawnLocationCalculation();
		bool calculatesSpawnLocationEarly() const;

		const RakNet::SystemAddress& getRemoteAddress() const;
		const SystemAddress getRemoteAddressAsRbxAddress() const;
		void setRemoteAddress(const RakNet::SystemAddress& address);

		void onTeleport(TeleportService::TeleportState teleportState, int placeId, std::string instanceIdOrSpawnName);
        void onTeleportInternal(TeleportService::TeleportState teleportState, shared_ptr<const Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI = shared_ptr<Instance>());
		bool getTeleported() const { return teleported; }
		bool getTeleportedIn() const { return teleportedIn; }
		void setTeleportedIn(bool value);

		int getFollowUserId() const { return followUserId; }
		void setFollowUserId(int followUserId) { this->followUserId = followUserId; }

		void loadStarterGear();
		void onCharacterDied();

		/*override*/ void destroy();

	private:


		void setupHumanoid(shared_ptr<Humanoid> humanoid);
		void characterChildAdded(shared_ptr<Instance> child);

		void doPeriodicIdleCheck();
		void checkContextReadyToSpawnCharacter();
		static void calculateNextSpawnLocationHelper(
				weak_ptr<Player>& weakPlayer, const ServiceProvider* serviceProvider);
		void calculateNextSpawnLocation(const ServiceProvider* serviceProvider);
		SpawnData calculateSpawnLocation(const std::string& preferedSpawnName);

		static void loadChatInfoInternal(weak_ptr<Player> weakPlayer);

		Time lastActivityTime;
		
		MembershipType membershipType;
		int accountAge;

		bool chatInfoHasBeenLoaded;
		ChatFilterType chatFilterType;

		shared_ptr<RBX::Mouse> mouse;
		RBX::Camera::CameraMode cameraMode;

		float nameDisplayDistance;
		float healthDisplayDistance;
		float cameraMaxZoomDistance;
		float cameraMinZoomDistance;
		bool enableMouseLockOption;
		RBX::StarterPlayerService::DeveloperTouchCameraMovementMode touchCameraMovementMode;
		RBX::StarterPlayerService::DeveloperComputerCameraMovementMode computerCameraMovementMode;
		RBX::StarterPlayerService::DeveloperCameraOcclusionMode cameraOcclusionMode;
		RBX::StarterPlayerService::DeveloperTouchMovementMode touchMovementMode;
		RBX::StarterPlayerService::DeveloperComputerMovementMode computerMovementMode;

		bool copiedGuiOnce;
	};

}  // namespace
}  // namespace
