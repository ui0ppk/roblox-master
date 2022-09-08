//
//  PlayerThumbnailCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBPlayerThumbnailCell.h"
#import "RobloxTheme.h"

#define FRIEND_AVATAR_SIZE CGSizeMake(110, 110)

@implementation RBPlayerThumbnailCell

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [RobloxTheme applyRoundedBorderToView:self];
    }
    return self;
}

- (void)setFriendInfo:(RBXFriendInfo *)friendInfo
{
    _friendInfo = friendInfo;
    
    [RobloxTheme applyToFriendCellLabel:self.nameLabel];
    self.nameLabel.backgroundColor = [UIColor whiteColor];
    
    if(_friendInfo)
    {
        self.nameLabel.text = friendInfo.username;
        
        self.avatar.animateInOptions = RBXImageViewAnimateInAlways;
        [self.avatar loadAvatarForUserID:[friendInfo.userID integerValue] prefetchedURL:friendInfo.avatarURL urlIsFinal:friendInfo.avatarIsFinal withSize:FRIEND_AVATAR_SIZE completion:nil];
        
        NSString* imageName = friendInfo.isOnline ? @"User Online" : @"User Offline";
        [self.isOnlineMarker setImage:[UIImage imageNamed:imageName]];
    }
    else
    {
        self.nameLabel.text = @"";
        self.avatar.image = nil;
        self.isOnlineMarker.image = nil;
    }
}

@end
