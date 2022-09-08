#pragma once

#include "lauxlib.h"

namespace LuaOsExtension
{
	extern const luaL_Reg registry[];
}

namespace LuaMathExtension
{
	int noise(lua_State* L);
}

namespace LuaDebugExtension
{
	extern const luaL_Reg registry[];
}

