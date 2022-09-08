#include "stdafx.h"

#include "Script/ThreadRef.h"
#include "Script/LuaArguments.h"
#include "Script/ScriptContext.h"
#include "Script/script.h"
#include "lua/lua.hpp"
#include "lua/luabridge.h"
#include "rbxformat.h"
#include "reflection/property.h"
#include "util/standardout.h"
#include <rbx/make_shared.h>

LOGVARIABLE(ThreadRefCounts, 1)

using namespace RBX;
using namespace RBX::Lua;

WeakThreadRef::Mutex WeakThreadRef::sync;

WeakThreadRef::WeakThreadRef(lua_State* thread)
:node(0), previous(0), next(0)
{
	Mutex::scoped_lock lock(sync);

	// Insert myself into the Node list
	node = Node::get(thread);

	addToNode();
	addRef(thread);
}


WeakThreadRef::WeakThreadRef(const WeakThreadRef& other)
:node(other.node)
,liveThreadRef(other.liveThreadRef)
{
	Mutex::scoped_lock lock(sync);
	addToNode();
}

WeakThreadRef::~WeakThreadRef()
{
	reset();
}

void WeakThreadRef::reset() {
	Mutex::scoped_lock lock(sync);
	removeFromNode();
	removeRef();
}


WeakThreadRef& WeakThreadRef::operator=(const WeakThreadRef& other)
{
	liveThreadRef = other.liveThreadRef;

	if (node!=other.node)
	{
		Mutex::scoped_lock lock(sync);
		removeFromNode();
		node = other.node;
		addToNode();
	}
	return *this;
}

void WeakThreadRef::addToNode()
{
	if (node)
	{
		FASTLOG1(FLog::WeakThreadRef, "WeakThreadRef::addToNode() for node %p", node);

		WeakThreadRef* first = node->first;
		if (first)
		{
			this->next = first;
			first->previous = this;
		}
		else
			this->next = NULL;
		this->previous = NULL;
		node->first = this;
	}
}

void WeakThreadRef::removeFromNode()
{
	if (node)
	{
		FASTLOG1(FLog::WeakThreadRef, "WeakThreadRef::removeFromNode() for node %p", node);

		if (next)
			next->previous = previous;
		if (previous)
			previous->next = next;
		if (node->first==this)
			node->first = next;
		next = NULL;
		previous = NULL;
		node = NULL;
	}
}

void WeakThreadRef::addRef(lua_State* L)
{
	RBXASSERT(!liveThreadRef);
	// Add a Lua ref to the thread so that it doesn't get collected
	if (L)
	{
		liveThreadRef = new detail::LiveThreadRef(L);
		FASTLOG2(FLog::WeakThreadRef, "WeakThreadRef::addRef() %p for node %p", this, node);
	}
}

void WeakThreadRef::removeRef()
{
	FASTLOG2(FLog::WeakThreadRef, "WeakThreadRef::removeRef() %p for node %p", this, node);
	liveThreadRef.reset();
}

void WeakThreadRef::Node::eraseAllRefs()
{
	Mutex::scoped_lock lock(sync);
	for (WeakThreadRef* ref = first; ref!=NULL; ref = ref->next)
	{
		ref->removeRef();
		ref->node = NULL;
	}

	first = NULL;
}

WeakThreadRef::Node::~Node()
{
	FASTLOG1(FLog::WeakThreadRef, "WeakThreadRef::Node::~Node(), node = %p", this);

	eraseAllRefs();
}

template<>
const char* Bridge<boost::intrusive_ptr<WeakThreadRef::Node> >::className = "WeakThreadRef::Node";

boost::intrusive_ptr<WeakThreadRef::Node> WeakThreadRef::Node::create(lua_State* thread)
{
	RobloxExtraSpace* space = RobloxExtraSpace::get(thread);

	space->createNewNode();

	FASTLOG1(FLog::WeakThreadRef, "WeakThreadRef::Node::create node %p", space->getNode());

	return space->getNode();
}

WeakThreadRef::Node* WeakThreadRef::Node::get(lua_State* thread)
{
	return RobloxExtraSpace::get(thread)->getNode();
}

namespace RBX
{
	namespace Lua
	{
		template<>
		int Bridge<boost::intrusive_ptr<WeakThreadRef::Node> >::on_index(const boost::intrusive_ptr<WeakThreadRef::Node>& object, const char* name, lua_State *L)
		{
			// Failure
			throw RBX::runtime_error("%s is not a valid member", name);
		}

		template<>
		void Bridge<boost::intrusive_ptr<WeakThreadRef::Node> >::on_newindex(boost::intrusive_ptr<WeakThreadRef::Node>& object, const char* name, lua_State *L)
		{
			// Failure
			throw RBX::runtime_error("%s cannot be assigned to", name);
		}


