//
//  GameThumbnailCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "CarouselThumbnailCell.h"
#import "RobloxImageView.h"
#import "RobloxData.h"
#import "RobloxTheme.h"

#define THUMBNAIL_SIZE RBX_SCALED_DEVICE_SIZE(455, 256)
#define VIEWCELL_CORNER_RADIUS  5.0

@interface CarouselThumbnailCell()
@end

@implementation CarouselThumbnailCell
{
	IBOutlet RobloxImageView *_image;
	IBOutlet UILabel *_title;
    IBOutlet UIView *_titleBackground;
	IBOutlet UIActivityIndicatorView *_activityIndicator;
    
    NSString* _placeID;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    [self setTitleStyle];
}

- (void)setTitleStyle
{
    _title.backgroundColor = [UIColor clearColor];
    [RobloxTheme applyToCarouselThumbnailTitle:_title];
    
    _titleBackground.layer.backgroundColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.95].CGColor;
    _titleBackground.layer.cornerRadius = VIEWCELL_CORNER_RADIUS - 1.0;
    _titleBackground.layer.masksToBounds = NO;
    _titleBackground.layer.shouldRasterize = NO;
    _titleBackground.frame = CGRectIntegral(_titleBackground.frame);
    _titleBackground.frame = CGRectIntegral(_titleBackground.frame);
}

-(void)setGameData:(RBXGameData*)data
{
    if(_placeID == nil || ![_placeID isEqualToString:data.placeID])
    {
        _placeID = data.placeID;
        
        [_activityIndicator startAnimating];
        _image.hidden = YES;
        _image.animateInOptions = RBXImageViewAnimateInAlways;
        
        _title.text = data.title;
        
        self.layer.cornerRadius = VIEWCELL_CORNER_RADIUS;
        
        UIActivityIndicatorView* indicator = _activityIndicator;
        RobloxImageView* image = _image;
        
        [_image loadThumbnailForGame:data withSize:THUMBNAIL_SIZE completion:^
        {
            [indicator stopAnimating];
            image.hidden = NO;
        }];
    }
}

@end
