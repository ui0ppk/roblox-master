#include "stdafx.h"
#include "script/LuaVM.h"

#include "util/Guid.h"
#include "util/ProtectedString.h"

#include "util/MD5Hasher.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/HackDefines.h"

#define LUAVM_DESERIALIZER
#include "LuaSerializer.inl"

struct CoreScriptBytecode 
{ 
    const char* name; 
    const unsigned char* value; 
    size_t dataSize; 
};

#include "LuaGenCS.inl"

// Dummy implementation of Lua parser (referenced by Lua sources)
Proto *luaY_parser(lua_State *L, ZIO *z, Mbuffer *buff, const char *name)
{
    lua_pushstring(L, "");
    luaD_throw(L, LUA_ERRSYNTAX);

    return NULL;
}

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

    int load(lua_State* L, const RBX::ProtectedString& source, const char* chunkname, unsigned int modkey)
    {
        try
        {
            return LuaDeserializer::deserialize(L, source.getBytecode(), chunkname, modkey);
        }
        catch (std::bad_alloc&)
        {
            return LuaDeserializer::deserializeFailure(L, chunkname);
        }
    }
    
    unsigned int getKey()
    {
        // This is an initial value, it will be corrected by the server via SET_GLOBALS packet
        return LUAVM_KEY_DUMMY;
    }

    std::string compileCore(const std::string& source)
    {
        return "";
    }

    unsigned int getKeyCore()
    {
        return LUAVM_INTERNAL_CORE_DECODE_KEY;
    }

    unsigned int getModKeyCore()
    {
        // This is an initial value, it will be corrected by the server via SET_GLOBALS packet
        return LUAVM_MODKEY_DUMMY;
    }

    bool useSecureReplication()
    {
        return true;
    }

    bool canCompileScripts()
    {
        return false;
    }

	std::string getBytecodeCore(const std::string& name)
    {
        std::string rotName = RBX::rot13(name);
        for (int i = 0; i < sizeof(gCoreScripts)/sizeof(gCoreScripts[0]); i++)
            if (gCoreScripts[i].name == rotName)
                return std::string(reinterpret_cast<const char*>(gCoreScripts[i].value), gCoreScripts[i].dataSize);

        return "";
    }

	boost::unordered_map<std::string, std::string> getBytecodeCoreModules()
	{
		boost::unordered_map<std::string, std::string> coreModuleScripts;

		for (int i = 0; i < sizeof(gCoreModuleScripts)/sizeof(gCoreModuleScripts[0]); i++)
		{
			coreModuleScripts[gCoreModuleScripts[i].name] = std::string(reinterpret_cast<const char*>(gCoreModuleScripts[i].value), gCoreModuleScripts[i].dataSize);
		}

		return coreModuleScripts;
	}

    unsigned int rbxOldEncode(unsigned int i, int pc, unsigned int key)
    {
        return i;
    } 

    unsigned int rbxDaxEncode(unsigned int i, int pc, unsigned int key) 
    {
        return i;
    }
}
