//
//  GamesCollectionViewController.h
//  RobloxMobile
//
//  Created by alichtin on 6/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface GamesCollectionView : UIView

- (void) loadGamesForKeywords:(NSString*)keywords;

- (void) loadGamesForSort:(NSNumber *)sortID playerID:(NSNumber *)playerID;
- (void) loadGamesForSort:(NSNumber *)sortID playerID:(NSNumber *)playerID genreID:(NSNumber *)genreID;

@end
