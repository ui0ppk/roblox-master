//
//  AppDelegate.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 10/9/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "AppDelegate.h"
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

#define HAS_SHOWN_KEY "HasShownBefore"

@interface AppDelegate : UIResponder <UIApplicationDelegate>
{
    UIBackgroundTaskIdentifier bgTask;
    BOOL leavingGame;
}

@property (strong, nonatomic) UIWindow *window;
@property (assign) UIBackgroundTaskIdentifier bgTask;

@end
