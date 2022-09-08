//
//  GameKitHelper.h
//  RobloxMobile
//
//  Created by Ganesh on 5/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

//   Include the GameKit framework
#import <GameKit/GameKit.h>


@interface GameKitHelper : NSObject
{
    NSString* gameCenterActiveNotification;
    NSString* gameCenterDisabledNotification;
}

+ (id) sharedInstance;
-(void) authenticateLocalPlayer;
-(NSString*) getGameCenterActiveNotification;
-(NSString*) getGameCenterDisabledNotification;


@end
