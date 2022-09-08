#include "stdafx.h"

#include "Util/ProgramMemoryChecker.h"
#include "rbx/rbxTime.h"

#include "FastLog.h"
#include "Rbx/Debug.h"
#include "Util/xxhash.h"

#include <boost/algorithm/string.hpp>

#include "Windows.h"
#include "VMProtect/VMProtectSDK.h"
#include <psapi.h>

#include "security/ApiSecurity.h"
#include "V8DataModel/HackDefines.h"
#include "humanoid/HumanoidState.h"

#include "g3d/vector3.h"
#include "g3d/matrix3.h"

#pragma comment(lib, "Psapi.lib")

LOGGROUP(US14116)

FASTFLAG(DebugLocalRccServerConnection)

// New stealthedit checks
namespace
{

__forceinline bool getSectionInfo(const HMODULE& module, const char* name, uintptr_t& baseAddr, size_t& size)
{
    // This gets the string address after some calculations.  This is to make static analysis difficult
    // the string ".text" is not directly referenced.
    unsigned int dest = ((unsigned int)(name) + RBX_BUILDSEED);
    volatile unsigned int tmp = dest;
    const char* const sectionName = (const char* const)(tmp - RBX_BUILDSEED);
    baseAddr = 0;
    size = 0;
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)module;
    PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)(pDosHeader->e_lfanew + (char *)pDosHeader);
    DWORD sectionCount = pNTHeader->FileHeader.NumberOfSections;
    PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
    for (DWORD sectionIndex = 0; sectionIndex < sectionCount; sectionIndex++, pSectionHeader++)
    {
        if (strcmp(sectionName, (char*)(pSectionHeader->Name)) == 0)
        {
            baseAddr = ((uintptr_t)module + pSectionHeader->VirtualAddress);
            size = pSectionHeader->Misc.VirtualSize;
            return true;
        }
    }
    return false;
}
    
bool isExePage(const MEMORY_BASIC_INFORMATION& info)
{
    return (info.State == MEM_COMMIT && (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)));
}

// Exploiters were targeting these initialized constants.  VMProtect has issues with optimizations:
#pragma optimize( "", off )
bool checkG3dConsts()
{
    volatile bool returnValue = true;
    VMProtectBeginMutation(NULL);
    returnValue = ( (G3D::Matrix3::identity() != G3D::Matrix3(1, 0, 0, 0, 1, 0, 0, 0, 1))
        || (G3D::Vector3::unitX() != G3D::Vector3(1, 0, 0))
        || (G3D::Vector3::unitY() != G3D::Vector3(0, 1, 0))
        || (G3D::Vector3::unitZ() != G3D::Vector3(0, 0, 1)) );
    VMProtectEnd();
    return returnValue;
}
#pragma optimize( "", on )

// these are here in case the hashing code is copied.  It isn't that useful, but makes
// things a bit harder for exploiters.
template<int N /*0*/> __forceinline unsigned int modifyArg(const char* p)
{
    return *(unsigned int*)(p)+(unsigned int)(p);
}
template<> __forceinline unsigned int modifyArg<1>(const char* p)
{
    return *(unsigned int*)(p)-(unsigned int)(p);
}
template<> __forceinline unsigned int modifyArg<2>(const char* p)
{
    return *(unsigned int*)(p)^(unsigned int)(p);
}
template<> __forceinline unsigned int modifyArg<3>(const char* p)
{
    return (unsigned int)(p)-*(unsigned int*)(p);
}

const char* const kDotText = ".text";

}

namespace RBX
{
    namespace Security{
        // The storage for hash checker related security constants.
        volatile const size_t rbxGoldHash = 0;

        // The lower part of .text
        volatile const uintptr_t rbxLowerBase = 0x00400000;
        volatile const size_t rbxLowerSize = 1024;

        // The upper part of .text
        volatile const uintptr_t rbxUpperBase = 0x00400000;
        volatile const size_t rbxUpperSize = 1024;

        // the .rdata section
        volatile const uintptr_t rbxRdataBase = 0x00400000;
        volatile const size_t rbxRdataSize = 1024;

        // the vmp sections
        volatile const uintptr_t rbxVmpBase = 0x00400000;
        volatile const size_t rbxVmpSize = 1024;

