//
//  LeaderboardsViewController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"

@interface LeaderboardsViewController : UIViewController

@property (nonatomic) RBXLeaderboardsTimeFilter timeFilter;
@property (strong, nonatomic) NSNumber* distributorID;

// Callback
@property (copy, nonatomic) void (^onLeaderboardsLoaded)(BOOL hasLeaderboards);

- (void) updateLeaderboards;

@end
