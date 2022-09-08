// This is here as some additional security measures that only exist inside RCC.
#include <random>
#include <windows.h>
#include "security/JunkCode.h"
#include "util/Analytics.h"

// p1 -- crash if reflection area written to
// p2 -- log probability = p2 / 100;
// p3 -- master enable
FASTFLAGVARIABLE(US30484p1, false)
FASTINTVARIABLE(US30484p2, 0)
FASTFLAGVARIABLE(US30484p3, false)

FASTFLAG(US31006)

namespace
{

    template<int N> __forceinline void rccBigFunc()
    {
        _asm
        {
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP6;
            RBX_NOP3; // 4 bytes
        }
        rccBigFunc<N-1>();
    }

    template<> __forceinline void rccBigFunc<0>() {}

    // __declspec(align(4096)) didn't work, so this should be 8192 bytes large to ensure it fills one
    // page fully.
    void rccBigFuncStorage()
    {
#if !defined(_NOOPT) && !defined(_DEBUG)
        rccBigFunc<128>();
#endif
    }

    static const size_t kPageSize = 4096;
    uintptr_t getGuardPageBase()
    {
        uintptr_t funcBase = reinterpret_cast<uintptr_t>(&rccBigFuncStorage);
        return kPageSize*((funcBase + kPageSize - 1)/kPageSize);
    }

    typedef std::vector<IMAGE_SECTION_HEADER*> SectionPtrVector;

    bool getSectionInfo(const SectionPtrVector& sections, const char* name, uintptr_t& baseAddr, size_t& size)
    {
        const size_t kPeSectionNameLimit = 9;
        for (size_t i = 0; i < sections.size(); ++i)
        {
            if (strncmp(reinterpret_cast<const char*>(sections[i]->Name), name, kPeSectionNameLimit) == 0) // 
            {
                baseAddr = sections[i]->VirtualAddress + reinterpret_cast<size_t>(GetModuleHandle(NULL));
                size = sections[i]->Misc.VirtualSize;
                return true;
            }
        }
        return false;
    }

    int getSections(void* pImageBase, SectionPtrVector& sections)
    {
        IMAGE_DOS_HEADER *pDosHdr = nullptr;
        IMAGE_SECTION_HEADER *pSection = nullptr;

        // Get DOS header
        pDosHdr = reinterpret_cast<IMAGE_DOS_HEADER*>(pImageBase);

        // File not a valid PE file
        if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
            return -1;

        // Get image header
        IMAGE_NT_HEADERS32* pImageHdr32 = reinterpret_cast<IMAGE_NT_HEADERS32*>(
            reinterpret_cast<BYTE*>(pDosHdr) + pDosHdr->e_lfanew);

        // File not a valid PE file
        if (pImageHdr32->Signature != IMAGE_NT_SIGNATURE)
            return -2;

        pSection = reinterpret_cast<IMAGE_SECTION_HEADER*>(pImageHdr32 + 1);

        // Sections
        for (int i = 0; i < pImageHdr32->FileHeader.NumberOfSections; ++i, ++pSection)
            sections.push_back( pSection );

        return 0;
    }