		void dumpThreadRefCounts()
		{
			FASTLOG3(FLog::ThreadRefCounts, "ThreadRef=%i WeakThreadRef=%i detail::LiveThreadRef=%i", 
				ThreadRef::getCount(), 
				WeakThreadRef::getCount(), 
				detail::LiveThreadRef::getCount());
		}
	}
}



WeakFunctionRef::WeakFunctionRef(lua_State* thread, int functionIndex)
:WeakThreadRef(thread)
{
	// copies the desired object to the top of the stack
	// NOTE: This could be a function *or* perhaps a table with an "call" metamethod
	//       Therefore, we don't do any type checking here. Let the errors occur downstream.
	lua_pushvalue(thread, functionIndex);
	// pops the value from the stack, stores it into the registry with a fresh integer key, and returns that key
	functionId = luaL_ref(thread, LUA_REGISTRYINDEX);
}

namespace RBX
{
	namespace Lua
	{
		typedef Bridge< shared_ptr<GenericFunction> > GenericFunctionBridge;
		template<>
		const char* GenericFunctionBridge::className = "GenericFunction";

		template<>
		int GenericFunctionBridge::on_index(const shared_ptr<GenericFunction>& object, const char* name, lua_State *L)
		{
			// Failure
			throw RBX::runtime_error("%s is not a valid member", name);
		}

		template<>
		void GenericFunctionBridge::on_newindex(shared_ptr<GenericFunction>& object, const char* name, lua_State *L)
		{
			// Failure
			throw RBX::runtime_error("%s cannot be assigned to", name);
		}


		typedef Bridge< shared_ptr<GenericAsyncFunction> > GenericAsyncFunctionBridge;
		template<>
		const char* GenericAsyncFunctionBridge::className = "GenericAsyncFunction";

		template<>
		int GenericAsyncFunctionBridge::on_index(const shared_ptr<GenericAsyncFunction>& object, const char* name, lua_State *L)
		{
			// Failure
			throw RBX::runtime_error("%s is not a valid member", name);
		}

		template<>
		void GenericAsyncFunctionBridge::on_newindex(shared_ptr<GenericAsyncFunction>& object, const char* name, lua_State *L)
		{
			// Failure
			throw RBX::runtime_error("%s cannot be assigned to", name);
		}
	}
}

WeakFunctionRef Lua::lua_tofunction(lua_State* L, int index)
{
	return WeakFunctionRef(L, index);
}
void Lua::lua_pushfunction(lua_State* L, const WeakFunctionRef& function)
{
	// Push the function onto the stack
	lua_rawgeti(L, LUA_REGISTRYINDEX, function.functionId);
}

static int callGenericFunctionBridge (lua_State *L)
{
	shared_ptr<GenericFunction> f(GenericFunctionBridge::getObject(L, lua_upvalueindex(1)));

	shared_ptr<Reflection::Tuple> args = LuaArguments::getValues(L);

	shared_ptr<const Reflection::Tuple> result = (*f)(args);

	if (result)
		return LuaArguments::pushTuple(*result, L);
	else
		return 0;
}





void Lua::lua_pushfunction(lua_State* L, shared_ptr<GenericFunction> function)
{
	GenericFunctionBridge::pushNewObject(L, function);
	lua_pushcclosure(L, callGenericFunctionBridge, 1);
}

static void onAsyncResult(ThreadRef thread, weak_ptr<ScriptContext> context, IAsyncResult* result)
{
	// This function is the async handler for a GenericAsyncFunction
	// It could be called from any thread, so we have to be careful. 

	shared_ptr<const Reflection::Tuple> value;
	try
	{
		value = result->getValue();
	}
	catch (RBX::base_exception& e)
	{
		// TODO: How do we resume an exception in Lua?
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Exception caught from async call. %s", e.what());
		return;
	}

	if (shared_ptr<RBX::ScriptContext> lockedContext = context.lock())
		lockedContext->scheduleResume(thread, value);
}

static int callGenericAsyncFunctionBridge (lua_State *L) 
{
	// This function is being called from a Lua script

	shared_ptr<GenericAsyncFunction> f(GenericAsyncFunctionBridge::getObject(L, lua_upvalueindex(1)));

	shared_ptr<Reflection::Tuple> args = LuaArguments::getValues(L);

	(*f)(args, boost::bind(&onAsyncResult, ThreadRef(L), weak_from(RobloxExtraSpace::get(L)->context()), _1));

	RBXASSERT(!RobloxExtraSpace::get(L)->yieldCaptured);
	RobloxExtraSpace::get(L)->yieldCaptured = true;

	return lua_yield(L, 0);
}


void Lua::lua_pushfunction(lua_State* L, shared_ptr<GenericAsyncFunction> function)
{
	GenericAsyncFunctionBridge::pushNewObject(L, function);
	lua_pushcclosure(L, callGenericAsyncFunctionBridge, 1);
}


