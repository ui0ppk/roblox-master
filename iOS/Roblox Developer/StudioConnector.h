//
//  StudioConnector.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/19/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "GCDAsyncUdpSocket.h"

@interface StudioConnector : NSObject <GCDAsyncUdpSocketDelegate>
{
    bool hasPaired;
    NSString* codeToUse;
    GCDAsyncUdpSocket *udpSocket;
    NSData* hostIPAddress;
    NSString* hostIPAddressString;
    NSString* didPairToHost;
}

+ (id)sharedInstance;

-(void) tryToPairWithStudioUsingCurrentCode;
-(void) tryToPairWithStudio:(NSString*) newCode;

-(void) stopPairing;
-(void) clearPairCode;

-(NSString*) getPairEndedNotificationString;

- (NSString *)hostnameForAddress:(NSString *)address;
@end
