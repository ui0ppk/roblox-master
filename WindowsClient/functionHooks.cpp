#include "stdafx.h"
#include "functionHooks.h"
#include "VMProtect/VMProtectSDK.h"
#include <windows.h>
#include <psapi.h>

namespace{
const DWORD kLenHotpatchNop = 2;
const WORD kHotpatchNop = 0xFF8B; // mov edi,edi  (little endian)
const DWORD kLenLongJumpWin32 = 5;
const DWORD kLenPushByte = 2;
const DWORD kLenPushDword = 5;
const BYTE kHotpatchJmp = 0xF9; // -6 + 1 = 5
const BYTE kNop = 0x90;
const BYTE kInt3 = 0xCC;
const BYTE kJmp8 = 0xEB;
const BYTE kJmp32 = 0xE9;
const BYTE kPushByte = 0x6A;
const BYTE kPushDword = 0x68;
const BYTE kCallDword = 0xE8;

bool hasHotpatchProlog(void* pfn)
{
    WORD fnProlog = *(reinterpret_cast<WORD*>(pfn));
    if (fnProlog != kHotpatchNop)
    {
        return 0;
    }
    for (size_t i = 1; i <= kLenLongJumpWin32; ++i)
    {
        unsigned char thisPatchByte = *(reinterpret_cast<unsigned char*>(pfn) - i);
        if( thisPatchByte != kNop && thisPatchByte != kInt3 )
        {
            return 0;
        }
    }
    return 1;
}

DWORD getLongJmpArg(void* src, void* dst)
{
    return reinterpret_cast<DWORD>(dst)
        - (reinterpret_cast<DWORD>(src)+kLenLongJumpWin32);
}
}

namespace RBX 
{

// Windows API functions often have a "hotpatch" prolog.  Basically either
// CC CC CC CC CC*8B FF ...  or 90 90 90 90 90*8B FF ... where * is the entry
// to a function.  8B FF is the two byte NOP "mov edi,edi".  The five int3 (CC)
// or nop (90) instructions are just enough for a jmp dword.  The original idea
// was to allow patches to fix functions without needing a reboot.
//
// http://blogs.msdn.com/b/oldnewthing/archive/2011/09/21/10214405.aspx
//
void* hotpatchHook(void* origFunction, void* hookFunction)
{
    // Can patch?
    if (!hasHotpatchProlog(origFunction))
    {
        return 0;
    }
    void* pfnResume = reinterpret_cast<void*>(
        reinterpret_cast<unsigned char*>(origFunction) + kLenHotpatchNop);

    // Must patch
    void* pfnWriteEntry = reinterpret_cast<void*>(
        reinterpret_cast<unsigned char*>(origFunction) - kLenLongJumpWin32);
    unsigned char patchBuffer[7] = {kJmp32, 0x00, 0x00, 0x00, 0x00, kJmp8, kHotpatchJmp};
    DWORD relativeAddr = getLongJmpArg(pfnWriteEntry, hookFunction);
    memcpy(&patchBuffer[1], &relativeAddr, sizeof(DWORD));
    DWORD bytesWritten = 0;
    BOOL wpmStatus = WriteProcessMemory(GetCurrentProcess(), pfnWriteEntry, patchBuffer, sizeof(patchBuffer), &bytesWritten);
    if (bytesWritten != sizeof(patchBuffer) || !wpmStatus)
    {
        return 0;
    }

    return (pfnResume);
}

void* hotpatchUnhook(void* pfn)
{
    if (pfn)
    {
        const unsigned char patchBuffer[7] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x8B, 0xFF};
        BYTE* writePfn = reinterpret_cast<BYTE*>(pfn);
        writePfn -= sizeof(patchBuffer);
        DWORD bytesWritten = 0;
        // Change the patch jump to be a nop first.
        BOOL wpmStatus = WriteProcessMemory(GetCurrentProcess(), writePfn+5, patchBuffer+5, 2, &bytesWritten);
        if (bytesWritten != 2 || !wpmStatus)
        {
            return 0;
        }
        // Now change the jump
        wpmStatus = WriteProcessMemory(GetCurrentProcess(), writePfn, patchBuffer, 5, &bytesWritten);
        if (bytesWritten != 5 || !wpmStatus)
        {
            return 0;
        }
        return (writePfn+5);
    }
    return 0;
}

// If any of these fail, it should be ok to just not enable that security feature.
bool hookingApiHooked()
{
    // (Hopefully) get handle to Kernel32
    HMODULE kernel32 = GetModuleHandle("Kernel32");
    MODULEINFO info;
    if (!GetModuleInformation(GetCurrentProcess(), kernel32, &info, sizeof(info)))
    {
        return true;
    }
    size_t k32Base = reinterpret_cast<size_t>(info.lpBaseOfDll);
    size_t k32Size = info.SizeOfImage;

    // is WPM hooked via IAT?  (this does the IAT lookup, which points to Kernel32 DLL)
    size_t wpmBase = reinterpret_cast<size_t>(&WriteProcessMemory);
    if (wpmBase - k32Base >= k32Size)
    {
        return true; 
    }

    // Does hotpatch exist (the first two bytes better be the hotpatch space)
    WORD wpmEntry = *reinterpret_cast<short*>(&WriteProcessMemory);
    if (wpmEntry != kHotpatchNop)
    {
        return true;
    }
    
    // It's possible HWBP, or access violation hooks could still be used.
    return false;
}

}
