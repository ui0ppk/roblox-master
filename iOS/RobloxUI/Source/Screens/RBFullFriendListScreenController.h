//
//  RBFullFriendListScreenController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "RobloxData.h"

@interface RBFullFriendListScreenController : UIViewController

typedef NS_ENUM(NSUInteger, SeeAllListType)
{
    RBListTypeFriends = 0,
    RBListTypeFollowers = 1,
    RBListTypeFollowing = 2
};

@property (strong, nonatomic) NSNumber* playerID;
@property (strong, nonatomic) NSString* playerName;
@property (nonatomic) SeeAllListType listTypeValue;

@end
