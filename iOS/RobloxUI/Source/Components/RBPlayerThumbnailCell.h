//
//  PlayerThumbnailCell.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "RobloxImageView.h"
#import "RobloxData.h"

@interface RBPlayerThumbnailCell : UICollectionViewCell

@property(strong, nonatomic) RBXFriendInfo* friendInfo;

@property(weak, nonatomic) IBOutlet RobloxImageView* avatar;
@property(weak, nonatomic) IBOutlet UILabel* nameLabel;
@property(weak, nonatomic) IBOutlet UIImageView* isOnlineMarker;

@end
