#include "Util/ProgramMemoryChecker.h"
#include "rbx/rbxTime.h"
#include "Util/MachOBaseAddr.h"
#include "Util/xxhash.h"

#if defined(__has_feature)
#	define ADDRESS_SANITIZER __has_feature(address_sanitizer)
#else
#	define ADDRESS_SANITIZER defined(__SANITIZE_ADDRESS__)
#endif

namespace RBX
{
using namespace Hasher;
PmcHashContainer pmcHash;

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
}

ScanRegion ScanRegion::getScanRegion(const char* module, const char* RegionName)
{
    return ScanRegion();
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
#if !defined(RBX_STUDIO_BUILD) && !defined(__ANDROID__) && !defined(RBX_PLATFORM_IOS)
    uint32_t baseAddr = machODynamicBaseAddress();
    uint32_t baseSize = machOTextSize();
    scanningRegions.resize(kNumberOfSectionHashes);
    
    // Set up the main hash.
    scanningRegions[kGoldHashStart] = ScanRegionTest(ScanRegion((char*)(baseAddr), baseSize));
    scanningRegions[kGoldHashStart].hashState = XXH32_init(kHASH_SEED_INIT);
    scanningRegions[kGoldHashStart].useHashValueInStructHash = true;
    scanningRegions[kGoldHashStart].closeHash = false;
    currentMemory = scanningRegions[0].startingAddress;
    
    // Set up rotational hash (and others)
    ScanRegionTest nonceLocation( ScanRegion(((char*) (&pmcHash.nonce)), sizeof(uint32_t)));
    nonceLocation.closeHash = false;
    nonceLocation.useHashValueInStructHash = false;
    nonceLocation.hashState = scanningRegions[0].hashState;
    for (int i = 1; i < scanningRegions.size(); ++i)
    {
        scanningRegions[i] = nonceLocation;
    }
    scanningRegions[scanningRegions.size()-1].closeHash = true;
    
    size_t totalMemoryToScan = 0;
    for (size_t i = 0; i < scanningRegions.size(); ++i)
    {
        totalMemoryToScan += scanningRegions[i].size;
    }
    
    bytesPerStep = (totalMemoryToScan + kSteps - 1) / kSteps;
    
    // setup mechanism so that the currentHash is valid (complete one full
    // hash computation cycle).
    for (unsigned int i = 0; i < kSteps; ++i) {
        step();
    }
#endif
}

bool ProgramMemoryChecker::areMemoryPagePermissionsSetupForHacking() 
{
    return false;
}

unsigned int ProgramMemoryChecker::getLastCompletedHash() const
{
    return lastCompletedHash;
}

unsigned int ProgramMemoryChecker::step() 
{
#if !defined(RBX_STUDIO_BUILD) && !defined(__ANDROID__) && !defined(RBX_PLATFORM_IOS) && !ADDRESS_SANITIZER
    unsigned int bytesLeftForThisStep = bytesPerStep;
    //bool allHashCompleted = false;
    
    while (bytesLeftForThisStep > 0)
    {
        ScanRegionTest& thisRegion = scanningRegions[currentRegion];
        const char* currentRegionEndAddress = thisRegion.startingAddress +
        thisRegion.size;
        const unsigned int memoryLeftInRegion = currentRegionEndAddress - currentMemory;
        const unsigned int bytesTakenFromCurrentRegion = std::min(bytesLeftForThisStep, memoryLeftInRegion);
        void* localState = thisRegion.hashState;
        
        if (thisRegion.size)
        {
            XXH32_feed(localState, currentMemory, bytesTakenFromCurrentRegion);
        }
        bytesLeftForThisStep -= bytesTakenFromCurrentRegion;
        currentMemory += bytesTakenFromCurrentRegion;
        
        if (currentMemory == currentRegionEndAddress)
        {
            if (thisRegion.closeHash && thisRegion.size)
            {
                thisRegion.lastHashValue = XXH32_result(localState);
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
            else if (thisRegion.size)
            {
                // C++, -1 % 16 is not specifically defined
                int padLen = (0x80000000 - thisRegion.size) % 16;
                if (padLen)
                {
                    XXH32_feed(localState,zeroPad,padLen);
                }
                thisRegion.lastHashValue = XXH32_getIntermediateResult(localState);
            }
            
            currentRegion = (currentRegion + 1) % scanningRegions.size();
            currentMemory = scanningRegions[currentRegion].startingAddress;
            
            if (currentRegion == 0)
            {
                lastCompletedHash = hashScanningRegions();
                lastCompletedTime = Time::nowFast();
                getLastHashes(pmcHash.hash);
                ++pmcHash.nonce;
                //allHashCompleted = true;
            }
        }
    }
#endif
    return 0;
}

// If you got here while debugging, note that SW breakpoints and 
// stepping will change the code being hashed!
unsigned int ProgramMemoryChecker::getLastGoldenHash() const 
{
    return lastCompletedHash;
}

Time ProgramMemoryChecker::getLastCompletedTime() const 
{
    return Time::nowFast();
}

void ProgramMemoryChecker::getLastHashes(PmcHashContainer::HashVector& outHashes) const
{
#if !defined(RBX_STUDIO_BUILD) && !defined(__ANDROID__) && !defined(RBX_PLATFORM_IOS)
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
#if !defined(RBX_STUDIO_BUILD) && !defined(__ANDROID__) && !defined(RBX_PLATFORM_IOS)
    void* hashState = XXH32_init(kHASH_SEED_INIT);
    const size_t termRegion = std::min(regions, scanningRegions.size());
    for (size_t i = 0; i < termRegion; ++i)
    {
        XXH32_feed(hashState, &(scanningRegions[i].startingAddress), sizeof(unsigned int));
        XXH32_feed(hashState, &(scanningRegions[i].size), sizeof(unsigned int));
        XXH32_feed(hashState, &(scanningRegions[i].closeHash), sizeof(bool));
        XXH32_feed(hashState, &(scanningRegions[i].useHashValueInStructHash), sizeof(bool));
        if (scanningRegions[i].useHashValueInStructHash)
        {
            XXH32_feed(hashState, &(scanningRegions[i].lastHashValue), sizeof(unsigned int));
        }
    }
    return XXH32_result(hashState);
#else
    return 0;
#endif
}

int ProgramMemoryChecker::isLuaLockOk() const
{
    return ProgramMemoryChecker::kLuaLockOk;
}

}
