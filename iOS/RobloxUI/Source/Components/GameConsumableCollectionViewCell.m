//
//  GameThumbnailCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameConsumableCollectionViewCell.h"
#import "RobloxImageView.h"
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RBActivityIndicatorView.h"

#define THUMBNAIL_SIZE CGSizeMake(420, 230)
#define VIEWCELL_CORNER_RADIUS  5.0

@interface GameConsumableCollectionViewCell()
@end

@implementation GameConsumableCollectionViewCell
{
	IBOutlet RobloxImageView *_image;
	IBOutlet UILabel *_title;
    IBOutlet UILabel *_priceLabel;
    
    NSNumber* _assetID;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    _title.backgroundColor = [UIColor clearColor];
    [RobloxTheme applyToConsumablePriceLabel:_priceLabel];
    [RobloxTheme applyToConsumableTitleLabel:_title];
}

-(void) setGameGearData:(RBXGameGear*)data
{
    if(data == nil)
    {
        _assetID = nil;
        
        _image.hidden = YES;
        _title.hidden = YES;
        _priceLabel.hidden = YES;
    }
    else if(_assetID == nil || ![_assetID isEqualToNumber:data.assetID])
    {
        _assetID = data.assetID;
        
        _image.hidden = NO;
        _title.hidden = NO;
        _priceLabel.hidden = NO;
        
        _image.hidden = YES;
        _image.animateInOptions = RBXImageViewAnimateInIfNotCached;
        
        _title.text = data.name;
        
        _priceLabel.text = data.userOwns
        ? [NSString stringWithFormat:NSLocalizedString(@"Purchased", nil)]
        : (data.priceInRobux > 0 ? [NSString stringWithFormat:@"%lu", (unsigned long)data.priceInRobux] : @"-");
        
        self.layer.cornerRadius = VIEWCELL_CORNER_RADIUS;
        
        RobloxImageView* image = _image;
        
        [_image loadWithAssetID:[data.assetID stringValue] withSize:THUMBNAIL_SIZE completion:^
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                image.hidden = NO;
            });
        }];
    }
}

-(void) setGamePassData:(RBXGamePass*)data
{
    if(data == nil)
    {
        _assetID = nil;
        
        _image.hidden = YES;
        _title.hidden = YES;
        _priceLabel.hidden = YES;
    }
    else if(_assetID == nil || ![_assetID isEqualToNumber:data.passID])
    {
        _assetID = data.passID;
        
        _image.hidden = NO;
        _title.hidden = NO;
        _priceLabel.hidden = NO;
        
        _image.hidden = YES;
        _image.animateInOptions = RBXImageViewAnimateInIfNotCached;
        
        _title.text = data.passName;
        _priceLabel.text = data.userOwns
        ? [NSString stringWithFormat:NSLocalizedString(@"Purchased", nil)]
        : [NSString stringWithFormat:@"%lu", (unsigned long)data.priceInRobux];
        
        self.layer.cornerRadius = VIEWCELL_CORNER_RADIUS;
        
        RobloxImageView* image = _image;
        
        [_image loadWithAssetID:[data.passID stringValue] withSize:THUMBNAIL_SIZE completion:^
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                image.hidden = NO;
            });
        }];
    }
}

@end
