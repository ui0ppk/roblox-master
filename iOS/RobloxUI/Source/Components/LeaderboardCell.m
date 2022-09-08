//
//  LeaderboardCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/6/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "LeaderboardCell.h"
#import "RobloxImageView.h"

#define AVATAR_SIZE CGSizeMake(110, 110)

@implementation LeaderboardCell
{
}

- (void) showPlayerInfo:(RBXLeaderboardEntry*)data
{
	_rankLabel.text = data.displayRank;
    _titleLabel.text = data.name;
    
	if(data.clanName != (id)[NSNull null])
	{
        _subtitleLabel.text = data.clanName;
	}
    else
	{
        _subtitleLabel.text = @"";
	}
	
	_avatar.animateInOptions = RBXImageViewAnimateInAlways;
    [_avatar loadAvatarForUserID:data.userID prefetchedURL:data.userAvatarURL urlIsFinal:data.userAvatarIsFinal withSize:AVATAR_SIZE completion:nil];
	
    _pointsLabel.text = data.displayPoints;
}

- (void) showClanInfo:(RBXLeaderboardEntry*)data
{
	_rankLabel.text = data.displayRank;
    _titleLabel.text = data.clanName;
    _subtitleLabel.text = data.name;
    _pointsLabel.text = data.displayPoints;

	_avatar.animateInOptions = RBXImageViewAnimateInAlways;
	[_avatar loadAvatarForUserID:data.clanEmblemID prefetchedURL:data.clanAvatarURL urlIsFinal:data.clanAvatarIsFinal withSize:AVATAR_SIZE completion:nil];
}

@end