        // the Import Address (thunk) Table
        volatile const uintptr_t rbxIatBase = 0x00400000;
        volatile const size_t rbxIatSize = 1024;

        // the vmp sections (plain .text section)
        volatile const uintptr_t rbxVmpPlainBase = 0x00400000;;
        volatile const size_t rbxVmpPlainSize = 1024;;

        // the vmp sections (mutation .text section)
        volatile const uintptr_t rbxVmpMutantBase = 0x00400000;;
        volatile const size_t rbxVmpMutantSize = 1024;;

        // the vmp sections (don't know)
        volatile const uintptr_t rbxVmp0MiscBase = 0x00400000;;
        volatile const size_t rbxVmp0MiscSize = 1024;;

        // the vmp sections (don't know)
        volatile const uintptr_t rbxVmp1MiscBase = 0x00400000;;
        volatile const size_t rbxVmp1MiscSize = 1024;;

        // the .rdata section without IAT
        volatile const uintptr_t rbxRdataNoIatBase = 0x00400000;
        volatile const size_t rbxRdataNoIatSize = 1024;
    }



    namespace CryptStrings{
        bool cmpNtQueryVirtualMemory(const char* inString)
        {
            //                                    N    t    Q    u   e    r   y   V    i    r    t    u   a   l    M    e    m    o    r    y   \0
            const unsigned char cmpString[21] = {74, 239, 233, 232, 11, 149, 45, 39, 163, 225, 218, 128, 87, 59, 129, 156, 135, 128, 204, 100, 124};
            if (!inString) return false;
            for (int i = 0; i < 21; ++i)
            {
                if ((unsigned char)((inString[i]+i)*83) != cmpString[i]) return false;
                if (!inString[i]) return (i == 20);
            };
            return false;
        }

        bool cmpZwFilterToken(const char* inString)
        {
            //                                    Z    w   F  i   l   t    e   r    T    o    k   e    n  \0
            const unsigned char cmpString[14] = {46, 232, 88, 4, 80, 59, 177, 59, 212, 232, 239, 80, 142, 55};
            if (!inString) return false;
            for (int i = 0; i < 14; ++i)
            {
                if ((unsigned char)((inString[i]+i)*83) != cmpString[i]) return false;
                if (!inString[i]) return (i == 13);
            };
            return false;
        }

        bool cmpNtGetContextThread(const char* inString)
        {
            //                                    N    t    G    e    t   C    o    n   t    e   x   t   T    h    r    e    a    d   \0
            const unsigned char cmpString[19] = {74, 239, 171, 184, 232, 88, 239, 239, 52, 170, 38, 45, 32, 239, 128, 156, 163, 239, 214};
            if (!inString) return false;
            for (int i = 0; i < 19; ++i)
            {
                if ((unsigned char)((inString[i]+i)*83) != cmpString[i]) return false;
                if (!inString[i]) return (i == 18);
            };
            return false;
        }

        bool cmpZwLoadKey(const char* inString)
        {
            //                                    Z    w   L    o    a   d   K  e    y   \0
            const unsigned char cmpString[10] = {46, 232, 74, 246, 191, 11, 67, 4, 211, 235};
            if (!inString) return false;
            for (int i = 0; i < 10; ++i)
            {
                if ((unsigned char)((inString[i]+i)*83) != cmpString[i]) return false;
                if (!inString[i]) return (i == 9);
            };
            return false;
        }
    }

