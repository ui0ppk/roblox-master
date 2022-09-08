//
//  GameSearchResultCell.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/10/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "RobloxData.h"

@interface GameSearchResultCell : UICollectionViewCell

@property(strong, nonatomic) RBXGameData* gameData;

+(CGSize) getCellSize;
+(NSString*) getNibName;

@end
