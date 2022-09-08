#pragma once 
#include <stdint.h>
#include "Security/FuzzyTokens.h"
#include "Security/RandomConstant.h"

#if defined(RBX_PLATFORM_DURANGO)
#define NOINLINE __declspec(noinline)
#elif defined(_WIN32)
#include <windows.h>
#include <winternl.h>
#undef min

#define FORCEINLINE __forceinline
#define NOINLINE __declspec(noinline)

// All of the .text
namespace RBX{ namespace Security {
    extern volatile const uintptr_t rbxTextBase;
    extern volatile const size_t rbxTextSize;
    extern volatile const uintptr_t rbxTextEndNeg;
    extern volatile const size_t rbxTextSizeNeg;
    extern volatile const uintptr_t rbxVmpBase;
    extern volatile const size_t rbxVmpSize;
}
}


#else
#define FORCEINLINE inline __attribute__((always_inline)) 
#define NOINLINE __attribute__((noinline)) 
#endif


// This file defines some secure caller function to prevent key functions
// from being called from a dll.
//
// For many functions, this is actually intractable.  With just a return address
// check, someone can simply push the location of a "C3" byte (ret) in our code
// and then jump to our function.  The return address will appear to be in our code.
//
// When the check becomes stronger, it is a simple matter to find the target
// function called from another function in our code.  From this point it is easy
// to call (into) that function.  Even if the result isn't directly returned, it
// will typically be on the stack.
// 
// callbacks are problematic.  On one hand, we can check callbacks before setting
// and before calling them.  However, callbacks could be set to "CC" bytes (int 3) in
// our code, which would throw an exception that could be handled.
//
// Secure calling in this file almost certainly will not work within a VM protected
// section as the registers and stack will be nonstandard.
//
// It will be trivial to find where these checks are done as well, as they all
// modify the same global values.

namespace RBX 
{
FORCEINLINE static bool isRbxTextAddr(const void* const ptr)
{
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_STUDIO_BUILD)
    return (reinterpret_cast<uintptr_t>(ptr) - RBX::Security::rbxTextBase < RBX::Security::rbxTextSize);
#else
    return true;
#endif
}

template<unsigned int value> FORCEINLINE void callCheckSetBasicFlag(unsigned int flags)
{
    Tokens::sendStatsToken.addFlagFast(value);
    Tokens::simpleToken |= value;
}

// 33222222222 2   1  111  11111100  000 0     0       000
// 10987654321 0   9  876  54321098  765 4     3       210
// rsvd        cpy pi veh  ntapi         cs    locked  name
static const unsigned int kNameApiOffset = 0;
static const unsigned int kRbxLockedApiOffset = 3;
static const unsigned int kChangeStateApiOffset = 4;
static const unsigned int kNtApiNoNtdll = (1<<8);
static const unsigned int kNtApiNoText = (1<<9);
static const unsigned int kNtApiNoApi = (1<<10);
static const unsigned int kNtApiNoSyscall = (1<<11);
static const unsigned int kNtApiNoTemplate = (1<<12);
static const unsigned int kNtApiEarly = (1<<13);
static const unsigned int kNtApiHash = (1<<14);
static const unsigned int kNtApiNoCall = (1<<15);
static const unsigned int kVehWpmFail = (1<<16);
static const unsigned int kVehPrologFail = (1<<17);
static const unsigned int kVehNoNtdll = (1<<18);
static const unsigned int kPingItem = (1<<19);
static const unsigned int kScriptContextCopy = (1<<20);
static const unsigned int kLuaHooked = (1<<21);


template<unsigned int offset> FORCEINLINE void callCheckSetApiFlag(unsigned int flags)
{
    Tokens::apiToken.addFlagSafe(flags << offset);
}

FORCEINLINE void callCheckNop(unsigned int flags)
{
}

