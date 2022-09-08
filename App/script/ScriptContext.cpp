#include "stdafx.h"
#include "Script/ScriptContext.h"
#include "Script/CoreScript.h"
#include "Script/DebuggerManager.h"
#include "Script/LuaArguments.h"
#include "Script/LuaAtomicClasses.h"
#include "Script/LuaCoreFunctions.h"
#include "Script/LuaEnum.h"
#include "Script/LuaInstanceBridge.h"
#include "Script/LuaLibrary.h"
#include "Script/LuaMemory.h"
#include "Script/LuaSettings.h"
#include "Script/LuaSignalBridge.h"
#include "Script/ModuleScript.h"
#include "Script/Script.h"
#include "Script/ScriptEvent.h"
#include "Script/ScriptStats.h"

#include "Util/AsyncHttpQueue.h"
#include "Util/RbxStringTable.h"
#include "Util/standardout.h"	// TODO: This cross-reference of library code sucks
#include "Util/ScopedAssign.h"
#include "Util/xxhash.h"

#include "v8datamodel/Stats.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/contentprovider.h"
#include "v8datamodel/hackdefines.h"
#include "v8datamodel/PluginManager.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/ServerScriptService.h"
#include "reflection/type.h"

#include "Network/Players.h"
#include "Network/api.h"
#include "RobloxServicesTools.h"

#include "rbx/Crypt.h"
#include "FastLog.h"
#include "rbx/atomic.h"

#include "RbxG3D/RbxTime.h"
#include "rbx/rbxTime.h"

#include "Util/RobloxGoogleAnalytics.h"

#include <boost/lexical_cast.hpp>

#include <lobject.h>
#include <lstate.h>
#include <ldebug.h>

#include "VMProtect/VMProtectSDK.h"

#include "rbx/Profiler.h"

LOGGROUP(CoreScripts)
LOGGROUP(UseLuaMemoryPool)
FASTFLAGVARIABLE(DebugCrashEnabled, true)

LOGGROUP(LuaProfiler)
LOGGROUP(LuaScriptTimeoutSeconds)
FASTFLAG(LuaDebugger)
FASTFLAG(LuaDebuggerBreakOnError)

DYNAMIC_FASTINTVARIABLE(LuaExceptionPlaceFilter, 0)
DYNAMIC_FASTINTVARIABLE(LuaExceptionThrottlingPercentage, 0)

DYNAMIC_FASTFLAGVARIABLE(BadTypeOnSpawnErrorEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(BadTypeOnDelayErrorEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(ScriptContextGuardAgainstCStackOverflow, false)

FASTFLAGVARIABLE(CustomEmitterLuaTypesEnabled, false)

DYNAMIC_FASTFLAGVARIABLE(LogPrivateModuleRequires, true)

DYNAMIC_FASTINTVARIABLE(LuaGcBoost, 1)
DYNAMIC_FASTINTVARIABLE(LuaGcMaxKb, 100)

DYNAMIC_FASTFLAGVARIABLE(LockViolationScriptCrash, false)

namespace RBX
{
	const char* const sScriptContext = "ScriptContext";
	const char *const sRuntimeScriptService = "RuntimeScriptService";
}

using namespace RBX;
using namespace RBX::Lua;
using namespace boost::posix_time;

REFLECTION_BEGIN();
static Reflection::EventDesc<ScriptContext, void(shared_ptr<Instance>, std::string, shared_ptr<Instance>)> event_camelCaseViolation(&ScriptContext::camelCaseViolation, "CamelCaseViolation", "object", "member", "script", RBX::Security::RobloxScript);
REFLECTION_END();
const char* const kModuleFailedOnLoadMessage = "Requested module experienced an error while loading";
const char* const kModuleDidNotReturnOneValue = "Module code did not return exactly one value";
const char* const kModuleNotFoundForAssetId = "Unable to find module for asset id";

static boost::thread_specific_ptr<int> kCLuaResumeStackSize;

/* 
Lua SECURITY:

* _G has been replaced by a simple table, not associated with the global table anymore. 
  Clients can mess with this as they please

* For each sandboxed thread, its global table shadowed with a new table 
 (that references the old table) whose metatable is protected. See sandboxThread()

* The global table's metatable is protected

* For each loaded library we shadow its table and protect the metatable. See shadowGlobal()

* Modified "newProxy" undocumented feature

* protected all metatables associated with Roblox userdata objects

* special-case code for protecting the string metatable

-------------------------------------------------------------------

SAMPLE HACKS

-- attempt to override a library:
rawset(math,'sin','hacko!')	-- fails

-- attempt to hack a library metatable:
getmetatable(math).__index = 'hacko!' -- fails

-- attempt to hack the string metatable:
getmetatable('').__index.upper = 'hacko!' -- fails

-- attampt to hack the global environment:
getfenv(0).wait = "hacko!" -- only hacks the script's environment

-- attempt to hack the global environment via _G:
_G.wait = "hacko!" -- _G no longer has access to pre-defined globals


-------------------------------------------------------------------

Notes from http://lua-users.org/wiki/SandBoxes

loadstring - SANDBOXED - uses the sandboxed environment

getfenv - SANDBOXED - getfenv(0).wait = "hacko!" does not change wait() for other threads
setfenv - SANDBOXED - setfenv(0) still only sets the environment of the current thread

getmetatable - PROTECTED - all tables we care about are protected with __metatable
setmetatable - PROTECTED - all tables we care about are protected with __metatable

rawequal - PROTECTED - all tables we care about are shadowed
rawget - PROTECTED - all tables we care about are shadowed
rawset - PROTECTED - all tables we care about are shadowed

string.dump - UNSAFE (potentially) - allows seeing implementation of functions.

_G - SANDBOXED - by default this contains the global environment. You may, however, want to set this variable to contain the environment of the sandbox.

load - SUPPRESSED. The function returned has the global environment, thereby breaking out of the sandbox
collectgarbage - MODIFIED - only "count" is supported. TODO: Allow other options if your privs are high enough
loadfile - OVERRIDEN
dofile - OVERRIDEN - The may read arbitrary files on the file system. It can also read standard input. The code is run under the global environment, not the sandbox, so this can be used to break out of the sandbox. Related discussion: DofileNamespaceProposal.

coroutine - LOCKED - modifying this table could affect code outside the sandbox
string - LOCKED - modifying this table could affect code outside the sandbox
table - LOCKED - modifying this table could affect code outside the sandbox
math - LOCKED - modifying this table could affect code outside the sandbox

newproxy - MODIFIED - only true and nil args are supported. Also prevent hooking of GC, which gets you out of the sandbox

module - NOT LOADED - for example, modifies globals (e..g package.loaded) and provides access to environments outside the sandbox.
require - NOT LOADED - modifies globals (e.g. package.loaded), provides access to environments outside the sandbox, and accesses the file system.
package - NOT LOADED
io - NOT LOADED
os - NOT LOADED
debug - NOT LOADED

*/

static int illegal(lua_State *L)
{
	throw std::runtime_error("can't modify this library");
}

void Lua::protect_metatable(lua_State* thread, int index)
{
	RBXASSERT_BALLANCED_LUA_STACK(thread);

	lua_pushstring(thread, "The metatable is locked");
	lua_setfield(thread, index-1, "__metatable");
}

static void loadLibraryProtected(lua_State* L, lua_CFunction func)
{
	RBXASSERT_BALLANCED_LUA_STACK(L);
    
    int count = func(L);
    
    // Since we only protect one table, we expect one result
    // Unfortunately, luaopen_base also leaves _G on the stack (why???)
    RBXASSERT(count == (func == luaopen_base) ? 2 : 1);
    
    lua_setreadonly(L, -1, true);
    
    lua_pop(L, count);
}

struct LuaProfiler
{
    static std::string getResumePosition(lua_State* L, int nargs)
    {
        StkId firstArg = L->top - nargs;
        
        if (L->status == 0)
        {
            // start coroutine
            StkId func = firstArg - 1;
            if (!ttisfunction(func))
                return "??";
            
            Closure& cl = func->value.gc->cl;
            
            if (cl.c.isC)
                return "=[C](-1)";

            const char* file = getstr(cl.l.p->source);
            int line = getline(cl.l.p, 0);
            bool string = (strchr(file, '\n') != 0);

            return RBX::format("%s(%d)", string ? "<string>" : file, line);
        }
        else
        {
            // resume from yield
            return "(resume)";
        }
    }

    struct StringCache
    {
        struct Function
        {
            const char* name;
            const char* namewhat;
            const char* file;
            int line;

            // used for std::map
            bool operator<(const Function& o) const
            {
                return (name != o.name) ? name < o.name : (namewhat != o.namewhat) ? namewhat < o.namewhat : (file != o.file) ? file < o.file : line < o.line;
            }
        };

        std::map<std::string, std::string> strings;
        std::map<Function, std::string> functions;

        const char* getBegin(const std::string& value)
        {
            std::string& res = strings[value];

            if (res.empty())
                res = "LUABEG" + value;

            return res.c_str();
        }

        const char* getCall(const char* name, const char* namewhat, const char* file, int line)
        {
            Function func = {name, namewhat, file, line};
            std::string& res = functions[func];

            if (res.empty())
            {
                const char* type =
                    namewhat ?
                        ((strcmp(namewhat, "index") == 0) ? "get " :
                         (strcmp(namewhat, "newindex") == 0) ? "set " :
                         "")
                        : "";

                res = RBX::format("LUACALL%s(%d): %s%s", file ? file : "?", line, type, name ? name : "?");
            }

            return res.c_str();
        }
    };

    static StringCache stringCache;
    static LuaProfiler* instance;

    void hookCall(lua_State* L, lua_Debug* ar)
    {
        lua_getinfo(L, "nS", ar);

        const char* func = stringCache.getCall(ar->name, ar->namewhat, ar->source, ar->linedefined);

        FLog::FastLogS(FLog::LuaProfiler, func, NULL);
    }

    void hookRet(lua_State* L, lua_Debug* ar)
    {
        FLog::FastLogS(FLog::LuaProfiler, "LUARET", NULL);
    }

    LuaProfiler(lua_State* L, int nargs)
    {
        RBXASSERT(instance == NULL);
        instance = this;

        std::string startPos = getResumePosition(L, nargs);
        const char* startPosString = stringCache.getBegin(startPos);

        FLog::FastLogS(FLog::LuaProfiler, startPosString, NULL);

        if (startPos == "(resume)")
        {
            // resume from yield, record fake callstack
            std::vector<const char*> stack;

            lua_Debug entry;
            int depth = 0;
         
            while (lua_getstack(L, depth, &entry))
            {
                int status = lua_getinfo(L, "Sln", &entry);
                assert(status);
                RBX_UNUSED(status);

                stack.push_back(stringCache.getCall(entry.name, entry.namewhat, entry.source, entry.currentline));
                depth++;
            }

            for (size_t i = stack.size(); i > 0; --i)
                FLog::FastLogS(FLog::LuaProfiler, stack[i - 1], NULL);
        }
    }

    ~LuaProfiler()
    {
        FLog::FastLogS(FLog::LuaProfiler, "LUAEND", NULL);

        RBXASSERT(instance == this);
        instance = NULL;
    }
};

LuaProfiler* LuaProfiler::instance = NULL;
LuaProfiler::StringCache LuaProfiler::stringCache;

static rbx::atomic<int> contextCount;

REFLECTION_BEGIN();
//LocalUser permissions (including Studio.ashx execution)
Reflection::BoundFuncDesc<ScriptContext, void(int)> func_AddStarterScript(&ScriptContext::addStarterScript, "AddStarterScript", "assetId", Security::LocalUser);

//RobloxScript permissions
Reflection::BoundFuncDesc<ScriptContext, void(int, shared_ptr<Instance>, std::string name)> func_AddCoreScript(&ScriptContext::addCoreScript, "AddCoreScript", "assetId","parent","name", Security::RobloxScript);
Reflection::BoundFuncDesc<ScriptContext, void(std::string name, shared_ptr<Instance>)> func_AddCoreScriptLocal(&ScriptContext::addCoreScriptLocal, "AddCoreScriptLocal", "name", "parent", Security::RobloxScript);

//General permissions
Reflection::BoundProp<bool> ScriptContext::propScriptsDisabled("ScriptsDisabled", category_State, &ScriptContext::scriptsDisabled, &ScriptContext::onChangedScriptEnabled, Reflection::PropertyDescriptor::UI, Security::LocalUser);

//Settings
Reflection::BoundFuncDesc<ScriptContext, void(double)> func_SetTimeout(&ScriptContext::setTimeout, "SetTimeout", "seconds", Security::Plugin);
Reflection::BoundFuncDesc<ScriptContext, shared_ptr<const Reflection::Tuple>(bool)> func_GetHeapStats(&ScriptContext::getHeapStats, "GetHeapStats", "clearHighwaterMark", true, Security::RobloxScript);
Reflection::BoundFuncDesc<ScriptContext, shared_ptr<const Reflection::ValueArray>()> func_GetScriptStats(&ScriptContext::getScriptStatsNew, "GetScriptStats", Security::RobloxScript);
Reflection::BoundFuncDesc<ScriptContext, void(bool)> func_CollectScriptStats(&ScriptContext::setCollectScriptStats, "SetCollectScriptStats", "enable", false, Security::RobloxScript);
 
// Experimental event for error-reporting
Reflection::EventDesc<ScriptContext, void(std::string, std::string, shared_ptr<Instance>)> event_Error(&ScriptContext::errorSignal, "Error", "message", "stackTrace", "script", Security::None);
REFLECTION_END();

namespace RBX
{
	class GcJob : public DataModelJob
	{
	public:
		weak_ptr<ScriptContext> const scriptContext;
		int frequency;
		GcJob(shared_ptr<ScriptContext> scriptContext)
			:DataModelJob("LuaGc", DataModelJob::Write, false, shared_from_dynamic_cast<DataModel>(scriptContext->getParent()), Time::Interval(0.003))
			,scriptContext(scriptContext)
		{
			cyclicExecutive = true;
            frequency = 30;
		}

		Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, frequency);
		}

		virtual Job::Error error(const Stats& stats)
		{
			return computeStandardError(stats, frequency);
		}

		TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
		{
			if (shared_ptr<ScriptContext> sc = scriptContext.lock())
			{
				DataModel* d = DataModel::get(sc.get());
				FASTLOG1(FLog::DataModelJobs, "GC Job start, data model: %p", d);
				DataModel::scoped_write_request request(d);	// TODO: Promote this to DataModelJob
				sc->stepGc();
				FASTLOG1(FLog::DataModelJobs, "GC Job finish, data model: %p", d);
			}
			return TaskScheduler::Stepped;
		}
	};
	class WaitingScriptsJob : public DataModelJob
	{
	public:
		weak_ptr<ScriptContext> const scriptContext;
		WaitingScriptsJob(shared_ptr<ScriptContext> scriptContext)
			:DataModelJob(
			"LuaResumeWaitingScripts", 
			DataModelJob::Write, 
			false, 
			shared_from_dynamic_cast<DataModel>(scriptContext->getParent()), 
			Time::Interval(LuaSettings::singleton().waitingThreadsBudget/60.0)	// That's a 10% duty cycle
			)
			,scriptContext(scriptContext)
		{
			cyclicExecutive = true;
		}

		Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, 60.0);
		}

		virtual Job::Error error(const Stats& stats)
		{
			if (!DataModel::throttleAt30Fps && TaskScheduler::singleton().isCyclicExecutive())
			{
				return computeStandardErrorCyclicExecutiveSleeping(stats, 60.0);
			}
			return computeStandardError(stats, 60.0);
		}

		TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
		{
			if (shared_ptr<ScriptContext> sc = scriptContext.lock())
			{
				DataModel* d = DataModel::get(sc.get());
				FASTLOG1(FLog::DataModelJobs, "Waiting scripts start, data model: %p", d);
				DataModel::scoped_write_request request(d);	// TODO: Promote this to DataModelJob
				sc->resumeWaitingScripts(Time::nowFast() + stepBudget);
				FASTLOG1(FLog::DataModelJobs, "Waiting scripts finish, data model: %p", d);
			}
			
			return TaskScheduler::Stepped;
		}
	};
}

