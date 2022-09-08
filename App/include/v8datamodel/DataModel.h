/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "gui/GUI.h"
#include "V8DataModel/DataModelJob.h"
#include "V8DataModel/PhysicsInstructions.h"
#include "V8DataModel/GuiBuilder.h"
#include "v8datamodel/Game.h"
#include "V8DataModel/InputObject.h"
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Util/GameMode.h"
#include "Util/Runstateowner.h"
#include "Util/InsertMode.h"
#include "Util/Region2.h"
#include "Util/IMetric.h"
#include "Util/HeapValue.h"
#include "Security/FuzzyTokens.h"
#include <boost/array.hpp>
#include <boost/optional.hpp>
#include <boost/unordered_set.hpp>

class XmlElement;

namespace RBX {

class Fonts;
class GuiRoot;
class GuiItem;
class ContentProvider;
class Hopper;
class PlayerHopper;
class StarterPackService;
class StarterGuiService;
class StarterPlayerService;
class CoreGuiService;
class Workspace;
class Adorn;
class Region2;
class UserInputService;
class ContextActionService;
class GuiObject;

extern const char *const sDataModel;

static inline void robloxScriptModifiedCheck(RBX::Security::Permissions perm)
{
#ifndef RBX_STUDIO_BUILD
    RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript);
    if (perm != RBX::Security::RobloxScript)
    {
        throw std::runtime_error("");
    }
#endif
}