    NtApiCaller::NtApiCaller()
        : ntQvmCallHash(0)
        , ntQvmAsUint(0)
        , ntGtxCallHash(0)
        , ntGtxAsUint(0)
        , hashEndSize(0)
        , ntdllSize(0)
        , ntdllTextBase(0)
    {
        VMProtectBeginMutation(NULL);
        // get information about ntdll.
        thisProcess = GetCurrentProcess();
        HMODULE ntdll = rbxGetNtdll();
        if (!ntdll)
        {
            // could not find ntdll
            Tokens::apiToken.addFlagSafe(kNtApiNoNtdll);
        }
        uintptr_t sectionBase;
        uintptr_t sectionSize;
        if (ntdll && !getSectionInfo(ntdll, kDotText, sectionBase, sectionSize))
        {
            // This is the case where .text couldn't be found
            Tokens::apiToken.addFlagSafe(kNtApiNoText);
        }
        ntdllSize = sectionSize;
        ntdllTextBase = sectionBase;
        sectionBase = 0;
        sectionSize = 0;

        // Get information about the location of NtQueryVirtualMemory
        uintptr_t pfn = reinterpret_cast<uintptr_t>(rbxNtdllProcAddress(ntdll, CryptStrings::cmpNtQueryVirtualMemory));
        uintptr_t callTemplate = reinterpret_cast<uintptr_t>(rbxNtdllProcAddress(ntdll, CryptStrings::cmpZwFilterToken));
        initApiFunction(pfn, ntQvmAsUint, callTemplate, ntQvmCallHash, hashEndSize, kNtQvmEndToken);

        pfn = reinterpret_cast<uintptr_t>(rbxNtdllProcAddress(ntdll, CryptStrings::cmpNtGetContextThread));
        callTemplate = reinterpret_cast<uintptr_t>(rbxNtdllProcAddress(ntdll, CryptStrings::cmpZwLoadKey));
        initApiFunction(pfn, ntGtxAsUint, callTemplate, ntGtxCallHash, hashEndSize, kNtGtxEndToken);

        pfn = 0;
        ntdll = 0;
        callTemplate = 0;
        VMProtectEnd();
    }

}


namespace RBX
{
using namespace Hasher;

PmcHashContainer pmcHash;

ScanRegion ScanRegion::getScanRegion(const char* moduleName, const char* regionName)
{
    HMODULE module = GetModuleHandle(moduleName);
    ScanRegion result;
    if (module != NULL) 
    {
        uintptr_t addr;
        size_t size;
        getSectionInfo(module, regionName, addr, size);
        result.startingAddress = (char *)addr;
        result.size = size;
    }
    return result;
}

PmcHashContainer::PmcHashContainer(const PmcHashContainer& init) : nonce(init.nonce), hash(init.hash) {}

ProgramMemoryChecker::ProgramMemoryChecker()
    : currentRegion(0)
    , currentMemory(NULL)
    , scanningRegions()
    , lastCompletedHash(0)
    , lastGoldenHash(0)
    , lastCompletedTime(Time::nowFast())
{
    #if !defined(RBX_STUDIO_BUILD)
    VMProtectBeginMutation(NULL);
    ScanRegionTest nonceLocation( ScanRegion(((char*) (&pmcHash.nonce)), 4));
    nonceLocation.closeHash = true;
    nonceLocation.useHashValueInStructHash = false;
    scanningRegions.resize(kNumberOfSectionHashes);
        
    // Set up the gold hashes
    scanningRegions[kGoldHashStart] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxLowerBase), Security::rbxLowerSize));
    scanningRegions[kGoldHashEnd] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxUpperBase), Security::rbxUpperSize));
    scanningRegions[kGoldHashRot] = nonceLocation;

    // Set up the diff-only hashes
    scanningRegions[kRdataHash] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxRdataNoIatBase), Security::rbxRdataNoIatSize));
    scanningRegions[kVmpPlainHash] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxVmpPlainBase), Security::rbxVmpPlainSize));
    scanningRegions[kVmpMutantHash] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxVmpMutantBase), Security::rbxVmpMutantSize));
    scanningRegions[kVmp0MiscHash] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxVmp0MiscBase), Security::rbxVmp0MiscSize));
    scanningRegions[kVmp1MiscHash] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxVmp1MiscBase), Security::rbxVmp1MiscSize));
    scanningRegions[kIatHash] = ScanRegionTest(ScanRegion(reinterpret_cast<char*>(Security::rbxIatBase), Security::rbxIatSize));
    scanningRegions[kMiscHash] = ScanRegionTest(ScanRegion::getScanRegion("winmm", ".text"));
    #ifdef _DEBUG
    scanningRegions[kMsvcHash] = ScanRegionTest(ScanRegion::getScanRegion("MSVCR110D", ".text"));
    #else
    scanningRegions[kMsvcHash] = ScanRegionTest(ScanRegion::getScanRegion("MSVCR110", ".text"));
    #endif
    scanningRegions[kNonGoldHashRot] = nonceLocation;

    // place the state assignments last to avoid within-group order dependence.
    // gold region
    scanningRegions[kGoldHashStart].hashState 
        = scanningRegions[kGoldHashEnd].hashState 
        = scanningRegions[kGoldHashRot].hashState = XXH32_init(kHASH_SEED_INIT);

    // Proposed Golden Hash Additions 
    // (these should be predicatable, but this hasn't been confirmed on other computers)
    scanningRegions[kRdataHash].hashState = XXH32_init(kHASH_SEED_INIT);
    scanningRegions[kVmpPlainHash].hashState = XXH32_init(kHASH_SEED_INIT);
    scanningRegions[kVmpMutantHash].hashState = XXH32_init(kHASH_SEED_INIT);
    scanningRegions[kRdataHash].closeHash = true;
    scanningRegions[kVmpPlainHash].closeHash = true;
    scanningRegions[kVmpMutantHash].closeHash = true;

    // diff-only region
    scanningRegions[kIatHash].hashState 
        = scanningRegions[kVmp0MiscHash].hashState
        = scanningRegions[kVmp1MiscHash].hashState
        = scanningRegions[kMiscHash].hashState 
        = scanningRegions[kMsvcHash].hashState 
        = scanningRegions[kNonGoldHashRot].hashState = XXH32_init(kHASH_SEED_INIT);

    // These regions are dll's, so addr/size is unknown
    scanningRegions[kMiscHash].useHashAddrSizeInStructHash 
        = scanningRegions[kMsvcHash].useHashAddrSizeInStructHash = false;

    size_t totalMemoryToScan = 0;
    for (size_t i = 0; i < scanningRegions.size(); ++i)
    {
        totalMemoryToScan += scanningRegions[i].size;
    }

    bytesPerStep = (totalMemoryToScan + kSteps - 1) / kSteps;
    bytesPerStep = kBlock*((bytesPerStep + kBlock - 1)/kBlock);

    // setup mechanism so that the currentHash is valid (complete one full
    // hash computation cycle).
    currentMemory = scanningRegions[0].startingAddress;
    for (unsigned int i = 0; i < kSteps; ++i) {
        step();
    }

    VMProtectEnd();
    hsceHashOrReduced = 0;
    hsceHashAndReduced = 0;
    hsceHashAndReduced = ~hsceHashAndReduced;
    updateHsceHash();
    #endif
}

    // Check for stealthedit. Stealthedit sets some pages to non-executable
    // and then catches the resulting exception, using the opportunity
    // to redirect to a modified page without disturbing the hash mechanism.
    // http://www.szemelyesintegracio.hu/cheats/41-game-hacking-articles/419-stealthedit	