static int panic(lua_State *L) 
{
	FASTLOG1(FLog::Error, "Lua panic: state %p", L);

	StandardOut::singleton()->printf(MESSAGE_ERROR, "Unprotected error in call to Lua API (%s)\n", lua_tostring(L, -1));

	if (shared_ptr<BaseScript> script = RobloxExtraSpace::get(L)->script.lock())
	{
		FASTLOG1(FLog::Error, "Lua panic: script %p", script.get());
		FASTLOGS(FLog::Error, "Lua panic: script name %s", script->getFullName().c_str());

		static std::string callstack;
		ScriptContext::printCallStack(L, &callstack);

		FASTLOG1(FLog::Error, "Lua panic: callstack logged to %p", &callstack);
	}

	RBXCRASH();
	return 0;
}

const int ScriptContext::hookCount = 1000;

void ScriptContext::hook(lua_State *L, lua_Debug *ar)
{
    switch (ar->event)
    {
    case LUA_HOOKCALL:
        if (LuaProfiler::instance) LuaProfiler::instance->hookCall(L, ar);
        break;

    case LUA_HOOKRET:
	case LUA_HOOKTAILRET:
        if (LuaProfiler::instance) LuaProfiler::instance->hookRet(L, ar);
        break;

    case LUA_HOOKCOUNT:
        {
            ScriptContext& context(getContext(L));
            
            FASTLOG1(FLog::ScriptContext, "ScriptContext::hook, timeout count %d", static_cast<int>(context.timedoutCount));

            if (context.timedoutCount>0)
            {
                context.onHook(L, ar);
            }
        } break;

    default:
        RBXASSERT(false);
    }
}

void ScriptContext::onHook(lua_State *L, lua_Debug *ar)
{
	if (timedout)
		if (Security::Context::current().identity==Security::GameScript_ || 
			Security::Context::current().identity==Security::RobloxGameScript_)
			luaL_error(L, "Game script timeout");
}


void RBX::registerScriptDescriptors()
{
	RBX::Reflection::Type::singleton<int>();
	RBX::Reflection::Type::singleton<bool>();
	RBX::Reflection::Type::singleton<float>();
	RBX::Reflection::Type::singleton<double>();
	RBX::Reflection::Type::singleton<std::string>();
	RBX::Reflection::Type::singleton<Lua::WeakFunctionRef>();
	RBX::Reflection::Type::singleton<shared_ptr<GenericFunction> >();
	RBX::Reflection::Type::singleton<shared_ptr<GenericAsyncFunction> >();
	RBX::Reflection::Type::singleton< shared_ptr<Instance> >();
	RBX::Reflection::Type::singleton<G3D::Vector3>();
	RBX::Reflection::Type::singleton<G3D::Vector2>();
    RBX::Reflection::Type::singleton<G3D::Rect2D>();
	RBX::Reflection::Type::singleton<PhysicalProperties>();
	RBX::Reflection::Type::singleton<RBX::RbxRay>();
	RBX::Reflection::Type::singleton<G3D::CoordinateFrame>();
	RBX::Reflection::Type::singleton<G3D::Color3>();
	RBX::Reflection::Type::singleton<BrickColor>();
}


int ScriptContext::crash(lua_State *L)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::LocalUser, "crash");

	RBXCRASH("User-generated crash");

	return 0;
}

int getGlobalStateIndex(RBX::Security::Identities identity)
{
    // transform identity into a particular VM. this can improve security by not mixing user scripts with ROBLOX scripts (we've
    // had problems with people circumnavigating the permission levels in the past).
    // for now, we just use two VM's. as our needs change, we may add more. each VM has a performance cost so we don't want
    // more than we need.
    int vm_index = Security::VM_Default;
    if (RBX::Security::Context::isInRole(identity, RBX::Security::RobloxScript))
        vm_index = Security::VM_RobloxScriptPlus;
#if defined(RBX_STUDIO_BUILD)
	else if (Security::Context::isInRole(identity, Security::Plugin))
		vm_index = Security::VM_StudioPlugin;
#endif

	return vm_index;
}

lua_State* ScriptContext::getGlobalState( RBX::Security::Identities identity )
{
	int vm_index = getGlobalStateIndex(identity);
	RBXASSERT(globalStates[vm_index].state);
	return globalStates[vm_index].state;
}

static bool preventLinkerStrip = false;

static int luaopen_os_rbx(lua_State* L)
{
    luaL_register(L, "os", LuaOsExtension::registry);
    return 1;
}

static int luaopen_math_rbx(lua_State* L)
{
	luaopen_math(L);

	lua_pushcfunction(L, LuaMathExtension::noise);
	lua_setfield(L, -2, "noise");

	return 1;
}

static int luaopen_debug_rbx(lua_State* L)
{
	luaL_register(L, "debug", LuaDebugExtension::registry);
	return 1;
}

bool ScriptContext::openState(size_t idx)
{
	if (preventLinkerStrip)
	{
		lua_getstack(0,0,0);
		lua_getinfo(0,0,0);
		lua_getlocal(0,0,0);
		lua_setlocal(0,0,0);
		lua_getupvalue(0,0,0);
		lua_setupvalue(0,0,0);

		lua_gethook(0);
		lua_gethookmask(0);
		lua_gethookcount(0);
	}

	if (!allocator)
		allocator.reset(new RBX::LuaAllocator(FLog::UseLuaMemoryPool != 0));

	lua_State* globalState = lua_newstate(LuaAllocator::alloc, allocator.get());
	if (globalState==NULL)
		throw std::runtime_error("Failed to create Lua state");

	contextCount++;

	lua_atpanic(globalState, &panic);

	lua_gc(globalState, LUA_GCSETSTEPMUL, RBX::LuaSettings::singleton().gcStepMul);
	lua_gc(globalState, LUA_GCSETPAUSE, RBX::LuaSettings::singleton().gcPause);

	RobloxExtraSpace::get(globalState)->setContext(this);

	{
		RBXASSERT_BALLANCED_LUA_STACK(globalState);
		// TODO: Security: We have a custom version of Lua that hides the I/O and OS functions of the base library.

        loadLibraryProtected(globalState, luaopen_base);
        loadLibraryProtected(globalState, luaopen_string);
        
		{
			RBXASSERT_BALLANCED_LUA_STACK(globalState);
			// Protect the metatable for strings
			lua_pushliteral(globalState, "");  /* dummy string */
			int i = lua_getmetatable(globalState, -1);
			RBXASSERT(i==1);
            RBX_UNUSED(i);
			Lua::protect_metatable(globalState, -1);
			lua_pop(globalState, 2);
		}

        loadLibraryProtected(globalState, luaopen_math_rbx);
        loadLibraryProtected(globalState, luaopen_table);
		loadLibraryProtected(globalState, luaopen_debug_rbx);

		// TODO: Where do these go?
		//load(thread, LUA_LOADLIBNAME, luaopen_package);
		//load(thread, LUA_DBLIBNAME, luaopen_debug);

		// Re-assign the _G variable (so that people can't hack into the global table)
		// This constitutes a breaking change to Lua's spec, but one we're willing to live with.
		// A more elegant solution would be to allow creation of new key-value pairs but not the
		// modification of pre-existing items (the ones registered on startup)
		lua_newtable(globalState);
		lua_setglobal(globalState, "_G");
	}

	{
		RBXASSERT_BALLANCED_LUA_STACK(globalState);
		LibraryBridge::registerClass(globalState);
		Enums::registerClass(globalState);
		Enum::registerClass(globalState);
		EnumItem::registerClass(globalState);
		ObjectBridge::registerClass(globalState);
		EventBridge::registerClass(globalState);
		SignalConnectionBridge::registerClass(globalState);
		CoordinateFrameBridge::registerClass(globalState);
		Region3Bridge::registerClass(globalState);
		Region3int16Bridge::registerClass(globalState);
		Vector3int16Bridge::registerClass(globalState);
		Vector2int16Bridge::registerClass(globalState);
        Rect2DBridge::registerClass(globalState);
		PhysicalPropertiesBridge::registerClass(globalState);
		Vector3Bridge::registerClass(globalState);
		Vector2Bridge::registerClass(globalState);
		RbxRayBridge::registerClass(globalState);
		Color3Bridge::registerClass(globalState);
		BrickColorBridge::registerClass(globalState);
		UDimBridge::registerClass(globalState);
		UDim2Bridge::registerClass(globalState);
		FacesBridge::registerClass(globalState);
		AxesBridge::registerClass(globalState);
		CellIDBridge::registerClass(globalState);
		Bridge< boost::intrusive_ptr<WeakThreadRef::Node> >::registerClass(globalState);
		Bridge< shared_ptr<GenericFunction> >::registerClass(globalState);
		Bridge< shared_ptr<GenericAsyncFunction> >::registerClass(globalState);
        if (FFlag::CustomEmitterLuaTypesEnabled)
        {{{
            NumberSequenceBridge::registerClass(globalState);
            ColorSequenceBridge::registerClass(globalState);
            NumberSequenceKeypointBridge::registerClass(globalState);
            ColorSequenceKeypointBridge::registerClass(globalState);
            NumberRangeBridge::registerClass(globalState);
        }}}
    }

	{
		RBXASSERT_BALLANCED_LUA_STACK(globalState);
		LibraryBridge::registerClassLibrary(globalState);
		Enums::registerClassLibrary(globalState);
		Enum::registerClassLibrary(globalState);
		EnumItem::registerClassLibrary(globalState);
		CoordinateFrameBridge::registerClassLibrary(globalState);
		Region3Bridge::registerClassLibrary(globalState);
		Region3int16Bridge::registerClassLibrary(globalState);
		Vector3int16Bridge::registerClassLibrary(globalState);
		Vector2int16Bridge::registerClassLibrary(globalState);
        Rect2DBridge::registerClassLibrary(globalState);
		PhysicalPropertiesBridge::registerClassLibrary(globalState);
		Vector3Bridge::registerClassLibrary(globalState);
		Vector2Bridge::registerClassLibrary(globalState);
		RbxRayBridge::registerClassLibrary(globalState);
		Color3Bridge::registerClassLibrary(globalState);
		BrickColorBridge::registerClassLibrary(globalState);
		UDimBridge::registerClassLibrary(globalState);
		UDim2Bridge::registerClassLibrary(globalState);
		FacesBridge::registerClassLibrary(globalState);
		AxesBridge::registerClassLibrary(globalState);
		CellIDBridge::registerClassLibrary(globalState);
		ObjectBridge::registerClassLibrary(globalState);
		ObjectBridge::registerInstanceClassLibrary(globalState);

        if (FFlag::CustomEmitterLuaTypesEnabled)
        {{{
            NumberSequenceBridge::registerClassLibrary(globalState);
            ColorSequenceBridge::registerClassLibrary(globalState);
            NumberSequenceKeypointBridge::registerClassLibrary(globalState);
            ColorSequenceKeypointBridge::registerClassLibrary(globalState);
            NumberRangeBridge::registerClassLibrary(globalState);
        }}}
    }

	{
		RBXASSERT_BALLANCED_LUA_STACK(globalState);

		WeakThreadRef::Node::create(globalState);

        // Declare the "game" object (DataModel)
        ObjectBridge::push(globalState, shared_from(getRootAncestor()));
        lua_setglobal(globalState, "game");

        // LEGACY
        // Declare the "Game" object (DataModel)
        ObjectBridge::push(globalState, shared_from(getRootAncestor()));
        lua_setglobal(globalState, "Game");

		// Declare the "workspace" object (Workspace)
		ObjectBridge::push(globalState, shared_from(ServiceProvider::find<Workspace>(this)));
		lua_setglobal(globalState, "workspace");

		// LEGACY
		// Declare the "Workspace" object (Workspace)
		ObjectBridge::push(globalState, shared_from(ServiceProvider::find<Workspace>(this)));
		lua_setglobal(globalState, "Workspace");

		// Declare the shared global table
		lua_newtable(globalState);  /* create it */
		lua_setglobal(globalState, "shared");

		// Declare the print() global function
		lua_pushcfunction(globalState, print);
		lua_setglobal(globalState, "print");

		if (FFlag::DebugCrashEnabled)
		{
			// Declare the crash__() global function for debugging
			lua_pushcfunction(globalState, crash);
			lua_setglobal(globalState, "crash__");
		}

		// Declare the tick() global function
		lua_pushcfunction(globalState, tick);
		lua_setglobal(globalState, "tick");

		// Declare the time() global function
		lua_pushcfunction(globalState, time);
		lua_setglobal(globalState, "time");

        // Declare the elapsedTime() global function
        lua_pushcfunction(globalState, rbxTime);
        lua_setglobal(globalState, "elapsedTime");

        // LEGACY
		// Declare the ElapsedTime() global function
		lua_pushcfunction(globalState, rbxTime);
		lua_setglobal(globalState, "ElapsedTime");

		// Declare the wait() global function
		lua_pushcfunction(globalState, wait);
		lua_setglobal(globalState, "wait");

        // LEGACY
		// Declare the Wait() global function
		lua_pushcfunction(globalState, wait);
		lua_setglobal(globalState, "Wait");

		// Declare the delay() global function
		lua_pushcfunction(globalState, delay);
		lua_setglobal(globalState, "delay");

		// LEGACY
		// Declare the Delay() global function
		lua_pushcfunction(globalState, delay);
		lua_setglobal(globalState, "Delay");

		// Declare the ypcall() global function
		lua_pushcfunction(globalState, ypcall);
		lua_setglobal(globalState, "ypcall");

		// Replace Lua pcall() global function with ypcall to handle yielding transparently
		lua_pushcfunction(globalState, ypcall);
		lua_setglobal(globalState, "pcall");

        // Declare the spawn() global function
        lua_pushcfunction(globalState, spawn);
        lua_setglobal(globalState, "spawn");

        // LEGACY
		// Declare the Spawn() global function
		lua_pushcfunction(globalState, spawn);
		lua_setglobal(globalState, "Spawn");

		// Declare the printidentity() global function
		lua_pushcfunction(globalState, printidentity);
		lua_setglobal(globalState, "printidentity");

		// Replace the definition of dofile
		// TODO: Security: Does this truly replace the Lua's version of dofile?
		lua_pushcfunction(globalState, dofile);
		lua_setglobal(globalState, "dofile");

		// Replace the definition of loadfile
		// TODO: Security: Does this truly replace the Lua's version of loadfile?
		lua_pushcfunction(globalState, loadfile);
		lua_setglobal(globalState, "loadfile");

        // Replace the definition of loadstring
		lua_pushcfunction(globalState, loadstring);
		lua_setglobal(globalState, "loadstring");

		// Replace the definition of load
		// TODO: Security: Does this truly replace the Lua's version of the function?
		// TODO: insert a security check into the regular implementation
		lua_pushcfunction(globalState, notImplemented);
		lua_setglobal(globalState, "load");

		// Declare the settings() global function
		lua_pushcfunction(globalState, settings);
		lua_setglobal(globalState, "settings");

		// Declare the UserSettings() global function
		lua_pushcfunction(globalState, usersettings);
		lua_setglobal(globalState, "UserSettings");

		// Declare the PluginManager() global function
		lua_pushcfunction(globalState, pluginmanager);
		lua_setglobal(globalState, "PluginManager");

		// Declare the loadlibrary(string) global function
		lua_pushcfunction(globalState, loadLibrary);
		lua_setglobal(globalState, "LoadLibrary");

        // Declare the warn(string) global function
        lua_pushcfunction(globalState, warn);
        lua_setglobal(globalState, "warn");

		lua_pushcfunction(globalState, requireModuleScript);
		lua_setglobal(globalState, "require");

		if (FFlag::LuaDebugger)
		{
			// Declare the DebuggerManager() global function
			lua_pushcfunction(globalState, debuggermanager);
			lua_setglobal(globalState, "DebuggerManager");
		}

		// LEGACY
		lua_pushcfunction(globalState, stats);
		lua_setglobal(globalState, "stats");

		// LEGACY
		lua_pushcfunction(globalState, stats);
		lua_setglobal(globalState, "Stats");

		// LEGACY
		lua_pushcfunction(globalState, version);
		lua_setglobal(globalState, "version");

		// LEGACY
		lua_pushcfunction(globalState, version);
		lua_setglobal(globalState, "Version");

        loadLibraryProtected(globalState, luaopen_os_rbx);

		Enums::declareAllEnums(globalState);
	}

	if (!yieldEvent)
		yieldEvent.reset(new YieldingThreads(this));

	preventNewConnection = false;

	if (!gcJob)
	{
		gcJob = shared_ptr<TaskScheduler::Job>(new GcJob(shared_from(this)));
		TaskScheduler::singleton().add(gcJob);
	}

	if (!waitingScriptsJob)
	{
		waitingScriptsJob = shared_ptr<TaskScheduler::Job>(new WaitingScriptsJob(shared_from(this)));
		TaskScheduler::singleton().add(waitingScriptsJob);
	}

    globalStates[idx].state = globalState;
    return true; // this prevents eax from holding globalState by accident
}

