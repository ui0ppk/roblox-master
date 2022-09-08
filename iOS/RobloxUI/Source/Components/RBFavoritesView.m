//
//  RBFavoritesView
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBFavoritesView.h"
#import "UIView+Position.h"
#import "RobloxData.h"
#import "RobloxHUD.h"
#import "RobloxTheme.h"

#define BAR_HEIGHT 4.0f
#define HORIZONTAL_MARGIN 33.0f
#define RADIUS 2.0f

#define FAVORITES_NORMAL @"Favorites"
#define FAVORITES_FILLED @"Favorites Filled"

@implementation RBFavoritesView
{
    NSString* _assetID;
    BOOL _userFavorited;
    NSUInteger _favoriteCount;
    
    UILabel* _label;
    UIButton* _button;
}

- (instancetype)init
{
    self = [super init];
    if(self)
    {
        [self initElements];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [self initElements];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self)
    {
        [self initElements];
    }
    return self;
}

- (void) setFavoritesForGame:(RBXGameData*)gameData
{
    _assetID = gameData.placeID;
    _userFavorited = gameData.userFavorited;
    _favoriteCount = gameData.favorited;
    
    [self updateFavorites];
}

- (void) setFavoritesForPass:(RBXGamePass*)gamePass
{
    _assetID = [gamePass.passID stringValue];
    _userFavorited = gamePass.userFavorited;
    _favoriteCount = gamePass.favoriteCount;

    [self updateFavorites];
}

- (void) setFavoritesForGear:(RBXGameGear*)gameGear
{
    _assetID = [gameGear.assetID stringValue];
    _userFavorited = gameGear.userFavorited;
    _favoriteCount = gameGear.favoriteCount;
    
    [self updateFavorites];
}

- (void) initElements
{
    self.backgroundColor = [UIColor clearColor];
    
    UIImage* favoritesImage = [UIImage imageNamed:FAVORITES_NORMAL];
    _button = [UIButton buttonWithType:UIButtonTypeCustom];
    _button.size = favoritesImage.size;
    _button.position = CGPointMake(0, self.height - favoritesImage.size.height);
    _button.contentMode = UIViewContentModeScaleToFill;
    [_button setImage:favoritesImage forState:UIControlStateNormal];
    [_button addTarget:self action:@selector(favoritedTouched) forControlEvents:UIControlEventTouchDown];
    [self addSubview:_button];

    CGRect labelRect;
    labelRect.size = CGSizeMake(70.0, 23.0);
    labelRect.origin = CGPointMake(30.0, self.height - labelRect.size.height);
    _label = [[UILabel alloc] initWithFrame:labelRect];
    [RobloxTheme applyToFavoriteLabel:_label];
    [self addSubview:_label];
}

- (void) updateFavorites
{
    NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
    [numberFormatter setMaximumFractionDigits:0];
    
    _label.text = [numberFormatter stringFromNumber:[NSNumber numberWithUnsignedInteger:_favoriteCount]];
    
    if(_userFavorited)
        [_button setImage:[UIImage imageNamed:FAVORITES_FILLED] forState:UIControlStateNormal];
    else
        [_button setImage:[UIImage imageNamed:FAVORITES_NORMAL] forState:UIControlStateNormal];
}

- (void) favoritedTouched
{
    [UIView animateWithDuration:0.25
        delay:0
        options:UIViewAnimationOptionCurveEaseInOut
        animations:^
        {
            _button.transform = CGAffineTransformMakeScale(2.0, 2.0);
        }
        completion:^(BOOL finished)
        {
            [UIView animateWithDuration:0.25
                delay:0
                options:UIViewAnimationOptionCurveEaseInOut
                animations:^
                {
                    _button.transform = CGAffineTransformIdentity;
                }
                completion:nil];
        }];
    
    [RobloxData favoriteToggleForAssetID:_assetID
        completion:^(BOOL success, NSString *message)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                if(success)
                {
                    if(_userFavorited)
                    {
                        if(_favoriteCount > 0)
                        {
                            _favoriteCount--;
                        }
                    }
                    else
                    {
                        _favoriteCount++;
                    }
                    
                    _userFavorited = !_userFavorited;
                    
                    [self updateFavorites];
                }
                else if(message != nil)
                {
                    [RobloxHUD showMessage:message];
                }

            });
        }];
}

@end
