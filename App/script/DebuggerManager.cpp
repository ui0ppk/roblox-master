#include "stdafx.h"
#include "script/DebuggerManager.h"

#include "script/ScriptContext.h"
#include "script/Script.h"
#include "script/ModuleScript.h"
#include "script/LuaArguments.h"
#include "util/ProtectedString.h"

#include "lua/lua.hpp"
#include "lstate.h"
#include "ltable.h"

#include "v8datamodel/DataModel.h"
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"

FASTFLAGVARIABLE(LuaDebugger, false)
FASTFLAGVARIABLE(LuaDebuggerBreakOnError, false)

namespace RBX 
{
	namespace Scripting 
	{
		const char* const sDebuggerManager = "DebuggerManager";
		const char* const sScriptDebugger = "ScriptDebugger";
		const char* const sDebuggerBreakpoint = "DebuggerBreakpoint";
		const char* const sDebuggerWatch = "DebuggerWatch";

		static void checkDebugging()
		{
			if (!FFlag::LuaDebugger)
				throw std::runtime_error("debugging is disabled");
		}

		class PauseNowBreakpoint : public ISpecialBreakpoint
		{
		public:
			PauseNowBreakpoint()
			{
				baseThread = NULL;
			}

			virtual bool hitTest(lua_State* L, lua_Debug *ar)
			{
				return ar->event == LUA_HOOKLINE;
			}
		};

		class StepInBreakpoint : public ISpecialBreakpoint
		{
			bool hitPausedLine;
		public:
			StepInBreakpoint():hitPausedLine(false)
			{
				baseThread = NULL;
			}
			virtual bool hitTest(lua_State* L, lua_Debug *ar)
			{
				if (baseThread && (baseThread != L))
					return false;

				if (!hitPausedLine)
				{
					hitPausedLine = ar->event == LUA_HOOKLINE;
					return false;
				}

				// The next line we hit is either a step-in or a step-over. Either is fine
				if (ar->event == LUA_HOOKLINE)
					return true;
				return false;
			}
		};

		class StepOutBreakpoint : public ISpecialBreakpoint
		{
			int depth;
			bool readyForLine;
		public:
			StepOutBreakpoint():depth(0),readyForLine(false) 
			{
				baseThread = NULL;
			}

			virtual bool hitTest(lua_State* L, lua_Debug *ar)
			{
				if (baseThread && (baseThread != L))
					return false;

				if (readyForLine)
					return ar->event == LUA_HOOKLINE;

				RBXASSERT(depth >= 0);

				if (strcmp(ar->what, "C") != 0)
				{
					if (ar->event == LUA_HOOKRET)	// what about LUA_HOOKTAILRET?
						depth--;
					else if (ar->event == LUA_HOOKCALL)
						depth++;
				}

				RBXASSERT(depth >= -1);
				readyForLine = depth == -1;
				return false;
			}
		};


		class StepOverBreakpoint : public ISpecialBreakpoint
		{
			int depth;
			bool hitPausedLine;
		public:
			StepOverBreakpoint():depth(0),hitPausedLine(false) 
			{
				baseThread = NULL;
			}

			virtual bool hitTest(lua_State* L, lua_Debug *ar)
			{
				if (baseThread && (baseThread != L))
					return false;

				if (!hitPausedLine)
				{
					hitPausedLine = ar->event == LUA_HOOKLINE;
					return false;
				}

				if (depth == 0 && ar->event == LUA_HOOKLINE)
					return true;

				if (depth < 0)
					// We stepped out, so all we are waiting for is the next line
						return ar->event == LUA_HOOKLINE;

				RBXASSERT(depth >= 0);

				//if (strcmp(ar->what, "C") != 0)
				{
					if (ar->event == LUA_HOOKRET)	// what about LUA_HOOKTAILRET?
						depth--;
					else if (ar->event == LUA_HOOKCALL)
						depth++;
				}

				RBXASSERT(depth >= -1);
				return false;
			}
		};
	}
}

using namespace RBX;
using namespace Scripting;

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<DebuggerManager, shared_ptr<Instance>(shared_ptr<Instance>)> func_AddDebugger(&DebuggerManager::addDebugger_Reflection, "AddDebugger", "script", Security::None);
static Reflection::BoundFuncDesc<DebuggerManager, void()> func_Enable(&DebuggerManager::enableDebugging, "EnableDebugging", Security::LocalUser);
static Reflection::PropDescriptor<DebuggerManager, bool> prop_Enabled("DebuggingEnabled", category_Data, &DebuggerManager::getEnabled, NULL);
static Reflection::BoundFuncDesc<DebuggerManager, void()> func_Resume(&DebuggerManager::resume, "Resume", Security::None);
static Reflection::BoundFuncDesc<DebuggerManager, void()> func_StepOver(&DebuggerManager::stepOver, "StepOver", Security::None);
static Reflection::BoundFuncDesc<DebuggerManager, void()> func_StepInto(&DebuggerManager::stepInto, "StepIn", Security::None);
static Reflection::BoundFuncDesc<DebuggerManager, void()> func_StepOut(&DebuggerManager::stepOut, "StepOut", Security::None);
static Reflection::BoundFuncDesc<DebuggerManager, shared_ptr<const Instances>()> func_GetDebuggers(&DebuggerManager::getDebuggers_Reflection, "GetDebuggers", Security::None);
static Reflection::EventDesc<DebuggerManager, void(shared_ptr<Instance>)> event_DebuggerAdded(&DebuggerManager::debuggerAdded, "DebuggerAdded", "debugger");
static Reflection::EventDesc<DebuggerManager, void(shared_ptr<Instance>)> event_DebuggerRemoved(&DebuggerManager::debuggerRemoved, "DebuggerRemoved", "debugger");

