//
//  RBVotesView.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"

@interface RBVotesView : UIView

- (void) setVotesForGame:(RBXGameData*)gameData;
- (void) setVotesForPass:(RBXGamePass*)gamePass;

@end
