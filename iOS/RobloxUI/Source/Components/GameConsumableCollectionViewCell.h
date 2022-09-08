//
//  GameThumbnailCell.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@class RBXGameGear;
@class RBXGamePass;

@interface GameConsumableCollectionViewCell : UICollectionViewCell

-(void) setGameGearData:(RBXGameGear*)gameGearData;
-(void) setGamePassData:(RBXGamePass*)gamePassData;

@end