// This checks that the return address:
//  1) is within the .text section 
//  2) calling instruction has a correct relative offset. (must be call imm32)
//  3) caller of calling function is within our .text section
// Disabled in NoOpt because this won't be inlined.
static const int kCallCheckCodeOnly = 1;
static const int kCallCheckCallArg = 2;
static const int kCallCheckCallersCode = 3;
static const int kCallCheckRegCall = 4;

template<int level, void(*action)(unsigned int)> 
FORCEINLINE static unsigned int checkRbxCaller(const void* const funcAddress)
{
#if defined(_WIN32) && !defined(_NOOPT) && !defined(LOVE_ALL_ACCESS) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
    unsigned int flags = 0;

    void* returnAddress = _ReturnAddress();
    flags |= isRbxTextAddr(returnAddress) ? 0 : (1<<0);
    if (!flags)
    {
        switch(level)
        {
        case kCallCheckCallersCode:
            {
                void* aora = _AddressOfReturnAddress();
                void* secondReturnAddress = (*((void***)(aora)-1))[1];
                flags |= isRbxTextAddr(secondReturnAddress) ? 0 : (1<<2);
            }
            // continue
        case kCallCheckCallArg:
            {
                unsigned int offset = *((unsigned int*)(returnAddress) - 1);
                flags |= (offset == ((unsigned int)(funcAddress) 
                    - (unsigned int)(returnAddress))) ? 0 : (1<<1);
            }
            break;
        case kCallCheckRegCall:
            {
                // this is a weaker check.  call r32 is a prefixed 2 byte instruction.
                // because the register may have changed before this is called, there
                // isn't much point in checking it.
                static const unsigned short kX86RegCall = 0xD7FF; // due to little endian
                unsigned short opcode = *((unsigned short*)(returnAddress) - 1);
                flags |= ((opcode|0x0700) == kX86RegCall) ? 0 : (1<<3);
            }
            break;
        default:
            break;
        }
    }
    if (flags)
    {
        action(flags);
    }
    return flags;
#else
    return 0;
#endif
}

namespace Security{
    static const unsigned int kCheckDefault = 0;
    static const unsigned int kCheckReturnAddr = 1;
    static const unsigned int kCheckNoThreadInit = 2;
    static const unsigned int kAllowVmpAll = 4; // I'm not sure if I only need to look at mutant, mutant+plain, etc...
}

// Only supporting ntdll for now.
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)

inline const WCHAR* getUnicodeDllName(const UNICODE_STRING& str)
{
    USHORT idx;
    for (idx = str.Length/sizeof(WCHAR); idx != 0; --idx)
    {
        if (str.Buffer[idx] == L'\\')
        {
            break;
        }
    }
    return str.Buffer + idx + 1;
}

// This gets the location of ntdll using the documented parts of the PE format.
// The intent is to make it harder for anyone who wants to hook GetModuleHandle.
// This is not foolproof as the PEB or PEB pointer could be modified.
inline HMODULE rbxGetNtdll()
{
    const WCHAR* kNtDll = L"ntdll.dll";
    PEB* peb = reinterpret_cast<PEB*>(__readfsdword(0x30));
    PEB_LDR_DATA* pebLdrData = peb->Ldr;
    LIST_ENTRY imoListBase = pebLdrData->InMemoryOrderModuleList;
    LIST_ENTRY* imoListWalk = &imoListBase;
    int moduleLimit = 256;
    while ((imoListWalk->Flink != imoListBase.Blink) && --moduleLimit)
    {
        imoListWalk = imoListWalk->Flink;
        LDR_DATA_TABLE_ENTRY* thisPe = reinterpret_cast<LDR_DATA_TABLE_ENTRY*>(reinterpret_cast<BYTE*>(imoListWalk) - 8);
        const WCHAR* dllName = getUnicodeDllName(thisPe->FullDllName);
        const int dllNameLen = (thisPe->FullDllName.Length - (dllName - thisPe->FullDllName.Buffer))/sizeof(WCHAR);

        //filter here
        unsigned int dest = ((unsigned int)(kNtDll) + RBX_BUILDSEED);
        volatile unsigned int tmp = dest;
        const WCHAR* const ntdllName = (const WCHAR* const)(tmp - RBX_BUILDSEED);
        if (_wcsnicmp(dllName, ntdllName, std::min(dllNameLen, 9)) == 0)
        {
            return reinterpret_cast<HMODULE>(thisPe->DllBase);
        }
    }
    return 0;
}


