#pragma once

#include "V8Tree/Service.h"
#include "Util/ProtectedString.h"
#include "util/runstateowner.h"
#include "Script/IScriptFilter.h"
#include "script/ThreadRef.h"
#include "script/ExitHandlers.h"
#include "Security/SecurityContext.h"
#include "Util/AsyncHttpQueue.h"
#include "util/RunningAverage.h"
#include "rbx/RunningAverage.h"

#define BOOST_DATE_TIME_NO_LIB
#include "boost/date_time/posix_time/posix_time.hpp"

struct lua_State;
struct lua_Debug;

LOGGROUP(ScriptContext)
LOGGROUP(ScriptContextRemove)
LOGGROUP(ScriptContextAdd)
LOGGROUP(ScriptContextClose)

namespace RBX
{
	class LuaSourceContainer;
	class LuaAllocator;
	class LibraryService;
	class ModuleScript;
	class ModelInstance;
	namespace Stats
	{
		class Item;
	}
	namespace Lua
	{
		class YieldingThreads;
		class WeakFunctionRef;
	}
	namespace Network
	{
		class Player;
	}
	class BaseScript;
	class CoreScript;
	class ScriptStats;
	class LuaStatsItem;

	void registerScriptDescriptors();

	extern const char* const sScriptContext;
	class ScriptContext 
		: public DescribedCreatable<ScriptContext, Instance, sScriptContext, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
		, public IScriptFilter
	{
		friend class LuaStatsItem;
		friend class GcJob;
		friend class WaitingScriptsJob;

	public:
		static const int hookCount;

		struct ScriptStartOptions
		{
			struct LuaSyntaxError : std::runtime_error
			{
				LuaSyntaxError(int lineNumber, std::exception& source)
					:std::runtime_error(source.what())
					,lineNumber(lineNumber)
				{
				}
				int lineNumber;
			};

			RBX::Security::Identities identity;
			Scripts::Continuations continuations;
			boost::function<std::string(const std::string&)> filter;	// may throw a LuaSyntaxError

			ScriptStartOptions():identity(RBX::Security::GameScript_)
			{
			}
		};

	private:
		typedef DescribedCreatable<ScriptContext, Instance, sScriptContext, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

		class ScriptImpersonator : public RBX::Security::Impersonator
		{
		public:
			ScriptImpersonator(lua_State *thread);
		};

        struct GlobalState
        {
            GlobalState()
                : state(0)
                , gcCount(0)
            {
            }

            lua_State* state;

            RunningAverage<double> gcAllocAvg; // average lua memory allocation per luaGcFrequency in KB
            int gcCount;
        };

		typedef boost::array<GlobalState, RBX::Security::COUNT_VM_Classes> GlobalStates; // separate Lua top-level states
		GlobalStates globalStates;
		Lua::WeakThreadRef commandLineSandbox;
		std::set<BaseScript*> scripts;
		RBX::Time nextPendingScripts;
		struct ScriptStart
		{
			shared_ptr<BaseScript> script;
			ScriptStartOptions options;
		};
		std::vector<ScriptStart> pendingScripts;		// scripts waiting to be executed
		std::vector<ScriptStart> loadingScripts;		// scripts waiting to be executed

        // An obfuscated pointer to a location near where the object was created.
        // copying the object becomes detectable.
        // https://en.wikipedia.org/wiki/Feistel_cipher
        // the "update" method might be targeted even if it is obfuscated.
        struct SecurityAnchor
        {
            size_t value[2];
            FORCEINLINE void update(const void* ptr)
            {
#ifdef _WIN32
                size_t localValue[2] = {reinterpret_cast<size_t>(ptr), ~reinterpret_cast<size_t>(ptr)};
                localValue[0] ^= localValue[1]*RBX_BUILDSEED | 20151112;
                localValue[1] ^= localValue[0]*20151112 | RBX_BUILDSEED;
                value[0] = localValue[0];
                value[1] = localValue[1];
#endif
            }

            FORCEINLINE bool checkAnchor(const void* ptr) const
            {
#ifdef _WIN32
                size_t localValue[2] = {value[0], value[1]};
                localValue[1] ^= localValue[0]*20151112 | RBX_BUILDSEED;
                localValue[0] ^= localValue[1]*RBX_BUILDSEED | 20151112;
                return !((reinterpret_cast<size_t>(ptr)+localValue[1])
                    ^ (~reinterpret_cast<size_t>(ptr)+localValue[0]));
#else
                return true;
#endif
            }
        };
        SecurityAnchor securityAnchor;

		shared_ptr<RunService> runService;

		boost::scoped_ptr<Lua::YieldingThreads> yieldEvent;		// collects all threads that have yielded, and periodically resumes them

		struct WaitingThread
		{
			Lua::ThreadRef thread;
			shared_ptr<const Reflection::Tuple> arguments;
		};
		rbx::safe_queue<WaitingThread> waitingThreads;

		bool robloxPlace;
		bool scriptsDisabled;	// == don't run the scripts contained in BaseScript objects
		bool preventNewConnection;

		shared_ptr<LuaStatsItem> statsItem;
		bool collectScriptStats;
		shared_ptr<ScriptStats> scriptStats;
		std::set<weak_ptr<ModuleScript> > loadedModules;

		int startScriptReentrancy;

        rbx::atomic<int> timedoutCount;
		Time::Interval timoutSpan;	// The time that is allowed per heartbeat before scripts stop running (0 means no timeouts)
		Time timoutTime;	// The system time when we should time-out scripts 
        rbx::atomic<int> timedout;     // == scripts should stop running
		boost::scoped_ptr<boost::thread> timeoutThread;
		boost::mutex timeoutMutex;
		volatile bool endTimoutThread;
		CEvent checkTimeout;

		struct AssetModuleInfo
		{
			enum State
			{
				NotFetchedYet = 0,
				Fetching,
				Fetched,
				Failed
			};
			State state;
			std::vector<Lua::WeakThreadRef> yieldedImporters;
			shared_ptr<ModuleScript> module;
			shared_ptr<ModelInstance> root;
			AssetModuleInfo()
				: state(NotFetchedYet)
			{}
		};
		typedef boost::unordered_map<int, AssetModuleInfo> LoadedAssetModules;
		LoadedAssetModules loadedAssetModules;
		
		Time luaGcStartTime;
		RunningAverage<double> avgLuaGcInterval; // in msec
		RunningAverage<double> avgLuaGcTime;	 // in msec

		RunningAverageTimeInterval<> resumedThreads;
		RunningAverage<> throttlingThreads;	// 1 if threads are being deffered

		bool statesClosed;

	public:
		ScriptContext();
		virtual ~ScriptContext();

		///////////////////////////////////////////////////
		// IScriptFilter
		/*override*/ virtual bool scriptShouldRun(BaseScript* script);
        
        static void setAdminScriptPath(const std::string& newPath);

		//////////////////////////////////////////////////
		// Reflection API
		static Reflection::BoundProp<bool> propScriptsDisabled;
		static Reflection::BoundProp<int> propLuaGcLimit;
		static Reflection::BoundProp<int> propLuaGcFrequency;
		static Reflection::BoundProp<int> propLuaGcStepSize;
		void setTimeout(double seconds);
		void setCollectScriptStats(bool);
		// Core & Starter Scripts
		void addStarterScript(int assetId);
		void addCoreScript(int assetId, shared_ptr<Instance> parent, std::string name);
		void addCoreScriptLocal(std::string scriptName, shared_ptr<Instance> parent);
		// Experimental error signal for catching errors server-side
		rbx::signal<void(std::string, std::string, shared_ptr<Instance>)> errorSignal;
		// A temporary signals used for diagnostic purposes
		rbx::signal<void(shared_ptr<Instance>, std::string, shared_ptr<Instance>)> camelCaseViolation;
		rbx::signal<void(lua_State*)> scriptErrorDetected;

		////////////////////////////////////////////////
		// Configuration
		void setRobloxPlace(bool robloxPlace);
        void initializeLuaStateSandbox(Lua::WeakThreadRef& threadRef, lua_State* parentState, Security::Identities identity);
        void setKeys(unsigned int scriptKey, unsigned int coreScriptModKey);

		////////////////////////////////////////////////
		// Helpers and utilities
		Reflection::Variant evaluateStudioCommandItem(const char* itemToEvaluate, shared_ptr<RBX::LuaSourceContainer> script);
		static bool checkSyntax(const std::string& code, int& line, std::string& errorMessage);
		static lua_State* getGlobalState(lua_State* thread);
		static ScriptContext& getContext(lua_State* thread);
		static void printCallStack(lua_State* thread, std::string* output = NULL, bool dontPrint = false);
		static std::string extractCallStack(lua_State* thread, shared_ptr<BaseScript>& source, int& line);
		// Shutdown helpers
		bool shouldPreventNewConnections() { return preventNewConnection; }
		void setPreventNewConnections() { preventNewConnection = true; }
		void closeStates(bool resettingSimulation);	// Closes down all threads
		bool haveStatesClosed() { return statesClosed; }
		void cleanupModules();

		/////////////////////////////////////////////////
		// Script Instance API
		// Called by IScriptOwner implementers
		void addScript(weak_ptr<BaseScript> script, ScriptStartOptions startOptions = ScriptStartOptions());	// checks pointer validity
		void removeScript(weak_ptr<BaseScript> script);
		size_t numScripts() {return scripts.size();}
		bool hasScript(BaseScript* script) {return (scripts.find(script) != scripts.end());}

		/////////////////////////////////////////////////
		// Calls that make lua run/resume
		void executeInNewThread(RBX::Security::Identities identity, const ProtectedString& script, const char* name);
		std::auto_ptr<Reflection::Tuple> executeInNewThread(RBX::Security::Identities identity, const ProtectedString& script, const char* name, const Reflection::Tuple& arguments);
		void executeInNewThreadWithExtraGlobals(RBX::Security::Identities identity,
			const ProtectedString& script, const char* name,
			const std::map<std::string, shared_ptr<Instance> >& extraGlobals);
		
		// Calls a function
        Reflection::Tuple callInNewThread(Lua::WeakFunctionRef& function, const Reflection::Tuple& arguments);

		// Thread-safe call:
		void scheduleResume(Lua::ThreadRef thread, shared_ptr<const Reflection::Tuple> arguments);
		typedef enum { Success, Yield, Error } Result;
		// Resumes the thread. Reports errors and queues yielding threads for later execution
		// NOTE: The caller is reponsible for balancing the stack
		Result resume(RBX::Lua::ThreadRef thread, int narg);
		
		/////////////////////////////////////////////
		// Stats
		void scriptResumedFromEvent() { resumedThreads.sample(); }

		size_t getThreadCount() const;
		shared_ptr<const Reflection::Tuple> getHeapStats(bool clearHighwaterMark);
		shared_ptr<const Reflection::Tuple> getScriptStats();	// deprecated. Don't use it anymore
		shared_ptr<const Reflection::ValueArray> getScriptStatsNew();

		struct ScriptStat
		{
			std::string hash;
			std::string name;
			Instances scripts;
			double activity;
			unsigned int invocationCount;
		};
		void getScriptStatsTyped(std::vector<ScriptStat>& result);

		double getAvgLuaGcTime() { return avgLuaGcTime.value(); }
		double getAvgLuaGcInterval() { return avgLuaGcInterval.value(); }

        void reloadModuleScript(shared_ptr<ModuleScript> moduleScript);

        bool checkSecurityAnchorValid() const
        {
            return securityAnchor.checkAnchor(&this->securityAnchor);
        }

	protected:
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	private:
		rbx::signals::scoped_connection heartbeatConnection;

		boost::scoped_ptr<LuaAllocator> allocator;

		shared_ptr<TaskScheduler::Job> gcJob;
		shared_ptr<TaskScheduler::Job> waitingScriptsJob;
		void onHeartbeat(const Heartbeat& heartbeat);
		void stepGc();
		void resumeWaitingScripts(Time expirationTime);

		static void sandboxThread(lua_State* thread);
		static void setThreadIdentityAndSandbox(lua_State* thread, RBX::Security::Identities identity, shared_ptr<BaseScript> script);
		static RBX::Security::Identities getThreadIdentity(lua_State* thread);

		// Executes a script, throws an std::exception on error
		// The script is spawned from the global root thread, but it is "sandboxed" to the extent that global declarations
		// don't affect other threads
		// If globalStateToExecuteIn is NULL we get the global state to execute in by our current identity, which is the first arg to this function
		void executeInNewThread(RBX::Security::Identities identity, const ProtectedString& script, const char* name,
			boost::function1<size_t, lua_State*> pushArguments, 
			boost::function2<void, lua_State*, size_t> readImmediateResults, 
			Scripts::Continuations continuations,
			lua_State* globalStateToExecuteIn = NULL,
			const std::map<std::string, shared_ptr<Instance> >* extraGlobals = NULL,
            unsigned int modkey = 1);

		// Resumes the thread (expects the top of the stack to contain a function)
		// Throws an std::exception if the thread throws a error
		void resumeWithArgs(Lua::ThreadRef thread, shared_ptr<const Reflection::Tuple> arguments);
		void resume(Lua::ThreadRef thread, boost::function1<size_t, lua_State*> pushArguments, boost::function2<void, lua_State*, size_t> readResults);

		void onChangedScriptEnabled(const Reflection::PropertyDescriptor&);
		void onCheckTimeout();
		void onHook(lua_State *L, lua_Debug *ar);

		struct ScriptStatInformation
		{
			ScriptStatInformation()
			{}
			std::string name;
			Instances scripts;
		};
		std::map<std::string, ScriptStatInformation> scriptHashInfo;

		// Functions exposed in the Lua environment:
	public:
		static void hook(lua_State *L, lua_Debug *ar);
		void reportError(lua_State* thread);

		lua_State* getGlobalState(RBX::Security::Identities identity);

	private:
		static int print(lua_State *L);
		static int doPrint(lua_State *thread, const MessageType& messageType = MESSAGE_OUTPUT);
		static int crash(lua_State *L);
		static int tick(lua_State* thread);
		static int rbxTime(lua_State* thread);
		static int time(lua_State* thread);
		static int wait(lua_State* thread);
		static int delay(lua_State* thread);
		static int ypcall(lua_State* thread);
		void on_ypcall_success(Lua::WeakThreadRef caller, lua_State* functor);
		void on_ypcall_failure(Lua::WeakThreadRef caller, lua_State* functor);
		static int spawn(lua_State* thread);
		static int printidentity(lua_State* thread);
		static int loadfile(lua_State* thread);
		static int loadstring(lua_State* thread);
		static int notImplemented(lua_State* thread);
		static int dofile(lua_State* thread);
		static int settings(lua_State* thread);
		static int usersettings(lua_State* thread);
		static int pluginmanager(lua_State* thread);
		static int debuggermanager(lua_State *thread);
		static int loadLibrary(lua_State* L);
		static int loadRobloxLibrary(lua_State* L);
        static int requireModuleScript(lua_State* L);
		static int stats(lua_State* thread);
		static int version(lua_State* thread);
		static int statsitemvalue(lua_State* thread);
		
		static int requireModuleScriptFromInstance(lua_State* L, shared_ptr<ModuleScript> moduleScript);
		static int requireModuleScriptFromAssetId(lua_State* L, int assetId);
		static void moduleContentLoaded(AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances,
			ScriptContext& sc, Security::Identities identity, lua_State* globalState, AssetModuleInfo* info);
		static void moduleContentLinkedSourcesResolved(shared_ptr<Instances> instances,
			shared_ptr<ModuleScript> foundModuleScript, ScriptContext& sc, Security::Identities identity,
			lua_State* globalState, AssetModuleInfo* info);
		void startRunningModuleScript(Security::Identities identity, lua_State* globalState, shared_ptr<ModuleScript> moduleScript);
		static void requireModuleScriptSuccessContinuation(shared_ptr<ModuleScript> moduleScript,
			lua_State* threadRunningModuleCode);
		static void requireModuleScriptErrorContinuation(shared_ptr<ModuleScript> moduleScript,
			lua_State* threadRunningModuleCode);
		
		static void reloadModuleScriptInternal(lua_State* globalState, shared_ptr<ModuleScript> moduleScript);

        static void reloadModuleScriptSuccessContinuation(shared_ptr<ModuleScript> moduleScript,
                                                          lua_State* reloadThread,
                                                          int oldResultRegistryIndex);
        
		static void reloadModuleScriptErrorContinuation(shared_ptr<ModuleScript> moduleScript,
                                                        lua_State* reloadThread);

        static int warn(lua_State *L);
		static void validateThreadAccess(lua_State* L);
		static int resumeImpl(lua_State* L, int nargs);
        
		int camelCaseViolationCount;
		rbx::signals::connection camelCaseViolationConnection;
		void onCamelCaseViolation(shared_ptr<Instance> object, std::string memberName, shared_ptr<Instance> script);

		friend class BaseScript;
		void disassociateState(BaseScript* script);

		bool openState(size_t idx);
		void closeState(lua_State* globalState);

		void startScript(ScriptStart scriptStart);
		static void eraseScript(std::vector<ScriptContext::ScriptStart>& container, BaseScript* script);
		void startPendingScripts();

        unsigned int coreScriptModKey;
	};

#ifdef _DEBUG
	class StackBalanceCheck
	{
		const int oldTop;
		lua_State *thread;
		bool cancelled;
	public:
		StackBalanceCheck(lua_State *thread);
		~StackBalanceCheck();
		void cancel() { cancelled = true; }
	};
#define RBXASSERT_BALLANCED_LUA_STACK(L) StackBalanceCheck stackBalanceCheck(L)
#define RBXASSERT_BALLANCED_LUA_STACK2(L) StackBalanceCheck stackBalanceCheck2(L)
#define CANCEL_BALLANCED_LUA_STACK_CHECK() stackBalanceCheck.cancel()
#define CANCEL_BALLANCED_LUA_STACK_CHECK2() stackBalanceCheck2.cancel()
#else
#define RBXASSERT_BALLANCED_LUA_STACK(L) ((void)0)
#define RBXASSERT_BALLANCED_LUA_STACK2(L) ((void)0)
#define CANCEL_BALLANCED_LUA_STACK_CHECK() ((void)0)
#define CANCEL_BALLANCED_LUA_STACK_CHECK2() ((void)0)
#endif
}
