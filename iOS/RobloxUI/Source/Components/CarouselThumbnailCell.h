//
//  GameThumbnailCell.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@class RBXGameData;
@class RobloxImageView;

@interface CarouselThumbnailCell : UICollectionViewCell

-(void) setGameData:(RBXGameData*)data;

@end
