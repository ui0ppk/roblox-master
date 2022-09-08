#include "stdafx.h"
#include "script/LuaVM.h"

#include "util/ProtectedString.h"

#define LUAVM_COMPILER
#include "../Lua-5.1.4/src/lcode.c"
#include "../Lua-5.1.4/src/lparser.c"

#include "../Lua-5.1.4/src/lauxlib.h"

#include "../Lua-5.1.4/src/lopcodes.h"

struct LoadS
{
    const char *s;
    size_t size;
};

static const char* getS(lua_State *L, void *ud, size_t *size)
{
    LoadS *ls = (LoadS *)ud;
    (void)L;
    if (ls->size == 0) return NULL;
    *size = ls->size;
    ls->size = 0;
    return ls->s;
}

#ifndef RBX_STUDIO_BUILD
static uint32_t rbxDaxEncodeOp(uint32_t x, uint32_t mulEven, uint32_t addEven, uint32_t mulOdd, uint32_t addOdd)
{
    uint32_t result      = 0;
    uint32_t mask        = 1;
    for (size_t i = 0; i < 8*sizeof(uint32_t); ++i)
    {
        uint32_t bitDesired = mask & x;
        uint32_t bitOdd     = mask & (result*mulOdd + addOdd);
        uint32_t bitEven    = mask & (result*mulEven + addEven);
        if ((bitEven ^ bitOdd) != bitDesired)
        {
            result |= mask;
        }
        mask <<= 1;
    }
    return result;
}
#endif

namespace LuaVM
{
    std::string compile(const std::string& source)
    {
        return "";
    }

    std::string compileLegacy(const std::string& source)
    {
        return "";
    }

    // This function is here to allow the unit tests to work.  
    // The unit tests are treated as desktop, but don't serialize the code.
    //
    // Server/Mobile runs unobfuscated code.
    // 
    // Desktop runs obfuscated code that was serialized
    //
    // Studio runs/compiles unobfuscated code.
    static void finalize(Proto* p, RbxOpEncoder encode, unsigned int ckey)
    {
        #ifndef RBX_STUDIO_BUILD
        lua_assert(ckey);

        // encode bytecode
        for (int i = 0; i < p->sizecode; ++i)
            p->code[i].v = encode(p->code[i].v, i, ckey);

        // lineinfo was previously encoded.

        for (int i = 0; i < p->sizep; ++i)
            finalize(p->p[i], encode, ckey);

        #endif
    }

    int load(lua_State* L, const RBX::ProtectedString& source, const char* chunkname, unsigned int modkey)
    {
        const std::string& code = source.getSource();
        
        LoadS ls = { code.c_str(), code.size() };
        
        int err = lua_load(L, getS, &ls, chunkname);

        if (err == 0)
        {
            const LClosure* cl = static_cast<const LClosure*>(lua_topointer(L, -1));
            finalize(cl->p, rbxDaxEncode, modkey);
        }

        return err;
    }
    
    unsigned int getKey()
    {
        return LUAVM_KEY_DUMMY;
    }

    std::string compileCore(const std::string& source)
    {
        return "";
    }

    unsigned int getKeyCore()
    {
        return LUAVM_KEY_DUMMY;
    }

    unsigned int getModKeyCore()
    {
        return LUAVM_MODKEY_DUMMY;
    }

    bool useSecureReplication()
    {
        return false;
    }

    bool canCompileScripts()
    {
        return true;
    }

	std::string getBytecodeCore(const std::string& name)
    {
        return "";
    }

	boost::unordered_map<std::string, std::string> getBytecodeCoreModules()
	{
		return boost::unordered_map<std::string, std::string>();
	}

    unsigned int rbxOldEncode(unsigned int i, int pc, unsigned int key)
    {
        return i;
    } 

    // code duplication...
    unsigned int rbxDaxEncode(unsigned int i, int pc, unsigned int key) 
    {
#ifndef RBX_STUDIO_BUILD
        Instruction enc = i;
        Instruction op = GET_OPCODE(i);
        switch (op) {
        case OP_CALL:
        case OP_TAILCALL:
        case OP_RETURN:
        case OP_CLOSURE:
            enc = rbxDaxEncodeOp(i, LUAVM_DAX_ME, pc, LUAVM_DAX_MO, LUAVM_DAX_AO);
            SET_OPCODE(enc, op);
            break;
        case OP_MOVE:
            SETARG_C(enc, (pc|1)); // non-zero
            break;
        default:
            break;
        }
        return LUAVM_ENCODEINSN(enc, key);
#else
        return i;
#endif
    }
}
