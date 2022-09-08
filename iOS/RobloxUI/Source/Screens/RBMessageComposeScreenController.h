//
//  RBMessageComposeScreenController.h
//  RobloxMobile
//
//  Created by alichtin on 9/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"

@interface RBMessageComposeScreenController : UIViewController

- (void) replyToMessage:(RBXMessageInfo*)message;

- (void) sendMessageToUser:(NSNumber*)userID name:(NSString*)userName;

@end
