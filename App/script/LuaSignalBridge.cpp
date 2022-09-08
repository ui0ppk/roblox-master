#include "stdafx.h"

#include "Script/LuaSignalBridge.h"

#include "Script/Script.h"
#include "Script/ScriptContext.h"
#include "Script/LuaInstanceBridge.h"
#include "Script/LuaAtomicClasses.h"
#include "script/LuaEnum.h"
#include "Script/ScriptEvent.h"
#include "Script/LuaArguments.h"
#include "reflection/EnumConverter.h"
#include "util/ScopedAssign.h"
#include "util/NormalId.h"
#include "util/ProtectedString.h"
#include "util/standardout.h"

LOGVARIABLE(LuaBridge, 0)
DYNAMIC_FASTFLAGVARIABLE(UseSubmitTaskWhenFiringSignalsOnSettings, true)

namespace RBX { namespace Lua {

template<>
const char* Bridge<EventInstance>::className("RBXScriptSignal"); 		

template<>
int Bridge<EventInstance>::on_index(const EventInstance& object, const char* name, lua_State* L)
{
	// The pre-defined "connect()" method
	if (strcmp(name, "connect") == 0 || strcmp(name, "Connect") == 0)
	{
		lua_pushcfunction(L, EventBridge::connect);
		return 1;
	}

	// TODO: Remove this when we remove it from any Roblox scripts.
	if (strcmp(name, "connectFirst")==0)
	{
		RBX::Security::Context::current().requirePermission(RBX::Security::LocalUser, "connectFirst");
		StandardOut::singleton()->print(MESSAGE_WARNING, "connectFirst is deprecated");
		lua_pushcfunction(L, EventBridge::connect);
		return 1;
	}

	// TODO: Remove this when we remove it from any Roblox scripts.
	if (strcmp(name, "connectLast")==0)
	{
		RBX::Security::Context::current().requirePermission(RBX::Security::LocalUser, "connectLast");
		StandardOut::singleton()->print(MESSAGE_WARNING, "connectLast is deprecated");
		lua_pushcfunction(L, EventBridge::connect);
		return 1;
	}

	// The pre-defined "wait()" method
	if (strcmp(name, "wait")==0 || strcmp(name, "Wait") == 0)
	{
		lua_pushcfunction(L, EventBridge::wait);
		return 1;
	}

	if (strcmp(name, "disconnect")==0)
		throw RBX::runtime_error("Event:disconnect() has been deprecated. Use connection object returned by connect()");

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<EventInstance>::on_newindex(EventInstance& object, const char* name, lua_State *L)
{
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

// This Slot is used when a script calls "connect" on a Signal
class FunctionScriptSlot 
	: public BaseScript::Slot
{
	typedef Reflection::TGenericSlotWrapper<FunctionScriptSlot> WrapperType;
    const Reflection::EventDescriptor* descriptor;
	ScriptContext& context;
	WeakFunctionRef function;
	WeakThreadRef cachedSlotThread;		// cached thread to be re-used if available
	int executionDepth;
	bool useSubmitTask;
	weak_ptr<WrapperType> weakSelf;
	weak_ptr<DataModel> weakDm;
public:

    FunctionScriptSlot(const Reflection::EventDescriptor* descriptor, lua_State* thread, int functionIndex, bool useSubmitTask)
    : descriptor(descriptor)
	, context(ScriptContext::getContext(thread))
	, function(thread, functionIndex)
	, executionDepth(0)
	, useSubmitTask(useSubmitTask)
	{
		if (useSubmitTask)
		{
			weakDm = weak_from(DataModel::get(&context));
		}
	}

	void setWeakSelf(weak_ptr<WrapperType> weakSelf)
	{
		this->weakSelf = weakSelf;
	}

	void operator()(const Reflection::EventArguments& arguments)
	{
		if (useSubmitTask)
		{
			if (shared_ptr<DataModel> dm = weakDm.lock())
			{
				dm->submitTask(boost::bind(&FunctionScriptSlot::safeDoEventFire,
					weakSelf, arguments /*copy arguments*/),
					DataModelJob::Write);
			}
		}
		else
		{
			doEventFire(arguments);
		}
	}

	static void safeDoEventFire(weak_ptr<WrapperType> weakSelf, const Reflection::EventArguments& arguments)
	{
		if (shared_ptr<WrapperType> self = weakSelf.lock())
		{
			self->slot.doEventFire(arguments);
		}
	}

	void doEventFire(const Reflection::EventArguments& arguments)
	{
		bool reentrant = executionDepth>0;
		RBX::ScopedAssign<int> assign(executionDepth, executionDepth+1);

		if (executionDepth>6)
		{
			// This should help against stack crashes...  Note that you could have a round-robin
			// re-entrancy problem. In that case the stack may contain 1 or 18 or more calls!
			StandardOut::singleton()->printf(MESSAGE_ERROR, "Maximum event re-entrancy depth exceeded for %s.%s", descriptor->owner.name.c_str(), descriptor->name.c_str());

			if (ThreadRef functionThread = function.lock())
			{
				lua_pushfunction(functionThread, function);

				lua_Debug ar;
				if (lua_getinfo(functionThread, ">S", &ar))
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "While entering function defined in script '%s', line %d", ar.short_src, ar.linedefined);
				}
			}

			return;
		}

		if (ThreadRef functionThread = function.lock())
		{
			//Count the number of invocations here per 'cycle'

			FASTLOG1(FLog::LuaBridge, "Executing lua from signal, execution depth: %u", executionDepth);

			// If this is a re-entrant call, then we can't use the cached thread
			ThreadRef slotThread;
			if(!reentrant)
				slotThread = cachedSlotThread.lock();

			{
				RBXASSERT_BALLANCED_LUA_STACK(functionThread);				//oldTop

				bool createdThread;
				if (slotThread)
					createdThread = false;
				else
				{
					// Create a child thread that will handle the event

					slotThread = lua_newthread(functionThread);

					RBXASSERT(lua_isthread(functionThread, -1));

					if (!slotThread)
						throw std::runtime_error("Unable to create a new event handler thread");
					createdThread = true;

					// Pop slotThread from the parent's stack
					RBXASSERT(lua_isthread(functionThread, -1));					// slotThread
					lua_pop(functionThread, 1);										// [empty]
				}
				                                               //createdThread ? (slotThread) : [empty]
				// Push the function onto the stack
				lua_pushfunction(slotThread, function);

				// Push arguments onto slotThread's stack
				const int nargs = LuaArguments::pushValues(arguments, slotThread);

				// resume slotThread
				const ScriptContext::Result result = context.resume(slotThread, nargs);

				switch (result)
				{
					case ScriptContext::Yield:
						// we can't re-use a yielding thread, since somebody else will resume it later
						if (!reentrant)
							cachedSlotThread.reset();		
						break;

					case ScriptContext::Error:
						// Don't re-use this thread
						if (!reentrant)
							cachedSlotThread.reset();

						break;
                        
                    default:
                        break;
				}

				if (function.empty())
				{
					// The thread that hosts our function has (or is being) terminated
					RBXASSERT(cachedSlotThread.empty());
					disconnect();					
					return;
				}

				if (createdThread)
				{
					if (!reentrant)
						if (result==ScriptContext::Success)
							// Since the slot didn't yield, we can re-use this thread the next time
							cachedSlotThread = WeakThreadRef(slotThread);
				}
			}
			RBXASSERT(slotThread);
			// balance the slot stack
			lua_resetstack(slotThread, 0);
		}
		else
		{
			// The thread that hosts our function has (or is being) terminated
			RBXASSERT(cachedSlotThread.empty());
			disconnect();
			return;
		}

	}
};

// This Slot is used when a script calls "wait" on a Signal
class WaitScriptSlot 
	: public BaseScript::Slot
{
	typedef Reflection::TGenericSlotWrapper<WaitScriptSlot> WrapperType;
	WeakThreadRef waitThread;
	bool useSubmitTask;
	weak_ptr<DataModel> weakDm;
	weak_ptr<WrapperType> weakSelf;
public:
	WaitScriptSlot(lua_State* thread, bool useSubmitTask)
		:waitThread(thread)
		,useSubmitTask(useSubmitTask)
	{
		if (useSubmitTask)
		{
			weakDm = weak_from(DataModel::get(&ScriptContext::getContext(thread)));
		}
	}

	void setWeakSelf(weak_ptr<WrapperType> weakSelf)
	{
		this->weakSelf = weakSelf;
	}

	void operator()(const Reflection::EventArguments& arguments)
	{
		// Waits are one-shots, so disconnect now
		disconnect();
		if (useSubmitTask)
		{
			if (shared_ptr<DataModel> dm = weakDm.lock())
			{
				dm->submitTask(boost::bind(&WaitScriptSlot::safeDoEventFire,
					weakSelf, arguments /*copy arguments*/),
					DataModelJob::Write);
			}
		}
		else
		{
			doEventFire(arguments);
		}
	}

	static void safeDoEventFire(weak_ptr<WrapperType> weakSelf, const Reflection::EventArguments& arguments)
	{
		if (shared_ptr<WrapperType> self = weakSelf.lock())
		{
			self->slot.doEventFire(arguments);
		}
	}

	void doEventFire(const Reflection::EventArguments& arguments)
	{
		if (ThreadRef thread = waitThread.lock())
		{
			// Push arguments onto slotThread's stack
			const int nargs = LuaArguments::pushValues(arguments, thread);

			// resume the waiting thread
			ScriptContext::getContext(thread).resume(thread, nargs);

			// After a resume the thread has either yielded, returned
			// or thrown an error. In all 3 cases we want to drop 
			// any return arguments
			lua_resetstack(thread, 0);
		}
	}
};

int EventBridge::connect(lua_State *L) 
{
	RobloxExtraSpace* space = RobloxExtraSpace::get(L);
	EventInstance& ei(getObject(L, 1));
	
	rbx::signals::connection connection;

	shared_ptr<Reflection::DescribedBase> source = ei.source.lock();
	if (source && ei.descriptor->isScriptable() && !space->context()->shouldPreventNewConnections())
	{
		if (ei.descriptor->security!=RBX::Security::None)
			RBX::Security::Context::current().requirePermission(ei.descriptor->security, ei.descriptor->name.c_str());

		Reflection::Variant varResult;
		Lua::LuaArguments::get(L, 2, varResult, false);
		if (!varResult.isType<Lua::WeakFunctionRef>())
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Attempt to connect failed: Passed value is not a function");
			ScriptContext::printCallStack(L);
		}

		bool useSubmitTask = DFFlag::UseSubmitTaskWhenFiringSignalsOnSettings && source->useSubmitTaskForLuaListeners();

		shared_ptr< RBX::Reflection::TGenericSlotWrapper<FunctionScriptSlot> > wrapper(
			rbx::make_shared< RBX::Reflection::TGenericSlotWrapper<FunctionScriptSlot> >(FunctionScriptSlot(ei.descriptor, L, 2, useSubmitTask))
		);
		if (useSubmitTask)
		{
			wrapper->slot.setWeakSelf(wrapper);
		}
		connection = ei.descriptor->connectGeneric(source.get(), wrapper);
		wrapper->slot.assignConnection(connection);
	}

