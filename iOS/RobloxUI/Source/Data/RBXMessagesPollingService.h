//
//  RBXNotificationService.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/17/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RobloxNotifications.h"
#import "RobloxData.h"
#import "UserInfo.h"

@interface RBXMessagesPollingService : NSObject

//constructors
+(id) sharedInstance;
-(id) init;
-(void) clean;

//properties
@property (readonly) bool requestCompleted;
@property (readonly) int totalMessages;
@property (readonly) int totalFriendRequests;
@property (readonly, strong) NSTimer* notificationTimer;

//service functions
-(void) beginPolling;
-(void) pausePolling;
@end
