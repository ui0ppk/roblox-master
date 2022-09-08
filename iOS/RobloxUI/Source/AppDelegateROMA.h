//
//  AppDelegate.h
//  RobloxMobile
//
//  Created by David York on 9/20/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

#include "PlaceLauncher.h"

@interface AppDelegateROMA : UIResponder <UIApplicationDelegate>
{
    rbx::signals::scoped_connection messageOutConnection;
}

@property (strong, nonatomic) UIWindow *window;
@property (assign) UIBackgroundTaskIdentifier bgTask;

@end