class DataModel 
	: public IMetric
	, public IDataState
	, public VerbContainer
	, public Diagnostics::Countable<DataModel>
	, public DataModelArbiter
    , public DescribedNonCreatable<DataModel, ServiceProvider, sDataModel>
{
public:

	enum CreatorType
	{
		CREATOR_USER  = 0,
		CREATOR_GROUP = 1
	};
	//genre == GENRE_ALL || (allowedGenres & (1 << (genre-1))) != 0;
	enum Genre
	{
		GENRE_ALL			= 0,
		GENRE_TOWN_AND_CITY = 1,
		GENRE_FANTASY		= 2,
		GENRE_SCI_FI		= 3,
		GENRE_NINJA			= 4,
		GENRE_SCARY			= 5,
		GENRE_PIRATE		= 6,
		GENRE_ADVENTURE		= 7,
		GENRE_SPORTS		= 8,
		GENRE_FUNNY			= 9,
		GENRE_WILD_WEST		= 10,
		GENRE_WAR			= 11,
		GENRE_SKATE_PARK	= 12,
		GENRE_TUTORIAL		= 13,
	};
	enum GearGenreSetting
	{
		GEAR_GENRE_ALL		= 0,
		GEAR_GENRE_MATCH	= 1,
	};

	//(allowedGearTypes & (1 << gearType)) != 0;
	enum GearType
	{
		GEAR_TYPE_MELEE_WEAPONS			= 0,
		GEAR_TYPE_RANGED_WEAPONS		= 1,
		GEAR_TYPE_EXPLOSIVES			= 2,
		GEAR_TYPE_POWER_UPS				= 3,
		GEAR_TYPE_NAVIGATION_ENHANCERS	= 4,
		GEAR_TYPE_MUSICAL_INSTRUMENTS	= 5,
		GEAR_TYPE_SOCIAL_ITEMS			= 6,
		GEAR_TYPE_BUILDING_TOOLS		= 7,
		GEAR_TYPE_PERSONAL_TRANSPORT	= 8,
	};

	enum RequestShutdownResult
	{
		CLOSE_NOT_HANDLED		= 0,
		CLOSE_REQUEST_HANDLED	= 1,
		CLOSE_LOCAL_SAVE		= 2,
		CLOSE_NO_SAVE_NEEDED	= 3,
	};

	void postCreate();
		
	static rbx::atomic<int> count;
	std::auto_ptr<RBX::Verb> lockVerb;

	rbx::signal<void()> screenshotSignal;
	rbx::signal<void(const std::string &)> screenshotReadySignal;
	rbx::signal<void(bool)> screenshotUploadSignal;

	rbx::signal<void(bool)> graphicsQualityShortcutSignal;

	rbx::signal<void()> allowedGearTypeChanged;
	rbx::signal<void(const shared_ptr<InputObject>& event)> InputObjectProcessed;

	rbx::signal<void()> workspaceLoadedSignal;
	rbx::signal<void()> gameLoadedSignal;

	Genre getGenre() const { return genre; }
	void setGenre(Genre genre);

	GearGenreSetting getGearGenreSetting() const { return gearGenreSetting; }
	void setGear(GearGenreSetting gearGenreSetting, int allowedGearTypes);

	void setVIPServerId(std::string value);
	std::string getVIPServerId() const;
	void setVIPServerOwnerId(int value);
	int getVIPServerOwnerId() const;

	void setRenderGuisActive(bool value) { renderGuisActive = value; }

	bool isGearTypeAllowed(GearType gearType);

	static void setLoaderFunction(boost::function<void(RBX::DataModel*)> loader) { loaderFunc = loader; }
	
	void loadPlugins();
	
	struct MouseStats
	{
		RunningAverageTimeInterval<> osMouseMove;
		RunningAverageTimeInterval<> mouseMove;

		MouseStats()
			:osMouseMove(0.5)
			,mouseMove(0.5)
		{
		}
	};
	MouseStats mouseStats;

	bool getSharedSuppressNavKeys() const { return (game ? game->getSuppressNavKeys() : suppressNavKeys); } 
	void setGame(RBX::Game* newGame) { game = newGame; }

    bool getIsShuttingDown() { return isShuttingDown; }
    void setIsShuttingDown(bool value) { isShuttingDown = value; }

private:			
	typedef DescribedNonCreatable<DataModel, ServiceProvider, sDataModel> Super;

	ThrottlingHelper savePlaceThrottle;

	// TODO: turn some of these into non-essential Services? Or at least, see if we can
	// get rid of the direct references here...

	shared_ptr<CoreGuiService>			coreGuiService;
    shared_ptr<ContextActionService>	contextActionService;

	shared_ptr<StarterPackService>			starterPackService;
	shared_ptr<StarterGuiService>			starterGuiService;
	shared_ptr<StarterPlayerService>		starterPlayerService;
	shared_ptr<RunService>					runService;
    shared_ptr<UserInputService>            userInputService;
	shared_ptr<Workspace>					workspace;		
	
	shared_ptr<GuiRoot>					guiRoot;

	bool								forceArrowCursor;
	virtual GuiResponse					processAccelerators(const shared_ptr<InputObject>& event);
    virtual GuiResponse                 processCameraCommands(const shared_ptr<InputObject>& event);
    
	std::string							uiMessage;		// short term hack - will improve
	int									drawId;			// flag to debug extra draws
	IMetric*							tempMetric;		// for output in ctrl-F1 message - only used during render call
	IMetric*							networkMetric;	// for output in ctrl-F1 message - only used during render call

	volatile bool						dirty;			// For IDataState
	bool                                isInitialized;	// Block gui events when closing - per crash reports on 12/05/07
	bool								isContentLoaded;
    volatile bool                       isShuttingDown;

    boost::mutex hackFlagSetMutex;
	boost::unordered_set<unsigned int> hackFlagSet;

	PhysicsInstructions					physicsInstructions;

	int									numPartInstances;
	int									numInstances;
	int									shutdownRequestedCount;
	int									physicsStepID;
	int									totalWorldSteps;
	bool								isGameLoaded;

    bool                                forceR15;
	bool								canRequestUniverseInfo;
	bool								universeDataRequested;
	void								requestGameStartInfo();

	bool								isPersonalServer;

    bool                                networkStatsWindowsOn;

	bool                                renderGuisActive;

	RBX::Game*							game;

	Time                                dataModelInitTime;
    
    TextureProxyBaseRef                 renderCursor;

	void initializeContents(bool startHeartbeat);

	void doDataModelStep(float timeInterval);
	bool updatePhysicsInstructions(Network::GameMode gameMode);
	bool onlyJobsLeftForThisArbiterAreGenericJobs();

	static boost::function<void(RBX::DataModel*)> loaderFunc;

	bool								areCoreScriptsLoaded;

	std::auto_ptr<std::istream> loadAssetIdIntoStream(int assetID);
public:
	static bool BlockingDataModelShutdown;

    static unsigned int perfStats; // another bitmask used to record detected hacks

	std::string jobId;

	static unsigned int sendStats; // legacy bitmask used to record detected hacks

	std::string getJobId() const { return jobId; };

	Time getDataModelInitTime() const { return dataModelInitTime; }

	static std::string hash; // set to the hash of the client exe

	//-----------------------------------
	// Concurrency violation detection
	// TODO: Merge with readwrite_concurrency_catcher class
	class scoped_write_request
	{
		DataModel* const dataModel;
	public:
		// Place this code around tasks that write to a DataModel
		scoped_write_request(Instance* context);
		~scoped_write_request();
	};
	class scoped_read_request
	{
		DataModel* const dataModel;
	public:
		// Place this code around tasks that read a DataModel
		scoped_read_request(Instance* context);
		~scoped_read_request();
	};	
	class scoped_write_transfer
	{
		DataModel* const dataModel;
		unsigned int oldWritingThread;
	public:
		// Place this code around tasks that write to a DataModel and expect some other task to hold the write lock for them
		scoped_write_transfer(Instance* context);
		~scoped_write_transfer();
	};
	friend class scoped_write_request;
	friend class scoped_read_request;
	volatile long write_requested;
	volatile long read_requested;
	volatile DWORD writeRequestingThread;
    
	bool currentThreadHasWriteLock() const;
	static bool currentThreadHasWriteLock(Instance* context);

    void setNetworkStatsWindowsOn(bool val) { networkStatsWindowsOn = val; }

	friend class Game;

	//
	//-----------------------------------

	// It is important for addHackFlag to be inlined for security reasons.
	// If this function is not inlined, it provides a single point of attack.
	// Also inlining the code (along with VMProtect) provides more obscurity.
#ifdef WIN32
	__forceinline
#else
	inline
#endif
	void addHackFlag(unsigned int flag) {
		boost::mutex::scoped_lock l(hackFlagSetMutex);
		hackFlagSet.insert(flag);
        sendStats |= flag;
	}

#ifdef WIN32
	__forceinline
#else
	inline
#endif
	void removeHackFlag(unsigned int flag) {
		boost::mutex::scoped_lock l(hackFlagSetMutex);
		hackFlagSet.erase(flag);
        sendStats &= ~flag;
	}

#ifdef WIN32
	__forceinline
#else
	inline
#endif
	bool isHackFlagSet(unsigned int flag) {
		boost::mutex::scoped_lock l(hackFlagSetMutex);
		return hackFlagSet.find(flag) != hackFlagSet.end() || (sendStats & flag);
	}

	unsigned int allHackFlagsOredTogether();

	weak_ptr<GuiObject> getMouseOverInteractable() const { return mouseOverInteractable; }
	bool getMouseOverGui() const { return mouseOverGui; }
    void processWorkspaceEvent(const shared_ptr<InputObject>& event);

	bool isClosed() const { return !isInitialized; }
	void setSuppressNavKeys(bool value) { suppressNavKeys = value; }

	static bool throttleAt30Fps;							// for debugging/benchmarking - default is false;

	// This event is fired anytime anything in the DataModel changes
	rbx::signal<void(shared_ptr<Instance>, const Reflection::PropertyDescriptor*)> itemChangedSignal;

	void clearContents(bool resettingSimulation);
    
    void setInitialScreenSize(RBX::Vector2 newScreenSize);

	// submits a task to be executed in a thread-safe manner
	typedef boost::function<void(DataModel*)> Task;
	void submitTask(Task task, DataModelJob::TaskType taskType);
	shared_ptr<const Reflection::ValueArray> getJobsInfo();

	void setCreatorID(int creatorID, CreatorType creatorType);
	int getCreatorID() const { return creatorID; }
	CreatorType getCreatorType() const { return creatorType; }

	int getPlaceID() const { return placeID; }
	int getPlaceIDOrZeroInStudio();
	void setPlaceID(int placeID, bool robloxPlace);

	void setGameInstanceID(std::string gameInstanceID) { this->gameInstanceID = gameInstanceID; }
	std::string getGameInstanceID() { return this->gameInstanceID; }

	int getUniverseId() { return universeId; }
	void setUniverseId(int uId);

	bool isStudio() const { return runningInStudio; }
	void setIsStudio(bool runningInStudio);

	bool isRunMode() const { return isStudioRunMode; }
	void setIsRunMode(bool value);

	int getPlaceVersion() const { return placeVersion; }
	void setPlaceVersion(int placeVersion);

	bool getIsPersonalServer() const { return isPersonalServer; }
	void setIsPersonalServer(bool value) { isPersonalServer = value; }

	bool getIsGameLoaded() { return isGameLoaded; }
	void setIsGameLoaded(bool value)
	{ 
		isGameLoaded = value;
		if(isGameLoaded)
			gameLoadedSignal();
	}
	void gameLoaded();

    bool getForceR15() const { return forceR15; }
    void setForceR15(bool v);
	boost::promise<void> universeDataLoaded;
	bool getUniverseDataRequested() const { return universeDataRequested; }
	void clearUniverseDataRequested() { universeDataRequested = false; }
	void setCanRequestUniverseInfo(bool value);

	void loadCoreScripts(const std::string &altStarterScript = "");


    int getNumPartInstances() const { return numPartInstances; }
    int getNumInstance() const { return numInstances; }

	void checkFetchExperimentalFeatures();

	bool canSaveLocal() const;
	void saveToRoblox(boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
	void completeShutdown(bool saveLocal);
	boost::function<bool()>				requestShutdownCallback;

	typedef boost::function<void(Reflection::AsyncCallbackDescriptor::ResumeFunction, Reflection::AsyncCallbackDescriptor::ErrorFunction)> CloseCallback;
	CloseCallback onCloseCallback;
	void onCloseCallbackChanged(const CloseCallback&);

	//Returns true if the shutdown is being handled by Lua, false to use the System Level Prompts
	RequestShutdownResult requestShutdown(bool useLuaShutdownForSave=true);

	// for perf stats reporting:
	// width of window to measure. Set to zero to turn off and release memory.
	void setJobsExtendedStatsWindow(double seconds);
	shared_ptr<const Reflection::ValueArray> getJobsExtendedStats();
	double getJobTimePeakFraction(std::string jobname, double greaterThan);
	double getJobIntervalPeakFraction(std::string jobname, double greaterThan);

	class GenericJob;

	class LegacyLock : boost::noncopyable
	{
		struct Implementation;
		boost::scoped_ptr<Implementation> const implementation;
	public:
        static int mainThreadId;
        
		class Impersonator : boost::noncopyable
		{
			// Use this class to override a Lock. This must only be used
			// when you know that a different thread is delegating the lock
			// to you.
			// TODO: For added safety we could write a stack-based Delegator
			// object that would be constructed at the source thread. The
			// Delegator would issue a token to be used by the thread that
			// uses the Impersonator
		public:
			Impersonator(shared_ptr<DataModel> dataModel, DataModelJob::TaskType taskType);
			~Impersonator();
		private:
			DataModel::GenericJob* previousJob;
		};
		LegacyLock(shared_ptr<DataModel> dataModel, DataModelJob::TaskType taskType);
		LegacyLock(DataModel* dataModel, DataModelJob::TaskType taskType);
		~LegacyLock();

		static bool hasLegacyLock(DataModel* dataModel);
	};

	// Please call these to construct and release a datamodel
	static shared_ptr<DataModel> createDataModel(bool startHeartbeat, RBX::Verb* lockVerb, bool shouldShowLoadingScreen);
    
	static void closeDataModel(shared_ptr<DataModel> dataModel);
    
	~DataModel();
	DataModel(RBX::Verb* lockVerb);
	static DataModel* get(Instance* context) ;
	static const DataModel* get(const Instance* context) ;

	void loadGame(int assetID);
	void loadWorld(int assetID);

	void loadContent(ContentId contentId);
	void processAfterLoad();
	rbx::signal<void()> saveFinishedSignal;


    class SerializationException : public std::runtime_error
    {
        public:

            SerializationException(const std::string& response) 
                : std::runtime_error(response) {}
    };

    void save(ContentId contentId);
	static bool canSave(const RBX::Instance* instance);

	bool getRemoteBuildMode();
	void setRemoteBuildMode(bool remoteBuildMode);

	void setServerSaveUrl(std::string url);
	void serverSave();

	bool serverSavePlace(const SaveFilter saveFilter, boost::function<void(bool)> resumeFunction = NULL, boost::function<void(std::string)> errorFunction = NULL);
	shared_ptr<std::stringstream> serializeDataModel(const Instance::SaveFilter saveFilter = Instance::SAVE_ALL);

	virtual void savePlaceAsync(const SaveFilter saveFilter, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

	void httpGetAsync(std::string url,  boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
	void httpPostAsync(std::string url, std::string data, std::string optionalContentType, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
	
	std::string httpGet(std::string url, bool synchronous);
	std::string httpPost(std::string url, std::string data, bool synchronous, std::string optionalContentType);

	void reportMeasurement(std::string id, std::string key1, std::string value1, std::string key2, std::string value2);
	void luaReportGoogleAnalytics(std::string category, std::string action, std::string label, int value);

	void close();

#if defined(RBX_STUDIO_BUILD) || defined(RBX_RCC_SECURITY) || defined(RBX_TEST_BUILD)
	shared_ptr<const Instances> fetchAsset(ContentId contentId);
#endif

	void raiseClose();

	static void TakeScreenshotTask(weak_ptr<RBX::DataModel> weakDataModel);
	static void ScreenshotReadyTask(weak_ptr<RBX::DataModel> weakDataModel, const std::string &filename);

	// true: finished, false: started
	static void ScreenshotUploadTask(weak_ptr<RBX::DataModel> weakDataModel, bool finished);

	static void ShowMessage(weak_ptr<RBX::DataModel> weakDataModel, int slot, const std::string &message, double duration);

	void setScreenshotSEOInfo(std::string str);
	void setVideoSEOInfo(std::string str);
	std::string getScreenshotSEOInfo();
	std::string getVideoSEOInfo() { return videoSEOInfo; }
	bool isScreenshotSEOInfoSet() { return screenshotSEOInfo != ""; }
	bool isVideoSEOInfoSet() { return videoSEOInfo != ""; }

	void addCustomStat(std::string name, std::string value);
	void removeCustomStat(std::string str);
	void writeStatsSettings();

	///////////////////////////////////////////////////////////////////////
	//
	// Command Processing
	int numChatOptions() {return 4;}		// To Do - make this some kind of dynamic loading

	void startCoreScripts(bool buildInGameGui, const std::string &altStarterScript = "");

	void setGuiTargetInstance(shared_ptr<Instance> newTargetInstance) {guiTargetInstance = newTargetInstance;}

	bool processInputObject(shared_ptr<InputObject> event);
	GuiRoot* getGuiRoot() {return guiRoot.get();}

	void setForceArrowCursor(bool value) { forceArrowCursor = value;}

	///////////////////////////////////////////////////////////////////////
	// IDataState
	virtual void setDirty(bool dirty) { this->dirty = dirty; }

	// Thread-safe:
	virtual bool isDirty() const { return dirty; }

	virtual std::string arbiterName() { return this->getName(); }

	////////////////////////////////////////////////////////////////////////////////////
	//
	// Workspace Stuff / Running Stuff
	//
	Workspace* getWorkspace() const			{return workspace.get();}

	float physicsStep(float timeInterval, double dt, double dutyDt, int numThreads);
	void renderStep(float timeIntervalSeconds);
	
	////////////////////////////////////////////////////////////////////////////////////
	//
	// DataModelArbiter
	//
	/*implement*/ int getNumPlayers() const;

	///////////////////////////////////////////////////////////////////////////
	//
	// timing state
	//
	double getGameTime() const;			// i.e. game time
	double getSmoothFps() const;
	
	///////////////////////////////////////////////////////////////////////////
	//
	// IMetric
	//
    //
    /*override*/ virtual std::string getMetric(const std::string& metric) const;
	/*override*/ virtual double getMetricValue(const std::string& metric) const;

	std::string getUpdatedMessageBoxText();
	void setNetworkMetric(IMetric* metric);

	///////////////////////////////////////////////////////////////////////////
	//
	// Rendering
	//
	virtual void renderPass2d(Adorn* adorn, IMetric* graphicsMetric); 
	void renderPass3dAdorn(Adorn* adorn); 
	void setUiMessage(std::string message);
	void clearUiMessage(); 
	void setUiMessageBrickCount();
	void toggleToolsOff();

	virtual void renderMouse(Adorn* adorn);
	ContentId getRenderMouseCursor();

	void computeGuiInset(Adorn* adorn);
	void renderPlayerGui(Adorn* adorn);
	void renderGuiRoot(Adorn* adorn);
	static void HttpHelper(std::string* response, std::exception* exception, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);


	GuiBuilder getGuiBuilder() { return guiBuilder; }

	TaskScheduler::Arbiter* getSyncronizationArbiter();

	bool uploadPlace(const std::string& uploadUrl, const SaveFilter saveFilter, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

	GuiResponse	processPlayerGui(const shared_ptr<InputObject>& event);

	static void processHttpRequestResponseOnLock(DataModel *dataModel, std::string* response, std::exception* exception, boost::function<void(shared_ptr<std::string>,shared_ptr<std::exception> exception)> onLockAcquired);

protected:
    static void doDataModelSetup(shared_ptr<DataModel> dataModel, bool startHeartbeat, bool shouldShowLoadingScreen);
    
	void internalSave(ContentId contentId);
	void internalSaveAsync(ContentId contentId, boost::function<void(bool)> resumeFunction);

	/////////////////////////////////////////////////////////////////////
	//
	// Instance overrides
	/*override*/ bool askAddChild(const Instance* instance) const;
	/*override*/ void onChildAdded(Instance* child);

	/*override*/ void onChildChanged(Instance* instance, const PropertyChanged& event);
	/*override*/ void onDescendantAdded(Instance* instance);
	/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
	void onRunTransition(RunTransition event);

    bool processEvent(const shared_ptr<InputObject>& event);
	GuiResponse processProfilerEvent(const shared_ptr<InputObject>& event);

	weak_ptr<Instance> guiTargetInstance;
	bool mouseOverGui;
	weak_ptr<GuiObject> mouseOverInteractable;

	bool getSuppressNavKeys() const { return suppressNavKeys; } 
private:
    rbx::signals::scoped_connection unbindResourceSignal;
    void onUnbindResourceSignal();

	bool suppressNavKeys;
	static std::string doHttpGet(const std::string& url);
	static void doHttpGet(const std::string& url, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);
	static std::string doHttpPost(const std::string& url, const std::string& data, const std::string& contentType);
	static void doHttpPost(const std::string& url, const std::string& data, const std::string& contentType, boost::function<void(std::string)> resumeFunction, boost::function<void(std::string)> errorFunction);


	static void doCloseDataModel(shared_ptr<DataModel> dataModel);
	
	void AsyncUploadPlaceResponseHandler(std::string* response, std::exception* exception, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
	bool uploadPlaceReturn(const bool succeeded, const std::string error, boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);

	bool canRenderMouse();

	GuiResponse processDevGamepadEvent(const shared_ptr<InputObject>& event);
	GuiResponse processCoreGamepadEvent(const shared_ptr<InputObject>& event);
	GuiResponse processGuiTarget(const shared_ptr<InputObject>& event);

	typedef boost::array<shared_ptr<GenericJob>, DataModelJob::TaskTypeMax> GenericJobs;
	RBX::mutex genericJobsLock;
	GenericJobs genericJobs;
	RBX::mutex debugLock;
	shared_ptr<GenericJob> tryGetGenericJob(DataModelJob::TaskType type);
	shared_ptr<GenericJob> getGenericJob(DataModelJob::TaskType type);

	std::map<std::string,int> dataModelReportingData;
	void traverseDataModelReporting(shared_ptr<Instance> child);

	Instance* getLightingDeprecated() const;
	static Reflection::RefPropDescriptor<DataModel, Instance> prop_lighting;

	bool remoteBuildMode;
	std::string serverSaveUrl;

	std::string screenshotSEOInfo;
	std::string videoSEOInfo;
	HeapValue<int> placeID;
	std::string gameInstanceID;
	int universeId;
	bool runningInStudio;
	bool isStudioRunMode;
	bool checkedExperimentalFeatures;

	int placeVersion;
	
	HeapValue<int> creatorID;
	CreatorType creatorType;

	Genre genre;
	GearGenreSetting gearGenreSetting;
	unsigned allowedGearTypes;

	std::string vipServerId;
	int vipServerOwnerId;

	GuiBuilder guiBuilder;
};

} // namespace