void ScriptContext::setKeys(unsigned int scriptKey, unsigned int coreScriptModKey)
{
    globalStates[Security::VM_Default].state->l_G->ckey = scriptKey;

    this->coreScriptModKey = coreScriptModKey;
}

void ScriptContext::setRobloxPlace(bool robloxPlace)
{
	this->robloxPlace = robloxPlace;

	// slightly obfuscated message info, since don't want users trying to change this bool

	FASTLOG1(FLog::CoreScripts, "srp: %u", robloxPlace);
}
void ScriptContext::closeStates(bool resettingSimulation)
{
	preventNewConnection = true;
	if (resettingSimulation)
		return;

	statesClosed = true;
	
	commandLineSandbox.reset();

	cleanupModules();

	try
	{
		FASTLOG1(FLog::ScriptContextClose, "ScriptContext::closeState -- ScriptCount=%u", scripts.size());
		if (yieldEvent)
			FASTLOG1(FLog::ScriptContextClose, "ScriptContext::closeState -- WaitingThreads=%u", this->yieldEvent->waiterCount());

		scripts.clear();

		if (yieldEvent)
			FASTLOG1(FLog::ScriptContextClose, "ScriptContext::closeState -- LeakedWaitingThreads=%u", this->yieldEvent->waiterCount());

		this->yieldEvent.reset();
	}
	catch (RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Exception thrown while cleaning up Lua: %s", e.what());
		RBXASSERT(false);	// we should NEVER get an exception in here
	}

	if (gcJob)
	{
		TaskScheduler::singleton().removeBlocking(gcJob);
		gcJob.reset();
	}
	if (waitingScriptsJob)
	{
		TaskScheduler::singleton().removeBlocking(waitingScriptsJob);
		waitingScriptsJob.reset();
	}

	waitingThreads.clear();

	for (GlobalStates::iterator iter = globalStates.begin(); iter != globalStates.end(); ++iter)
	{
		if (iter->state)
		{
			closeState(iter->state);
            iter->state = NULL;
		}
	}

	if (allocator)
	{
		size_t heapSize;
		size_t heapCount;
		size_t maxHeapSize;
		size_t maxHeapCount;
		allocator->getHeapStats(heapSize, heapCount, maxHeapSize, maxHeapCount);
		FASTLOG2(FLog::ScriptContextClose, "ScriptContext::closeState -- heapSize=%u heapCount=%u", heapSize, heapCount);
		FASTLOG2(FLog::ScriptContextClose, "ScriptContext::closeState -- maxHeapSize=%u maxHeapCount=%u", maxHeapSize, maxHeapCount);

		allocator->clearHeapMax();

		if (contextCount==0)
			if (heapSize>0 || heapCount>0)
				StandardOut::singleton()->printf(MESSAGE_ERROR, "Script memory leaks: %d bytes, %d blocks", (int)heapSize, (int)heapCount);
	}
}

void ScriptContext::closeState(lua_State* globalState)
{
	try
	{
		dumpThreadRefCounts();
		RobloxExtraSpace::get(globalState)->eraseRefsFromAllNodes();
		dumpThreadRefCounts();
	}
	catch (std::exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Exception thrown while erasing Lua refs: %s", e.what());
		RBXASSERT(false);	// we should NEVER get an exception in here
	}

	dumpThreadRefCounts();
	lua_close(globalState);
	dumpThreadRefCounts();
	contextCount--;
}

void ScriptContext::onCamelCaseViolation(shared_ptr<Instance> object, std::string memberName, shared_ptr<Instance> script)
{
	if (++camelCaseViolationCount > 100)
	{
		camelCaseViolationConnection.disconnect();
		return;
	}
	std::string trueName = memberName;
	trueName[0] = toupper(memberName[0]);

	if (script)
		StandardOut::singleton()->printf(MESSAGE_WARNING, "\"%s.%s\" should be \"%s\" in %s", object->getClassName().c_str(), memberName.c_str(), trueName.c_str(), script->getFullName().c_str());
	else
		StandardOut::singleton()->printf(MESSAGE_WARNING, "\"%s.%s\" should be \"%s\"", object->getClassName().c_str(), memberName.c_str(), trueName.c_str());
}

ScriptContext::ScriptContext()
:scriptsDisabled(false)
,preventNewConnection(false)
,robloxPlace(false)
,nextPendingScripts(RBX::Time::now<Time::Fast>())
,timedout(false)
,startScriptReentrancy(0)
,endTimoutThread(false)
,checkTimeout(false)
,collectScriptStats(false)
,avgLuaGcInterval(0.05)
,avgLuaGcTime(0.05)
,camelCaseViolationCount(0)
,timedoutCount(0)
,coreScriptModKey(LUAVM_MODKEY_DUMMY)
,statesClosed(false)
{
	setName("Script Context");
    
	camelCaseViolationConnection = camelCaseViolation.connect(boost::bind(&ScriptContext::onCamelCaseViolation, this, _1, _2, _3));
}

ScriptContext::~ScriptContext()
{
}

shared_ptr<const Reflection::Tuple> ScriptContext::getScriptStats()
{
	if(scriptStats){
		const ScriptStats::ScriptActivityMeterMap& stats = scriptStats->getScriptActivityMap();
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(stats.size()));
        int pos = 0;
		for(ScriptStats::ScriptActivityMeterMap::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
			std::string hash = iter->first;
			shared_ptr<Reflection::Tuple> entry(new Reflection::Tuple(5));
			entry->values[0] = hash;									//Hash

			std::map<std::string, ScriptStatInformation>::const_iterator hashCountIter = scriptHashInfo.find(hash);
			if(hashCountIter != scriptHashInfo.end()){
				entry->values[1] = (std::string)hashCountIter->second.name;	//Name
				entry->values[2] = (int)hashCountIter->second.scripts.size();		//Count
			}
			else{
				entry->values[1] = std::string("[Unknown]");	//Name
				entry->values[2] = (int)-1;						//Count
			}
			entry->values[3] = (double)iter->second.activity->averageValue() * 100;	//Activity
			entry->values[4] = (double)iter->second.invocations->getTotalValuePerSecond();		//InvocationCount

			result->values[pos++] = shared_ptr<const Reflection::Tuple>(entry);
		}

		return result;
	}
	return shared_ptr<Reflection::Tuple>();
}


