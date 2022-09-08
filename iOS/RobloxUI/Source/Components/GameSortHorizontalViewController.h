//
//  GameSortHorizontalViewController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/28/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"
#import "RBXEventReporter.h"

@interface GameSortHorizontalViewController : UIViewController

// Initialize the sort
-(void) setSort:(RBXGameSort*)sort;
- (void) setSort:(RBXGameSort *)sort andGenre:(RBXGameGenre *)genre;

// Initialize with a prefetched game list
-(void) setSort:(RBXGameSort*)sort withGames:(NSArray*)games;

-(void) setAnalyticsLocation:(RBXAnalyticsGameLocations)location andContext:(RBXAnalyticsContextName)context;

@property (strong, nonatomic) NSString* sortTitle;
@property (nonatomic) NSUInteger startIndex;

// Callback handlers
@property (copy) void (^seeAllHandler)(NSNumber* sortID);
@property (copy) void (^gameSelectedHandler)(RBXGameData* gameData);

@end
