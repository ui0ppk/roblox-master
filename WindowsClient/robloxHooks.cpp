#include "stdafx.h"
#include "functionHooks.h"
#include "robloxHooks.h"
#include "v8datamodel/HackDefines.h"
#include "security/FuzzyTokens.h"
#include "security/ApiSecurity.h"
#include "util/CheatEngine.h"
#include "v8datamodel/FastLogSettings.h"
#include "VMProtect/VMProtectSDK.h"
#include <windows.h>
#include <psapi.h>

namespace
{
size_t moduleStart = 0;
size_t moduleSize = 0xFFFFFFFF;

typedef HWND (WINAPI *FindWindowSplice)(LPCTSTR, LPCSTR);

// Can call this function.
FindWindowSplice resumeFindWindow = 0;

// asm stub to run when findWindowA is called.
HWND WINAPI findWindowHook(LPCTSTR className, LPCSTR windowName)
{
    size_t returnAddress;
    size_t argDiff;
    static const size_t kHalf = 1 << 23;
    static const size_t kFull = 1 << 24;  
    VMProtectBeginMutation("35");
    returnAddress = reinterpret_cast<size_t>(_ReturnAddress());
    argDiff = (reinterpret_cast<size_t>(windowName) - returnAddress);
    
    // This is an attempt at a very conservative check for the window.  It looks for
    // DLLs that called out ROBLOX by name inside their code and not from some scan.
    if (windowName // user passed a windowName
        && ((returnAddress - moduleStart) > moduleSize) // but not us
        && ((argDiff+kHalf) < kFull) // Argument is +-8MB from call location, probably .rdata.
        && (_strnicmp(windowName, "ROBLOX", 6) == 0)) // with roblox as argument
    {
        RBX::hotpatchUnhook(resumeFindWindow);
        RBX::Tokens::simpleToken |= HATE_DLL_INJECTION;
    }
    VMProtectEnd();
    return resumeFindWindow(className,windowName);
}

// This will request a kick if a possible access violation hook is found.  This will set
// a flag and allow normal exception handling to occur.  If the user was not hacking,
// there will be a crash.
void RtlDispatchExceptionCheck(PEXCEPTION_RECORD exRec, PCONTEXT ctx)
{
    const DWORD code = exRec->ExceptionCode;
    const DWORD codeStart = RBX::Security::rbxTextBase;
    const DWORD codeSize = RBX::Security::rbxTextSize;
    if( code == EXCEPTION_ACCESS_VIOLATION)
    {
        const DWORD avAddr = exRec->ExceptionInformation[1];
        if ((avAddr - codeStart) <= codeSize)
        {
            RBX::Security::setHackFlagVs<LINE_RAND1>(RBX::Security::hackFlag6, HATE_VEH_HOOK);
            RBX::Tokens::sendStatsToken.addFlagFast(HATE_VEH_HOOK);
        }
    }
    else if ((code == EXCEPTION_BREAKPOINT) ||
        (code == EXCEPTION_SINGLE_STEP) ||
        (code == EXCEPTION_ILLEGAL_INSTRUCTION) ||
        (code == EXCEPTION_PRIV_INSTRUCTION))
    {
        const DWORD addr = reinterpret_cast<DWORD>(exRec->ExceptionAddress);
        if ((addr - codeStart) <= codeSize)
        {
            RBX::Security::setHackFlagVs<LINE_RAND1>(RBX::Security::hackFlag6, HATE_VEH_HOOK);
            RBX::Tokens::sendStatsToken.addFlagFast(HATE_VEH_HOOK);
        }
    }
}

bool cmpKiUserExceptionDispatcher(const char* inString)
{
    const unsigned char cmpString[26] = {129, 254, 37, 162, 27, 133, 129, 157, 225, 138, 46, 157, 191, 244, 244, 153, 75, 12, 70, 220, 152, 104, 186, 244, 94, 43};
    if (!inString) return false;
    for (int i = 0; i < 26; ++i)
    {
        if ((unsigned char)((inString[i]+i)*227) != cmpString[i]) return false;
        if (!inString[i]) return (i == 25);
    };
    return false;
}

const unsigned char kiUserExceptionDispatcherProlog[10] = 
    {0x8B, 0x4C, 0x24, 0x04, // mov ecx,dword ptr [esp+4]
    0x8B, 0x1C, 0x24,        // mov ebx,dword ptr [esp]
    0x51,                    // push ecx
    0x53,                    // push ebx
    0xE8 /* XX XX XX XX */}; // call ntdll!RtlDisapatchException

}

