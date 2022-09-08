#pragma once

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace RBX
{
	namespace Lua
	{
		extern const char* safe_lua_tostring(lua_State *L, int idx);
		extern const char* throwable_lua_tostring(lua_State *L, int idx);
		extern float lua_tofloat(lua_State *L, int idx);
		extern void protect_metatable(lua_State* thread, int index);
		inline void lua_pushstring(lua_State* thread, const std::string& s)
		{
			lua_pushlstring(thread, s.c_str(), s.size());
		}

        const char* lua_checkstring_secure(lua_State* L, int idx);

        void lua_resetstack(lua_State* L, int idx);

		// Pops items from the stack when it goes out of scope
		class ScopedPopper
		{
			int popCount;
			lua_State* const thread;
		public:
			ScopedPopper(lua_State* thread, int popCount)
				:thread(thread),popCount(popCount) 
			{}

			ScopedPopper& operator +=(int popCount)
			{
				this->popCount += popCount;
				return *this;
			}

			ScopedPopper& operator -=(int popCount)
			{
				this->popCount -= popCount;
				return *this;
			}

			~ScopedPopper()
			{
				lua_pop(thread, popCount);
			}
		};

		class ScopedState
		{
			lua_State* const thread;
		public:
			ScopedState()
				:thread(luaL_newstate()) 
			{}

			~ScopedState()
			{
				lua_close(thread);
			}

			operator lua_State*()
			{
				return thread;
			}
		};

	}

}
