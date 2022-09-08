//
//  FriendsRequestsDetailController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/19/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface FriendsRequestsDetailController : UIViewController

- (void) loadData;
- (void) refreshRequests:(UIRefreshControl*)refreshControl;

@end