bool ProgramMemoryChecker::areMemoryPagePermissionsSetupForHacking() 
{
    #if !defined(RBX_STUDIO_BUILD)
    VMProtectBeginMutation("12");
    ScanRegion region = ScanRegion::getScanRegion(NULL, ".text");

    MEMORY_BASIC_INFORMATION memInfo;
    if (VirtualQuery(region.startingAddress, &memInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
        return (memInfo.RegionSize < region.size) ||
            (memInfo.Protect != PAGE_EXECUTE_READ);
    }
    
    if (::checkG3dConsts())
    {
        RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag8, HATE_CONST_CHANGED);
    }
    
    VMProtectEnd();
    #endif
    return false;
}

unsigned int ProgramMemoryChecker::step() 
{
    const unsigned int kPrime32_1 = 2654435761U;
    const unsigned int kPrime32_2 = 2246822519U;

    bool allHashCompleted = false;
    #if !defined(RBX_STUDIO_BUILD)
    unsigned int bytesLeftForThisStep = bytesPerStep;
    unsigned int blocksLeftForThisStep = 1 + bytesPerStep/kBlock;
    while (blocksLeftForThisStep > 0)
    {
        VMProtectBeginMutation(NULL);
        ScanRegionTest& thisRegion = scanningRegions[currentRegion];
        const char* currentRegionEndAddress = thisRegion.startingAddress +
            thisRegion.size;
        const unsigned int fullBlocksLeft = (currentRegionEndAddress - currentMemory)/kBlock;
        const unsigned int blocksTaken = std::min(blocksLeftForThisStep, fullBlocksLeft);
        XXH_state32_t* const&  thisHashState = reinterpret_cast<XXH_state32_t*>(thisRegion.hashState);
        const char* volatile expectedCurrentMemory = currentMemory + blocksTaken*kBlock;

        // The hash function has been inlined here in a way that makes it more difficult to modify.
        // The member variables are copied because the compiler will favor registers over memory.
        // the code runs half as fast when using the member variables directly (vs2012)
        const char* const limit = currentMemory + kBlock*blocksTaken - kBlock;

        // This is a manual asm block designed to target a batch of exploits released xmas2015
        // (can't use constant variables as literals in asm)
//#define PMC_PUSHAD_EDI  0
//#define PMC_PUSHAD_ESI  4
//#define PMC_PUSHAD_EBP  8
#define PMC_PUSHAD_ESP 12
//#define PMC_PUSHAD_EBX 16
//#define PMC_PUSHAD_EDX 20
//#define PMC_PUSHAD_ECX 24
//#define PMC_PUSHAD_EAX 28
#define PMC_ROT 3
        __asm
        {
            mov eax, expectedCurrentMemory; // load value
            ror eax, PMC_ROT;               // make it look odd
            pushad;                         // This saves esp to [esp+12] and eax to [esp+28]
        }

        unsigned int v1 = thisHashState->v1;
        unsigned int v2 = thisHashState->v2;
        unsigned int v3 = thisHashState->v3;
        unsigned int v4 = thisHashState->v4;
        const char* p = currentMemory;
        VMProtectEnd();
        __asm
        {
            mov edx, esp;                    // junk       (orig_esp at esp+12)
            pop expectedCurrentMemory;       // add esp, 4 (orig_esp at esp+8)
            mov esp, [esp+PMC_PUSHAD_ESP-4]; // mov esp, orig_esp
        }

        // This is the core of the function and must execute in a tight loop.  As a result, it can't
        // be in the VMP section.  The address is added to the data to make it more robust to attacks
        // where the program is cloned. This part of the code should be small, as it is not protected
        // by VMProtect.
        while (currentMemory <= limit)
        {
            v1 += modifyArg<(RBX_BUILDSEED>>0)%4>(currentMemory) * kPrime32_2; v1 = _rotl(v1, 13); v1 *= kPrime32_1; currentMemory +=4;
            v2 += modifyArg<(RBX_BUILDSEED>>2)%4>(currentMemory) * kPrime32_2; v2 = _rotl(v2, 13); v2 *= kPrime32_1; currentMemory +=4;
            v3 += modifyArg<(RBX_BUILDSEED>>4)%4>(currentMemory) * kPrime32_2; v3 = _rotl(v3, 13); v3 *= kPrime32_1; currentMemory +=4;
            v4 += modifyArg<(RBX_BUILDSEED>>6)%4>(currentMemory) * kPrime32_2; v4 = _rotl(v4, 13); v4 *= kPrime32_1; currentMemory +=4;
        }  

        // allocate a little extra space on the stack
#define PMC_DEADSPACE 12
//#define PMC_NEWTOP   0
//#define PMC_UNUSED   4
#define PMC_SAVEDEAX   8
        __asm
        {
            lea esp, [esp-PMC_DEADSPACE]; // subtract 12 from esp
        }
        VMProtectBeginMutation(NULL);
        thisHashState->v1 = v1;
        thisHashState->v2 = v2;
        thisHashState->v3 = v3;
        thisHashState->v4 = v4;
        currentMemory = p;

        // This is a new version of the check for the xmas2015 thing
        const char* stackCurrentMemory;
        __asm
        {
            mov eax, esp;                 // switch registers
            mov eax, [eax+PMC_SAVEDEAX];  // look for the secret here at [esp-12+8] = [esp-4]
            rol eax, PMC_ROT;             // correct for the obfuscation
            mov stackCurrentMemory, eax;  // move this to the arg to check
            add esp, PMC_DEADSPACE;       // remove the extra stack space
        }

        thisHashState->total_len += kBlock*blocksTaken;
        blocksLeftForThisStep -= blocksTaken;

        // This can only occur at the end of a section
        const uintptr_t remainingAmount = currentRegionEndAddress - currentMemory;
        if (remainingAmount > 0 && remainingAmount < kBlock) 
        {
            // The PMC sends over a sequence of hashes.  There is a function on the server to determine
            // the 32b nonce that gets added, but only when the word is added after the end of a block
            // of data.  
            if (thisRegion.startingAddress == reinterpret_cast<char*>(&pmcHash.nonce))
            {
                XXH32_feed(thisRegion.hashState, currentMemory, remainingAmount);
            }
            else
            {
                int padLen = (0x80000000 - thisRegion.size) % kBlock;
                XXH32_feed(thisRegion.hashState, currentMemory, remainingAmount);
                XXH32_feed(thisRegion.hashState, zeroPad, padLen);
            }
        }

        // continue on from the manually inlined hash function above.
        if ( (currentMemory == currentRegionEndAddress) || (remainingAmount > 0 && remainingAmount < kBlock) )
        {
            if (thisRegion.closeHash)
            {
                thisRegion.lastHashValue = XXH32_result(thisRegion.hashState);
                thisRegion.hashState = XXH32_init(kHASH_SEED_INIT);

                // update the state pointers for related sections.
                for (int i = currentRegion-1; i >= 0; --i)
                {
                    if (!scanningRegions[i].closeHash)
                    {
                        scanningRegions[i].hashState = thisRegion.hashState;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                thisRegion.lastHashValue = XXH32_getIntermediateResult(thisRegion.hashState);
            }

            if (currentRegion == kGoldHashRot)
            {
                lastGoldenHash = this->hashScanningRegions(kGoldHashRot+1);
            }

            currentRegion = (currentRegion + 1) % scanningRegions.size();

            if (currentRegion == 0)
            {
                lastCompletedHash = hashScanningRegions();
                lastCompletedTime = Time::nowFast();
                getLastHashes(pmcHash.hash);
                if (currentMemory != stackCurrentMemory)
                {
                    // This increments the nonce by a slightly different amount.
                    pmcHash.nonce += kPmcNonceBadInc;
                }
                else
                {
                    pmcHash.nonce += kPmcNonceGoodInc;
                }
                allHashCompleted = true;
            }

            currentMemory = scanningRegions[currentRegion].startingAddress;
        }
        VMProtectEnd();
    }
    #endif

    return (ProgramMemoryChecker::kAllDone * allHashCompleted);
}

#pragma optimize("" , off)
int ProgramMemoryChecker::isLuaLockOk() const
{
#if !defined(RBX_STUDIO_BUILD) && defined(_WIN32)
    VMProtectBeginMutation(NULL);
    // This is an added check
    size_t checkStart = RBX::Security::rbxTextBase;
    size_t checkSize = RBX::Security::rbxTextSize;
    size_t pmcStart = reinterpret_cast<size_t>(scanningRegions[kGoldHashStart].startingAddress);
    size_t pmcSize = reinterpret_cast<size_t>(scanningRegions[kGoldHashEnd].startingAddress) 
        + scanningRegions[kGoldHashEnd].size - pmcStart;
    int returnValue;
    if (((checkSize != 0x01800000) && (checkSize != pmcSize)) 
        || ((checkStart != 0x00400000) && (checkStart != pmcStart)))
    {
        returnValue = ProgramMemoryChecker::kLuaLockBad;
    }
    else
    {
        returnValue = ProgramMemoryChecker::kLuaLockOk;
    }
    VMProtectEnd();
    return returnValue;
#else
    return ProgramMemoryChecker::kLuaLockOk;
#endif
}
#pragma optimize("" , on)

unsigned int ProgramMemoryChecker::getLastCompletedHash() const 
{
    return lastCompletedHash;
}

// If you got here while debugging, note that SW breakpoints and 
// stepping will change the code being hashed!
unsigned int ProgramMemoryChecker::getLastGoldenHash() const 
{
    return lastGoldenHash;
}

Time ProgramMemoryChecker::getLastCompletedTime() const 
{
    return lastCompletedTime;
}

void ProgramMemoryChecker::getLastHashes(PmcHashContainer::HashVector& outHashes) const 
{
    #if !defined(RBX_STUDIO_BUILD)
    outHashes.resize(kNumberOfHashes);
    for (int i = 0; i < kNumberOfHashes-2; ++i)
    {
        outHashes[i] = scanningRegions[i].lastHashValue;
    }
    outHashes[kGoldHashStruct] = lastGoldenHash;
    outHashes[kAllHashStruct] = lastCompletedHash;
    #endif
}

unsigned int ProgramMemoryChecker::hashScanningRegions(size_t regions) const
{
    #if !defined(RBX_STUDIO_BUILD)
    VMProtectBeginMutation("15");
    void* hashState = XXH32_init(kHASH_SEED_INIT);
    const size_t termRegion = scanningRegions.size();
    for (size_t i = 0; i < termRegion; ++i)
    {
        if (scanningRegions[i].useHashAddrSizeInStructHash)
        {
            XXH32_feed(hashState, &(scanningRegions[i].startingAddress), sizeof(unsigned int));
            XXH32_feed(hashState, &(scanningRegions[i].size), sizeof(unsigned int));
        }
        XXH32_feed(hashState, &(scanningRegions[i].closeHash), sizeof(bool));
        XXH32_feed(hashState, &(scanningRegions[i].useHashValueInStructHash), sizeof(bool));
        XXH32_feed(hashState, &(scanningRegions[i].useHashAddrSizeInStructHash), sizeof(bool));
        if ( (i < regions) && scanningRegions[i].useHashValueInStructHash)
        {
            XXH32_feed(hashState, &(scanningRegions[i].lastHashValue), sizeof(unsigned int));
        }
    }
    VMProtectEnd();
    return XXH32_result(hashState);
    #else
    return 0;
    #endif
}

unsigned int ProgramMemoryChecker::updateHsceHash()
{
#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD)
    VMProtectBeginMutation(NULL);
    XXH_state32_t hsceHashState;
    hsceHashState.v1 = hsceHashState.v2 = hsceHashState.v3 = hsceHashState.v4 = 0xCCCCCCCC;
    hsceHashState.seed = 0xCCCCCCCC;
    hsceHashState.memsize = 0;
    hsceHashState.total_len = 0;
    const void* hsceBaseAddress = RBX::HUMAN::HumanoidState::getComputeEventBaseAddress();
    if (hsceBaseAddress)
    {
        XXH32_feed(&hsceHashState, hsceBaseAddress, 1024);
    }
    unsigned int hsceHash = XXH32_getIntermediateResult(&hsceHashState);
    hsceHashOrReduced |= hsceHash;
    hsceHashAndReduced &= hsceHash;
    VMProtectEnd();
    return hsceHash;
#else
    return 0;
#endif
}

unsigned int ProgramMemoryChecker::getHsceOrHash() const
{
    return hsceHashOrReduced;
}

unsigned int ProgramMemoryChecker::getHsceAndHash() const
{
    return hsceHashAndReduced;
}

unsigned int protectVmpSections()
{
    HMODULE thisModule = GetModuleHandle(NULL);
    MODULEINFO thisModuleInfo;
    GetModuleInformation(GetCurrentProcess(), thisModule, &thisModuleInfo, sizeof(thisModuleInfo));
    uintptr_t addr = reinterpret_cast<uintptr_t>(thisModule);
    uintptr_t termAddr = addr + thisModuleInfo.SizeOfImage;
    MEMORY_BASIC_INFORMATION thisMemInfo;
    while (addr < termAddr)
    {
        VirtualQuery(reinterpret_cast<void*>(addr), &thisMemInfo, sizeof(thisMemInfo));
        if (thisMemInfo.State == MEM_COMMIT && (thisMemInfo.Protect & (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        {
            DWORD unused;
            VirtualProtect(reinterpret_cast<void*>(addr), thisMemInfo.RegionSize, PAGE_EXECUTE_READ, &unused);
        }
        addr = reinterpret_cast<uintptr_t>(thisMemInfo.BaseAddress) + thisMemInfo.RegionSize;
    }

    // For some reason, a follow-on security check fails.
    // The vmp sections are contiguous, and between non-exe sections
    VirtualQuery(reinterpret_cast<void*>(RBX::Security::rbxVmpBase), &thisMemInfo, sizeof(thisMemInfo));
    if (thisMemInfo.State != MEM_COMMIT || !(thisMemInfo.Protect & PAGE_EXECUTE_READ))
    {
        return RBX::Security::rbxVmpSize;
    }
    return thisMemInfo.RegionSize - RBX::Security::rbxVmpSize;
}

}
