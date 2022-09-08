#ifndef ROBLOXINFO_H
#define ROBLOXINFO_H

#include <iostream>

#include "rbx/Debug.h"

class RobloxInfo
{
public:

    static RobloxInfo& getRobloxInfo()
    {
        static RobloxInfo robloxInfo;
        return robloxInfo;
    }

    std::string getAssetFolderPath()
    {
        if(assetFolderPath.empty())
            RBXASSERT("Asset Path is not Set");

        return assetFolderPath;
    }

    void setAssetFolderPath(std::string assetPath)
    {
        assetFolderPath = assetPath;
    }

private:
    RobloxInfo() {};
    RobloxInfo(RobloxInfo const&);
    void operator=(RobloxInfo const&);
    std::string assetFolderPath;

};

#endif // ROBLOXINFO_H