void ScriptContext::getScriptStatsTyped(std::vector<ScriptStat>& result)
{
	if(!scriptStats)
		throw std::runtime_error("Script stats collection is not enabled");

	const ScriptStats::ScriptActivityMeterMap& stats = scriptStats->getScriptActivityMap();
	for(ScriptStats::ScriptActivityMeterMap::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
	{
		std::string hash = iter->first;

		ScriptStat entry;
		entry.hash = hash;
		std::map<std::string, ScriptStatInformation>::const_iterator hashCountIter = scriptHashInfo.find(hash);
		if(hashCountIter != scriptHashInfo.end())
		{
			entry.name = (std::string)hashCountIter->second.name;
			entry.scripts = hashCountIter->second.scripts;
		}
		else
		{
			entry.name = "[Unknown]";
		}
		entry.activity = (double)iter->second.activity->averageValue() * 100;
		entry.invocationCount = (double)iter->second.invocations->getTotalValuePerSecond();

		result.push_back(entry);
	}
}


shared_ptr<const Reflection::ValueArray> ScriptContext::getScriptStatsNew()
{
	if(!scriptStats)
		throw std::runtime_error("Script stats collection is not enabled");

	const ScriptStats::ScriptActivityMeterMap& stats = scriptStats->getScriptActivityMap();
	shared_ptr<Reflection::ValueArray> result(rbx::make_shared<Reflection::ValueArray>(stats.size()));
	int pos = 0;
	for(ScriptStats::ScriptActivityMeterMap::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
	{
		std::string hash = iter->first;
		shared_ptr<Reflection::ValueTable> entry(new Reflection::ValueTable());
		(*entry)["Hash"] = hash;

		std::map<std::string, ScriptStatInformation>::const_iterator hashCountIter = scriptHashInfo.find(hash);
		if(hashCountIter != scriptHashInfo.end()){
			(*entry)["Name"] = (std::string)hashCountIter->second.name;
			(*entry)["Scripts"] = shared_ptr<const Instances>(new Instances(hashCountIter->second.scripts));
		}
		else{
			(*entry)["Name"] = std::string("[Unknown]");
			(*entry)["Scripts"] = shared_ptr<const Instances>();
		}
		(*entry)["Activity"] = (double)iter->second.activity->averageValue() * 100;
		(*entry)["InvocationCount"] = (double)iter->second.invocations->getTotalValuePerSecond();

		(*result)[pos++] = shared_ptr<const Reflection::ValueTable>(entry);
	}

	return result;
}

shared_ptr<const Reflection::Tuple> ScriptContext::getHeapStats(bool clearHighwaterMark)
{
	size_t heapSize;
	size_t heapCount;
	size_t maxHeapSize;
	size_t maxHeapCount;
	allocator->getHeapStats(heapSize, heapCount, maxHeapSize, maxHeapCount);

	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(4));
	result->values[0] = (int)heapSize;
	result->values[1] = (int)heapCount;
	result->values[2] = (int)maxHeapSize;
	result->values[3] = (int)maxHeapCount;

	if (clearHighwaterMark)
		allocator->clearHeapMax();

	return result;

}

void ScriptContext::setTimeout(double seconds)
{
	boost::mutex::scoped_lock lock(timeoutMutex);

	timoutSpan = Time::Interval(seconds);

	if (timoutSpan.isZero())
		timoutTime = Time();
	else
	{
		FASTLOG1(FLog::ScriptContext, "Set lua time out: %d", (int)seconds);
		timoutTime = Time::now<Time::Fast>() + timoutSpan;
		if (!timeoutThread)
			timeoutThread.reset(new boost::thread(RBX::thread_wrapper(boost::bind(&ScriptContext::onCheckTimeout, shared_from(this)), "ScriptContext Timeout")));
		else
			checkTimeout.Set();
	}
}


#ifndef _WIN32
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif

void ScriptContext::onCheckTimeout()
{
	Time::Interval sleepTimeInterval = timoutSpan;

	while (true)
	{
		{
			double sleepTime = sleepTimeInterval.seconds();
			int ticks = sleepTime > 0 ? std::max(1,(int)(1010.0 * sleepTime)) : INFINITE;
			checkTimeout.Wait(ticks);
			if (endTimoutThread)
				return;	// endTimoutThread was set
		}

		{
			boost::mutex::scoped_lock lock(timeoutMutex);
			if (!timoutSpan.isZero())
			{
				sleepTimeInterval = timoutTime - Time::now<Time::Fast>();
				if (sleepTimeInterval.seconds()<=0)
				{
					// We've timed out!
					sleepTimeInterval = timoutSpan;
					if (timedout.swap(1)==0)
						++timedoutCount;
				}
			}
			else
				sleepTimeInterval = Time::Interval(-1);	// disabled
		}
	}
}

void ScriptContext::sandboxThread(lua_State* thread)
// Give the thread its own globals (so that the trustKey is hidden from others)
{
	RBXASSERT_BALLANCED_LUA_STACK(thread);

	//thread is an independent thread, but its globals
	//table is still the same as LA's globals table,
	//so we'll replace it with a new table.
	//The new table is set up so it will still look
	//to the spawning thread's globals for items it doesn't contain.

	lua_newtable(thread);                      // new globals table
	lua_newtable(thread);                      // metatable, new globals table
	Lua::protect_metatable(thread, -1);

	lua_pushliteral(thread, "__index");        // __index, metatable, new globals table
	lua_pushvalue(thread, LUA_GLOBALSINDEX);   // original globals, __index, metatable, new globals table

	// metatable[__index] = original globals
	lua_settable(thread, -3);                  // metatable, new globals table
	lua_setmetatable(thread, -2);              // new globals table

	//replace LB's globals
	lua_replace(thread, LUA_GLOBALSINDEX);     // [empty]
}


void ScriptContext::setThreadIdentityAndSandbox(lua_State* thread, RBX::Security::Identities identity, shared_ptr<BaseScript> script)
{
	sandboxThread(thread);

	RobloxExtraSpace::get(thread)->identity = identity;
	RobloxExtraSpace::get(thread)->script = script;

	RBXASSERT(getThreadIdentity(thread)==identity);
}

size_t ScriptContext::getThreadCount() const {
	size_t count = 0;
	for (GlobalStates::const_iterator iter = globalStates.begin(); iter != globalStates.end(); ++iter)
		if (iter->state)
			count += RobloxExtraSpace::get(iter->state)->getThreadCount();
	return count;
}

RBX::Security::Identities ScriptContext::getThreadIdentity(lua_State* thread)
{
	return thread ? RobloxExtraSpace::get(thread)->identity : RBX::Security::Anonymous;
}

ScriptContext& ScriptContext::getContext(lua_State* thread)
{
	return *RobloxExtraSpace::get(thread)->context();
}

lua_State* ScriptContext::getGlobalState(lua_State* thread)
{
    ScriptContext& context = getContext(thread);

    for (GlobalStates::const_iterator iter = context.globalStates.begin(); iter != context.globalStates.end(); ++iter)
        if (iter->state && iter->state->l_G == thread->l_G)
            return iter->state;

    return NULL;
}

static size_t pushNoArguments(lua_State* thread)
{
	return 0;
}

void ScriptContext::executeInNewThread(RBX::Security::Identities identity, const ProtectedString& script, const char* name)
{
	executeInNewThread(
		identity, 
		script,
		name, 
		boost::bind(&pushNoArguments, _1),
		NULL,
		Scripts::Continuations()
		);
}

static void readResults(std::auto_ptr<Reflection::Tuple>& result, lua_State* thread, size_t returnCount)
{
	result.reset(new Reflection::Tuple(returnCount));
	for (size_t i = 0; i<returnCount; ++i)
	{
		Reflection::Variant& v = result->values.at(i);
		LuaArguments::get(thread, i+1, v, true);
	}
}

Reflection::Tuple ScriptContext::callInNewThread(Lua::WeakFunctionRef& function, const Reflection::Tuple& arguments)
{
    ThreadRef functionThread = function.lock();
    if (!functionThread)
        throw std::runtime_error("Script that implemented this callback has been destroyed");
    
    // Create a child thread that will execute the callback
    ThreadRef callbackThread = lua_newthread(functionThread);
    lua_pop(functionThread, 1);

    // Execute the function; we don't care about the results/error
    lua_pushfunction(callbackThread, function);
	
	std::auto_ptr<Reflection::Tuple> results;
	resume(callbackThread, boost::bind(&LuaArguments::pushTuple, boost::cref(arguments), _1), boost::bind(&readResults, boost::ref(results), _1, _2));

	return *results;
}

std::auto_ptr<Reflection::Tuple> ScriptContext::executeInNewThread(
		RBX::Security::Identities identity, const ProtectedString& script,
		const char* name, const Reflection::Tuple& arguments)
{
	VMProtectBeginMutation("1");
	std::auto_ptr<Reflection::Tuple> result;
	executeInNewThread(
		identity, 
		script,
		name, 
		boost::bind(&LuaArguments::pushTuple, boost::cref(arguments), _1),
		boost::bind(&readResults, boost::ref(result), _1, _2),
		Scripts::Continuations()
		);
	VMProtectEnd();

	return result;
}

void ScriptContext::executeInNewThreadWithExtraGlobals(
	RBX::Security::Identities identity, const ProtectedString& script,
	const char* name, const std::map<std::string, shared_ptr<Instance> >& extraGlobals)
{
	VMProtectBeginMutation("2");
	executeInNewThread(
		identity,
		script,
		name,
		boost::bind(&pushNoArguments, _1),
		NULL,
		Scripts::Continuations(),
		NULL,
		&extraGlobals);
	VMProtectEnd();
}

Lua::Continuations::Continuations(const Scripts::Continuations& eh)
{
	if (eh.successHandler)
		success = boost::bind(&Continuations::onSuccessHandler, _1, eh.successHandler);
	if (eh.errorHandler)
		error = boost::bind(&Continuations::onErrorHandler, _1, eh.errorHandler);
}

void Lua::Continuations::onSuccessHandler(lua_State* thread, Scripts::SuccessHandler handler)
{
	RBXASSERT(!handler.empty());

	shared_ptr<Reflection::Tuple> args = Lua::LuaArguments::getValues(thread);
	handler(args);
}

void Lua::Continuations::onErrorHandler(lua_State* thread, Scripts::ErrorHandler handler)
{
	RBXASSERT(!handler.empty());

	const char* error = lua_tostring(thread, -1);	
	if (!error || strlen(error)==0)
		error = "An error occurred";		

	shared_ptr<BaseScript> source;
	int line;
	std::string callStack = ScriptContext::extractCallStack(thread, source, line);
	handler(error, callStack.c_str(), source, line);
}

void ScriptContext::executeInNewThread(RBX::Security::Identities identity, const ProtectedString& script, const char* name,
	boost::function1<size_t, lua_State*> pushArguments, 
	boost::function2<void, lua_State*, size_t> readImmediateResults, 
	Scripts::Continuations continuations,
	lua_State* globalStateToExecuteIn,
	const std::map<std::string, shared_ptr<Instance> >* extraGlobals,
    unsigned int modkey)
{
	
#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD)
    VMProtectBeginMutation("3");
	// check xxhash integrity
	static const char* kXHIntData = STRING_BY_ID(ExecScriptNewThread);
	static const unsigned int kIntermediateGolden = 976374109;
	static const unsigned int kGolden = 0xCDF961F8;
	// Note: keep usage of xxhash api here in line with ProgramMemoryChecker useage
	// Specifically, make sure all reachabile lines in xxhash code are exercised here.
	void* hashState = XXH32_init(42);
	// need to exercise all branches in feed
	XXH32_feed(hashState, kXHIntData, 15); // update + remnant < 16 also size % 4 !=0
	//FASTLOG1(FLog::US14116, "ScriptInt = %u", XXH32_getIntermediateResult(hashState));
	DataModel::get(this)->addHackFlag((XXH32_getIntermediateResult(hashState) != kIntermediateGolden) *
		HATE_XXHASH_BROKEN);
	XXH32_feed(hashState, kXHIntData + 15, strlen(kXHIntData + 15)); // prevStreamed != 0
	// need to assert that total length % 16 != 0 (that is the last branch in XXH32_feed)
	//RBXASSERT_SLOW(strlen(kXHIntData) % 16 != 0);
	DataModel::get(this)->addHackFlag((XXH32_result(hashState) != kGolden) *
		HATE_XXHASH_BROKEN); 
#endif

	WeakThreadRef thread;
	lua_State* owner;

    const bool isCmdLine = (identity==RBX::Security::CmdLine_);
    const bool clSandboxEmpty = (commandLineSandbox.empty());
#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD)
    VMProtectEnd();
#endif

	if (isCmdLine)
	{
		if (clSandboxEmpty)
		{
			lua_State* globalState = getGlobalState(RBX::Security::CmdLine_);
			RBXASSERT_BALLANCED_LUA_STACK(globalState);
		    initializeLuaStateSandbox(commandLineSandbox, globalState, RBX::Security::CmdLine_);
			lua_pop(globalState, 1);
		}
		ThreadRef safeCommandlineSandbox = commandLineSandbox.lock();
		RBXASSERT(safeCommandlineSandbox);
			
		owner = safeCommandlineSandbox;
		thread = lua_newthread(owner);
	}
	else
	{
		if(globalStateToExecuteIn) // we have an explicit vm to execute in, use this
			owner = globalStateToExecuteIn;
		else // otherwise, find an appropriate vm for our identity
			owner = getGlobalState(identity);

        initializeLuaStateSandbox(thread, owner, identity);
	}

	if (ThreadRef safeThread = thread.lock())
	{
		if (extraGlobals)
		{
			for (std::map<std::string, shared_ptr<Instance> >::const_iterator itr = extraGlobals->begin();
				itr != extraGlobals->end(); ++itr)
			{
				ObjectBridge::push(safeThread, itr->second);
				lua_setglobal( safeThread, itr->first.c_str() );
			}
		}

		// Pop the thread from the stack when we leave
		Lua::ScopedPopper popper(owner, 1);

		std::string sName = "=";
		if (name)
			sName += name;
		else
			sName += STRING_BY_ID(ScriptStr);

		bool luaFailedLoad = LuaVM::load(safeThread, script, sName.c_str(), modkey) != 0;

		if (luaFailedLoad)
		{
			std::string err = safe_lua_tostring(safeThread, -1);
			lua_pop(safeThread, 1);		// pop the message

			//StandardOut::singleton()->printf(MESSAGE_ERROR, err.c_str());
			throw std::runtime_error(err);
		}

		RBXASSERT(continuations.empty());
		RobloxExtraSpace::get(safeThread)->continuations.reset(new Lua::Continuations(continuations));

		// Check for somebody using CheatEngine to inject a direct call.
		// They probably haven't used a scoped_write_request
#if !defined(RBX_STUDIO_BUILD)
        VMProtectBeginMutation("4");
		if (DataModel* dataModel = DataModel::get(this))
		{
			if (!dataModel->currentThreadHasWriteLock())
				dataModel->addHackFlag(HATE_ILLEGAL_SCRIPTS);
		}
        VMProtectEnd();
#endif

		resume(safeThread, pushArguments, readImmediateResults);
	}
	else{
		StandardOut::singleton()->print(MESSAGE_ERROR, STRING_BY_ID(EnableToCreateNewThread));
		throw std::runtime_error(STRING_BY_ID(EnableToCreateNewThread));
	}
}

// On entry thread's stack contains a function to resume
void ScriptContext::resume(ThreadRef thread, boost::function1<size_t, lua_State*> pushArguments, boost::function2<void, lua_State*, size_t> readResults)
{
	const int stackSize = lua_gettop(thread);

	// TODO: Exception handling. If this throws, what kind of cleanup do we need to do???
	int argCount = pushArguments(thread);

	if (resume(thread, argCount) != Error)
	{
		// Collect all the return arguments into a Tuple
		const int returnCount = lua_gettop(thread) - stackSize + 1;
		if (readResults)
			try
			{
				readResults(thread, returnCount);
			}
			catch (RBX::base_exception&)
			{
				// Clean up the stack
				lua_pop(thread, returnCount);
				throw;
			}

		// Clean up the stack
		lua_pop(thread, returnCount);
	}
	else
	{
		// Only throw if we are using immediate results. Otherwise assume errors
		// are handled by continuations
		if (readResults)
		{
			std::string err = safe_lua_tostring(thread, -1);
			// Clear the stack
			lua_resetstack(thread, stackSize);
			throw std::runtime_error(err);
		}

		// Clear the stack
		lua_resetstack(thread, stackSize);
	}
}

int ScriptContext::spawn(lua_State *thread)
{
	//Keep a live reference to the thread so it doesn't get away from us
	ThreadRef threadLock(thread);

	RBXASSERT_BALLANCED_LUA_STACK(thread);

	if  (lua_gettop(thread)<1)
		throw std::runtime_error("Spawn function requires 1 argument");

	if (DFFlag::BadTypeOnSpawnErrorEnabled)
	{
		Reflection::Variant varResult;
		Lua::LuaArguments::get(thread, 1, varResult, false);
		if (!varResult.isType<Lua::WeakFunctionRef>())
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Attempt to call spawn failed: Passed value is not a function");
			ScriptContext::printCallStack(thread);
		}
	}

	// Create a callback thread
	WeakThreadRef functor(::lua_newthread(thread));
	ThreadRef safeFunctor = functor.lock();
	RBXASSERT(safeFunctor);

	// Push the callable argument onto the stac
	// NOTE: This could be a function *or* perhaps a table with a "call" metamethod
	lua_pushvalue(thread, 1);

	// Transfer function to the callback thread
	lua_xmove(thread, safeFunctor, 1);

	// Register the callback thread for later execution
	Lua::YieldingThreads* yieldEvent = ScriptContext::getContext(thread).yieldEvent.get();
	if(yieldEvent)
		yieldEvent->queueWaiter(safeFunctor, 0);

	// Pop the callback thread from the stack
	lua_pop(thread, 1);

	return 0;
}

static void cleanTimeout(LUA_NUMBER& timeout)
{
	if (timeout < RBX::LuaSettings::singleton().defaultWaitTime)
		timeout = RBX::LuaSettings::singleton().defaultWaitTime;
	else if (Math::isNanInf(timeout))
		timeout = RBX::LuaSettings::singleton().defaultWaitTime;
}

int ScriptContext::delay(lua_State *thread)
{
	ThreadRef threadLock(thread);

	RBXASSERT_BALLANCED_LUA_STACK(thread);

	if  (lua_gettop(thread)<2)
		throw std::runtime_error("delay function requires 2 arguments");

	if (DFFlag::BadTypeOnDelayErrorEnabled)
	{
		Reflection::Variant varResult;
		Lua::LuaArguments::get(thread, 2, varResult, false);
		if (!varResult.isType<Lua::WeakFunctionRef>())
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Attempt to call delay failed: Passed value is not a function");
			ScriptContext::printCallStack(thread);
		}
	}

	// First argument is the amount of time to wait
	LUA_NUMBER timeout = lua_tonumber(thread, 1);
    cleanTimeout(timeout);

	// Create a callback thread
	WeakThreadRef functor(::lua_newthread(thread));
	ThreadRef safeFunctor = functor.lock();
	RBXASSERT(safeFunctor);

	// Push the callable argument onto the stack
	// NOTE: This could be a function *or* perhaps a table with an "call" metamethod
	lua_pushvalue(thread, 2);

	// Transfer function to the callback thread
	lua_xmove(thread, safeFunctor, 1);

	// Register the callback thread for later execution

	Lua::YieldingThreads* yieldEvent = ScriptContext::getContext(thread).yieldEvent.get();
	if(yieldEvent)
		yieldEvent->queueWaiter(safeFunctor, timeout);

	// Pop the callback thread from the stack
	lua_pop(thread, 1);

	return 0;
}

