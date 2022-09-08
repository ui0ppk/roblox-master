//
//  RBProfileViewController.h
//  RobloxMobile
//
//  Created by Christian Hresko on 6/6/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RBBaseViewController.h"
#import "RobloxData.h"

@interface RBProfileViewController : RBBaseViewController<UICollectionViewDelegate, UICollectionViewDataSource>

@property (strong, nonatomic) NSNumber* userId;
@property (strong, nonatomic) RBXUserProfileInfo* profile;

- (void)updateFriendshipButton;
- (void)updateFollowButton;
@end