WeakFunctionRef::~WeakFunctionRef()
{
	// TODO: Crash? Are use guaranteed to be in a safe datamodel thread?
	// remove the reference to the function
	if (functionId!=0 && !empty())
    {
		luaL_unref(ScriptContext::getGlobalState(thread()), LUA_REGISTRYINDEX, functionId);
    }
}

void WeakFunctionRef::removeRef()
{
	if (functionId!=0 && !empty())
	{
		FASTLOG1(FLog::WeakThreadRef, " WeakFunctionRef::removeRef() for node %p", node);

		luaL_unref(ScriptContext::getGlobalState(thread()), LUA_REGISTRYINDEX, functionId);

		functionId = 0;
	}
	Super::removeRef();
}


WeakFunctionRef::WeakFunctionRef(const WeakFunctionRef& other)
:WeakThreadRef(other)
{
	if (other.empty())
		functionId = 0;
	else
	{
		lua_State* L = ScriptContext::getGlobalState(thread());
		lua_rawgeti(L, LUA_REGISTRYINDEX, other.functionId);
		// pops the value from the stack, stores it into the registry with a fresh integer key, and returns that key
		functionId = luaL_ref(L, LUA_REGISTRYINDEX);
	}
}

RBX::Lua::detail::LiveThreadRef::LiveThreadRef (lua_State* thread)
:L(thread)
{
	RBXASSERT(L);

	// copies the current thread onto the stack
	lua_pushthread(L);
	// pops the value from the stack, stores it into the registry with a fresh integer key, and returns that key
	threadId = luaL_ref(L, LUA_REGISTRYINDEX);
}

RBX::Lua::detail::LiveThreadRef::~LiveThreadRef()
{
	luaL_unref(ScriptContext::getGlobalState(L), LUA_REGISTRYINDEX, threadId);
}

bool WeakThreadRef::operator==(const WeakThreadRef& other) const
{
	return (thread()==other.thread());
}

bool WeakFunctionRef::operator==(const WeakFunctionRef& other) const
{
	if (functionId!=other.functionId)
		return false;
	return *((WeakThreadRef*)this)==*((WeakThreadRef*)&other);
}

bool WeakFunctionRef::operator!=(const WeakFunctionRef& other) const
{
	return !(*this==other);
}


WeakFunctionRef& WeakFunctionRef::operator=(const WeakFunctionRef& other)
{
	if (!empty())
		luaL_unref(ScriptContext::getGlobalState(thread()), LUA_REGISTRYINDEX, functionId);

	// Call inherited operator
	WeakThreadRef& t = *this;
	t = other;

	if (other.empty())
		functionId = 0;
	else
	{
		lua_State* L = ScriptContext::getGlobalState(thread());
		lua_rawgeti(L, LUA_REGISTRYINDEX, other.functionId);
		// pops the value from the stack, stores it into the registry with a fresh integer key, and returns that key
		functionId = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	return *this;
}

namespace RBX
{
	namespace Reflection
	{
		template<>
		const Type& Type::getSingleton<RBX::Lua::WeakFunctionRef>()
		{
			static TType<RBX::Lua::WeakFunctionRef> type("Function");
			return type;
		}

		template<>
		RBX::Lua::WeakFunctionRef& Variant::convert<RBX::Lua::WeakFunctionRef>(void)
		{
			// Interpret "void" (Lua nil) as a null function
			// TODO: Is that correct behavior????
			if (isType<void>())
			{
				value = RBX::Lua::WeakFunctionRef();
				_type = &Type::singleton<RBX::Lua::WeakFunctionRef>();
			}

			RBX::Lua::WeakFunctionRef* val = tryCast<RBX::Lua::WeakFunctionRef>();
			if (val==NULL)
				throw std::runtime_error("Unable to cast value to function");
			return *val;
		}

		template<>
		const Type& Type::getSingleton<shared_ptr<GenericFunction> >()
		{
			static TType<shared_ptr<GenericFunction> > type("GenericFunction");
			return type;
		}

		template<>
		shared_ptr<GenericFunction>& Variant::convert<shared_ptr<GenericFunction> >(void)
		{
			shared_ptr<GenericFunction>* val = tryCast<shared_ptr<GenericFunction> >();
			if (val == NULL)
				throw std::runtime_error("The value is not a function");
			return *val;
		}

		template<>
		const Type& Type::getSingleton<shared_ptr<GenericAsyncFunction> >()
		{
			static TType<shared_ptr<GenericAsyncFunction> > type("GenericAsyncFunction");
			return type;
		}

		template<>
		shared_ptr<GenericAsyncFunction>& Variant::convert<shared_ptr<GenericAsyncFunction> >(void)
		{
			shared_ptr<GenericAsyncFunction>* val = tryCast<shared_ptr<GenericAsyncFunction> >();
			if (val == NULL)
				throw std::runtime_error("The value is not a function");
			return *val;
		}
	}
}

