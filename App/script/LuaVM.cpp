#include "stdafx.h"
#include "script/LuaVM.h"
#include "v8datamodel/HackDefines.h"
#include "v8datamodel/DataModel.h"
#include "security/FuzzyTokens.h"
#include "security/ApiSecurity.h"

#include "script/ScriptContext.h"
#include "network/api.h"

#include "luaconf.h"

#include "VMProtect/VMProtectSDK.h"

struct lua_State;

// If you are getting to this point in a debugger, you probably added a lua_*
// call to one of the VM protected sections.
void lua_vmhooked_handler(lua_State* L) {
    VMProtectBeginVirtualization(NULL);
    RBX::Tokens::sendStatsToken.addFlagFast(HATE_LUA_VM_HOOKED);
    RBX::DataModel* dm = RBX::DataModel::get(RobloxExtraSpace::get(L)->context());
    if (dm)
    {
        dm->addHackFlag(HATE_LUA_VM_HOOKED);
    }
    VMProtectEnd();
};

#ifdef RBX_SECURE_DOUBLE
RBX_ALIGN(16) int LuaSecureDouble::luaXorMask[4];

// For lua, create a random bitstring to use for doubles.
// Mainly care about the exponent bits.  This ensures at least one of the top 7 exp
// bits gets changed.
void LuaSecureDouble::initDouble()
{
    uint32_t luaRandMask;
    do
    {
        luaRandMask = (rand() << 16) | rand();
    } while ((luaRandMask & 0x7F000000) == 0);

    for (int i = 0; i < sizeof(LuaSecureDouble::luaXorMask)/sizeof(int); ++i)
    {
        LuaSecureDouble::luaXorMask[i] = luaRandMask;
    }
}
#endif

#ifdef _WIN32
// used for the patching process.  this is the .text section, 
// fallback defaults are to start at 0x00400000 and be 20MB
namespace RBX { namespace Security {
    // All of the .text
    volatile const uintptr_t rbxTextBase = 0x00400000;
    volatile const size_t rbxTextSize = 0xFFFFFFFF;
}
}
#endif