// "wait" returns 2 arguments:
//      1) Time elapsed since call
//      2) Current time (sim time, or Heartbeat::time)
int ScriptContext::wait(lua_State *thread)
{
	// First argument is the amount of time to wait
	LUA_NUMBER timeout = lua_tonumber(thread, 1);
    cleanTimeout(timeout);
	
	Lua::YieldingThreads* yieldEvent = ScriptContext::getContext(thread).yieldEvent.get();
	if(yieldEvent)
		yieldEvent->queueWaiter(thread, timeout);

	return lua_yield(thread, 0);
}

void ScriptContext::on_ypcall_success(WeakThreadRef caller, lua_State* functor)
{
	if (ThreadRef thread = caller.lock())
	{
		lua_pushboolean(thread, true);
		const int nargs = lua_gettop(functor);
		lua_xmove(functor, thread, nargs);
		resume(thread, 1 + nargs);
	}
}

void ScriptContext::on_ypcall_failure(WeakThreadRef caller, lua_State* functor)
{
	if (ThreadRef thread = caller.lock())
	{
		lua_pushboolean(thread, false);
		lua_pushstring(thread, safe_lua_tostring(functor, -1));

		resume(thread, 2);
	}
}

int ScriptContext::ypcall(lua_State *thread)
{
	ThreadRef threadLock(thread);

	const int top = lua_gettop(thread);

	if  (top < 1)
		throw std::runtime_error("ypcall requires at least 1 argument"); // TODO: What is pcall's error message?

	if (FFlag::LuaDebuggerBreakOnError)
	{
		int argType = lua_type(thread, 1);
		if (!argType)
			throw std::runtime_error("attempt to call a nil value");
	}

	// Create a callback thread
	WeakThreadRef functor(::lua_newthread(thread));
	ThreadRef safeFunctor = functor.lock();
	RBXASSERT(safeFunctor);

	// Push the callable argument onto the stack
	// NOTE: This could be a function *or* perhaps a table with an "call" metamethod
	for (int i = 1; i <= top; ++i)
		lua_pushvalue(thread, i);

	ScriptContext& sc = ScriptContext::getContext(thread);

	// Transfer function to the callback thread
	lua_xmove(thread, safeFunctor, top);

	// Pop the callback thread from the stack
	lua_pop(thread, 1);

	ScriptContext::Result result;
	switch (resumeImpl(safeFunctor, top - 1))
	{
	case 0:
		result = ScriptContext::Success;
		break;
	case LUA_YIELD:
		result = ScriptContext::Yield;
		break;
	default:
		result = ScriptContext::Error;
		break;
	}

	switch (result)
	{
	default:
		throw std::runtime_error("");

	case ScriptContext::Success:
		{
			RBXASSERT(top == lua_gettop(thread));

			lua_pushboolean(thread, true);
			const int nargs = lua_gettop(safeFunctor);
			lua_xmove(safeFunctor, thread, nargs);
			return nargs + 1;
		}

	case ScriptContext::Error:
		{
			lua_pushboolean(thread, false);

			const char* errorString = safe_lua_tostring(safeFunctor, -1);
			if (!errorString || strlen(errorString) == 0)
			{
				errorString = "An error occurred";
			}
			lua_pushstring(thread, errorString);

			// balance the thread's stack
			lua_resetstack(safeFunctor, 0);
			return 2;
		}

	case ScriptContext::Yield:
		{
			Lua::Continuations continuations;
			continuations.success = boost::bind(&ScriptContext::on_ypcall_success, &sc, WeakThreadRef(thread), _1);
			continuations.error = boost::bind(&ScriptContext::on_ypcall_failure, &sc, WeakThreadRef(thread), _1);

			RBXASSERT(RobloxExtraSpace::get(safeFunctor)->continuations.get() == NULL);
			RobloxExtraSpace::get(safeFunctor)->continuations.reset(new Lua::Continuations(continuations));

			//Capture the yield
			RobloxExtraSpace::get(thread)->yieldCaptured = true;

			// TODO: 0 args?
			return lua_yield(thread, 0);
		}
	}

}

int ScriptContext::settings(lua_State *thread)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::Plugin, "settings()");

	ObjectBridge::push(thread, GlobalAdvancedSettings::singleton());
	return 1;
}

int ScriptContext::usersettings(lua_State *thread)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::None, "UserSettings()");

	ObjectBridge::push(thread, GlobalBasicSettings::singleton());
	return 1;
}


int ScriptContext::pluginmanager(lua_State *thread)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::Plugin, "PluginManager()");

	ObjectBridge::push(thread, PluginManager::singleton());
	return 1;
}

int ScriptContext::debuggermanager(lua_State *thread)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::Plugin, "DebuggerManager()");

	ObjectBridge::push(thread, shared_from(&RBX::Scripting::DebuggerManager::singleton()));
	return 1;
}

int ScriptContext::printidentity(lua_State *thread)
{
	if (lua_gettop(thread)>0)
		StandardOut::singleton()->printf(MESSAGE_OUTPUT, "%s %d", lua_tostring(thread, -1), Security::Context::current().identity);
	else
		StandardOut::singleton()->printf(MESSAGE_OUTPUT, "Current identity is %d", Security::Context::current().identity);
	return 0;
}

static int load_aux (lua_State *L, int status) {
	if (status == 0)  /* OK? */
		return 1;
	else {
		lua_pushnil(L);
		lua_insert(L, -2);  /* put before error message */
		return 2;  /* return nil plus error message */
	}
}

static bool isValidLibraryName(const std::string& name)
{
    for (size_t i = 0; i < name.size(); ++i)
        if (!isalpha(name[i]))
            return false;

    return true;
}

int ScriptContext::loadLibrary(lua_State *L)
{
	std::string libraryName = throwable_lua_tostring(L, -1);

    if (!isValidLibraryName(libraryName))
    {
		lua_pushnil(L);
		lua_pushstring(L, "Unknown library " + libraryName);
        return 2;
    }

	// Returns 1 if it finds table, 2 if it finds an error, and 0 if it finds nothing
	int results = Lua::LibraryBridge::find(L, libraryName);

    if (results != 0)
        return results;

    // First-time load into this VM
    ScriptContext& context = getContext(L);

    boost::optional<ProtectedString> source = CoreScript::fetchSource("Libraries/" + libraryName);

    if (!source)
    {
        lua_pushnil(L);
        lua_pushstring(L, "Unknown library " + libraryName);
        return 2;
    }

    // On the client, coreScriptModKey is fetched from the server in the packet and represents
    // the transform needed to convert from fixed core-script key to dynamic instance key.
    // Loading core script bytecode into privileged VM does not require conversion since the keys
    // match; loading it into the normal VM has to happen with the modkey from the server.
    unsigned int modkey =
        (getGlobalState(L) == context.globalStates[Security::VM_RobloxScriptPlus].state)
        ? LUAVM_MODKEY_DUMMY
        : context.coreScriptModKey;

    context.executeInNewThread(
        Security::GameScript_, source.get(), libraryName.c_str(),
        boost::bind(&pushNoArguments, _1),
        boost::bind(&LibraryBridge::saveLibraryResult, _1, _2, libraryName),
        Scripts::Continuations(),
        getGlobalState(L),
        NULL,
        modkey);

    // Libraries can't use yielding in the definition so it must be immediately available
	results = Lua::LibraryBridge::find(L, libraryName);
    RBXASSERT(results > 0);

	return results;
}

int ScriptContext::warn(lua_State *L)
{
	return doPrint(L, MESSAGE_WARNING);
}

static void resumeThreadsWithValue(std::vector<WeakThreadRef>& threads, int referenceId)
{
	for (std::vector<WeakThreadRef>::iterator itr = threads.begin();
		itr != threads.end(); ++itr)
	{
		if (ThreadRef thread = itr->lock())
		{
			lua_rawgeti(thread, LUA_REGISTRYINDEX, referenceId);
			ScriptContext::getContext(thread).resume(thread, 1);
		}
	}
}

static void endThreadsWithError(std::vector<WeakThreadRef>& threads, const char* message)
{
	for (std::vector<WeakThreadRef>::iterator itr = threads.begin(); itr != threads.end(); ++itr)
	{
		if (ThreadRef thread = itr->lock())
		{
			lua_pushstring(thread, message);

			RobloxExtraSpace* space = RobloxExtraSpace::get(thread);
			space->context()->reportError(thread);
			if (space->continuations && space->continuations->error)
			{
				space->continuations->error(thread);
			}
		}
	}
}

void ScriptContext::requireModuleScriptSuccessContinuation(shared_ptr<ModuleScript> moduleScript,
	lua_State* threadRunningModuleCode)
{
	// Check to see if there is exactly 1 value returned from the script
	if (lua_gettop(threadRunningModuleCode) != 1)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, kModuleDidNotReturnOneValue);
		requireModuleScriptErrorContinuation(moduleScript, threadRunningModuleCode);
		return;
	}

	// take a reference to the top of the module thread's stack
	int libraryResultRef = luaL_ref(threadRunningModuleCode, LUA_REGISTRYINDEX);
	lua_State* globalState = ScriptContext::getGlobalState(threadRunningModuleCode);
	ModuleScript::PerVMState& vmState = moduleScript->vmState(globalState);
	// store the reference in the module script, along with the global state
	vmState.setCompletedSuccess(globalState, libraryResultRef);

	std::vector<WeakThreadRef> yieldedStates;
	vmState.getAndClearYieldedImporters(&yieldedStates);
	resumeThreadsWithValue(yieldedStates, libraryResultRef);

    if (moduleScript->getReloadRequested())
    {
        moduleScript->setReloadRequested(false);
        ScriptContext::getContext(threadRunningModuleCode).reloadModuleScript(moduleScript);
    }
}

void ScriptContext::requireModuleScriptErrorContinuation(shared_ptr<ModuleScript> moduleScript,
	lua_State* threadRunningModuleCode)
{
	lua_State* globalState = ScriptContext::getGlobalState(threadRunningModuleCode);
	ModuleScript::PerVMState& vmState = moduleScript->vmState(globalState);
	vmState.setCompletedError();

	std::vector<WeakThreadRef> yieldedStates;
	vmState.getAndClearYieldedImporters(&yieldedStates);
	endThreadsWithError(yieldedStates, kModuleFailedOnLoadMessage);

    if (moduleScript->getReloadRequested())
    {
        moduleScript->setReloadRequested(false);
		ScriptContext::getContext(threadRunningModuleCode).reloadModuleScript(moduleScript);
    }
	// Let the ThreadRefs leave scope without resuming them. They should get GCed.
}

void ScriptContext::reloadModuleScriptSuccessContinuation(shared_ptr<ModuleScript> moduleScript,
                                                          lua_State* reloadThread,
                                                          int oldResultRegistryIndex)
{
	moduleScript->vmState(ScriptContext::getGlobalState(reloadThread))
		.reassignResultRegistryIndex(oldResultRegistryIndex);
}

void ScriptContext::reloadModuleScriptErrorContinuation(shared_ptr<ModuleScript> moduleScript,
                                                        lua_State* reloadThread)
{
    // empty
}

void ScriptContext::reloadModuleScript(shared_ptr<ModuleScript> moduleScript)
{
	for (GlobalStates::iterator itr = globalStates.begin(); itr != globalStates.end(); ++itr)
	{
		reloadModuleScriptInternal(itr->state, moduleScript);
	}
}

void ScriptContext::reloadModuleScriptInternal(lua_State* globalState, shared_ptr<ModuleScript> moduleScript)
{
	ModuleScript::PerVMState& vmState = moduleScript->vmState(globalState);
	ModuleScript::ScriptSetupState state = vmState.getCurrentState();

    if (state == ModuleScript::NotRunYet)
    {
        return;
    }
    else if (state == ModuleScript::Running)
    {
        moduleScript->setReloadRequested(true);
        return;
    }
    
    // Save off the current result of the script to be used later for patching this
    // result in place with the new one.
    int oldResultIndex = vmState.getResultRegistryIndex();

    // If we've never had a successful require of this module
    // then cleanup and prepare for rerun.
    if (oldResultIndex == LUA_NOREF)
    {
        ModuleScript::cleanupAndResetState(moduleScript);
        return;
    }
    
    moduleScript->resetState();
        
    lua_State* reloadThread = lua_newthread(globalState);
    
    // The reload will require the current module script and patch the old result with a newly
	// required result.
    const std::string reloadCode =
        "oldResult, moduleRef  = ...\n"
        "newResult = require(moduleRef)\n"
        "t1 = newResult\n"
        "t2 = oldResult\n"
        "if type(t2) ~= \"table\" then return end\n"
        "for k,v in pairs(t2) do\n"
        "    if type(t1[k]) == \"nil\" then \n"
        "        t2[k] = nil\n"
        "    end\n"
        "end\n"
        "for k,v in pairs(t1) do\n"
        "    t2[k] = v\n"
        "end\n"
    ;

    bool loaded = LuaVM::load(reloadThread, ProtectedString::fromTrustedSource(reloadCode), "") == 0;

    if (!loaded)
    {
        std::string err = Lua::safe_lua_tostring(reloadThread, -1);
        throw RBX::runtime_error("syntax error: %s", err.c_str());
    }
    
	// Push params.
    lua_rawgeti(reloadThread, LUA_REGISTRYINDEX, oldResultIndex);
    ObjectBridge::push(reloadThread, moduleScript);
    
    int reloadResult = resumeImpl(reloadThread, 2);
	
    if (reloadResult == LUA_YIELD)
    {
        // Because we're using the global state to spawn a thread, we no longer need to handle
        // yield capturing.
        RBXASSERT(RobloxExtraSpace::get(reloadThread)->continuations == NULL);
        Lua::Continuations continuations;
        continuations.success = boost::bind(&ScriptContext::reloadModuleScriptSuccessContinuation, moduleScript, _1, oldResultIndex);
        continuations.error = boost::bind(&ScriptContext::reloadModuleScriptErrorContinuation,
                                          moduleScript, _1);
        RobloxExtraSpace::get(reloadThread)->continuations.reset(new Lua::Continuations(continuations));
        return;
    }
    else if (reloadResult != 0)
    {
        const char* message = lua_tostring(reloadThread, -1);
        throw RBX::runtime_error("runtime error: %s", message);
    }
   
    // Unrefs the new state created by the reloadThread.
    vmState.reassignResultRegistryIndex(oldResultIndex);

    RBXASSERT(moduleScript->getReloadRequested() == false);
    moduleScript->setReloadRequested(false);
}

