#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Util/HeapValue.h"

#ifdef _WIN32
#if defined(_NOOPT) || defined(_DEBUG) || defined(RBX_TEST_BUILD)
//#define DEBUG_TELEPORT
#endif
#endif

namespace RBX {

	class DataModel;
	class TeleportCallback;

	extern const char *const sTeleportService;

	class TeleportService 
		: public DescribedNonCreatable<TeleportService, Instance, sTeleportService>,
		public Service
	{
	private:
		static TeleportCallback *_callback;
		static std::string _spawnName;
		static std::string _baseUrl;
		static std::string _browserTrackerId;
        static bool _waitingForUserInput;
        bool requestingTeleport;
        std::string url;
        RBX::Timer<RBX::Time::Fast> retryTimer;
        boost::scoped_ptr<boost::thread> teleportThread;

        static HeapValue<int> previousPlaceId;
		static HeapValue<int> previousCreatorType;
		static HeapValue<int> previousCreatorId;
		static bool teleported;
		static boost::shared_ptr<Instance> customTeleportLoadingGui;
		static Reflection::Variant dataTable;
		boost::shared_ptr<Instance> tempCustomTeleportLoadingGui;

		typedef boost::unordered_map<std::string, Reflection::Variant> SettingsMap;
		static SettingsMap settingsTable;

    public:
        enum TeleportState
        {
            TeleportState_RequestedFromServer = 0,
            TeleportState_Started,
            TeleportState_WaitingForServer,
            TeleportState_Failed,
            TeleportState_InProgress,
            TeleportState_NumStates
        };

		enum TeleportType
		{
			TeleportType_ToPlace = 0,
			TeleportType_ToInstance,
			TeleportType_ToReservedServer
		};

        boost::function<bool(std::string,int,std::string)> confirmationCallback;
		boost::function<void(std::string)> showErrorCallback;
        bool customizedTeleportUI;

		TeleportService();

		static void SetCallback(TeleportCallback *callback);
		static void SetBaseUrl(const char *baseUrl);
		static void SetBrowserTrackerId(const std::string& browserTrackerId);
		
		void TeleportImpl(shared_ptr<const Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI = shared_ptr<Instance>());
        void TeleportCancel();
        void TeleportThreadImpl(shared_ptr<const Reflection::ValueTable> teleportInfo);

        void ServerTeleport(shared_ptr<Instance> characterOrPlayerInstance, shared_ptr<Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI);
		void Teleport(int placeId, shared_ptr<Instance> characterOrPlayerInstance, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI);
		void TeleportToSpawnByName(int placeId, std::string spawnName, shared_ptr<Instance> characterOrPlayerInstance, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI);
		void TeleportToPrivateServer(int placeId, std::string reservedServerAccessCode, shared_ptr<const Instances> players, std::string spawnName, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI);

		void ProcessGrantAccessSuccess(shared_ptr<const Instances> players, shared_ptr<Reflection::ValueTable> teleportInfo, shared_ptr<Instance> customLoadingGUI, std::string response);
		void ProcessGrantAccessError(shared_ptr<const Instances> players, shared_ptr<Reflection::ValueTable> teleportInfo, std::string error);

		void ReserveServer(int placeId, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction); 
		void ProcessReserveServerResultsSuccess(std::string response, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void ProcessReserveServerResultsError(std::string error, boost::function<void(std::string)> errorFunction);

		void ProcessGetPlayerPlaceInstanceResultsSuccess(std::string response, 
			boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void ProcessGetPlayerPlaceInstanceResultsError(std::string error, boost::function<void(std::string)> errorFunction);
	
        void GetPlayerPlaceInstanceAsync(int playerId, boost::function<void(shared_ptr<const Reflection::Tuple>)> resumeFunction, boost::function<void(std::string)> errorFunction);
        void TeleportToPlaceInstance(int placeId, std::string instanceId, shared_ptr<Instance> characterOrPlayerInstance, std::string spawnName, Reflection::Variant teleportData, shared_ptr<Instance> customLoadingGUI);

		static std::string & GetSpawnName();
		static void ResetSpawnName();

		void SetTeleportSetting(std::string key, Reflection::Variant value);
		Reflection::Variant GetTeleportSetting(std::string key);

        bool attemptingTeleport()
        {
            return requestingTeleport;
        }

		static Reflection::EventDesc<TeleportService, void(shared_ptr<Instance>, Reflection::Variant)> event_playerArrivedFromTeleport;
		void sendPlayerArrivedFromTeleportSignal(shared_ptr<Instance> loadingGui, Reflection::Variant teleportDataTable) { playerArrivedFromTeleportSignal(loadingGui, teleportDataTable); }
		rbx::signal<void(shared_ptr<Instance>, Reflection::Variant)> playerArrivedFromTeleportSignal;
        static int getPreviousPlaceId() {return previousPlaceId;}
		static int getPreviousCreatorType() {return previousCreatorType;}
		static int getPreviousCreatorId() {return previousCreatorId;}
		static boost::shared_ptr<Instance> getCustomTeleportLoadingGui()	{return customTeleportLoadingGui;}
		static void setCustomTeleportLoadingGui(shared_ptr<Instance> value) {customTeleportLoadingGui = value;}
		boost::shared_ptr<Instance> getTempCustomTeleportLoadingGui()		{return tempCustomTeleportLoadingGui;}
		void setTempCustomTeleportLoadingGui(shared_ptr<Instance> value)	{tempCustomTeleportLoadingGui = value;}
		static bool didTeleport() {return teleported;}
		static Reflection::Variant getDataTable() {return dataTable;}
		Reflection::Variant getLocalPlayerTeleportData();
	};
}
