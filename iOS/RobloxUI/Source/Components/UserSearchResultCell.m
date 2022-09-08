//
//  GameSearchResultCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/10/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "UserSearchResultCell.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"

@implementation UserSearchResultCell

+(CGSize) getCellSize
{
    return [RobloxInfo thisDeviceIsATablet] ? CGSizeMake(160, 215) : CGSizeMake(140, 215);
}
+(NSString*)getNibName
{
    return [RobloxInfo thisDeviceIsATablet] ? @"UserSearchResultCell" : @"UserSearchResultCell_iPhone";
}
-(void) awakeFromNib
{
    [super awakeFromNib];
    
    [_lblName setTextColor:[UIColor whiteColor]];
    
    self.clipsToBounds = NO;
    self.backgroundColor = [UIColor clearColor];
}

- (void)setInfo:(RBXUserSearchInfo *)searchInfo
{
    _searchInfo = searchInfo;
    if (_searchInfo)
    {
        [_lblName setText:_searchInfo.userName];
        _lblName.hidden = NO;
        
        [self addSubview:_imgLoadingSpinner];
        [_imgLoadingSpinner centerInFrame:_imgAvatar.frame];
        [_imgLoadingSpinner startAnimating];
        
        _imgAvatar.animateInOptions = RBXImageViewAnimateInAlways;
        [_imgAvatar loadAvatarForUserID:_searchInfo.userId.intValue
                          prefetchedURL:_searchInfo.avatarURL
                             urlIsFinal:_searchInfo.avatarIsFinal
                               withSize:[RobloxTheme sizeProfilePictureMedium]
                             completion:^(void)
         {
             dispatch_async(dispatch_get_main_queue(), ^
             {
                 _imgAvatar.hidden = NO;
                 _imgAvatar.layer.cornerRadius = 5;
                 _imgAvatar.layer.masksToBounds = YES;
                 [_imgLoadingSpinner stopAnimating];
                 [_imgLoadingSpinner removeFromSuperview];
             });
         }];
        
        NSString* imageName = _searchInfo.isOnline ? @"User Online" : @"User Offline";
        [_imgIsOnline setImage:[UIImage imageNamed:imageName]];
        _imgIsOnline.hidden = NO;
    }
    else
    {
        [_lblName setText:@""];
        [_imgAvatar setHidden:YES];
        //[_imgAvatar loadAvatarForUserID:-1 withSize:[RobloxTheme sizeProfilePictureMedium] completion:nil];
        [_imgIsOnline setImage:[UIImage imageNamed:@"User Offline"]];
    }
}
@end
