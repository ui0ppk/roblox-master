#include "util/MemoryStats.h"

#include <fstream>
#include <sstream>
#include <string>
#include <regex>

namespace RBX
{
using RBX::MemoryStats::memsize_t;

class MemInfo
{
    // Next 3 fields summed define how much memory is actually free.
    memsize_t cacheKB;
    memsize_t bufferKB;
    memsize_t memfreeKB;

    memsize_t totalKB;

    bool readLine(const std::string& line, const std::string& target, memsize_t* field)
    {
        if (0 == line.find(target))
        {
            std::string expr = target + "\\s*(\\d+).*";
            std::regex e(expr);
            std::smatch sm;
            if (std::regex_search(line, sm, e)) {
                if (sm.size() >= 1) {
                    std::string o = sm[1].str(); // gets the first capture group
                    std::sscanf(o.c_str(), "%llu", field);
                }
            }
            
            return true;
        }
        return false;
    }

public:
    MemInfo() : cacheKB(1), bufferKB(1), memfreeKB(1), totalKB(3)
    {
        std::ifstream fin("/proc/meminfo");

        unsigned fieldsLoaded = 0;
        while (fin && !fin.eof() && fieldsLoaded < 4)
        {
            std::string line;
            std::getline(fin, line);

            if (readLine(line, "MemTotal:", &totalKB))
            {
                ++fieldsLoaded;
            }

            if (readLine(line, "MemFree:", &memfreeKB))
            {
                ++fieldsLoaded;
            }

            if (readLine(line, "Buffers:", &bufferKB))
            {
                ++fieldsLoaded;

            }

            if (readLine(line, "Cached:", &cacheKB))
            {
                ++fieldsLoaded;
            }
        }
    }

    memsize_t freeBytes() const
    {
        return (memfreeKB + bufferKB + cacheKB) * 1024;
    }

    memsize_t totalBytes() const
    {
        return totalKB * 1024;
    }
};
} // namespace rbx

namespace RBX {
namespace MemoryStats {

memsize_t usedMemoryBytes()
{
    MemInfo mi;
    return mi.totalBytes() - mi.freeBytes();
}

memsize_t freeMemoryBytes()
{
    MemInfo mi;
    return mi.freeBytes();
}

memsize_t totalMemoryBytes()
{
    MemInfo mi;
    return mi.totalBytes();
}

} // namespace MemoryStats
} // namespace RBX