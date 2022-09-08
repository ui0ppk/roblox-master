//
//  Teleporter.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 1/30/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "PlaceLauncher.h"

#include "v8datamodel/TeleportCallback.h"
#include "FunctionMarshaller.h"

class Teleporter: public RBX::TeleportCallback
{
    PlaceLauncher* mLauncher;
    RBX::FunctionMarshaller* mMarshaller;
    
    static void teleportImpl(PlaceLauncher* launcher, std::string url, std::string ticket, std::string script)
    {
        NSString* urlString = [NSString stringWithCString:url.c_str() encoding:[NSString defaultCStringEncoding]];
        NSString* ticketString = [NSString stringWithCString:ticket.c_str() encoding:[NSString defaultCStringEncoding]];
        NSString* scriptString = [NSString stringWithCString:script.c_str() encoding:[NSString defaultCStringEncoding]];
        
        [launcher teleport:ticketString withAuthentication:urlString withScript:scriptString];
    }
    
public:
    Teleporter(PlaceLauncher* launcher, RBX::FunctionMarshaller* marshaller): mLauncher(launcher), mMarshaller(marshaller)
    {
    }
    
    virtual void doTeleport(const std::string& url, const std::string& ticket, const std::string& script)
    {
        mMarshaller->Submit(boost::bind(teleportImpl, mLauncher, url, ticket, script));
    }
    
    virtual bool isTeleportEnabled() const { return true; }
};
