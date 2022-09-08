#include "stdafx.h"

#include "Util/ProgramMemoryChecker.h"


namespace RBX
{
using namespace Hasher;
// I'm very sorry for this.  I have no idea how to find the location of an int
// in the executable.  However, strings that are obvious can be found easier.
// This will actually be an unsigned int* after the exe is patched.

_declspec(align(8)) const char* const maskAddr = "1(m$n9.[?y`z+f : a&Rn5<d*nGD9.@93Dr7&"; // these are gibberish.
_declspec(align(8)) const char* const goldHash = "1N9S6,*%D-&m8sH%/~m _qvZ=&be*db elI^s"; // no meaning.

PmcHashContainer pmcHash;

namespace Security{
    // The storage for hash checker related security constants.
    volatile const size_t rbxGoldHash = 0;
}

ScanRegion ScanRegion::getScanRegion(const char* moduleName, const char* RegionName)
{
    return ScanRegion();
}

ProgramMemoryChecker::ProgramMemoryChecker()
    : currentRegion(0)
    , currentMemory(NULL)
    , scanningRegions()
    , lastCompletedHash(0)
    , lastGoldenHash(0)
    , lastCompletedTime(Time::nowFast())
{
}

    // Check for stealthedit. Stealthedit sets some pages to non-executable
    // and then catches the resulting exception, using the opportunity
    // to redirect to a modified page without disturbing the hash mechanism.
    // http://www.szemelyesintegracio.hu/cheats/41-game-hacking-articles/419-stealthedit	
bool ProgramMemoryChecker::areMemoryPagePermissionsSetupForHacking() 
{
    return false;
}

unsigned int ProgramMemoryChecker::step() 
{
    bool allHashCompleted = false;
    return (ProgramMemoryChecker::kAllDone * allHashCompleted);
}

int ProgramMemoryChecker::isLuaLockOk() const
{
    return ProgramMemoryChecker::kLuaLockOk;
}

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
}

unsigned int ProgramMemoryChecker::hashScanningRegions(size_t regions) const
{
    return 0;
}

unsigned int ProgramMemoryChecker::updateHsceHash()
{
    return 0;
}

unsigned int ProgramMemoryChecker::getHsceOrHash() const
{
    return hsceHashOrReduced;
}

unsigned int ProgramMemoryChecker::getHsceAndHash() const
{
    return hsceHashAndReduced;
}

void dumpSection (const char* regionName, int hash, int goldHash, const char* fileName)
{
}

PmcHashContainer::PmcHashContainer(const PmcHashContainer& init)
{

}

}