static shared_ptr<ModuleScript> getLoadedModuleFromModelOrNull(shared_ptr<Instances> instances)
{
	for (size_t i = 0; i < instances->size(); ++i)
	{
		if (shared_ptr<ModuleScript> module = Instance::fastSharedDynamicCast<ModuleScript>(instances->at(i)))
		{
			if (module->getName() == "MainModule")
			{
				return module;
			}
		}
	}
	return shared_ptr<ModuleScript>();
}

void ScriptContext::moduleContentLoaded(AsyncHttpQueue::RequestResult result, shared_ptr<Instances> instances,
	ScriptContext& sc, Security::Identities identity, lua_State* globalState, AssetModuleInfo* info)
{
	RBXASSERT(result != AsyncHttpQueue::Waiting);
	RBXASSERT(info->state == AssetModuleInfo::Fetching);

	shared_ptr<ModuleScript> foundModuleScript;

	if (result == AsyncHttpQueue::Succeeded)
	{
		foundModuleScript = getLoadedModuleFromModelOrNull(instances);	
	}

	LuaSourceContainer::loadLinkedScriptsForInstances(
		shared_from(ServiceProvider::create<ContentProvider>(&sc)), *instances, AsyncHttpQueue::AsyncWrite,
		boost::bind(&ScriptContext::moduleContentLinkedSourcesResolved,
			instances, foundModuleScript, boost::ref(sc), identity, globalState, info));
}

void ScriptContext::moduleContentLinkedSourcesResolved(shared_ptr<Instances> instances,
	shared_ptr<ModuleScript> foundModuleScript, ScriptContext& sc, Security::Identities identity, lua_State* globalState,
	AssetModuleInfo* info)
{
	// Pull yielded importers out of info, so that the weak thread refs
	// lifetime is tied to this scope.
	std::vector<WeakThreadRef> yieldedImporters;
	yieldedImporters.swap(info->yieldedImporters);

	if (!foundModuleScript)
	{
		info->state = AssetModuleInfo::Failed;
		info->module = shared_ptr<ModuleScript>();
		
		// fail all of the pending states
		endThreadsWithError(yieldedImporters, kModuleNotFoundForAssetId);
		
		return;
	}
	
	// mark complete before resuming any scripts, in case resuming the script
	// triggers another remote load of the same module.
	info->state = AssetModuleInfo::Fetched;
	info->module = foundModuleScript;
	info->root = Creatable<Instance>::create<ModelInstance>();
	for (size_t i = 0; i < instances->size(); ++i)
	{
		instances->at(i)->setParent(info->root.get());
	}

	sc.startRunningModuleScript(identity, globalState, info->module);

	ModuleScript::PerVMState& vmState = info->module->vmState(globalState);
	switch (vmState.getCurrentState())
	{
		case ModuleScript::CompletedError:
			endThreadsWithError(yieldedImporters, kModuleFailedOnLoadMessage);
			break;
		case ModuleScript::Running:
			// preserve the weak thread refs in the module's yielded list
			std::for_each(yieldedImporters.begin(), yieldedImporters.end(),
				boost::bind(&ModuleScript::PerVMState::addYieldedImporter, &vmState, _1));
			break;
		case ModuleScript::CompletedSuccess:
			resumeThreadsWithValue(yieldedImporters, vmState.getResultRegistryIndex());
			break;
		default:
			RBXASSERT(false);
	}	
}

void ScriptContext::startRunningModuleScript(Security::Identities identity, lua_State* rootGlobalState, shared_ptr<ModuleScript> moduleScript)
{
	validateThreadAccess(rootGlobalState);

	loadedModules.insert(moduleScript);

	ThreadRef thread = lua_newthread(rootGlobalState);
	lua_pop(rootGlobalState, 1);
	ModuleScript::PerVMState& vmState = moduleScript->vmState(rootGlobalState);
	vmState.setRunning(WeakThreadRef::Node::create(thread));

	setThreadIdentityAndSandbox(thread, identity, shared_ptr<BaseScript>());
	// Use impersonation to ensure we are all the way down to correct
	// permission level.
	ScriptImpersonator impersonate(thread);

	// declare "script" global
	ObjectBridge::push(thread, moduleScript);
	lua_setglobal(thread, "script"); 

	std::string name = std::string("=") + moduleScript->getFullName();

	// load the source
	bool loaded = false;
	if (moduleScript->getScriptId().isNull())
	{
		loaded = LuaVM::load(thread, moduleScript->getSource(), name.c_str()) == 0;
	}
	else if (moduleScript->getCachedRemoteSourceLoadState() == LuaSourceContainer::Loaded)
	{
		loaded = LuaVM::load(thread, moduleScript->getCachedRemoteSource(), name.c_str()) == 0;
	}
	
	if (!loaded)
	{
		vmState.setCompletedError();

		// report the original error (thread will vanish after this)
		reportError(thread);
		return;
	}

	// we are about to execute module script code, emit starting signal
	moduleScript->starting(thread);

	RBXASSERT(lua_gettop(thread) == 1);
	int luaResumeState = resumeImpl(thread, 0);
	bool returnedOneResult = lua_gettop(thread) == 1;

	if (luaResumeState == Success && returnedOneResult)
	{
		// Take a reference to the top of the stack in the module's thread (the return value).
		int libraryResultRef = luaL_ref(thread, LUA_REGISTRYINDEX);
		// Store the reference and global state in order to retrieve the value later.
		vmState.setCompletedSuccess(rootGlobalState, libraryResultRef);
	}
	else if (luaResumeState == Yield)
	{
		RBXASSERT(RobloxExtraSpace::get(thread)->continuations == NULL);
		Lua::Continuations continuations;
		continuations.success = boost::bind(&requireModuleScriptSuccessContinuation, moduleScript, _1);
		continuations.error = boost::bind(&ScriptContext::requireModuleScriptErrorContinuation,
			moduleScript, _1);
		RobloxExtraSpace::get(thread)->continuations.reset(new Lua::Continuations(continuations));
	}
	else
	{
		vmState.setCompletedError();

		if (luaResumeState == Success && !returnedOneResult)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, kModuleDidNotReturnOneValue);
		}
		else
		{
			// report the original error (thread will vanish after this)
			reportError(thread);
		}
	}
}

// Allows scripts to be run an their result to be loaded from another script
// running in the _same_ VM.
int ScriptContext::requireModuleScript(lua_State *L)
{
	LuaArguments args(L, 0);
	Reflection::Variant v;

	bool loadedVariant = args.getVariant(1, v);

	if (loadedVariant && v.isNumber())
	{
        if (Network::Players::clientIsPresent(&getContext(L)))
        {
            throw runtime_error("require(assetId) cannot be called from a client");
        }
		lua_pop(L, 1);
		return requireModuleScriptFromAssetId(L, v.get<int>());
	}

	if (loadedVariant && v.isType<shared_ptr<Instance> >())
	{
		if (shared_ptr<ModuleScript> moduleScript =
			Instance::fastSharedDynamicCast<ModuleScript>(v.cast<shared_ptr<Instance> >()))
		{
			lua_pop(L, 1);
			return requireModuleScriptFromInstance(L, moduleScript);
		}
	}

	throw runtime_error(
		"Attempted to call require with invalid argument(s).");
}

int ScriptContext::requireModuleScriptFromInstance(lua_State* L, shared_ptr<ModuleScript> moduleScript)
{
	lua_State* globalState = getGlobalState(L);
	ModuleScript::PerVMState& vmState = moduleScript->vmState(globalState);
	ModuleScript::ScriptSetupState state = vmState.getCurrentState();

	if (state == ModuleScript::NotRunYet)
	{
		getContext(L).startRunningModuleScript(Security::Context::current().identity, globalState, moduleScript);
		state = vmState.getCurrentState();
	}
	RBXASSERT(state != ModuleScript::NotRunYet);

	if (state == ModuleScript::CompletedSuccess)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, vmState.getResultRegistryIndex());
		return 1;
	}
	else if (state == ModuleScript::CompletedError)
	{
		// have module loader also error
		throw runtime_error(kModuleFailedOnLoadMessage);
	}
	else if (state == ModuleScript::Running)
	{
		vmState.addYieldedImporter(WeakThreadRef(L));

		RBXASSERT(RobloxExtraSpace::get(L)->yieldCaptured == false);
		RobloxExtraSpace::get(L)->yieldCaptured = true;
		return lua_yield(L, 0);
	}
	else // state == ModuleScript::NotRunYet, or some new state
	{
		throw runtime_error("Unknown module state: %d", state);
	}
}

int ScriptContext::requireModuleScriptFromAssetId(lua_State* L, int assetId)
{
	LoadedAssetModules& loadedAssetModules = getContext(L).loadedAssetModules;

	AssetModuleInfo& info = loadedAssetModules[assetId];

	if (info.state == AssetModuleInfo::NotFetchedYet)
	{
		info.state = AssetModuleInfo::Fetching;
		RBXASSERT(info.yieldedImporters.empty());
		info.yieldedImporters.push_back(WeakThreadRef(L));

		ContentProvider* contentProvider = ServiceProvider::find<ContentProvider>(&getContext(L));
		ContentId contentId;
		if (DFFlag::LogPrivateModuleRequires)
		{
			DataModel* dm = DataModel::get(contentProvider);
			contentId = ContentId(RBX::format("%s/asset/?id=%d&modulePlaceId=%d", ::trim_trailing_slashes(contentProvider->getBaseUrl()).c_str(), assetId, dm->getPlaceID()));
		}
		else
		{
			contentId = ContentId(RBX::format("rbxassetid://%d", assetId));
		}

		contentProvider->loadContent(
			contentId, ContentProvider::PRIORITY_INSERT,
			boost::bind(&ScriptContext::moduleContentLoaded, _1, _2, boost::ref(getContext(L)),
				Security::Context::current().identity, getGlobalState(L), &loadedAssetModules[assetId]),
			AsyncHttpQueue::AsyncWrite);

		RBXASSERT(RobloxExtraSpace::get(L)->yieldCaptured == false);
		RobloxExtraSpace::get(L)->yieldCaptured = true;
		return lua_yield(L, 0);
	}
	else if (info.state == AssetModuleInfo::Fetching)
	{
		info.yieldedImporters.push_back(WeakThreadRef(L));

		RBXASSERT(RobloxExtraSpace::get(L)->yieldCaptured == false);
		RobloxExtraSpace::get(L)->yieldCaptured = true;
		return lua_yield(L, 0);
	}
	else if (info.state == AssetModuleInfo::Failed)
	{
		throw runtime_error(kModuleNotFoundForAssetId);
	}
	else if (info.state == AssetModuleInfo::Fetched)
	{
		RBXASSERT(info.module != NULL);
		return requireModuleScriptFromInstance(L, info.module);
	}
	else
	{
		RBXASSERT(false);
		throw runtime_error("Unknown error with remote module loading asset id %d", assetId);
	}
}

int ScriptContext::notImplemented(lua_State *L)
{
	throw std::runtime_error("not implemented");
}

// Slightly more secure version of Lua's loadfile implementation
int ScriptContext::loadfile(lua_State *L)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::LocalUser, "loadfile");

	ContentId contentId(throwable_lua_tostring(L, -1)); /* get file name */
	
	if (contentId.isNull())
		throw RBX::runtime_error("Unable to load %s", contentId.c_str());

	if(!RBX::Network::isTrustedContent(contentId.c_str()))
		throw std::runtime_error("invalid request 5");

	Http request(contentId.toString());

	std::string message;
	request.get(message);

	//The script needs to be signed
	ProtectedString verifiedSource = ProtectedString::fromTrustedSource(message);
    ContentProvider::verifyScriptSignature(verifiedSource, true);

	return load_aux(L, LuaVM::load(L, verifiedSource, ("=" + contentId.toString()).c_str()));
}

static void sendLoadStringStats(int placeId)
{
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "loadstring", format("%d", placeId).c_str());
}

int ScriptContext::loadstring(lua_State *L)
{
	ScriptContext& context = ScriptContext::getContext(L);

    DataModel* dm = DataModel::get(&context);

	if (dm)
    {
        if (Network::Players::serverIsPresent(dm))
        {
            ServerScriptService* sss = ServiceProvider::find<ServerScriptService>(dm);

            if (sss && sss->getLoadStringEnabled())
            {
                static boost::once_flag flag = BOOST_ONCE_INIT;
                boost::call_once(flag, boost::bind(sendLoadStringStats, dm->getPlaceID()));
            }
            else
            {
                throw std::runtime_error("loadstring() is not available");
            }
        }
        else
        {
            if ((Network::Players::clientIsPresent(dm) && !Network::Players::isCloudEdit(dm)) || !LuaVM::canCompileScripts())
            {
                throw std::runtime_error("loadstring() is not available");
            }
        }
    }

    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    const char* chunkname = luaL_optstring(L, 2, s);

    return load_aux(L, LuaVM::load(L, ProtectedString::fromTrustedSource(std::string(s, l)), chunkname));
}

int ScriptContext::version(lua_State *L)
{
	lua_pushstring(L, DebugSettings::robloxVersion);
	return 1;
}


int ScriptContext::stats(lua_State *L)
{
	ObjectBridge::push(L, shared_from(RBX::ServiceProvider::create<RBX::Stats::StatsService>(&ScriptContext::getContext(L))));
	return 1;
}

int ScriptContext::dofile (lua_State *L)
{
    throw std::runtime_error("not implemented");
}


int ScriptContext::time(lua_State *L)
{
	lua_pushnumber(L, ScriptContext::getContext(L).runService->gameTime());
	return 1;
}

int ScriptContext::rbxTime(lua_State *L)
{
	lua_pushnumber(L, Time::now<Time::Fast>().timestampSeconds());
	return 1;
}

int ScriptContext::tick(lua_State *L)
{
	lua_pushnumber(L, G3D::System::time());
	return 1;
}

int ScriptContext::print(lua_State *L)
{
	return doPrint(L);
}

// Adapted from luaB_print
int ScriptContext::doPrint(lua_State *L, const RBX::MessageType& messageType)
{
	std::string message;

	int n = lua_gettop(L);  /* number of arguments */
	int i;

	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		if (i>1)
		{
			message += ' ';
		}
		
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		const char* s = lua_tostring(L, -1);  /* get result */
		if (s == NULL)
			throw std::runtime_error(LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		message += s;
		if (i>1) fputs("\t", stdout);
		lua_pop(L, 1);  /* pop result */
	}

	StandardOut::singleton()->print(messageType, message);
	return 0;
}

