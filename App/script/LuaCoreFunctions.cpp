#include "stdafx.h"

#include "script/LuaCoreFunctions.h"
#include "Script/ScriptContext.h"

#include <time.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lobject.h"


namespace LuaOsExtension
{
//////////////////////////////////////////////////////////////////////
// BEGIN copy/paste from loslib.c

/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static int getboolfield (lua_State *L, const char *key) {
  int res;
  lua_getfield(L, -1, key);
  res = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);
  lua_pop(L, 1);
  return res;
}


static int getfield (lua_State *L, const char *key, int d) {
  int res;
  lua_getfield(L, -1, key);
  if (lua_isnumber(L, -1))
    res = (int)lua_tointeger(L, -1);
  else {
    if (d < 0)
      return luaL_error(L, "field " LUA_QS " missing in date table", key);
    res = d;
  }
  lua_pop(L, 1);
  return res;
}


static int os_time (lua_State *L) {
  time_t t;
  if (lua_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0);
    ts.tm_min = getfield(L, "min", 0);
    ts.tm_hour = getfield(L, "hour", 12);
    ts.tm_mday = getfield(L, "day", -1);
    ts.tm_mon = getfield(L, "month", -1) - 1;
    ts.tm_year = getfield(L, "year", -1) - 1900;
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
  }
  if (t == (time_t)(-1))
    lua_pushnil(L);
  else
    lua_pushnumber(L, (lua_Number)t);
  return 1;
}


static int os_difftime (lua_State *L) {
  lua_pushnumber(L, difftime((time_t)(luaL_checknumber(L, 1)),
                             (time_t)(luaL_optnumber(L, 2, 0))));
  return 1;
}

///////////////////////////////////////////////////////////////////////////////////
// END copy/paste from loslib.c

const luaL_Reg registry[] = 
{
	{"difftime",  os_difftime},
	{"time",      os_time},
	{NULL, NULL}
};

}

namespace LuaMathExtension
{
	static const unsigned char kPerlin[512] = {
		151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
		140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
		247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
		57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
		74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
		60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
		65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
		200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
		52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
		207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
		119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
		129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
		218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
		81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
		184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
		222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180,

		151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
		140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
		247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
		57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
		74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
		60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
		65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
		200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
		52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
		207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
		119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
		129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
		218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
		81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
		184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
		222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180,
	};

	static float fade(float t)
	{
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	static float lerp(float t, float a, float b)
	{
		return a + t * (b - a);
	}

	static float grad(unsigned char hash, float x, float y, float z)
	{
		unsigned char h = hash & 15;
		float u = (h < 8) ? x : y;
		float v = (h < 4) ? y : (h == 12 || h == 14) ? x : z;

		return (h & 1 ? -u : u) + (h & 2 ? -v : v);
	}

	static float perlin(float x, float y, float z)
	{
		float xflr = floorf(x);
		float yflr = floorf(y);
		float zflr = floorf(z);

		int xi = int(xflr) & 255;
		int yi = int(yflr) & 255;
		int zi = int(zflr) & 255;

		float xf = x - xflr;
		float yf = y - yflr;
		float zf = z - zflr;

		float u = fade(xf);
		float v = fade(yf);
		float w = fade(zf);

		const unsigned char* p = kPerlin;

		int a = p[xi] + yi;
		int aa = p[a] + zi;
		int ab = p[a+1] + zi;

		int b = p[xi+1] + yi;
		int ba = p[b] + zi;
		int bb = p[b+1] + zi;

		return
			lerp(w, lerp(v, lerp(u, grad(p[aa  ], xf  , yf  , zf   ),
									grad(p[ba  ], xf-1, yf  , zf   )),
							lerp(u, grad(p[ab  ], xf  , yf-1, zf   ),
									grad(p[bb  ], xf-1, yf-1, zf   ))),
					lerp(v, lerp(u, grad(p[aa+1], xf  , yf  , zf-1 ),
									grad(p[ba+1], xf-1, yf  , zf-1 )),
							lerp(u, grad(p[ab+1], xf  , yf-1, zf-1 ),
									grad(p[bb+1], xf-1, yf-1, zf-1 ))));
	}

	int noise(lua_State* L)
	{
		double x = luaL_checknumber(L, 1);
		double y = luaL_optnumber(L, 2, 0.0);
		double z = luaL_optnumber(L, 3, 0.0);

		double r = perlin(x, y, z);

		lua_pushnumber(L, r);

		return 1;
	}
}

namespace LuaDebugExtension
{
	
static int debug_traceback(lua_State *L)
{
	std::string callStack;

	RBX::ScriptContext::printCallStack(L, &callStack, true);
	lua_pushstring(L, callStack.c_str());
	return 1;
}

const luaL_Reg registry[] = 
{
	{"traceback",  debug_traceback},
	{NULL, NULL}
};

}