// Find the location of the function matched by "filter" within ntdll's export table.
// Doesn't directly expose the name of the function that has been matched.
inline void* rbxNtdllProcAddress(HMODULE module, bool filter(const char*) )
{
    DWORD* loc = 0;
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)module;
    PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)(pDosHeader->e_lfanew + (char *)pDosHeader);
    IMAGE_DATA_DIRECTORY* imageDataDir = pNTHeader->OptionalHeader.DataDirectory;
    DWORD exportVa = imageDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD base = reinterpret_cast<DWORD>(module);
    PIMAGE_EXPORT_DIRECTORY pExport = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(base + exportVa);
    DWORD* pAddressOfNames = reinterpret_cast<DWORD*>(pExport->AddressOfNames + base);
    DWORD* pAddressOfFuncs = reinterpret_cast<DWORD*>(pExport->AddressOfFunctions + base);
    WORD*  pAddressOfOrds  = reinterpret_cast<WORD*> (pExport->AddressOfNameOrdinals + base);
    for (DWORD i = 0; i < pExport->NumberOfNames; ++i)
    {
        if (filter(reinterpret_cast<const char*>(base + pAddressOfNames[i])))
        {
            return reinterpret_cast<DWORD*>(base + pAddressOfFuncs[pAddressOfOrds[i]]);
        }
    }
    return loc;
}
#endif


struct CallChainInfo
{
    uint32_t handler;
    uint32_t ret;
    CallChainInfo() : handler(0), ret(0) {}
    CallChainInfo(uint32_t handler, uint32_t ret) : handler(handler), ret(ret) {}
};

#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_RCC_SECURITY) && !defined(RBX_PLATFORM_DURANGO)
// Call Stack, function with:
// 
// == C++ Exceptions == | == SEH3 Exceptions ==
// +1C  locals          | +20  locals
//       ...            |       ...
// +10  GS Cookie       | +10 *nextHandler()
// + C *nextHandler     | + C *handler()
// + 8 *handler()       | + 8  scopeTable
// + 4  state           | + 4  tryLevel
// +00  saved ebp       | +00  saved ebp
// - 4  return address  | - 4  return address
// - 8  first arg       | - 8  first arg
// -xx  last arg        | -xx  last arg
//
// This will get generated in some functions that don't have try-catch blocks.  Basically
// if something could throw, objects might need to be destroyed.
//
// fs:[0] also contains a pointer to nextHandler, but is out of place in the middle of a
// function.
//
// The list will always have a size of at least 2 -- the entry in ntdll and the entry we
// started with.
// 
// There is a case with boost::threads, as these get created with msvcr's threadstartex
// (after ntdll rtlInitThread).  However, boost::threads will have three exception handlers
// in the boost code before getting into msvcr.

