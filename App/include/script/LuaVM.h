#pragma once

#if (defined(_WIN32) || (defined(__APPLE__) && !defined(RBX_PLATFORM_IOS))) && !defined(RBX_STUDIO_BUILD)
#define RBX_SECURE_DOUBLE
#endif

#ifdef _WIN32
#define RBX_ALIGN(s) _declspec(align(s)) 
#else
#define RBX_ALIGN(s) __attribute__((__aligned__(s))) 
#endif

#include <boost/unordered_map.hpp>
#include <string>
#if defined(RBX_SECURE_DOUBLE)
#include <emmintrin.h>
#endif

#ifndef RBX_STUDIO_BUILD
#define LUAVM_SECURE
#endif

// Utilities for shuffling fields and enum values
#define LUAVM_SHUFFLE_COMMA ,

#ifdef LUAVM_SECURE
#define LUAVM_SHUFFLE2(sep,a0,a1) a1 sep a0
#define LUAVM_SHUFFLE3(sep,a0,a1,a2) a1 sep a2 sep a0
#define LUAVM_SHUFFLE4(sep,a0,a1,a2,a3) a3 sep a1 sep a0 sep a2
#define LUAVM_SHUFFLE5(sep,a0,a1,a2,a3,a4) a4 sep a0 sep a2 sep a1 sep a3
#define LUAVM_SHUFFLE6(sep,a0,a1,a2,a3,a4,a5) a3 sep a5 sep a2 sep a0 sep a1 sep a4
#define LUAVM_SHUFFLE7(sep,a0,a1,a2,a3,a4,a5,a6) a2 sep a3 sep a0 sep a4 sep a6 sep a1 sep a5
#define LUAVM_SHUFFLE8(sep,a0,a1,a2,a3,a4,a5,a6,a7) a7 sep a0 sep a5 sep a6 sep a3 sep a1 sep a2 sep a4
#define LUAVM_SHUFFLE9(sep,a0,a1,a2,a3,a4,a5,a6,a7,a8) a2 sep a6 sep a4 sep a7 sep a1 sep a8 sep a0 sep a3 sep a5
#else
#define LUAVM_SHUFFLE2(sep,a0,a1) a0 sep a1
#define LUAVM_SHUFFLE3(sep,a0,a1,a2) a0 sep a1 sep a2
#define LUAVM_SHUFFLE4(sep,a0,a1,a2,a3) a0 sep a1 sep a2 sep a3
#define LUAVM_SHUFFLE5(sep,a0,a1,a2,a3,a4) a0 sep a1 sep a2 sep a3 sep a4
#define LUAVM_SHUFFLE6(sep,a0,a1,a2,a3,a4,a5) a0 sep a1 sep a2 sep a3 sep a4 sep a5
#define LUAVM_SHUFFLE7(sep,a0,a1,a2,a3,a4,a5,a6) a0 sep a1 sep a2 sep a3 sep a4 sep a5 sep a6
#define LUAVM_SHUFFLE8(sep,a0,a1,a2,a3,a4,a5,a6,a7) a0 sep a1 sep a2 sep a3 sep a4 sep a5 sep a6 sep a7
#define LUAVM_SHUFFLE9(sep,a0,a1,a2,a3,a4,a5,a6,a7,a8) a0 sep a1 sep a2 sep a3 sep a4 sep a5 sep a6 sep a7 sep a8
#endif

// Utility class for obfuscating fields of primitive types
// WARNING: this will give incorrect results if T = float.
template <typename T> class LuaVMValue
{
public:
    operator const T() const
    {
    #ifdef LUAVM_SECURE
        return (T)((uintptr_t)storage + reinterpret_cast<uintptr_t>(this));
    #else
        return storage;
    #endif
    }
    
    void operator=(const T& value)
    {
    #ifdef LUAVM_SECURE
        storage = (T)((uintptr_t)value - reinterpret_cast<uintptr_t>(this));
    #else
        storage = value;
    #endif
    }
    
    const T operator->() const
    {
        return operator const T();
    }
    
private:
    T storage;
};

// Encoding/decoding lineinfo
#if defined(LUAVM_SECURE)
#define LUAVM_ENCODELINE(line, pc) ((line) ^ ((pc) << 8))
#define LUAVM_DECODELINE(line, pc) ((line) ^ ((pc) << 8))
#else
#define LUAVM_ENCODELINE(line, pc) (line)
#define LUAVM_DECODELINE(line, pc) (line)
#endif

// Encoding/decoding instructions
#if defined(LUAVM_SECURE)
#define LUAVM_ENCODEINSN(insn, key) ((insn) * key)
#define LUAVM_DECODEINSN(insn, key) ((insn).v * key)
#else
#define LUAVM_ENCODEINSN(insn, key) (insn)
#define LUAVM_DECODEINSN(insn, key) (insn).v
#endif

typedef unsigned int (*RbxOpEncoder)(unsigned int i, int pc, unsigned key);

// Utility class
struct lua_State;

namespace RBX
{
    class ProtectedString;
}

// Core scripts have a fixed key
// Don't use these except in LuaVM*.cpp!
// These are defines to make sure they don't end up in an executable by complete accident
#define LUAVM_INTERNAL_CORE_ENCODE_KEY 641
#define LUAVM_INTERNAL_CORE_DECODE_KEY 6700417

// Constants for key values
#define LUAVM_KEY_DUMMY 1
#define LUAVM_KEY_INVALID 0
#define LUAVM_MODKEY_DUMMY 1

namespace LuaVM
{
    // Utilities for working with regular scripts
    std::string compile(const std::string& source);

    std::string compileLegacy(const std::string& source);

    int load(lua_State* L, const RBX::ProtectedString& source, const char* chunkname, unsigned int modkey = 1);
    
    unsigned int getKey();

    // Utilities for working with core scripts
    std::string compileCore(const std::string& source);

    unsigned int getKeyCore();
    unsigned int getModKeyCore();

    // Controls whether replication uses bytecode or source code
    bool useSecureReplication();

    // Controls whether scripts can be compiled from source code
    bool canCompileScripts();

    // Gets embedded bytecode for core scripts/libraries
	std::string getBytecodeCore(const std::string& name);

	//const ref
	boost::unordered_map<std::string, std::string> getBytecodeCoreModules();

    // Old Encoding Scheme
    unsigned int rbxOldEncode(unsigned int i, int pc, unsigned int key);

    // Dual-Affine-Xor Encoding
    unsigned int rbxDaxEncode(unsigned int i, int pc, unsigned int key);

}


#if defined(RBX_SECURE_DOUBLE)
// Note that users who can find a value can still change magnitude or sign easily.

// sse2+ only
class LuaSecureDouble
{
private:
    double storage;
public:
    static RBX_ALIGN(16) int luaXorMask[4];

    operator const double() const
    {
        __m128d xmmKey = _mm_load_pd((double*)(luaXorMask));
        __m128d xmmData = _mm_load_sd(&storage);
        __m128d xmmResult = _mm_xor_pd(xmmData, xmmKey );
        return _mm_cvtsd_f64(xmmResult);
    }

    void operator=(const double& value)
    {
        __m128d xmmKey = _mm_load_pd((double*)(luaXorMask));
        __m128d xmmData = _mm_load_sd(&value);
        __m128d xmmResult = _mm_xor_pd(xmmData, xmmKey );
        storage = _mm_cvtsd_f64(xmmResult);
    }

    static void initDouble();

};
#endif