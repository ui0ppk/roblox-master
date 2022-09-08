//
//  HomeScreenController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RBXEventReporter.h"

@interface HomeScreenController : UITabBarController<UITabBarControllerDelegate>

-(RBXAnalyticsCustomData) getCurrentTabContext;

@end