static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<Instance>(int)> func_SetBreakpoint(&ScriptDebugger::setBreakpoint_Reflection, "SetBreakpoint", "line", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<const Instances>()> func_GetBreakpoints(&ScriptDebugger::getBreakpoints_Reflection, "GetBreakpoints", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<Instance>(std::string)> func_AddWatch(&ScriptDebugger::addWatch_Reflection, "AddWatch", "expression", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<const Instances>()> func_GetWatches(&ScriptDebugger::getWatches_Reflection, "GetWatches", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, Reflection::Variant(shared_ptr<Instance>)> func_GetValue(&ScriptDebugger::getWatchValue_Reflection, "GetWatchValue", "watch", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, void()> func_Resume_dep(&ScriptDebugger::resume, "Resume", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<ScriptDebugger, void()> func_StepOver_dep(&ScriptDebugger::stepOver, "StepOver", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<ScriptDebugger, void()> func_StepInto_dep(&ScriptDebugger::stepInto, "StepIn", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<ScriptDebugger, void()> func_StepOut_dep(&ScriptDebugger::stepOut, "StepOut", Security::None, Reflection::Descriptor::Attributes::deprecated());
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<const Reflection::ValueArray>()> func_GetStack(&ScriptDebugger::getStack_Reflection, "GetStack", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<const Reflection::ValueMap>(int)> func_GetLocals(&ScriptDebugger::getLocals, "GetLocals", "stackFrame", 0, Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<const Reflection::ValueMap>(int)> func_GetUpvalues(&ScriptDebugger::getUpvalues, "GetUpvalues", "stackFrame", 0, Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, shared_ptr<const Reflection::ValueMap>()> func_GetGlobals(&ScriptDebugger::getGlobals, "GetGlobals", Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, void(std::string, Reflection::Variant, int)> func_SetLocal(&ScriptDebugger::setLocal, "SetLocal", "name", "value", "stackFrame", 0, Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, void(std::string, Reflection::Variant, int)> func_SetUpvalue(&ScriptDebugger::setUpvalue, "SetUpvalue", "name", "value", "stackFrame", 0, Security::None);
static Reflection::BoundFuncDesc<ScriptDebugger, void(std::string, Reflection::Variant)> func_SetGlobal(&ScriptDebugger::setGlobal, "SetGlobal", "name", "value", Security::None);
// The Script property is read-only by design. If we want to make it writable, then we need to watch for changes and update DebuggerManager::debuggers
const Reflection::RefPropDescriptor<ScriptDebugger, Instance> prop_Script("Script", category_Data, &ScriptDebugger::getScript, NULL, Reflection::PropertyDescriptor::UI);
const Reflection::PropDescriptor<ScriptDebugger, std::string> prop_ScriptPath("ScriptPath", category_Data, &ScriptDebugger::getScriptPath, &ScriptDebugger::setScriptPath, Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<ScriptDebugger, bool> prop_IsDebugging("IsDebugging", category_Data, &ScriptDebugger::isDebugging, NULL, Reflection::PropertyDescriptor::UI);																	
static Reflection::PropDescriptor<ScriptDebugger, bool> prop_IsPaused("IsPaused", category_Data, &ScriptDebugger::isPaused, NULL, Reflection::PropertyDescriptor::UI);																	
static Reflection::PropDescriptor<ScriptDebugger, int> prop_CurrentLine("CurrentLine", category_Data, &ScriptDebugger::getCurrentLine, NULL, Reflection::PropertyDescriptor::UI);																	
static Reflection::EventDesc<ScriptDebugger, void(int)> event_EncounteredBreak(&ScriptDebugger::encounteredBreak, "EncounteredBreak", "line");
static Reflection::EventDesc<ScriptDebugger, void()> event_Resuming(&ScriptDebugger::resuming, "Resuming");
static Reflection::EventDesc<ScriptDebugger, void(shared_ptr<Instance>)> event_BreakpointAdded(&ScriptDebugger::breakpointAdded, "BreakpointAdded", "breakpoint");
static Reflection::EventDesc<ScriptDebugger, void(shared_ptr<Instance>)> event_BreakpointRemoved(&ScriptDebugger::breakpointRemoved, "BreakpointRemoved", "breakpoint");
static Reflection::EventDesc<ScriptDebugger, void(shared_ptr<Instance>)> event_WatchAdded(&ScriptDebugger::watchAdded, "WatchAdded", "watch");
static Reflection::EventDesc<ScriptDebugger, void(shared_ptr<Instance>)> event_WatchRemoved(&ScriptDebugger::watchRemoved, "WatchRemoved", "watch");

// The Line property is read-only by design. If we want to make it writable, then we need to watch for changes and update ScriptDebugger::breakpoints
static Reflection::PropDescriptor<DebuggerBreakpoint, int> prop_Line("Line", category_Data, &DebuggerBreakpoint::getLine, NULL, Reflection::PropertyDescriptor::UI);																	
Reflection::BoundProp<int> DebuggerBreakpoint::prop_Line_Data("line", category_Data, &DebuggerBreakpoint::line, Reflection::PropertyDescriptor::STREAMING);																	
Reflection::BoundProp<bool> DebuggerBreakpoint::prop_Enabled("IsEnabled", category_Data, &DebuggerBreakpoint::enabled);
Reflection::BoundProp<std::string> DebuggerBreakpoint::prop_Condition("Condition", category_Data, &DebuggerBreakpoint::condition);

Reflection::BoundProp<std::string> DebuggerWatch::prop_Expression("Expression", category_Data, &DebuggerWatch::expression);
static Reflection::BoundFuncDesc<DebuggerWatch, void()> func_CheckExpressionSyntax(&DebuggerWatch::checkExpressionSyntax, "CheckSyntax", Security::None);
REFLECTION_END();

static DebuggerManager* gDebuggerManager = NULL;
static void doBasicSingleton()
{
	static shared_ptr<DebuggerManager> sing = Creatable<Instance>::create<DebuggerManager>();
	gDebuggerManager = sing.get();
}

static void initDebuggerManagerSingleton()
{
	doBasicSingleton();
}

DebuggerManager& DebuggerManager::singleton()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initDebuggerManagerSingleton, flag);
	return *gDebuggerManager;
}

DebuggerManager::DebuggerManager(void)
:enabled(false)
,dataModel(NULL)
,executionMode(ExecutionMode_Continue)
,breakOnErrorMode(BreakOnErrorMode_Never)
,resuming(false)
,scriptAutoResume(true)
{
	setName(sDebuggerManager);
}

DebuggerManager::~DebuggerManager(void)
{
}

void DebuggerManager::setDataModel(RBX::DataModel *pDataModel)
{	
	if (pDataModel == dataModel)
		return;

	if (dataModel)
		removeAllChildren();

	errorSignalConnection.disconnect();
	descendantAddedSignalConnection.disconnect();
	breakOnErrorMode = BreakOnErrorMode_Never;
	debuggersLookup.clear();
	debuggers.clear();
	unaddedDebuggers.clear();
	reset();

	dataModel = pDataModel;

	if (dataModel)
		descendantAddedSignalConnection = dataModel->getOrCreateDescendantAddedSignal()->connect(boost::bind(&DebuggerManager::addUnaddedDebuggerForAddedDescendant, this, _1));
}

RBX::DataModel* DebuggerManager::getDataModel()
{	return dataModel; }

void DebuggerManager::reset()
{
	specialBreakpoint.reset();
	pausedThreads.clear();
	executionMode = ExecutionMode_Continue;
	resuming = false;
	scriptAutoResume = true;
}

void DebuggerManager::enableDebugging()
{
	enabled = true;
	raiseChanged(prop_Enabled);
}

void DebuggerManager::setBreakOnErrorMode(BreakOnErrorMode mode)
{
	if (!FFlag::LuaDebuggerBreakOnError || (mode == breakOnErrorMode) || !dataModel)
		return;

	errorSignalConnection.disconnect();

	breakOnErrorMode = mode;

	// connect with ScriptContext signal, using this we will catch errors which are reported in Output window
	if (breakOnErrorMode != BreakOnErrorMode_Never)
	{
		ScriptContext* pScriptContext = ServiceProvider::find<ScriptContext>(dataModel);
		if (pScriptContext)
			errorSignalConnection = pScriptContext->scriptErrorDetected.connect(boost::bind(&DebuggerManager::onErrorSignal, this, _1));
	}

	// update hooks: this will make sure correct mask is set for lua threads
	for (Debuggers::iterator iter = debuggers.begin(); iter != debuggers.end(); ++iter)
		iter->second->updateHook();
}

shared_ptr<const Instances> DebuggerManager::getDebuggers_Reflection()
{
	shared_ptr<Instances> result = rbx::make_shared<Instances>(debuggers.size());
	int i = 0;
	for (Debuggers::const_iterator iter = debuggers.begin(); iter != debuggers.end(); ++iter, ++i)
		(*result)[i] = shared_from(iter->second);
	return result;
}

ScriptDebugger* DebuggerManager::findDebugger( lua_State* L )
{
	DebuggersLookup::const_iterator iter = debuggersLookup.find(L);
	if (iter != debuggersLookup.end())
		return iter->second;

	ScriptDebugger* scriptDebugger = findDebugger(RobloxExtraSpace::get(L)->script.lock().get());
	if (scriptDebugger)
		debuggersLookup[L] = scriptDebugger;

	return scriptDebugger;
}

bool DebuggerManager::askForbidChild(const Instance* newChild) const
{
	return !Instance::fastDynamicCast<ScriptDebugger>(newChild);
}

void DebuggerManager::verifyAddChild(const Instance* newChild) const
{
	if (!askForbidChild(newChild))
		return;

	throw std::runtime_error("Only ScriptDebuggers can be children of DebuggerManager");
}


shared_ptr<Instance> DebuggerManager::addDebugger_Reflection( shared_ptr<Instance> script )
{
	if (Script* s = Instance::fastDynamicCast<Script>(script.get()))
		return addDebugger(s);
	throw std::runtime_error("Can only add debugger for a Script");
}


ScriptDebugger* DebuggerManager::findDebugger( Instance* script )
{
	if (!debuggers.empty() && script)
	{
		Debuggers::const_iterator iter = debuggers.find(script);
		if (iter != debuggers.end())
			return iter->second;
	}
	return NULL;
}

shared_ptr<ScriptDebugger> DebuggerManager::addDebugger( Instance* script )
{
	checkDebugging();

	if (ScriptDebugger* debugger = findDebugger(script))
		return shared_from(debugger);

	shared_ptr<ScriptDebugger> debugger = Creatable<Instance>::create<ScriptDebugger>(boost::ref(*script));
	debugger->setParent(this);
	return debugger;
}

void DebuggerManager::addDebugger(shared_ptr<ScriptDebugger> debugger)
{
	if (!debugger || !debugger->getScript())
		return;

	// check if debugger already added
	RBX::Instance* script = debugger->getScript();
	if (debuggers.find(script) != debuggers.end())
		return;

	// if script has been added into datamodel, directly add debugger
	if (dataModel->isAncestorOf(script))
		debugger->setParent(this);
	// else wait till it get's added
	else
		unaddedDebuggers[script] = debugger;
}

void DebuggerManager::addUnaddedDebuggerForAddedDescendant(shared_ptr<RBX::Instance> instance)
{
	if (instance && (instance->isA<RBX::Script>() || instance->isA<RBX::ModuleScript>()))
	{
		// check if we've an unadded debugger for the added instance
		UnaddedDebuggers::iterator iter = unaddedDebuggers.find(instance.get());
		if (iter != unaddedDebuggers.end())
		{
			RBXASSERT(debuggers.find(instance.get()) == debuggers.end());
			(iter->second)->setParent(this);
			unaddedDebuggers.erase(iter);
		}
	}
}

void DebuggerManager::populateForLookup(lua_State* L, ScriptDebugger* debugger)
{
	debuggersLookup[L] = debugger;
}

void DebuggerManager::onChildRemoved( Instance*  child )
{
	if (ScriptDebugger* debugger = Instance::fastDynamicCast<ScriptDebugger>(child))
	{
		debuggers.erase(debugger->getScript());

		for (std::list<Lua::WeakThreadRef>::iterator iter = pausedThreads.begin(); iter != pausedThreads.end();)
		{
			if (debugger->isPausedThread(reinterpret_cast<uintptr_t>( iter->lock().get() )))
			{
				pausedThreads.erase(iter++);
			}
			else
			{
				++iter;
			}
		}

		for (DebuggersLookup::iterator iter = debuggersLookup.begin(); iter != debuggersLookup.end();)
		{
			if (iter->second == debugger)
			{
				debuggersLookup.erase(iter++);
			}
			else 
			{
				++iter;
			}
		}

		debuggerRemoved(shared_from(debugger));
	}

	Super::onChildRemoved(child);
}

void DebuggerManager::addScriptDebugger(Instance* instance)
{
	if (ScriptDebugger* debugger = Instance::fastDynamicCast<ScriptDebugger>(instance))
	{
		if (debugger->getScript() != 0)
		{
			RBXASSERT(debuggers.find(debugger->getScript()) == debuggers.end());
			debuggers[debugger->getScript()] = debugger;
			debuggerAdded(shared_from(debugger));
		}
	}
}

void DebuggerManager::onChildAdded( Instance*  child )
{
	Super::onChildAdded(child);
	addScriptDebugger(child);
}

void DebuggerManager::onChildChanged(Instance* instance, const PropertyChanged& event)
{
	Super::onChildChanged(instance, event);

	if (event.getDescriptor() == prop_ScriptPath)
		addScriptDebugger(instance);
}

void DebuggerManager::onErrorSignal(lua_State* L)
{
	ScriptDebugger* pDebugger = findDebugger(L);
	if (pDebugger && !pDebugger->hasError())
	{
		pDebugger->handleError(L);
		errorThreads.push_back(L);
	}
}

void DebuggerManager::pause()
{
	executionMode = ExecutionMode_Break;
}

void DebuggerManager::resume()
{
	executionMode = ExecutionMode_Continue;

	// update error threads, resumeThread will just reset the debugger data
	while (!errorThreads.empty())
	{
		Lua::WeakThreadRef weakref = *(errorThreads.begin());
		if (RBX::Lua::ThreadRef thread = weakref.lock())
		{
			ScriptDebugger* debugger = findDebugger(thread);
			if (debugger && debugger->hasError())
				debugger->resumeThread(thread.get());
		}
		errorThreads.remove(weakref);
	}

	pausedThreads.insert(pausedThreads.begin(), resumingPausedThreads.begin(), resumingPausedThreads.end());
	resumingPausedThreads.clear();
	
	resuming = true;
	bool evalLineHookForCurrentLine = false;
	try 
	{
		while (!pausedThreads.empty())
		{
			Lua::WeakThreadRef weakref = *(pausedThreads.begin());
			if (RBX::Lua::ThreadRef thread = weakref.lock())
			{
				ScriptDebugger* debugger = findDebugger(thread);
				if (debugger)
				{
					ScriptContext::Result result = debugger->resumeThread(thread.get(), evalLineHookForCurrentLine);
					// if there's an error during resume, remove the thread from list so we do not resume it again
					if (result == ScriptContext::Error)
					{
						pausedThreads.remove(weakref);
						break;
					}
					// if thread has yielded
					else if (result == ScriptContext::Yield)
					{
						// do nothing if we have break mode enabled, get out of loop
						if (executionMode == ExecutionMode_Break)
						{
							break;
						}

						// if we have a special breakpoint without a baseThread, then set the current yielded thread
						// this will make sure special breakpoint will handle instructions specific to this thread only
						if (specialBreakpoint && !specialBreakpoint->baseThread)
						{														
							specialBreakpoint->baseThread = thread;
							if (!dynamic_cast<StepOutBreakpoint*>(specialBreakpoint.get()))
								break;
						}

						// TODO: we might want to break resume here
						// if root thread is yielded (using wait) and there's no special breakpoint, then how do we know it's resumed via yieldedthreads queue?
						// knowing this we can resume other paused threads also, this will make sure we do not have incorrect stepping in child threads.
					}
					// thread is successfully executed, check if we need to reset specialbreakpoint
					if ((specialBreakpoint && specialBreakpoint->baseThread == thread.get()) || debugger->isRootThreadResumed())
						specialBreakpoint.reset();
					// now we need to force evaluation of line hook for current line in each debugger (just in case if we miss breakpoint set on current line)
					evalLineHookForCurrentLine = true;
				}
			}
			pausedThreads.remove(weakref);
		}
	}
	catch (...)
	{
	}
	resuming = false;
}

void DebuggerManager::stepOver()
{
	if (pausedThreads.empty() && resumingPausedThreads.empty())
		throw std::runtime_error("Can't step while running");

	specialBreakpoint.reset(new StepOverBreakpoint());
	if (scriptAutoResume)
		resume();
}

void DebuggerManager::stepInto()
{
	if (pausedThreads.empty() && resumingPausedThreads.empty())
		throw std::runtime_error("Can't step while running");

	specialBreakpoint.reset(new StepInBreakpoint());
	if (scriptAutoResume)
		resume();
}

void DebuggerManager::stepOut()
{
	if (pausedThreads.empty() && resumingPausedThreads.empty())
		throw std::runtime_error("Can't step while running");

	specialBreakpoint.reset(new StepOutBreakpoint());
	if (scriptAutoResume)
		resume();
}

void DebuggerManager::hook( lua_State* L, lua_Debug *ar )
{
	if (ar->event == LUA_HOOKCOUNT)
	{
		ScriptContext::hook(L, ar);
	}
	else
	{
		gDebuggerManager->onHook(L, ar);
	}
}

void DebuggerManager::onHook(lua_State* L, lua_Debug *ar)
{
	ScriptDebugger* debugger = findDebugger(L);
	if (!debugger)
		return;

	if (debugger->handleHook(L, ar) || debugger->isPausedThread(reinterpret_cast<uintptr_t>(L)))
		return;

	bool paused = false;
	// first verify if we are running in breakmode
	if (executionMode == ExecutionMode_Break)
	{
		if (ar->event == LUA_HOOKLINE)
		{
			debugger->debuggerBreak(L, ar);
			paused = true;
		}
	}
	// second is to check for a special breakpoint
	else if (specialBreakpoint)
	{
		lua_getinfo(L, "Sf", ar);

		if (specialBreakpoint->hitTest(L, ar))
		{
			debugger->debuggerBreak(L, ar);
			executionMode = ExecutionMode_Break;
			paused = true;
		}
	}

	// last check for a user created breakpoint
	if (!paused && (ar->event == LUA_HOOKLINE) && debugger->onLineHook(L, ar))
	{
		executionMode = ExecutionMode_Break;
		paused = true;
	}

	if (paused)
	{
		// reset special breakpoint as we do not need it anymore
		specialBreakpoint.reset();
		// make sure we do not add an already paused thread in the list
		if (std::find(pausedThreads.begin(), pausedThreads.end(), L) != pausedThreads.end())
			return;
		// if a threads gets paused during resume then it must be pushed on the top of the list else push it in back	
		resuming ? resumingPausedThreads.push_back(L) : pausedThreads.push_back(L);
		//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "paused debugger = %d", L);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      ScriptDebugger
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ScriptDebugger::ScriptDebugger( Instance& script )
:currentLine(0) 
,breakpointHookData(NULL)
,globalRawScriptPtr(NULL)
,prevFuncTable(NULL)
,prevRawScriptPtr(NULL)
,ignoreDebuggerBreak(false)
,currentThreadID(0)
,rootThreadResumed(false)
{
	setName(RBX::format("%sDebugger", script.getName().c_str()));
	setScript(&script);
}

ScriptDebugger::~ScriptDebugger()
{
    scriptStartedConnection.disconnect();
	scriptStoppedConnection.disconnect();
	scriptParentChangedConnection.disconnect();
	scriptClonedConnection.disconnect();
}

shared_ptr<const Instances> ScriptDebugger::getBreakpoints_Reflection()
{
	shared_ptr<Instances> result = rbx::make_shared<Instances>(breakpoints.size());
	int i = 0;
	for (Breakpoints::const_iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter, ++i)
		(*result)[i] = shared_from(iter->second);
	return result;
}


shared_ptr<DebuggerWatch> ScriptDebugger::addWatch( std::string expression )
{
	shared_ptr<DebuggerWatch> watch = Creatable<Instance>::create<DebuggerWatch>(expression);
	watch->setParent(this);
	return watch;
}


shared_ptr<Instance> ScriptDebugger::addWatch_Reflection( std::string expression )
{
	return addWatch(expression);
}


shared_ptr<const Instances> ScriptDebugger::getWatches_Reflection()
{
	shared_ptr<Instances> result = rbx::make_shared<Instances>(watches.size());
	int i = 0;
	for (Watches::const_iterator iter = watches.begin(); iter != watches.end(); ++iter, ++i)
		(*result)[i] = shared_from(*iter);
	return result;
}

Reflection::Variant ScriptDebugger::getWatchValue_Reflection( shared_ptr<Instance> instance )
{
	if (DebuggerWatch* watch = Instance::fastDynamicCast<DebuggerWatch>(instance.get()))
		return getWatchValue(watch);
	throw std::runtime_error("bad watch argument");
}

static int setIndexInfo(lua_State * L)
{
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "setIndexInfo");
	return 0;
}

static int getIndexInfo(lua_State * L)
{
	//check if there's a key value set?
	const char * key = lua_tostring(L, 2);
	if (!key)
		return 0;

	// Find the extra 'arguments' in the metatable

	lua_getmetatable(L, 1);
	int metatableIndex = lua_gettop(L);

	// Get the original environment table
	lua_pushstring(L, "env");
	lua_rawget(L, metatableIndex);
	int envIndex = lua_gettop(L);

	// Get the chunk function that was originally called
	lua_pushstring(L, "function");
	lua_rawget(L, metatableIndex);
	//int functionIndex = lua_gettop(L);

	// Get the current stack frame
	lua_pushstring(L, "frame");
	lua_rawget(L, metatableIndex);
	int frame = (int) lua_tointeger(L, -1);

	// Get the thread
	lua_pushstring(L, "thread");
	lua_rawget(L, metatableIndex);
	lua_State * parentThread = lua_tothread(L, -1);

	//
	// Check in locals first
	//

	// If we're examining the main thread we need to skip over the watch expression call, and the metatable __index call
	if(parentThread == L)
		frame += 2;

	lua_Debug debugInfo;
	memset(&debugInfo, 0, sizeof(lua_Debug));
	if(lua_getstack(parentThread, frame, &debugInfo) == 1 && lua_getinfo(parentThread, "uf", &debugInfo))
	{
		int funcIndex = lua_gettop(parentThread);

		// Look in the upvalues
		if(debugInfo.nups > 0)
		{
			for(int index = 1; index <= debugInfo.nups; ++index)
			{
				const char * name = lua_getupvalue(parentThread, funcIndex, index);
				if(name != NULL && strcmp(name, key) == 0)
				{
					// Move the value across
					lua_xmove(parentThread, L, 1);

					// Pop the function off the stack
					lua_remove(parentThread, funcIndex);

					// Don't bother removing the other stuff on the stack, lua will do it for us
					return 1;
				}	
				lua_pop(parentThread, 1);
			}
		}

		// Check in the locals
		int index = 1;
		const char * name;
		while( (name = lua_getlocal(parentThread, &debugInfo, index++)) != NULL)
		{
			if(strcmp(name, key) == 0)
			{
				// Move the value across
				lua_xmove(parentThread, L, 1);

				// Pop the function off the stack
				lua_remove(parentThread, funcIndex);

				// Don't bother removing the other stuff on the stack, lua will do it for us
				return 1;
			}
			lua_pop(parentThread, 1);
		}

		// This might trigger an error
		lua_pushvalue(L, 2);
		lua_gettable(L, envIndex);

		// If it's nil, leave it as the return value anyway
		return 1;
	}
	else
	{
		// There's no lua function running, so just look in the globals
		lua_getglobal(L, key);
		return 1;
	}
}

static void setEvalThreadEnvironment(lua_State * parentThread, lua_State* evalThread, int stackDepth)
{
	int functionIndex = lua_gettop(evalThread);
	lua_checkstack(evalThread, 6);

	// Create environment table
	lua_createtable(evalThread, 0, 2);
	int envIndex = lua_gettop(evalThread);

	// Create metatable
	lua_createtable(evalThread, 0, 5);
	int metatableIndex = lua_gettop(evalThread);

	lua_pushstring(evalThread, "__index");
	lua_pushcfunction(evalThread, &getIndexInfo);
	lua_settable(evalThread, metatableIndex);

	lua_pushstring(evalThread, "__newindex");
	lua_pushcfunction(evalThread, &setIndexInfo);
	lua_settable(evalThread, metatableIndex);

	// Store the stack index
	lua_pushstring(evalThread, "frame");
	lua_pushnumber(evalThread, stackDepth);
	lua_settable(evalThread, metatableIndex);

	// Store the original thread (this will be required to access stack information)
	lua_pushstring(evalThread, "thread");
	lua_pushthread(parentThread);
	lua_xmove(parentThread, evalThread, 1);
	lua_settable(evalThread, metatableIndex);

	// Store the function
	lua_pushstring(evalThread, "function");
	lua_pushvalue(evalThread, functionIndex);
	lua_settable(evalThread, metatableIndex);

	// Store the original function environment
	lua_pushstring(evalThread, "env");
	lua_Debug debugInfo;
	memset(&debugInfo, 0, sizeof(lua_Debug));
	if(lua_getstack(parentThread, stackDepth, &debugInfo) == 1 && lua_getinfo(parentThread, "f", &debugInfo))
	{
		int funcIndex = lua_gettop(parentThread);
		lua_getfenv(parentThread, funcIndex);
		lua_xmove(parentThread, evalThread, 1);
		lua_remove(parentThread, funcIndex);
	}
	else
	{
		// If there's nothing running, fall back on the globals table.
		lua_pushvalue(evalThread, LUA_GLOBALSINDEX);
	}

	lua_settable(evalThread, metatableIndex);

	// Assign the metatable to the environment
	lua_setmetatable(evalThread, envIndex);

	// Set our environment for the function
	lua_setfenv(evalThread, functionIndex);

	//again set the function index at the top of the stack
	lua_settop(evalThread, functionIndex);
}

Reflection::Variant DebuggerManager::readWatchValue(std::string expression, int stackDepth, lua_State* L)
{
	RBXASSERT_BALLANCED_LUA_STACK(L);

	lua_State* evalThread = lua_newthread(L);
	lua_sethook(evalThread, NULL, 0, 0);	// Prevent re-entrancy

	Lua::ScopedPopper pop(L, 1);

	RBXASSERT_BALLANCED_LUA_STACK2(evalThread);

	const std::string code = "return " + expression;
	bool loaded;
	loaded = LuaVM::load(evalThread, ProtectedString::fromTrustedSource(code), "") == 0;
	Lua::ScopedPopper pop2(evalThread, 1);

	if (!loaded)
	{
		std::string err = Lua::safe_lua_tostring(evalThread, -1);
		throw RBX::runtime_error("syntax error: %s", err.c_str());
	}

	// set new function environment for evaluation thread so we can access locals/upvalues/environment of the original context
	setEvalThreadEnvironment(L, evalThread, stackDepth);

	if (lua_pcall(evalThread, 0, 1, 0))
	{
		//const char* message = lua_tostring(evalThread, -1);
		//throw RBX::runtime_error("runtime error: %s", message);
		return std::string("*** Value not found ***");
	}

	Reflection::Variant result;
	try
	{
		Lua::LuaArguments::get(evalThread, -1, result, false);
	}
	catch (std::runtime_error const& exp)
	{
		return std::string(RBX::format("*** %s ***", exp.what()));
	}

	return result;
}

Reflection::Variant ScriptDebugger::getWatchValue( DebuggerWatch* watch, int stackFrame )
{
	Reflection::Variant result;

	if (hasError())
	{
		// for error thread, we cannot do a resume. since it is not yielded try querying values directly.
		if (Lua::ThreadRef threadRef = errorThread.lock())
		{
			result = DebuggerManager::readWatchValue(watch->getExpression(), stackFrame, threadRef);
		}
	}
	else
	{
		result = withPausedThread<Reflection::Variant>(boost::bind(&DebuggerManager::readWatchValue, watch->getExpression(), stackFrame, _1));
	}

	return result;
}

Reflection::Variant ScriptDebugger::getKeyValue(std::string key, int stackFrame)
{
	Reflection::Variant result;

	if (hasError())
	{
		// for error thread, we cannot do a resume. since it is not yielded try querying values directly.
		if (Lua::ThreadRef threadRef = errorThread.lock())
		{
			result = DebuggerManager::readWatchValue(key, stackFrame, threadRef);
		}
	}
	else
	{
		result = withPausedThread<Reflection::Variant>(boost::bind(&DebuggerManager::readWatchValue, key, stackFrame, _1));
	}

	return result;
}

template<class R>
void ScriptDebugger::withPausedThreadHook( lua_State* L, lua_Debug *ar, boost::function<R(lua_State* L, lua_Debug *ar)> f, R& r, shared_ptr<std::string>& error)
{
	RBXASSERT(ar->event == LUA_HOOKLINE);
	RBXASSERT(currentLine == ar->currentline);

	try
	{
		r = f(L, ar);
	}
	catch (RBX::base_exception& ex)
	{
		error = rbx::make_shared<std::string>(ex.what());
	}

	RBXASSERT(pausedThread.lock() == L);
	//pausedThread = L;
	RBXASSERT(!RobloxExtraSpace::get(L)->yieldCaptured);
	RobloxExtraSpace::get(L)->yieldCaptured = true;
	lua_yield(L, 0);
}



template<class R>
R ScriptDebugger::withPausedThread( boost::function<R(lua_State* L, lua_Debug *ar)> f )
{
	// When pausedThread is set, then Lua stack is not usable.
	// Therefore, we resume the thread back into the hook and run our code there.
	// See http://lua-users.org/lists/lua-l/2012-06/msg00169.html

	if (Lua::ThreadRef thread = pausedThread.lock())
	{
		if (breakpointHookData)
		{
			// We haven't yielded yet, so go ahead and make the call directly
			return f(thread, breakpointHookData);
		}

		R r;
		shared_ptr<std::string> error;
		RBXASSERT(!hookFunction);
		hookFunction = boost::bind(&ScriptDebugger::withPausedThreadHook<R>, this, _1, _2, f, boost::ref(r), boost::ref(error));
		try
		{
			if (Lua::ThreadRef thread = pausedThread.lock())
			{
				RBXASSERT_BALLANCED_LUA_STACK(thread);
				ScriptContext::getContext(thread).resume(thread, 0);
			}
		}
		catch (...)
		{
			hookFunction.clear();
			throw;
		}
		hookFunction.clear();

		if (error)
			throw std::runtime_error(*error);

		return r;
	}
	else
		throw std::runtime_error("Cannot perform this operation unless the thread is paused");
}

void ScriptDebugger::debuggerBreak( lua_State* L, lua_Debug *ar )
{
	if (ignoreDebuggerBreak)
	{
		StandardOut::singleton()->print(MESSAGE_WARNING, "Breakpoints are not supported for the executed script code");	
		return;
	}

	lua_getinfo(L, "S", ar);

	switch (ar->event)
	{
	case LUA_HOOKLINE:
		currentLine = ar->currentline;
		break;
	case LUA_HOOKCALL:
		currentLine = ar->linedefined;
		break;
	case LUA_HOOKRET:
	case LUA_HOOKTAILRET:
		currentLine = ar->lastlinedefined;
		break;
	}

	RBXASSERT(currentLine > 0);

	//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Encountered breakpoint: %s line %d", ar->short_src, currentLine);

	pausedThread = L;
	currentThreadID = reinterpret_cast<uintptr_t>(L);

	PausedThreadData data;
	data.pausedLine = currentLine;
	data.thread = L;
	data.callStack = getStack();

	pausedThreads[reinterpret_cast<uintptr_t>(L)] = data;

	breakpointHookData = ar;
	encounteredBreak(currentLine);
	breakpointHookData = NULL;

	if (pausedThread.empty())
	{
		// If pausedThread was reset during encounteredBreak, then don't yield. We're done.
		//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Resuming: %s line %d", ar->short_src, ar->currentline);
		resuming();
		currentLine = 0;
	}
	else
	{
		RBXASSERT(!RobloxExtraSpace::get(L)->yieldCaptured);
		RobloxExtraSpace::get(L)->yieldCaptured = true;
		lua_yield(L, 0);
	}
}

bool ScriptDebugger::handleHook(lua_State* L, lua_Debug *ar)
{
	if (hookFunction)
	{
		hookFunction(L, ar);
		return true;
	}

	return false;
}

shared_ptr<Reflection::ValueMap> ScriptDebugger::readLocals(int stackIndex, lua_State* L)
{
	lua_Debug ar;
	if (lua_getstack(L, stackIndex, &ar) == 0)
		throw std::runtime_error("stackIndex out of range");

	shared_ptr<Reflection::ValueMap> locals = rbx::make_shared<Reflection::ValueMap>();
	RBXASSERT_BALLANCED_LUA_STACK(L);
	int n = 1;
	while (const char* name = lua_getlocal(L, &ar, n++))
	{
		if (name[0] > 0)
			if (strcmp(name, "(*temporary)") != 0)
			{
				Reflection::Variant value;
				Lua::LuaArguments::get(L, -1, value, false);
				(*locals)[name] = value;
			}
		lua_pop(L, 1); // pop value
	}
	return locals;
}

shared_ptr<Reflection::ValueMap> ScriptDebugger::readGlobals(lua_State* L)
{
	RBXASSERT_BALLANCED_LUA_STACK(L);

#if 0
	lua_Debug ar;
	if (!lua_getstack(L, stackIndex, &ar))
		throw std::runtime_error("stackIndex out of range");
	if (!lua_getinfo(L, "f", &ar))
		throw std::runtime_error("failed to get function info");
	// The stack now contains the function we are interested in

	lua_getfenv(L, -1);
	// The stack now contains the the environment table
	const int t = lua_gettop(L);
#else
	const int t = LUA_GLOBALSINDEX;
	lua_getfenv(L, t);
#endif
	shared_ptr<Reflection::ValueMap> result = rbx::make_shared<Reflection::ValueMap>();

	lua_pushnil(L);  /* first key */
	while (lua_next(L, t))
	{
		if (lua_isstring(L, -2))
		{
			const char* name = lua_tostring(L, -2);
			RBXASSERT(result->find(name) == result->end());
			
			Reflection::Variant value;
			Lua::LuaArguments::get(L, -1, value, false);
			(*result)[name] = value;
		}

		/* removes 'value'; keeps 'key' for next iteration */
		lua_pop(L, 1);
	}

	lua_pop(L, 1);

	return result;
}

shared_ptr<Reflection::ValueMap> ScriptDebugger::readUpvalues(int stackIndex, lua_State* L)
{
	lua_Debug ar;
	if (lua_getstack(L, stackIndex, &ar) == 0)
		throw std::runtime_error("stackIndex out of range");

	shared_ptr<Reflection::ValueMap> arguments = rbx::make_shared<Reflection::ValueMap>();

	RBXASSERT_BALLANCED_LUA_STACK(L);
	int result = lua_getinfo(L, "fu", &ar);	
	RBXASSERT(result);
	// stack contains current function

	int n = 1;
	while (const char* name = lua_getupvalue(L, -1, n++))
	{
		if (name[0] > 0)
		{
			Reflection::Variant value;
			Lua::LuaArguments::get(L, -1, value, false);
			RBXASSERT(arguments->find(name) == arguments->end());
			(*arguments)[name] = value;
		}
		lua_pop(L, 1); // pop value
	}

	lua_pop(L, 1); // pop current function

	return arguments;
}

static const char *traverseGlobals(lua_State *L, const TValue *o) 
{
  Table *globalTable = hvalue(gt(L));
  if (globalTable)
  {
	  int tableSize = sizenode(globalTable);
	  while (tableSize--) 
	  {
		  Node *node = gnode(globalTable, tableSize);
		  if (luaO_rawequalObj(o, gval(node)) && ttisstring(gkey(node)))
			  return getstr(tsvalue(gkey(node)));
	  }
  }
  return NULL;
}

static void populateFunctionName(lua_State *L, ScriptDebugger::FunctionInfo& funcInfo)
{
	StkId func = L->top - 1;
	if (func && isLfunction(func))
	{
		const char* globalName = traverseGlobals(L, func);
		if (globalName)
		{
			funcInfo.name = globalName;
			funcInfo.namewhat = "global";
		}
	}
}

std::vector<ScriptDebugger::FunctionInfo> ScriptDebugger::readStack(lua_State* L)
{
	std::vector<FunctionInfo> result;

	for (int frame=0; ; ++frame)
	{
		lua_Debug ar;
		if (lua_getstack(L, frame, &ar) == 0)
			break;
		int r = lua_getinfo(L, "nSlf", &ar);
		RBXASSERT(r);
		FunctionInfo fi;
		fi.frame = frame;
		fi.currentline = ar.currentline;
		fi.lastlinedefined = ar.lastlinedefined;
		fi.linedefined = ar.linedefined;
		if (ar.name)
			fi.name = ar.name;
		if (ar.what)
			fi.what = ar.what;
		if (ar.namewhat)
			fi.namewhat = ar.namewhat;
		if (ar.short_src)
			fi.short_src = ar.short_src;
		fi.script = shared_from(getScriptForLuaState(L));

		// if we couldn't get the function name, try populating using global table
		if (fi.name.empty() && fi.what != "main")
			populateFunctionName(L, fi);

		result.push_back(fi);
	}

	return result;
}


bool ScriptDebugger::onLineHook( lua_State* L, lua_Debug *ar )
{
	if (currentLine > 0)
	{
		// Step over the current line. This prevents an infinite loop of breaks
		lua_getinfo(L, "S", ar);
		//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Resuming: %s line %d", ar->short_src, ar->currentline);
		currentLine = 0;
		return false;
	}

	DebuggerBreakpoint* bp = NULL;
	if (hasDifferentScriptInstances(L))
	{
		lua_getinfo(L, "f", ar);
		RBX::Instance* scriptInstance = getScriptForLuaState(L);
		if (scriptInstance)
		{
			if (ScriptDebugger* debugger = gDebuggerManager->findDebugger(scriptInstance))
				bp = debugger->findBreakpoint(ar->currentline);
		}
	}
	else
	{
		bp = findBreakpoint(ar->currentline);
	}

	if (bp && shouldBreak(bp, L))
	{
		debuggerBreak(L, ar);
		return true;
	}

	return false;
}

bool ScriptDebugger::shouldBreak(DebuggerBreakpoint* bp, lua_State* L)
{
	if (!bp->isEnabled())
		return false;

	if (bp->getCondition().empty())
		return true;

	RBXASSERT_BALLANCED_LUA_STACK(L);

	lua_State* evalThread = lua_newthread(L);
	RBXASSERT_BALLANCED_LUA_STACK2(evalThread);

	std::string condition = "return " + bp->getCondition();
	bool conditionFailedToLoad;
	conditionFailedToLoad = LuaVM::load(evalThread, ProtectedString::fromTrustedSource(condition), "") != 0;
	if (conditionFailedToLoad)
	{
		std::string err = Lua::safe_lua_tostring(evalThread, -1);
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Breakpoint %d condition syntax error: %s", bp->getLine(), err.c_str());
		lua_pop(evalThread, 1);		// pop the message
		lua_pop(L, 1);
		return false;
	}

	if (lua_pcall(evalThread, 0, 1, 0))
	{
		const char* message = lua_tostring(evalThread, -1);
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Breakpoint %d condition runtime error: %s", bp->getLine(), message);
		lua_pop(evalThread, 1);
		lua_pop(L, 1);
		return false;
	}

	bool result = lua_toboolean(evalThread, -1) != 0;
	lua_pop(evalThread, 1);
	lua_pop(L, 1);

	return result;
}

bool ScriptDebugger::askForbidChild(const Instance* newChild) const
{
	if (Instance::fastDynamicCast<DebuggerWatch>(newChild))
		return false;

	if (Instance::fastDynamicCast<DebuggerBreakpoint>(newChild))
		return false;

	return true;

}

void ScriptDebugger::verifyAddChild(const Instance* newChild) const
{
	if (!askForbidChild(newChild))
		return;

	throw std::runtime_error("Only DebuggerWatch and DebuggerBreakpoint can be children of ScriptDebugger");
}

void ScriptDebugger::verifySetParent( const Instance* newParent ) const
{
	if (newParent && !Instance::fastDynamicCast<DebuggerManager>(newParent))
		throw std::runtime_error("ScriptDebugger must be a child of DebuggerManager");
}

shared_ptr<Instance> ScriptDebugger::setBreakpoint_Reflection( int line )
{
	return setBreakpoint(line);
}


DebuggerBreakpoint* ScriptDebugger::findBreakpoint( int line )
{
	if (!breakpoints.empty())
	{
		Breakpoints::const_iterator iter = breakpoints.find(line);
		if (iter != breakpoints.end())
			return (iter->second);
	}
	return NULL;
}

shared_ptr<DebuggerBreakpoint> ScriptDebugger::setBreakpoint( int line )
{
	if (DebuggerBreakpoint* breakpoint = findBreakpoint(line))
	{
		DebuggerBreakpoint::prop_Enabled.setValue(breakpoint, true);
		return shared_from(breakpoint);
	}

	shared_ptr<DebuggerBreakpoint> breakpoint = Creatable<Instance>::create<DebuggerBreakpoint>(line);
	breakpoint->setParent(this);
	return breakpoint;
}

void ScriptDebugger::onChildRemoved( Instance* child )
{
	if (DebuggerBreakpoint* breakpoint = Instance::fastDynamicCast<DebuggerBreakpoint>(child))
	{
		breakpoints.erase(breakpoint->getLine());
		breakpointRemoved(shared_from(breakpoint));
	}
	if (DebuggerWatch* watch = Instance::fastDynamicCast<DebuggerWatch>(child))
	{
		watches.erase(std::remove(watches.begin(), watches.end(), watch), watches.end());
		watchRemoved(shared_from(watch));
	}

	Super::onChildRemoved(child);
}

void ScriptDebugger::onChildAdded( Instance* child )
{
	if (DebuggerBreakpoint* breakpoint = Instance::fastDynamicCast<DebuggerBreakpoint>(child))
	{
		RBXASSERT(breakpoint->getLine() > 0);
		breakpoints[breakpoint->getLine()] = breakpoint;
		breakpointAdded(shared_from(breakpoint));
	}
	if (DebuggerWatch* watch = Instance::fastDynamicCast<DebuggerWatch>(child))
	{
		watches.push_back(watch);
		watchAdded(shared_from(watch));
	}

	updateHook();
	Super::onChildAdded(child);
}

void ScriptDebugger::setScript( Instance* value )
{
	if (Script* scriptPtr = Instance::fastDynamicCast<Script>(value))
	{
		setScript(scriptPtr);
	}
	else if (ModuleScript* moduleScriptPtr = Instance::fastDynamicCast<ModuleScript>(value))
	{
		setScript(moduleScriptPtr);
	}
	else
	{
		throw std::runtime_error("Debugger can be attached to a script or module script");
	}

	// add signal so we can clone debugger on script clone
	scriptClonedConnection = value->onDemandWrite()->instanceClonedSignal.connect(boost::bind(&ScriptDebugger::onScriptCloned, this, _1)); 
}

void ScriptDebugger::setScript( Script* value )
{
	if (!value)
		throw std::runtime_error("Cannot attach debugger to a null script");

	if (script.get() == value)
		return;

	RBXASSERT(!script);	// Changing the script is not supported. DebuggerManager doesn't track changes like this

	// check if we can get rootThread from threadNode
	// NOTE: if script has been executed by the time this function is called then we will not have any root thread
	//       still debugger must be attached, so on reload of script debugging can take place
	if (value->threadNode)
		value->threadNode->forEachRefs(boost::bind(&ScriptDebugger::updateRootThread, this, _1));

	scriptStartedConnection.disconnect();
	scriptStoppedConnection.disconnect();
	scriptParentChangedConnection.disconnect();

	script = shared_from(value);

	scriptStartedConnection = value->starting.connect(boost::bind(&ScriptDebugger::onScriptStarting, this, _1));
	scriptStoppedConnection = value->stopped.connect(boost::bind(&ScriptDebugger::onScriptStopped, this));
	scriptParentChangedConnection = value->ancestryChangedSignal.connect(boost::bind(&ScriptDebugger::onScriptParentChanged, this, _2));

	raiseChanged(prop_Script);
	raiseChanged(prop_ScriptPath);
}

void ScriptDebugger::setScript(ModuleScript* value)
{
	if (!value)
		throw std::runtime_error("Cannot attach debugger to a null script");

	if (script.get() == value)
		return;

	RBXASSERT(!script);	// Changing the script is not supported. DebuggerManager doesn't track changes like this

	scriptParentChangedConnection.disconnect();

	script = shared_from(value);

	scriptStartedConnection = value->starting.connect(boost::bind(&ScriptDebugger::onScriptStarting, this, _1));
	scriptParentChangedConnection = value->ancestryChangedSignal.connect(boost::bind(&ScriptDebugger::onScriptParentChanged, this, _2));
}

void ScriptDebugger::onScriptStarting( lua_State* L )
{
	if (!FFlag::LuaDebugger)
		return;

	DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();
	if (!debuggerManager.getEnabled())
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Can't debug script %s because debugging is disabled", script->getName().c_str());
		return;
	}

	specialBreakpoint.reset();
	rootThreadResumed = false;
	currentThreadID = 0;

	debuggerManager.populateForLookup(L, this);

	lua_getfield(L, LUA_GLOBALSINDEX, "script");
	globalRawScriptPtr = lua_touserdata(L, -1);
	lua_pop(L, 1);

	rootThread = L;
	updateHook();
}

void ScriptDebugger::onScriptStopped()
{
	specialBreakpoint.reset();
	rootThread.reset();
	pausedThread.reset();
	errorThread.reset();
	currentLine = 0;
	breakpointHookData = NULL;
	globalRawScriptPtr = NULL;
	prevFuncTable = NULL;
    prevRawScriptPtr = NULL;
	rootThreadResumed = false;
	currentThreadID = 0;
	pausedThreads.clear();
}

void ScriptDebugger::onScriptParentChanged(shared_ptr<RBX::Instance> newParent)
{
	if (!newParent)
	{
		// first remove all children
		removeAllChildren();
		// now reset debugger data
		onScriptStopped();
		// do the cleanup
		setParent(NULL);
	}
	else
	{
		// else inform children about change in hierarchy
		onAncestorChanged(AncestorChanged(this, script->getParent(), newParent.get()));
	}
}

void ScriptDebugger::onScriptCloned(shared_ptr<RBX::Instance> clonedScript)
{
	boost::shared_ptr<ScriptDebugger> clonedDebugger = createClone(clonedScript);
	if (clonedDebugger)
		RBX::Scripting::DebuggerManager::singleton().addDebugger(clonedDebugger);
}

bool ScriptDebugger::isPaused() const
{
	return !pausedThreads.empty();
}

void ScriptDebugger::resume()
{
	throw std::runtime_error("Depricated API - Use DebuggerManager::resume instead");
}

ScriptContext::Result ScriptDebugger::resumeThread(lua_State* thread, bool evalLineHookForCurrentLine)
{
	ScriptContext::Result result(ScriptContext::Success);
	// if we do not have paused thread then reset breakpoint data so we do not have incorrect execution halt when the thread resumes
	if (!isPausedThread(reinterpret_cast<uintptr_t>(thread)) || isErrorThread(reinterpret_cast<uintptr_t>(thread)))
	{
		specialBreakpoint.reset();
		breakpointHookData = NULL;
		errorThread.reset();
		pausedThreads.erase(reinterpret_cast<uintptr_t>(thread));
		return result;
	}

	RBXASSERT_BALLANCED_LUA_STACK(thread);
	pausedThread.reset();
	pausedThreads.erase(reinterpret_cast<uintptr_t>(thread));

	// if we need to evaluate line hook for current line then reset it to 0 (DE8670)
	if (evalLineHookForCurrentLine)
		currentLine = 0;

	resuming();
	if (breakpointHookData)
	{
		// We are in the breakpoint, so the yield will be aborted
	}
	else
	{
		updateHook();
		result = ScriptContext::getContext(thread).resume(thread, 0);
		if (!rootThreadResumed && isRootThread(reinterpret_cast<uintptr_t>(thread)) && (result == ScriptContext::Success))
			rootThreadResumed = true;
	}

	return result;
}

bool ScriptDebugger::isPausedThread(long threadID)
{
	PausedThreads::iterator iter = pausedThreads.find(threadID);
	return (iter != pausedThreads.end());
}

bool ScriptDebugger::isErrorThread(long threadID)
{
	PausedThreads::iterator iter = pausedThreads.find(threadID);
	if (iter != pausedThreads.end())
		return iter->second.hasError;
	return false;
}

bool ScriptDebugger::isRootThread(long threadID)
{
	if (!rootThread.empty() && (threadID == reinterpret_cast<uintptr_t>( rootThread.lock().get())))
		return true;
	return false;
}

shared_ptr<const Reflection::ValueMap> ScriptDebugger::getLocals(int stackIndex)
{
	typedef shared_ptr<const Reflection::ValueMap> Result;
	Result locals = withPausedThread<Result>(boost::bind(&readLocals, stackIndex, _1));
	return locals;
}

shared_ptr<const Reflection::ValueMap> ScriptDebugger::getUpvalues(int stackIndex)
{
	typedef shared_ptr<const Reflection::ValueMap> Result;
	Result upvalues = withPausedThread<Result>(boost::bind(&readUpvalues, stackIndex, _1));
	return upvalues;
}
shared_ptr<const Reflection::ValueMap> ScriptDebugger::getGlobals()
{
	typedef shared_ptr<const Reflection::ValueMap> Result;
	Result globals = withPausedThread<Result>(boost::bind(&readGlobals, _1));
	return globals;
}

static bool doSetLocal(std::string valueName, const Reflection::Variant& value, int stackIndex, lua_State* L)
{
	lua_Debug ar;
	if (lua_getstack(L, stackIndex, &ar) == 0)
		throw std::runtime_error("stackIndex out of range");

	RBXASSERT_BALLANCED_LUA_STACK(L);
	int n = 1;
	bool foundIt = false;
	while (const char* name = lua_getlocal(L, &ar, n))
	{
		lua_pop(L, 1); // pop value
		if (valueName == name)
		{
			RBXASSERT_BALLANCED_LUA_STACK(L);
			int count = Lua::LuaArguments::push(value, L);
            RBXASSERT(count > 0);
			const char* name2 = lua_setlocal(L, &ar, n);
			RBXASSERT(strcmp(name, name2) == 0);
			lua_pop(L, count-1);
			foundIt = true;
			break;
		}
		n++;
	}

	return foundIt;
}

void ScriptDebugger::setLocal( std::string name, Reflection::Variant value, int stackFrame)
{
	if (!withPausedThread<bool>(boost::bind(&doSetLocal, name, boost::cref(value), stackFrame, _1)))
		throw std::runtime_error(RBX::format("Bad local %s", name.c_str()));
}

static bool doSetUpvalue(std::string valueName, const Reflection::Variant& value, int stackIndex, lua_State* L)
{
	lua_Debug ar;
	if (lua_getstack(L, stackIndex, &ar) == 0)
		throw std::runtime_error("stackIndex out of range");

	RBXASSERT_BALLANCED_LUA_STACK(L);
	int result = lua_getinfo(L, "fu", &ar);	
	RBXASSERT(result);
	// stack contains current function

	bool foundIt = false;
	int n = 1;
	while (const char* name = lua_getupvalue(L, -1, n))
	{
		lua_pop(L, 1); // pop value
		if (valueName == name)
		{
			RBXASSERT_BALLANCED_LUA_STACK(L);
			int count = Lua::LuaArguments::push(value, L);
            RBXASSERT(count > 0);
            lua_pop(L, count-1); // We only want 1 value on the stack (strip out tuples)

			const char* name2 = lua_setupvalue(L, -2, n);
			RBXASSERT(strcmp(name, name2) == 0);
			foundIt = true;
			break;
		}
		n++;
	}

	lua_pop(L, 1); // pop current function

	return foundIt;
}

void ScriptDebugger::setUpvalue( std::string name, Reflection::Variant value, int stackFrame)
{
	if (!withPausedThread<bool>(boost::bind(&doSetUpvalue, name, boost::cref(value), stackFrame, _1)))
		throw std::runtime_error(RBX::format("Bad upvalue %s", name.c_str()));
}


static bool doSetGlobal(std::string valueName, const Reflection::Variant& value, lua_State* L)
{
	RBXASSERT_BALLANCED_LUA_STACK(L);

	int count = Lua::LuaArguments::push(value, L);
	if (count == 0)
		return false;
	lua_setglobal(L, valueName.c_str());
	lua_pop(L, count - 1);
	return true;
}

void ScriptDebugger::setGlobal( std::string name, Reflection::Variant value)
{
	if (!withPausedThread<bool>(boost::bind(&doSetGlobal, name, boost::cref(value), _1)))
		throw std::runtime_error(RBX::format("Bad global %s", name.c_str()));
}


std::vector<ScriptDebugger::FunctionInfo> ScriptDebugger::getStack()
{
	if (Lua::ThreadRef thread = pausedThread.lock())
		return readStack(thread);
	else
		throw std::runtime_error("Cannot perform this operation unless the thread is paused");
}

shared_ptr<const Reflection::ValueArray> ScriptDebugger::getStack_Reflection()
{
	shared_ptr<Reflection::ValueArray> result = rbx::make_shared<Reflection::ValueArray>();

	Stack stack = getStack();
	for (Stack::const_iterator iter = stack.begin(); iter != stack.end(); ++iter)
	{
		shared_ptr<Reflection::ValueMap> item = rbx::make_shared<Reflection::ValueMap>();
		(*item)["frame"] = iter->frame;
		(*item)["name"] = iter->name;
		(*item)["currentline"] = iter->currentline;
		(*item)["linedefined"] = iter->linedefined;
		(*item)["lastlinedefined"] = iter->lastlinedefined;
		(*item)["what"] = iter->what;
		(*item)["namewhat"] = iter->namewhat;
		(*item)["short_src"] = iter->short_src;
		result->push_back(shared_ptr<const Reflection::ValueMap>(item));
	}

	return result;
}

void ScriptDebugger::pause()
{
	if (isPaused())
		return;
	
	specialBreakpoint.reset(new PauseNowBreakpoint());
	updateHook();
}

void ScriptDebugger::stepInto()
{
	throw std::runtime_error("Depricated API - Use DebuggerManager::stepInto instead");
}

void ScriptDebugger::stepOut()
{
	throw std::runtime_error("Depricated API - Use DebuggerManager::stepOut instead");
}

void ScriptDebugger::stepOver()
{
	throw std::runtime_error("Depricated API - Use DebuggerManager::stepOver instead");
}

// Can only be called after lua_getinfo with "f" included in the parameters.
RBX::Instance* ScriptDebugger::getScriptForLuaState(lua_State* L)
{
	RBXASSERT(lua_isnil(L, -1) || lua_isfunction(L, -1));
	if (lua_isnil(L, -1) || lua_iscfunction(L, -1))
	{
		return NULL;
	}

	Lua::ScopedPopper pop(L, 2);

	lua_getfenv(L, -1);
	int fIndex = lua_gettop(L);
	lua_getfield(L, fIndex, "script");

	Reflection::Variant varResult;
	Lua::LuaArguments::get(L, -1, varResult, false);
	if (varResult.isType<boost::shared_ptr<RBX::Instance> >())
		return varResult.get<boost::shared_ptr<RBX::Instance> >().get();

	return NULL;
}

bool ScriptDebugger::hasDifferentScriptInstances(lua_State* L)
{
	Closure* func  = curr_func(L);
	// verify if function environment is different
	if (prevFuncTable != func->c.env)
	{
		// save env table
		prevFuncTable = func->c.env;

		// get "script" value
		lua_getfield(L, LUA_ENVIRONINDEX, "script");
		prevRawScriptPtr = lua_touserdata(L, -1);
		lua_pop(L, 1);

		// alternate way of accessing "script" field (it uses internal lua calls)
		//prevRawScriptPtr = rawuvalue(luaH_getstr(prevFuncTable, luaS_new(L, "script"))) + 1;
	}

	return (globalRawScriptPtr != prevRawScriptPtr);
}

static std::string getHashString(RBX::Instance* pInstance)
{
	if (Script* scriptPtr = Instance::fastDynamicCast<Script>(pInstance))
		return scriptPtr->requestHash();
	
	if (ModuleScript* moduleScriptPtr = Instance::fastDynamicCast<ModuleScript>(pInstance))
		return moduleScriptPtr->requestHash();

	return std::string();
}

//script path - indices of the children hirearchy upto datamodel
/*
Following is an example, showing generated script path

DataModel
    Workspace
	    Script*
        Model
          Part
          Part
          Script**
        Model
          Model
             Script***
             Script
          Script****

Script*    : 0|0 (1st child of datamodel --> 1st child of workspace)
Script**   : 0|1|2
Script***  : 0|2|0|0
Script**** : 0|2|1

With Hash string, 
Script*    : 0|0-Hash_String
Script**   : 0|1|2-Hash_String
Script***  : 0|2|0|0-Hash_String
Script**** : 0|2|1-Hash_String

Scripts will be retrieved using datamodel and the hiereachy saved.
*/
std::string ScriptDebugger::getScriptPath() const
{ 
	if (!script)
		return "";

	const RBX::Instance* pParent = script->getParent();
	const RBX::Instance* pChild = script.get();

	std::vector<int> childIndices;
	while (pParent)
	{
		childIndices.push_back(pParent->findChildIndex(pChild));
		
		pChild = pParent;
		pParent = pChild->getParent();
	}

	if (!childIndices.size())
		return "";

	std::ostringstream ss;
	std::copy(childIndices.rbegin(), childIndices.rend() - 1, std::ostream_iterator<int>(ss, "|"));
	ss << childIndices.front();

	std::string result(ss.str());
	result.append("-");
	result.append(getHashString(script.get()));
	return result;
}

void ScriptDebugger::setScriptPath(std::string scriptPath)
{
	RBX::Instance* pParent = RBX::DataModel::get(RBX::Scripting::DebuggerManager::singleton().getDataModel());
	RBX::Instance* pChild = NULL;

	std::string childPath(scriptPath), savedHashString;

	std::vector<std::string> strings;
	boost::split(strings, scriptPath, boost::is_any_of("-"));

	// we must have 2 strings separated by "-"
	if (strings.size() != 2)
		throw std::runtime_error("Debugger data cannot be loaded");

	childPath = strings[0];
	savedHashString = strings[1];

	int childNum = 0;
	boost::tokenizer<boost::char_separator<char> > tokens(childPath, boost::char_separator<char>("|"));
	for (boost::tokenizer<boost::char_separator<char> > ::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
	{
		if (!pParent)
			throw std::runtime_error("Debugger data cannot be loaded");

		//get the child from index
		childNum = atoi((*tok_iter).c_str());
		if (childNum < 0 || childNum >= (int)pParent->numChildren())
			throw std::runtime_error("Debugger data cannot be loaded");

		pChild = pParent->getChild(childNum);
		pParent = pChild;
	}

	if (getHashString(pChild) != savedHashString)
		throw std::runtime_error("Debugger data cannot be loaded");

	setScript(pChild);
}

void ScriptDebugger::updateRootThread(ScriptDebugger* scriptDebugger, lua_State *L)
{
	// set root thread if it's not there
	if (L && scriptDebugger && scriptDebugger->rootThread.empty())
		scriptDebugger->rootThread = L;

}

void ScriptDebugger::setLuaHook(ScriptDebugger* scriptDebugger, int hookMask, lua_State *L)
{
	if (L && (L->hookmask != hookMask))
	{
		const RobloxExtraSpace* pExtraSpace = RobloxExtraSpace::get(L);
		if (pExtraSpace)
		{
			shared_ptr<BaseScript> script = pExtraSpace->script.lock();
			if (script && (scriptDebugger->script == script))
			{
				lua_sethook(L, DebuggerManager::hook, hookMask, ScriptContext::hookCount);
			}
			else
			{
				shared_ptr<RBX::ModuleScript> pModuleScript = RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(scriptDebugger->script);
				if (pModuleScript)
					lua_sethook(L, DebuggerManager::hook, hookMask, ScriptContext::hookCount);
			}
		}
	}
}

void ScriptDebugger::updateHook()
{
	if (!script)
		return;

	if (rootThread.empty())
	{
		Script* scriptPtr = Instance::fastDynamicCast<Script>(script.get());
		if (!scriptPtr || !scriptPtr->threadNode)
			return;

		// get root thread from threadNode
		scriptPtr->threadNode->forEachRefs(boost::bind(&ScriptDebugger::updateRootThread, this, _1));
		if (rootThread.empty())
			return;
	}

	// set hook for all the dependent threads
	RobloxExtraSpace* pExtraSpace = RobloxExtraSpace::get(rootThread.lock());
	if (pExtraSpace)
	{
		int hookMask = LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT;
		if (RBX::Scripting::DebuggerManager::singleton().getBreakOnErrorMode() == BreakOnErrorMode_AllExceptions)
			hookMask = LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT | LUA_MASKERROR;
		pExtraSpace->forEachThread(boost::bind(&ScriptDebugger::setLuaHook, this, hookMask, _1));
	}
}

void ScriptDebugger::handleError(lua_State* thread)
{
	if (!FFlag::LuaDebuggerBreakOnError)
		return;

	// save thread
	errorThread = thread;

	// get error message and read stack information
	std::string errorMessage = RBX::Lua::safe_lua_tostring(thread, -1);
	Stack stack = readStack(thread);
	
	// cleanup stack (for a C function currentline can be -1, so we cannot break there)
	if (stack.size() > 0 && stack[0].currentline < 0)
		stack.erase(stack.begin());
	
	handleError(errorMessage, stack);
}

void ScriptDebugger::handleError(std::string errorMessage, const Stack& stack)
{
	if (!FFlag::LuaDebuggerBreakOnError)
		return;

	// for syntax error related errors, we need to extract line number from message itself 
	if (!errorMessage.empty())
	{
		size_t start = errorMessage.find(':');
		size_t stop = errorMessage.find(':', start + 1);
		if (stop > start)
		{
			currentLine = atoi(errorMessage.substr(start + 1, stop).c_str());
			errorMessage = errorMessage.substr(stop + 1);
		}
	}
	
	if (stack.size() > 0)
		currentLine = stack[0].currentline;

	// we must have a valid error line
	if (currentLine > 0)
	{
		if (!errorThread.empty())
		{
			PausedThreadData data;

			data.pausedLine = currentLine;
			data.callStack = stack;

			data.hasError = true;
			data.errorMessage = errorMessage;
			data.thread = errorThread;

			long threadID = reinterpret_cast<uintptr_t>(errorThread.lock().get());
			pausedThreads[threadID] = data;
			currentThreadID = threadID;

			scriptErrorDetected(currentLine, errorMessage, stack);
		}
		else
		{
			scriptErrorDetected(currentLine, errorMessage, stack);
		}
	}
}

void ScriptDebugger::setCurrentThread(long threadID)
{
	PausedThreads::iterator iter = pausedThreads.find(threadID);
	if (iter != pausedThreads.end())
	{
		currentThreadID = threadID;

		PausedThreadData data = iter->second;
		currentLine = data.pausedLine;
		data.hasError ? errorThread = data.thread : pausedThread = data.thread;
	}
}

boost::shared_ptr<ScriptDebugger> ScriptDebugger::createClone(boost::shared_ptr<Instance> clonedScript)
{
	// first create debugger
	boost::shared_ptr<ScriptDebugger> clonedDebugger = Creatable<Instance>::create<ScriptDebugger>(boost::ref(*clonedScript));

	// now it's children
	const Scripting::ScriptDebugger::Breakpoints& breakpoints = getBreakpoints();
	for (RBX::Scripting::ScriptDebugger::Breakpoints::const_iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter)
	{
		if (iter->second)
		{
			shared_ptr<Instance> breakpoint = iter->second->clone(RBX::EngineCreator);
			if (breakpoint)
				breakpoint->setParent(clonedDebugger.get());
		}
	}

	const Scripting::ScriptDebugger::Watches& watches = getWatches();
	for (RBX::Scripting::ScriptDebugger::Watches::const_iterator iter = watches.begin(); iter != watches.end(); ++iter)
	{
		if (*iter)
		{
			shared_ptr<Instance> watch = (*iter)->clone(RBX::EngineCreator);
			if (watch)
				watch->setParent(clonedDebugger.get());
		}
	}

	return clonedDebugger;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      DebuggerBreakpoint
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DebuggerBreakpoint::DebuggerBreakpoint(void)
:enabled(true)
,line(0)
{
}

DebuggerBreakpoint::DebuggerBreakpoint( int line )
:enabled(true)
,line(line)
{
	RBXASSERT(line > 0);
	setName(RBX::format("Breakpoint%d", line));
}

DebuggerBreakpoint::~DebuggerBreakpoint(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                      DebuggerWatch
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DebuggerWatch::DebuggerWatch(std::string expression)
	:expression(expression)
{
	setName(sDebuggerWatch);
}

void DebuggerBreakpoint::verifySetParent( const Instance* newParent ) const
{
	if (newParent && !Instance::fastDynamicCast<ScriptDebugger>(newParent))
		throw std::runtime_error("Breakpoints must be a child of ScriptDebugger");
}

void DebuggerWatch::verifySetParent( const Instance* newParent ) const
{
	if (newParent && !Instance::fastDynamicCast<ScriptDebugger>(newParent))
		throw std::runtime_error("DebuggerWatch must be a child of ScriptDebugger");
}


void DebuggerWatch::checkExpressionSyntax()
{
	Lua::ScopedState L;
	RBXASSERT_BALLANCED_LUA_STACK(L);

	const std::string code = "return " + expression;
	bool error;
	error = LuaVM::load(L, ProtectedString::fromTrustedSource(code), "") == LUA_ERRSYNTAX;
	Lua::ScopedPopper pop(L, 1);	
	if (error)
	{
		std::string err = Lua::safe_lua_tostring(L, -1);
		throw RBX::runtime_error("syntax error: %s", err.c_str());
	}
}