void ScriptContext::onHeartbeat(const Heartbeat& heartbeat)
{
	FASTLOG(FLog::ScriptContext, "Script context heartbeat start");
	if (timoutSpan.seconds()>0)
		timoutTime = Time::now<Time::Fast>() + timoutSpan;
	if (timedout.swap(0) == 1)
		--timedoutCount;

	if (RBX::Time::now<Time::Fast>() >= nextPendingScripts)
	{
		RBXASSERT(!kCLuaResumeStackSize.get() || (*kCLuaResumeStackSize) == 0);
		// Try starting pending scripts, because some of them might be postponed for the ScriptId to download...
		startPendingScripts();
		nextPendingScripts = RBX::Time::now<Time::Fast>() + RBX::Time::Interval(1);
		RBXASSERT(!kCLuaResumeStackSize.get() || (*kCLuaResumeStackSize) == 0);
	}

	FASTLOG(FLog::ScriptContext, "Script context heartbeat finish");
}

void ScriptContext::resumeWaitingScripts(const Time expirationTime)
{
	FASTLOG(FLog::ScriptContext, "ScriptContext::resumeWaitingScripts start");

	bool throttling = false;
	RBXASSERT(!kCLuaResumeStackSize.get() || (*kCLuaResumeStackSize) == 0);

	{
		WaitingThread wt;
		// Use count to prevent an infinite loop
		int count = waitingThreads.size();
		while (count-- > 0 && waitingThreads.pop_if_present(wt))
		{
			resumeWithArgs(wt.thread, wt.arguments);
			resumedThreads.sample();
			if (RBX::Time::nowFast() > expirationTime)
			{
				throttling = true;
				break;
			}
		}
	}

	if (yieldEvent.get() && runService)
		yieldEvent->resume(runService->wallTime(), expirationTime, throttling);

	throttlingThreads.sample(throttling);

	RBXASSERT(!kCLuaResumeStackSize.get() || (*kCLuaResumeStackSize) == 0);
	FASTLOG(FLog::ScriptContext, "ScriptContext::resumeWaitingScripts finish");
}

void ScriptContext::stepGc()
{
	// sample time since last gc
	avgLuaGcInterval.sample((Time::now<Time::Fast>() - luaGcStartTime).msec());
	luaGcStartTime = Time::now<Time::Fast>();

	for (GlobalStates::iterator iter = globalStates.begin(); iter != globalStates.end(); ++iter)
		if (iter->state)
		{
			RBXPROFILER_SCOPE("Lua", "GC");

			int gcCountPre = lua_gc(iter->state, LUA_GCCOUNT, 0);
			int gcAlloc = std::max(0, gcCountPre - iter->gcCount);

			iter->gcAllocAvg.sample(gcAlloc);

			int gcRequest = std::max(1, std::min(DFInt::LuaGcMaxKb, int(iter->gcAllocAvg.value() * DFInt::LuaGcBoost)));

			lua_gc(iter->state, LUA_GCSTEP, gcRequest);

			int gcCountPost = lua_gc(iter->state, LUA_GCCOUNT, 0);

			iter->gcCount = gcCountPost;

			RBXPROFILER_LABELF("Lua", "Allocated %d Kb (total %d Kb)", gcAlloc, gcCountPre);
			RBXPROFILER_LABELF("Lua", "Freed %d Kb (requested %d Kb)", gcCountPre - gcCountPost, gcRequest);
		}

	// sample gc time
	avgLuaGcTime.sample((Time::now<Time::Fast>() - luaGcStartTime).msec());
}

void ScriptContext::startPendingScripts()
{
	{
		//Start scripts that are in a loading state, even if we're disabled (they're CoreScripts)
		std::vector<ScriptStart> copy;
		std::swap(loadingScripts, copy);
		std::for_each(
			copy.begin(), 
			copy.end(), 
			boost::bind(&ScriptContext::startScript, this, _1)
			);
	}
	
	if (scriptsDisabled)
		return;

	std::vector<ScriptStart> copy;
	std::swap(pendingScripts, copy);

	RBX::Timer<RBX::Time::Fast> timer;
	timer.reset();

	for (unsigned int i = 0; i < copy.size(); i++)
	{
		FASTLOG1(FLog::ScriptContext, "Starting pending script: %p", copy[i].script.get());
		FASTLOGS(FLog::ScriptContext, "Pending script name: %p", copy[i].script->getName());
		startScript(copy[i]);

		if (timer.delta().seconds() > 30)
			FASTLOG1(FLog::Error, "startPendingScripts time out, num left %d", copy.size() - i);
	}
}

void ScriptContext::setCollectScriptStats(bool enabled)
{
	collectScriptStats = enabled;
	if (collectScriptStats) 
	{
		if (!scriptStats && ServiceProvider::find<Stats::StatsService>(this))
			scriptStats.reset(new ScriptStats());
	}
	else
		scriptStats.reset();
}

void ScriptContext::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	heartbeatConnection.disconnect();

	if (oldProvider)
	{
		endTimoutThread = true;
		checkTimeout.Set();
		if (timeoutThread)
			timeoutThread->join();

		closeStates(false);
	}

	if (statsItem) {
		statsItem->setParent(NULL);
		statsItem.reset();
	}

	if(scriptStats){
		scriptStats.reset();
	}

	Super::onServiceProvider(oldProvider, newProvider);

	Stats::StatsService* stats = ServiceProvider::create<Stats::StatsService>(newProvider);
	if (stats) {
		statsItem = LuaStatsItem::create(this);
		statsItem->setParent(stats);

		if(collectScriptStats){
			scriptStats = shared_ptr<ScriptStats>(new ScriptStats());
		}
	}



	if (newProvider)
	{
		// Here is where we create the lua_state(s)
		for (size_t i = 0; i < globalStates.size(); ++i)
		{
			openState(i);
		}

        if (LuaVM::canCompileScripts())
        {
            for (GlobalStates::iterator itr = globalStates.begin(); itr != globalStates.end(); ++itr)
            {
                itr->state->l_G->ckey = LUAVM_KEY_DUMMY;
            }
        }
        else
        {
            globalStates[Security::VM_RobloxScriptPlus].state->l_G->ckey = LuaVM::getKeyCore();
        }

        VMProtectBeginVirtualization("");
        securityAnchor.update(&(this->securityAnchor));
        VMProtectEnd();

		if (FLog::LuaScriptTimeoutSeconds > 0) {
			setTimeout(FLog::LuaScriptTimeoutSeconds);
		}
	}

	runService = shared_from(ServiceProvider::create<RunService>(newProvider));

	if (runService)
		heartbeatConnection = runService->heartbeatSignal.connect(boost::bind(&ScriptContext::onHeartbeat, this, _1));
}

void ScriptContext::onChangedScriptEnabled(const Reflection::PropertyDescriptor&)
{
	if (scriptsDisabled)
		startPendingScripts();
}

bool ScriptContext::scriptShouldRun(BaseScript* script)
{
	return Instance::fastDynamicCast<CoreScript>(script) != NULL;
}

void ScriptContext::setAdminScriptPath(const std::string& newPath)
{
    RBX::BaseScript::adminScriptsPath = newPath;
}

void ScriptContext::addStarterScript(int assetId)
{
    // Hack required to have PBS work; we should change web code in gameserver.ashx...
    if (assetId == 124885177)
        return addCoreScriptLocal("CoreScripts/BuildToolsScripts/PersonalServerScript", shared_from(this));

    throw RBX::runtime_error("AddStarterScript disabled (asset %d)", assetId);
}

void ScriptContext::addCoreScriptLocal(std::string name, shared_ptr<Instance> parent)
{
	if (!parent)
		parent = shared_from(this);

	shared_ptr<CoreScript> script = Creatable<Instance>::create<CoreScript>();
	script->setName(name);
	script->setParent(parent.get());

	// this is to make sure that CoreScripts still get added into ScriptContext and run,
	// even if its parent is set to something else
	if(script->getParent() != this)
		addScript(script);
}

void ScriptContext::addCoreScript(int assetId, shared_ptr<Instance> parent, std::string name)
{
    // Hack required to have build mode work; we should change web code in visit.ashx...
    if (assetId == 59431535)
        return addCoreScriptLocal("CoreScripts/BuildToolsScripts/BuildToolsScript", shared_from(this));

    throw RBX::runtime_error("AddCoreScript disabled (asset %d)", assetId);
}

void ScriptContext::addScript(weak_ptr<BaseScript> script, ScriptStartOptions startOptions)
{
	if (shared_ptr<BaseScript> locked = script.lock())
	{
		bool ok = scripts.insert(locked.get()).second;
		RBXASSERT(ok);

		FASTLOGS(FLog::ScriptContextAdd, "ScriptContext::addScript -- %s", locked->getName().c_str());

		ScriptStart scriptStart = { locked, startOptions };
		startScript(scriptStart);
	}
}

void ScriptContext::disassociateState(BaseScript* script)
{
	if (script->threadNode)
	{
		FASTLOGS(FLog::ScriptContext, "ScriptContext::disassociateState -- %s", script->getName().c_str());

		const std::string& hash = script->requestHash();
		if (!hash.empty())
		{
			ScriptStatInformation& info = scriptHashInfo[hash];
			for (Instances::iterator iter = info.scripts.begin(); iter != info.scripts.end(); ++ iter)
				if (iter->get() == script)
				{
					info.scripts.erase(iter);
					if (info.scripts.empty())
						scriptHashInfo.erase(hash);
					break;
				}
		}

		script->threadNode->eraseAllRefs();
		script->threadNode.reset();

		script->stopped();
	}
}

void ScriptContext::eraseScript(std::vector<ScriptContext::ScriptStart>& container, BaseScript* script)
{
	for (std::vector<ScriptContext::ScriptStart>::iterator iter = container.begin(); iter != container.end(); ++iter)
		if (iter->script.get() == script)
		{
			container.erase(iter);
			return;
		}
}

void ScriptContext::removeScript(weak_ptr<BaseScript> script)
{
	if (shared_ptr<BaseScript> locked = script.lock())
	{
		int num = scripts.erase(locked.get());
		RBXASSERT(num == 1);

		FASTLOGS(FLog::ScriptContextRemove, "RemoveScript %s", locked->getName().c_str());

		disassociateState(locked.get());

		eraseScript(pendingScripts, locked.get());
		eraseScript(loadingScripts, locked.get());
	}
}

std::string ScriptContext::extractCallStack(lua_State* thread, shared_ptr<BaseScript>& source, int& line)
{
	line = -1;
	source = RobloxExtraSpace::get(thread)->script.lock();

	std::stringstream str;
	lua_Debug ar;
	if (lua_getstack(thread, 0, &ar)) 
	{
		int level = 0;
		const int maxLevel = 12;
		while (level<maxLevel)
		{
			if (lua_getinfo(thread, "nSlu", &ar))
			{
				if (level == 0)
					line = ar.currentline;
				if (ar.currentline>=0)
				{
					char source[LUA_IDSIZE];
					luaO_chunkid(source, ar.source, LUA_IDSIZE);
					if (ar.name)
						str << source << ", line " << ar.currentline << " - " << ar.namewhat << " " << ar.name << std::endl;
					else
						str << source << ", line " << ar.currentline << std::endl;
				}
			}

			if (!lua_getstack(thread, ++level, &ar))
				break;
		}
	}
	return str.str();
}

void ScriptContext::printCallStack(lua_State* thread, std::string * output, bool dontPrint)
{
	if (!RBX::DebugSettings::singleton().getStackTracingEnabled())
		return;

	std::stringstream stream;

	lua_Debug ar;
	if (lua_getstack(thread, 0, &ar)) 
	{
		bool moreStack = false;
		int level = 0;
		const int maxLevel = 12;
		while (level<maxLevel && lua_getstack(thread, level++, &ar)){
			if (lua_getinfo(thread, "nSlu", &ar))
			{
				if (ar.currentline>=0)
				{
					moreStack = true;
					char source[LUA_IDSIZE];
					luaO_chunkid(source, ar.source, LUA_IDSIZE);
					std::string messageLine;
					if (level==1)
					{
						if (!dontPrint)
							RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "Stack Begin");

						if(output)
							stream << "Stack Begin";
					}
					if (ar.name)
						messageLine = RBX::format("Script '%s', Line %d - %s %s", source, ar.currentline, ar.namewhat, ar.name);
					else
						messageLine = RBX::format("Script '%s', Line %d", source, ar.currentline);

					if (!dontPrint)
						RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "%s", messageLine.c_str());

					if(output)
						stream << messageLine << std::endl;
				}
			}
		}
		if (moreStack && level<maxLevel)
		{
			if (!dontPrint)
				RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "Stack End");

			if(output)
				stream << "Stack End";
		}
	}

	if(output)
		*output = stream.str();
}

void ScriptContext::reportError(lua_State* thread)
{
	std::string errorMessage;

	const char* error = lua_tostring(thread, -1);
	if (error && strlen(error)>0) {
		StandardOut::singleton()->printf(MESSAGE_ERROR, "%s", error);		
		errorMessage = error;
	}
	else {
		StandardOut::singleton()->print(MESSAGE_ERROR, "An error occurred");		
		errorMessage = "Error occurred, no output from Lua.";
	}

	std::string exceptionStack;

	DataModel* dm = DataModel::get(this);

	bool reportingStack = DFInt::LuaExceptionPlaceFilter && (DFInt::LuaExceptionPlaceFilter == dm->getPlaceIDOrZeroInStudio());
	printCallStack(thread, reportingStack ? &exceptionStack : NULL);

	Stats::StatsService* stats = ServiceProvider::find<Stats::StatsService>(dm);
	if(stats && reportingStack)
	{
		shared_ptr<Reflection::ValueTable> entry(new Reflection::ValueTable());
		(*entry)["PlaceId"] = dm->getPlaceID();
		(*entry)["CallStack"] = exceptionStack;
		(*entry)["Error"] = errorMessage;
		(*entry)["PlaceVersion"] = dm->getPlaceVersion();
		stats->report("LuaExceptions", entry, DFInt::LuaExceptionThrottlingPercentage);
	}
	
	shared_ptr<BaseScript> scriptPtr = RobloxExtraSpace::get(thread)->script.lock();
	
	if (!errorSignal.empty())
	{
		shared_ptr<BaseScript> source;
		int line;
		// fire ScriptContext.Error signal for any listening lua code
		std::string callStackString = extractCallStack(thread, source, line);
		RBXASSERT(scriptPtr == source);
		errorSignal(errorMessage, callStackString, scriptPtr);
	}

	if (!scriptErrorDetected.empty())
		scriptErrorDetected(thread);

	if (scriptPtr)
		scriptPtr->extraErrorReporting(thread);
}

ScriptContext::ScriptImpersonator::ScriptImpersonator(lua_State *thread)
:RBX::Security::Impersonator(getThreadIdentity(thread))
{
}


void ScriptContext::scheduleResume(Lua::ThreadRef thread, shared_ptr<const Reflection::Tuple> arguments)
{
	WaitingThread wt;
	wt.thread = thread;
	wt.arguments = arguments;
	waitingThreads.push(wt);
}