    static uintptr_t luaBase = 0;
    static size_t luaSize = 0;
    static void* luaVehHandle = nullptr;
    LONG CALLBACK rccReflectionModifiedVeh(EXCEPTION_POINTERS* rec)
    {
        DWORD unused;
        uintptr_t codeAddr = reinterpret_cast<uintptr_t>(rec->ExceptionRecord->ExceptionAddress);
        uintptr_t dataAddr = rec->ExceptionRecord->ExceptionInformation[1];
        uintptr_t rccBaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
        if ((rec->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) // is access violation
            && (dataAddr - luaBase < luaSize) // data is in .lua
            && (codeAddr - luaBase >= luaSize)) // code is not in .lua
        {
            RBX::Analytics::InfluxDb::Points analyticsPoints;
            analyticsPoints.addPoint("code", RBX::format("0x%08X", codeAddr-rccBaseAddress).c_str());
            analyticsPoints.addPoint("data", RBX::format("0x%08X", dataAddr-rccBaseAddress).c_str());
            analyticsPoints.report("RccReflectionAv", FInt::US30484p2, true); // add fflag
            if (!FFlag::US30484p1)
            {
                VirtualProtect(reinterpret_cast<void*>(luaBase), luaSize, PAGE_READWRITE, &unused);
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

}

namespace RBX
{
    extern bool gCrashIsSpecial;
    extern std::string specialCrashType;
    LONG CALLBACK rccBigFuncExceptionHandler(EXCEPTION_POINTERS* rec)
    {
        uintptr_t funcBase = reinterpret_cast<uintptr_t>(&rccBigFuncStorage);
        uintptr_t guardPage = kPageSize*((funcBase + kPageSize - 1)/kPageSize);
        // the location of the data that was accessed is in ExceptionInformation[1]
        // the location of the code that accessed it is in ExceptionAddress
        if ((rec->ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE)
            && (rec->ExceptionRecord->ExceptionInformation[1] - getGuardPageBase() < kPageSize))
        {
            gCrashIsSpecial = true;
            specialCrashType = "page";
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
 
    LONG CALLBACK rccHwbpExceptionHandler(EXCEPTION_POINTERS* rec)
    {
        if (rec->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
        {
            gCrashIsSpecial = true;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void initAntiMemDump()
    {
#if !defined(_NOOPT) && !defined(_DEBUG)
        // Add the exception handler that will process page guard exceptions in a specific address range.
        AddVectoredExceptionHandler(1, &rccBigFuncExceptionHandler);

        // If someone was scanning RAM, seeing repeating patterns would be suspicious and detectable.
        DWORD unused;
        VirtualProtect(&rccBigFuncStorage, 2*kPageSize, PAGE_EXECUTE_READWRITE, &unused);
        unsigned char* fakeCode = reinterpret_cast<unsigned char*>(GetModuleHandleA(NULL)) + 0x1000;
        memcpy(&rccBigFuncStorage, fakeCode, 2*kPageSize);
        VirtualProtect(&rccBigFuncStorage, 2*kPageSize, PAGE_EXECUTE_READ, &unused);

        // Finally, set the guard page.
        VirtualProtect(reinterpret_cast<void*>(getGuardPageBase()), kPageSize, PAGE_GUARD | PAGE_EXECUTE_READ, &unused);
#endif
    }

    void initHwbpVeh()
    {
        if (!FFlag::US31006)
        {
            return;
        }
        AddVectoredExceptionHandler(1, &rccHwbpExceptionHandler);
    }

    void initLuaReadOnly()
    {
        if (!FFlag::US30484p3)
        {
            return;
        }

        SectionPtrVector sections;
        getSections(reinterpret_cast<void*>(GetModuleHandleA(NULL)), sections);
        getSectionInfo(sections, ".lua", luaBase, luaSize);
        if (luaBase && luaSize)
        {
            DWORD unused;
            luaVehHandle = AddVectoredExceptionHandler(0, &rccReflectionModifiedVeh);
            VirtualProtect(reinterpret_cast<void*>(luaBase), luaSize, PAGE_READONLY, &unused);
        }
    }

    // From testing, it looks like the crash reporter results in a crash and does not call
    // all destructors, etc...  However, in the normal case, this would prevent a crash at exit.
    void clearLuaReadOnly()
    {
        if (!FFlag::US30484p3)
        {
            return;
        }

        if (luaBase && luaSize)
        {
            DWORD unused;
            RemoveVectoredExceptionHandler(luaVehHandle);
            VirtualProtect(reinterpret_cast<void*>(luaBase), luaSize, PAGE_READWRITE, &unused);
            luaBase = 0;
            luaSize = 0;
            luaVehHandle = nullptr;
        }
    }
}
