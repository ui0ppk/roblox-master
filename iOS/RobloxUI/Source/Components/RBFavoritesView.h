//
//  RBFavoritesView
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"

@interface RBFavoritesView : UIView

- (void) setFavoritesForGame:(RBXGameData*)gameData;
- (void) setFavoritesForPass:(RBXGamePass*)gamePass;
- (void) setFavoritesForGear:(RBXGameGear*)gameGear;

@end
