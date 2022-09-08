#include "stdafx.h"
#include "script/LuaVM.h"

#include "util/Guid.h"
#include "util/ProtectedString.h"

#define LUAVM_COMPILER
#include "../Lua-5.1.4/src/lcode.c"
#include "../Lua-5.1.4/src/lparser.c"

#define LUAVM_SERIALIZER
#include "LuaSerializer.inl"

static long long multiplicativeInverse(long long a, long long n)
{
    long long t = 0;
    long long newt = 1;
    
    long long r = n;
    long long newr = a;
    
    while (newr != 0)
    {
        long long q = r / newr;
        
        long long curt = t;
        t = newt;
        newt = curt - q * newt;
        
        long long curr = r;
        r = newr;
        newr = curr - q * newr;
    }
    
    RBXASSERT(r == 1);
    
    return (t < 0) ? t + n : t;
}

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

static std::pair<unsigned int, unsigned int> createLuaKeyPair()
{
    // encode key has to be a sufficiently random odd integer
    std::string guid;
    unsigned int encode = 1;
    do
    {
        RBX::Guid::generateStandardGUID(guid);
        encode = boost::hash_value(guid) * 2 + 1;
    } while (encode == 1);
    
    // decode key has to be a multiplicative inverse mod 2^32
    // note that the inverse has to exist because the encode key is odd
    // encode * u + 2^32 * v = 1
    unsigned int decode = multiplicativeInverse(encode, 1ll << 32);
    
    RBXASSERT(encode * decode == 1);

    return std::make_pair(encode, decode);
}

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

namespace LuaVM
{
    static std::pair<unsigned int, unsigned int> gLuaKeyPair = createLuaKeyPair();
    
    std::string compile(const std::string& source)
    {
        lua_State* L = luaL_newstate();
        
        std::string result = LuaSerializer::serialize(L, source, rbxDaxEncode, gLuaKeyPair.first);

        lua_close(L);
        
        return result;
    }

    std::string compileLegacy(const std::string& source)
    {
        lua_State* L = luaL_newstate();
        
        std::string result = LuaSerializer::serialize(L, source, rbxOldEncode, gLuaKeyPair.first);

        lua_close(L);
        
        return result;
    }

    int load(lua_State* L, const RBX::ProtectedString& source, const char* chunkname, unsigned int modkey)
    {
        const std::string& code = source.getSource();
            
        LoadS ls = { code.c_str(), code.size() };
            
        return lua_load(L, getS, &ls, chunkname);
    }
    
    unsigned int getKey()
    {
        return gLuaKeyPair.second;
    }

    std::string compileCore(const std::string& source)
    {
        lua_State* L = luaL_newstate();
        
        std::string result = LuaSerializer::serialize(L, source, rbxDaxEncode, LUAVM_INTERNAL_CORE_ENCODE_KEY);

        lua_close(L);
        
        return result;
    }

    unsigned int getKeyCore()
    {
		return gLuaKeyPair.second;
    }

    unsigned int getModKeyCore()
    {
        return LUAVM_INTERNAL_CORE_DECODE_KEY * gLuaKeyPair.first;
    }

    bool useSecureReplication()
    {
        return true;
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
        (void)(pc);
        return LUAVM_ENCODEINSN(i, key);
    } 

    unsigned int rbxDaxEncode(unsigned int i, int pc, unsigned int key) 
    {
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
    }

}