void ScriptContext::resumeWithArgs(Lua::ThreadRef thread, shared_ptr<const Reflection::Tuple> arguments)
{
	int count = LuaArguments::pushTuple(*arguments, thread);

	// resume the waiting thread
	if (resume(thread, count) != ScriptContext::Yield)
	{
		// success or error means thread is finished, clear the stack
		lua_resetstack(thread, 0);
	}
}

ScriptContext::Result ScriptContext::resume(ThreadRef thread, int narg)
{
	validateThreadAccess(thread);

	int result;
	{
		boost::shared_ptr<BaseScript> script = RobloxExtraSpace::get(thread)->script.lock();

		ScriptImpersonator impersonate(thread);

		if (timedout && (Security::Context::current().identity==Security::GameScript_ || Security::Context::current().identity==Security::RobloxGameScript_))
		{
			if (script)
				StandardOut::singleton()->printf(MESSAGE_ERROR, "%s: timeout before resuming thread", script->getFullName().c_str());
			else
				StandardOut::singleton()->print(MESSAGE_ERROR, "Timeout before resuming thread");
			return Error;
		}

		RBXPROFILER_SCOPE("Lua", "$Script");
		RBXPROFILER_LABEL("Lua", script ? script->getName().c_str() : "Unknown");

		if(scriptStats && script){
			scriptStats->scriptResumeStarted(script->requestHash());
		}
		
		if (!lua_gethook(thread) && script)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "No lua hook set for %s", script->getName().c_str());
			FASTLOGS(FLog::Error, "No lua hook set for %s", script->getName().c_str());
		}

		FASTLOG1(FLog::ScriptContext, "Resuming script: %p", script.get());
		if(script.get())
			FASTLOGS(FLog::ScriptContext, "Pending script name: %s", script->getName());

        if (FLog::LuaProfiler && !LuaProfiler::instance)
        {
            LuaProfiler prof(thread, narg);
			result = resumeImpl(thread, narg);
        }
        else
        {
			result = resumeImpl(thread, narg);
        }
			
		Continuations* continuations = RobloxExtraSpace::get(thread)->continuations.get();
		if (continuations)
        {
			if (result == Error)
			{
				if (continuations->error)
					(continuations->error)(thread);
			}
			else if (result == Success)
			{
				if (continuations->success)
				{
					try 
					{
						(continuations->success)(thread);
					}
					catch (const RBX::base_exception& e)
					{
						lua_pushstring(thread, e.what());
						if (continuations->error)
							(continuations->error)(thread);
					}
				}
			}
        }
		
		if(scriptStats && script){
			scriptStats->scriptResumeStopped(script->requestHash());
		}
	}


	switch (result)
	{
	case 0:
		return Success;

	case LUA_YIELD:
		// Add to yield queue
		if (!RobloxExtraSpace::get(thread)->yieldCaptured && yieldEvent.get())
			yieldEvent->queueWaiter(thread);
		return Yield;
	default:
		// Error condition
		reportError(thread);
		return Error;
	}
}

// TODO: This function is too big. Break it up
void ScriptContext::startScript(ScriptStart scriptStart)
{
	shared_ptr<BaseScript> script = scriptStart.script;
	RBXASSERT(script->getRootAncestor() == getRootAncestor());
	RBXASSERT(scripts.find(script.get()) != scripts.end());

	if (scriptsDisabled && Instance::fastDynamicCast<CoreScript>(script.get()) == NULL)
	{
		pendingScripts.push_back(scriptStart);
		return;
	}

	// Scripts must be in the world to be run
	if (ServiceProvider::findServiceProvider(script.get()) != ServiceProvider::findServiceProvider(this))
		return;

	BaseScript::Code code = script->requestCode();
	if (!code.loaded)
	{
		return;
	}

	if (startScriptReentrancy>3)
		throw std::runtime_error("startScript re-entrancy has exceeded 3");

	startScriptReentrancy++;

	FASTLOG1(FLog::ScriptContext, "Running script %p", script.get());
	if (RBX::LuaSettings::singleton().areScriptStartsReported)
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Running Script \"%s\"", script->getFullName().c_str());

	try
	{
		RBX::Security::Identities identity;

		if (script->isA<CoreScript>())
			identity = RBX::Security::RobloxGameScript_;
		else if (scriptStart.options.identity != RBX::Security::GameScript_)
			identity = scriptStart.options.identity;
		else if (robloxPlace)
			identity = RBX::Security::GameScriptInRobloxPlace_;
		else
			identity = RBX::Security::GameScript_;
		
		lua_State* globalState = getGlobalState(identity);
		RBXASSERT_BALLANCED_LUA_STACK(globalState);

		lua_State* thread = lua_newthread(globalState);
		if (thread==NULL)
			throw RBX::runtime_error("Unable to create a new thread for %s", script->getName().c_str());

		bool profile = false;
		if (FLog::LuaProfiler)
		{
			profile = true;
		}

		// add debug hook to this thread
		FASTLOGS(FLog::ScriptContext, "lua_sethook %s", script->getName().c_str());
		lua_sethook(thread, ScriptContext::hook, (profile ? LUA_MASKCALL | LUA_MASKRET : 0) | LUA_MASKCOUNT, hookCount);

		// Pop the thread from the stack when we leave
		Lua::ScopedPopper popper(globalState, 1);

		// Give each script its own environment
		setThreadIdentityAndSandbox(thread, identity, script);

		// Associate a WeakThreadRef::Node with the BaseScript's thread.
		RBXASSERT(!script->threadNode);
		script->threadNode = WeakThreadRef::Node::create(thread);

		const std::string& hash = script->requestHash();
		if (!hash.empty())
		{
			scriptHashInfo[hash].name = script->getName();
			scriptHashInfo[hash].scripts.push_back(script);
		}

		{
			ThreadRef safeThread(thread);
			RBXASSERT_BALLANCED_LUA_STACK(thread);
			RBXASSERT(lua_gettop(thread)==0);

			// declare "script" global
			ObjectBridge::push(thread, script);
			lua_setglobal( thread, "script" ); 

			std::string name = std::string("=") + script->getFullName();

			// This code gets the source. It is a little tricky. It uses
			// pointers to avoid copies when possible.
			const ProtectedString* protectedSource = &code.script.get();
			ProtectedString macroExpandedSource;

			if (LuaVM::canCompileScripts() && scriptStart.options.filter)
			{
				try
				{
					macroExpandedSource = ProtectedString::fromTrustedSource(
						scriptStart.options.filter(protectedSource->getSource()));
					protectedSource = &macroExpandedSource;
				}
				catch (const ScriptStartOptions::LuaSyntaxError& e)
				{
					// TODO: Don't we need to clean up the Lua stack? Alternately, do the filtering
					// at the beginning of the function, before we've touched Lua
					std::string message = RBX::format("%s(%d) - %s", script->getFullName().c_str(), e.lineNumber, e.what());
					throw std::runtime_error(message);
				}
			}

			if (!scriptStart.options.continuations.empty())
				RobloxExtraSpace::get(thread)->continuations.reset(new Lua::Continuations(scriptStart.options.continuations));

			bool luaFailedLoad = LuaVM::load(thread, *protectedSource, name.c_str()) != 0;

			if (luaFailedLoad)
			{
				reportError(thread);
				Continuations* continuations = RobloxExtraSpace::get(thread)->continuations.get();
				if (continuations && continuations->error)
					(continuations->error)(thread);
				lua_pop(thread, 1);		// pop the message
			}
			else
			{
				script->starting(thread);

				resume(safeThread, 0);
			}

			// balance the thread's stack
			lua_resetstack(thread, 0);
		}
	}
	catch (RBX::base_exception&)
	{
		startScriptReentrancy--;
		throw;
	}
	startScriptReentrancy--;
}



#ifdef _DEBUG
StackBalanceCheck::StackBalanceCheck(lua_State *thread)
:thread(thread),oldTop(lua_gettop(thread)),cancelled(false)
{
}
StackBalanceCheck::~StackBalanceCheck()
{
	if (!cancelled)
	{
		int newTop = lua_gettop(thread);
		RBXASSERT(newTop==oldTop);
	}
}
#endif

void ScriptContext::initializeLuaStateSandbox(Lua::WeakThreadRef& threadRef, lua_State* parentState, Security::Identities identity)
{
    threadRef = lua_newthread(parentState);
    ThreadRef safeThread = threadRef.lock();
	RBXASSERT(safeThread);

	setThreadIdentityAndSandbox(safeThread, identity, shared_ptr<BaseScript>());
}

Reflection::Variant ScriptContext::evaluateStudioCommandItem(const char* itemToEvaluate, shared_ptr<LuaSourceContainer> script)
{
    if (commandLineSandbox.empty())
        initializeLuaStateSandbox(commandLineSandbox, getGlobalState(RBX::Security::CmdLine_), RBX::Security::CmdLine_);

    ThreadRef safeCommandlineSandbox = commandLineSandbox.lock();

    if (!safeCommandlineSandbox)
        return Reflection::Variant();

    lua_State* L = safeCommandlineSandbox;
	if (script)
	{
		ObjectBridge::push(L, script);
		lua_setglobal(L, "script");
	}

	RBX::Reflection::Variant result = Reflection::Variant();

	try
	{
		result = RBX::Scripting::DebuggerManager::readWatchValue(itemToEvaluate, 0, L);
	}
	catch (std::runtime_error const& e)
	{
		RBX::Log::current()->writeEntry(RBX::Log::Information, RBX::format("IntelleSenseException: %s", e.what()).c_str());
	}

	if (script)
	{
		lua_pushnil(L);
		lua_setglobal(L, "script");
}
    return result;
}

bool ScriptContext::checkSyntax(const std::string& code, int& line, std::string& errorMessage)
{
	ScopedState L;
	int result;
	result = LuaVM::load(L, ProtectedString::fromTrustedSource(code), "");
	if (result == LUA_ERRSYNTAX)
	{
		// [string ""]:1: '=' expected near 'blah'
		errorMessage = safe_lua_tostring(L, -1);
		size_t start = errorMessage.find(':');
		size_t stop = errorMessage.find(':', start + 1);
		line = atoi(errorMessage.substr(start + 1, stop).c_str());
		errorMessage = errorMessage.substr(stop + 2);
	}
	return result != LUA_ERRSYNTAX;
}

void RuntimeScriptService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		runTransitionConnection.disconnect();
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		RunService* runService = ServiceProvider::create<RunService>(newProvider);
		onRunState(runService->getRunState());
		runTransitionConnection = runService->runTransitionSignal.connect(boost::bind(&RuntimeScriptService::onRunTransition, this, _1));
	}
}

void RuntimeScriptService::onRunState(RunState state) 
{
	const bool running = state != RS_STOPPED;
	if (running == isRunning)
		return;

	isRunning = running;

	ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(this);

	if (isRunning)
	{
		runningScripts.insert(pendingScripts.begin(), pendingScripts.end());

		std::set<weak_ptr<BaseScript> > temp;
		std::swap(pendingScripts, temp);
		std::for_each(temp.begin(), temp.end(), boost::bind(&ScriptContext::addScript, scriptContext, _1, ScriptContext::ScriptStartOptions()));
	}
	else
	{
		pendingScripts.insert(runningScripts.begin(), runningScripts.end());

		std::set<weak_ptr<BaseScript> > temp;
		std::swap(runningScripts, temp);
		std::for_each(temp.begin(), temp.end(), boost::bind(&ScriptContext::removeScript, scriptContext, _1));

		scriptContext->cleanupModules();
	}
}

void RuntimeScriptService::runScript(BaseScript* script) 
{
	RBXASSERT(runningScripts.find(weak_from(script)) == runningScripts.end());

	ScriptContext* scriptContext = ServiceProvider::create<ScriptContext>(this);

	RBXASSERT(!scriptContext->hasScript(script));

	if (script->fastDynamicCast<CoreScript>())
		scriptContext->addScript(weak_from(script));	// start and forget. CoreScripts are not runtime scripts
	else if (Network::Players::backendProcessing(this) && !isRunning)
		// Backend processing - hold off until simulation
		pendingScripts.insert(weak_from(script));
	else
	{
		// otherwise run immediately
		runningScripts.insert(weak_from(script));
		scriptContext->addScript(weak_from(script));	
	}
}

void RuntimeScriptService::releaseScript(BaseScript* script)
{
	ScriptContext* scriptContext = ServiceProvider::find<ScriptContext>(this);
	if (!scriptContext || scriptContext->haveStatesClosed())
		return;		// we must be shutting down

	weak_ptr<BaseScript> weakPtr = weak_from(script);
	// Mirrors the short-circuit in RuntimeScriptService::runScript.
	// CoreScripts shut themselves down in CoreScript::onServiceProvider
	// TODO: This whole CoreScript running code is a mess. A better option
	// would be to move RuntimeScriptService related code to out of BaseScript
	// and into Script. Then have CoreScripts be simpler and cleaner. They should
	// be started in onServiceProvider, just like they are stopped.
	if (script->fastDynamicCast<CoreScript>())
	{
		RBXASSERT(pendingScripts.find(weakPtr) == pendingScripts.end());
		RBXASSERT(runningScripts.find(weakPtr) == runningScripts.end());
		RBXASSERT(!scriptContext->hasScript(script));
		return;
	}

	int num = pendingScripts.erase(weakPtr);
	RBXASSERT(num <= 1);

	if (num == 0) 
	{
		RBXASSERT(runningScripts.find(weakPtr) != runningScripts.end());
		runningScripts.erase(weakPtr);

		RBXASSERT(scriptContext->hasScript(script));
		scriptContext->removeScript(weakPtr);	
	}
	else 
	{
		RBXASSERT(!scriptContext->hasScript(script));
	}
}

void ScriptContext::cleanupModules()
{
	std::for_each(loadedModules.begin(), loadedModules.end(),
		boost::bind(&ModuleScript::cleanupAndResetState, _1));
	loadedModules.clear();
}

void ScriptContext::validateThreadAccess(lua_State* L)
{
	ScriptContext* context = RobloxExtraSpace::get(L)->context();
	
    if (DFFlag::LockViolationScriptCrash)
    {
        if (!DataModel::currentThreadHasWriteLock(context))
            RBXCRASH();
    }
    else
    {
        RBXASSERT(DataModel::currentThreadHasWriteLock(context));
    }
}

int ScriptContext::resumeImpl(lua_State* L, int nargs)
{
	if (DFFlag::ScriptContextGuardAgainstCStackOverflow)
	{
		if (!kCLuaResumeStackSize.get())
			kCLuaResumeStackSize.reset(new int(0));
		if ((*kCLuaResumeStackSize) > 200)
			throw runtime_error("C stack overflow");

		{
			ScopedAssign<int> stackSizeCounter(*kCLuaResumeStackSize, *kCLuaResumeStackSize + 1);
			return lua_resume(L, nargs);
		}
	}
	else
	{
		return lua_resume(L, nargs);
	}
}

