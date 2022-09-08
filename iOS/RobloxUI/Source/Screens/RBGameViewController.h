//
//  RBGameViewController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/22/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PlaceLauncher.h"

typedef void(^RBGameGameCompletionHandler)();

@interface RBGameViewController : UIViewController

@property RBXGameLaunchParams* launchParams;
@property (nonatomic, copy) RBGameGameCompletionHandler completionHandler;

- (id) initWithLaunchParams:(RBXGameLaunchParams*)parameters;

+ (BOOL) isAppRunning;

@end
