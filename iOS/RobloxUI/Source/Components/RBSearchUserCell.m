//
//  RBSearchUserCell.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 1/14/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBSearchUserCell.h"
#import "RobloxTheme.h"
#import "RBActivityIndicatorView.h"
#import "UIView+Position.h"
#import "RobloxInfo.h"

#define DEFAULT_CELL_HEIGHT 154

@interface RBSearchUserCell ()

@end

@implementation RBSearchUserCell

+ (CGSize) getCellSize
{
    return [RobloxInfo thisDeviceIsATablet] ? CGSizeMake(514, 154) : CGSizeMake(300, 200);
}
+ (NSString*) getNibName
{
    return [RobloxInfo thisDeviceIsATablet] ? @"RBSearchUserCell" : @"RBSearchUserCell_iPhone";
}

- (void)awakeFromNib
{
    dispatch_async(dispatch_get_main_queue(), ^
   {
       [RobloxTheme applyRoundedBorderToView:self];
       self.backgroundColor = [UIColor whiteColor];
       
       [_lblName setText:@""];
       [_lblBlurb setText:@""];
       [_lblOtherNames setText:@""];
       _imgAvatar.image = nil;
       _imgIsOnline.image = nil;
       
       self.clipsToBounds = YES;
   });
}

-(void) setInfo:(RBXUserSearchInfo *)searchInfo
{
    _searchInfo = searchInfo;
    
    if(_searchInfo)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            [_lblName setText:_searchInfo.userName];
            [_lblBlurb setText:_searchInfo.blurb];
            [_lblOtherNames setText:_searchInfo.previousNames];
            [_lblOtherNamesWord setText:NSLocalizedString(@"UsersPreviousNamesWord", nil)];
            
            NSString* imageName = _searchInfo.isOnline ? @"User Online" : @"User Offline";
            [_imgIsOnline setImage:[UIImage imageNamed:imageName]];
            _imgIsOnline.hidden = NO;
            
            //check if there are other names, hide them if there are not
            bool shouldHideOtherNames = _searchInfo.previousNames.length < 3;
            _lblOtherNames.hidden = shouldHideOtherNames;
            _lblOtherNamesWord.hidden = shouldHideOtherNames;
            
            _lblName.hidden = NO;
            _lblBlurb.hidden = NO;
            
            
            if ([RobloxInfo thisDeviceIsATablet])
            {
                //extend the text areas if there are no other names
                CGFloat margin =  _lblBlurb.y - (_lblOtherNamesWord.y + _lblOtherNamesWord.height);
                CGFloat blurbHeight = 90;
                [_lblBlurb setY:shouldHideOtherNames ? _lblOtherNamesWord.y : _lblOtherNamesWord.y + _lblOtherNamesWord.height + margin];
                [_lblBlurb setHeight:shouldHideOtherNames ? (blurbHeight + _lblOtherNamesWord.height + margin) : blurbHeight];
            }
        });
        
        
        //load the avatar image
        RBActivityIndicatorView* spinner = [[RBActivityIndicatorView alloc] initWithFrame:_imgAvatar.frame];
        [self addSubview:spinner];
        [spinner startAnimating];
        
        _imgAvatar.animateInOptions = RBXImageViewAnimateInAlways;
        [_imgAvatar loadAvatarForUserID:_searchInfo.userId
                          prefetchedURL:_searchInfo.avatarURL
                             urlIsFinal:_searchInfo.avatarIsFinal
                               withSize:[RobloxTheme sizeProfilePictureLarge]
                             completion:^
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _imgAvatar.hidden = NO;
                [spinner stopAnimating];
                [spinner removeFromSuperview];
            });
            
        }];
            
        
    }
}

@end