template<size_t kMaxDepth> FORCEINLINE uint32_t detectDllByExceptionChain(void* addrOfChain, unsigned int kFlags)
{
    static const size_t kStackNextIdx = 0;
    static const size_t kStackHandlerIdx = 1;
    static const size_t kStackReturnIdx = 4;
    static const size_t kStackEndingArgIdx = 6;
    DWORD* stkPtr = reinterpret_cast<DWORD*>(addrOfChain);
    uintptr_t textEndNeg = RBX::Security::rbxTextEndNeg;
    size_t textSizeNeg = RBX::Security::rbxTextSizeNeg;
    uintptr_t vmpBase = RBX::Security::rbxVmpBase;
    size_t vmpSize = RBX::Security::rbxVmpSize;
    uint32_t result = 0;
    for (size_t i = 0; i < kMaxDepth; ++i)
    {
        // check for end of chain in ntdll.
        if (!(kFlags & Security::kCheckNoThreadInit) && (stkPtr[0] == 0xFFFFFFFF))
        {
            // check if handler chain init in roblox. (equivalent to "addr - base >= size")
            if ((stkPtr[kStackEndingArgIdx] + textEndNeg ) <= textSizeNeg)
            {
                result |= 1 << (i*3);
            }
            break;
        }
        // check if handler in roblox code. (equivalent to "addr - base >= size")
        if ((stkPtr[kStackHandlerIdx] + textEndNeg ) <= textSizeNeg)
        {
            result |= 1 << (i*3 + 1);
            break;
        }
        if ( (kFlags & Security::kCheckReturnAddr)                     // check return enabled
            && (stkPtr[kStackReturnIdx] + textEndNeg ) <= textSizeNeg) // not in .text
        {
            if (!((kFlags&Security::kAllowVmpAll)                   // allow vmp
                && (stkPtr[kStackReturnIdx] - vmpBase ) > vmpSize)) // not in vmp
            {
                result |= 1 << (i*3 + 2);
                break;
            }
        }
        // follow next
        stkPtr = reinterpret_cast<DWORD*>(stkPtr[kStackNextIdx]);
    }
    return result;
}

template<size_t kMaxDepth> FORCEINLINE void generateCallInfo(void* addrOfFirstArg, std::vector<CallChainInfo>& info)
{
    static const size_t kStackNextIdx = 0;
    static const size_t kStackHandlerIdx = 1;
    static const size_t kStackReturnIdx = 4;
    static const size_t kStackEndingArgIdx = 6;
    static const size_t kStackFirstArgToNextHandler = 5;
    DWORD* stkPtr = reinterpret_cast<DWORD*>(addrOfFirstArg) - kStackFirstArgToNextHandler;
    for (size_t i = 0; i < kMaxDepth; ++i)
    {
        info.push_back(CallChainInfo(stkPtr[kStackHandlerIdx],stkPtr[kStackReturnIdx]));
        // check for end of chain in ntdll.
        if (stkPtr[0] == 0xFFFFFFFF)
        {
            break;
        }
        // follow next
        stkPtr = reinterpret_cast<DWORD*>(stkPtr[kStackNextIdx]);
    }
}

template<size_t kMaxDepth> FORCEINLINE uint32_t detectDllByExceptionChainStack(void* addrOfFirstArg, unsigned int kFlags)
{
    static const size_t kStackFirstArgToNextHandler = 5;
    DWORD* stkPtr = reinterpret_cast<DWORD*>(addrOfFirstArg) - kStackFirstArgToNextHandler;
    return detectDllByExceptionChain<kMaxDepth>(stkPtr, kFlags);
}

// not all functions add an exception handler to the chain, so the stack based method doesn't
// always work.
template<size_t kMaxDepth> FORCEINLINE uint32_t detectDllByExceptionChainTeb(unsigned int kFlags)
{
    return detectDllByExceptionChain<kMaxDepth>(reinterpret_cast<void*>(__readfsdword(0)), kFlags );
}
#else
template<size_t kMaxDepth> FORCEINLINE uint32_t detectDllByExceptionChainTeb(unsigned int kFlags)
{
    return 0;
}
template<size_t kMaxDepth> FORCEINLINE uint32_t detectDllByExceptionChainStack(void* addrOfFirstArg, unsigned int kFlags)
{
    return 0;
}

template<size_t kMaxDepth> FORCEINLINE void generateCallInfo(void* addrOfFirstArg, std::vector<CallChainInfo>& info)
{
    return;
}

#endif

}
