#pragma once

#include <string>

namespace RBX {

// The TeleportCallback describes the interface required for teleporting
// players across different places. You have to define a child class and
// implement your own version of doTeleport.
class TeleportCallback
{
public:
	virtual ~TeleportCallback() {}
	virtual void doTeleport(const std::string& url, const std::string& ticket,
		const std::string& script) = 0;
    virtual bool isTeleportEnabled() const = 0;
    virtual std::string xBox_getGamerTag() const { return ""; } // needed for placeLauncher.ashx when connecting from xbox client
};
 
}  // namespace RBX
