//
//  GameSearchResultCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/10/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameSearchResultCell.h"
#import "RobloxImageView.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RBActivityIndicatorView.h"

@implementation GameSearchResultCell
{
    IBOutlet RobloxImageView* _thumbnail;
    IBOutlet UILabel *_title;
    IBOutlet RBActivityIndicatorView *_activityIndicator;
}

+(CGSize)getCellSize
{
    return [RobloxInfo thisDeviceIsATablet] ? CGSizeMake(160, 220) : CGSizeMake(140, 215);
}
+(NSString*)getNibName
{
    return [RobloxInfo thisDeviceIsATablet] ? @"GameSearchResultCell" : @"GameSearchResultCell_iPhone";
}

- (void)awakeFromNib
{
	[RobloxTheme applyToGameSearchToolbarCell:_title];
    _thumbnail.animateInOptions = RBXImageViewAnimateInIfNotCached;
    _thumbnail.layer.cornerRadius = 5;
    _thumbnail.layer.masksToBounds = YES;
    _thumbnail.layer.borderColor = [UIColor whiteColor].CGColor;
    _thumbnail.layer.borderWidth = 1;
    
    _title.hidden = YES;
    _activityIndicator.hidden = NO;
}

- (void)setGameData:(RBXGameData *)gameData
{
    if (gameData)
    {
        _gameData = gameData;
        _title.text = gameData.title;
        _title.hidden = NO;
        
        [_activityIndicator startAnimating];
        [_thumbnail loadThumbnailForGame:gameData withSize:[RobloxTheme sizeGameCoverSquare] completion:^{
            dispatch_async(dispatch_get_main_queue(), ^
           {
               [_activityIndicator stopAnimating];
               _thumbnail.hidden = NO;
           });
        }];
    }
    else
    {
        _title.hidden = YES;
        _thumbnail.hidden = YES;
        [_activityIndicator startAnimating];
    }
    
}

@end