	// Return a proxy to the connection object
	SignalConnectionBridge::pushNewObject(L, connection);
	return 1;
}

int EventBridge::wait(lua_State *L) 
{
	{
		RBXASSERT_BALLANCED_LUA_STACK(L);
		
		EventInstance& ei(getObject(L, 1));

		shared_ptr<Reflection::DescribedBase> source = ei.source.lock();
		if (source && ei.descriptor->isScriptable())
		{
			if (ei.descriptor->security!=RBX::Security::None)
				RBX::Security::Context::current().requirePermission(ei.descriptor->security, ei.descriptor->name.c_str());

			bool useSubmitTask = DFFlag::UseSubmitTaskWhenFiringSignalsOnSettings && source->useSubmitTaskForLuaListeners();

			shared_ptr<RBX::Reflection::TGenericSlotWrapper<WaitScriptSlot> > wrapper(
				rbx::make_shared< RBX::Reflection::TGenericSlotWrapper<WaitScriptSlot> >(WaitScriptSlot(L, useSubmitTask))
			);
			if (useSubmitTask)
			{
				wrapper->slot.setWeakSelf(wrapper);
			}
			wrapper->slot.assignConnection(ei.descriptor->connectGeneric(source.get(), wrapper));
		}

		RBXASSERT(!RobloxExtraSpace::get(L)->yieldCaptured);
		RobloxExtraSpace::get(L)->yieldCaptured = true;
	}

	return lua_yield(L, 0);
}

template<>
const char* Bridge< rbx::signals::connection >::className("RBXScriptConnection"); 		

template<>
int Bridge< rbx::signals::connection >::on_index(const rbx::signals::connection& object, const char* name, lua_State *L)
{
	// The pre-defined "disconnect()" method
	if (strcmp(name, "disconnect")==0)
	{
		lua_pushcfunction(L, SignalConnectionBridge::disconnect);
		return 1;
	}

	// The pre-defined "connected" property
	if (strcmp(name, "connected")==0)
	{
		lua_pushboolean(L, object.connected());
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge< rbx::signals::connection >::on_newindex(rbx::signals::connection& object, const char* name, lua_State *L)
{
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

int SignalConnectionBridge::disconnect(lua_State *L) {
	
	getObject(L, 1).disconnect();

	return 0;
}

		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge<EventInstance>::on_tostring(const EventInstance& object, lua_State *L)
		{
			std::string s = "Signal ";
			s += object.descriptor->name.toString();
			lua_pushstring(L, s);
			return 1;
		}
	}
}
