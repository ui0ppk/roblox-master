#ifndef ROBLOXUTILITIES_H
#define ROBLOXUTILITIES_H

#include "rbx/rbxTime.h"

class RobloxUtilities
{

public:

    static RobloxUtilities& getRobloxUtilities()
    {

        static RobloxUtilities robloxUtilities;
        return robloxUtilities;
    }

    RBX::Timer<RBX::Time::Fast>& getGameTimer() { return gameTimer; };


private:
    RobloxUtilities() {};
    RobloxUtilities(RobloxUtilities const&);
    void operator=(RobloxUtilities const&);

    RBX::Timer<RBX::Time::Fast> gameTimer;
};



#endif // ROBLOXUTILITIES_H