namespace RBX
{
    DWORD* vehHookLocation;

    // This will prevent our code from appearing on the callstack of the exception functions.
    __declspec(naked) BOOLEAN RtlDispatchExceptionHook(PEXCEPTION_RECORD exRec, PCONTEXT ctx)
    {
        __asm
        {
            push ebp;
            mov ebp, esp;
            sub esp, __LOCAL_SIZE;
            push ebx;
            push ecx;
        }
        RtlDispatchExceptionCheck(exRec, ctx);
        __asm
        {
            pop ecx;
            pop ebx;
            mov esp, ebp;
            pop ebp;
            cld;
            jmp vehHookContinue;
        }
    }

    bool hookPreVeh()
    {
        volatile bool result = false;
        VMProtectBeginMutation(NULL);
        HMODULE ntdll = GetModuleHandleA("ntdll");
        DWORD* loc = reinterpret_cast<DWORD*>(rbxNtdllProcAddress(ntdll, cmpKiUserExceptionDispatcher));
        ntdll = 0;
        if (loc)
        {
            // On win7/8 the prolog is preceeded by "cld", a 1 byte instruction.  If the prolog isn't found,
            // retry.  if it is found on the first try, this is winXp.
            bool foundWinXp = true;
            if (0 != memcmp(loc, kiUserExceptionDispatcherProlog, sizeof(kiUserExceptionDispatcherProlog)))
            {
                loc = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(loc) + 1);
                foundWinXp = false;
            }
            if (foundWinXp || (0 == memcmp(loc, kiUserExceptionDispatcherProlog, sizeof(kiUserExceptionDispatcherProlog))))
            {
                // hook location is the argument to the long call, at eip+1
                // it will jump to (eip+5)+arg, which is (loc+4)+*loc
                // (eip+5)+arg=pfn, so (loc+4)+arg=pfn, arg = pfn-loc-4
                vehHookLocation = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(loc) + sizeof(kiUserExceptionDispatcherProlog));
                DWORD newOffset = reinterpret_cast<DWORD>(&RtlDispatchExceptionHook) - reinterpret_cast<DWORD>(vehHookLocation) - 4;
                vehHookContinue = reinterpret_cast<void*>(reinterpret_cast<DWORD>(vehHookLocation) + 4 + *vehHookLocation);
                DWORD bytesWritten;
                BOOL hookResult = WriteProcessMemory(GetCurrentProcess(), vehHookLocation, &newOffset, sizeof(DWORD), &bytesWritten);
                if (hookResult && bytesWritten == 4)
                {
                    result = true;
                }
                else
                {
                    Tokens::apiToken.addFlagSafe(kVehWpmFail);
                }
            }
            else
            {
                Tokens::apiToken.addFlagSafe(kVehPrologFail);
            }
        }
        else
        {
            Tokens::apiToken.addFlagSafe(kVehNoNtdll);
        }
        result = result; // vmprotect workaround.
        VMProtectEnd();
        return result;
    }

    void hookApi()
    {
        if (hookingApiHooked())
        {
            return;
        }

        MODULEINFO info;
        if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &info, sizeof(MODULEINFO)))
        {
            moduleStart = reinterpret_cast<size_t>(info.lpBaseOfDll);
            moduleSize = info.SizeOfImage;
        }
        resumeFindWindow = reinterpret_cast<FindWindowSplice>(hotpatchHook(&FindWindowA,findWindowHook));
        hookPreVeh();
    }

    // In case something goes wrong or we need to be stealthy
    void unhookApi()
    {
        hotpatchUnhook(resumeFindWindow);
    }

}