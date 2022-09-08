//
//  LoginScreenController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/20/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "NonRotatableViewController.h"

typedef enum : NSUInteger {
    LoginScreenDismissalUnknown = 0,
    LoginScreenDismissalCancelled = 1,
    LoginScreenDismissalLoginSuccess = 2,
    LoginScreenDismissalLoginFailed = 3
} LoginScreenDismissalType;

typedef void(^DismissalCompletionHandler)(LoginScreenDismissalType dismissType, NSError *loginError);

@interface LoginScreenController : NonRotatableViewController <UIGestureRecognizerDelegate>

@property (nonatomic, copy) DismissalCompletionHandler dismissalCompletionHandler;

-(void) setUsername:(NSString*)username andPassword:(NSString*)password;

@end
