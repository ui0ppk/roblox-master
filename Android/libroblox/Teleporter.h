#pragma once

#include "v8datamodel/TeleportCallback.h"
#include "FunctionMarshaller.h"

class PlaceLauncher;
class Teleporter: public RBX::TeleportCallback
{
    RBX::FunctionMarshaller* mMarshaller;
    
    static void teleportImpl(std::string url, std::string ticket, std::string script);

public:
    Teleporter(RBX::FunctionMarshaller* marshaller): mMarshaller(marshaller)
    {
    }
    
    virtual void doTeleport(const std::string& url, const std::string& ticket, const std::string& script)
    {
        mMarshaller->Submit(boost::bind(teleportImpl, url, ticket, script));
    }
    
    virtual bool isTeleportEnabled() const { return true; }
};
