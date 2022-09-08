//
//  RBMoreViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/9/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RBBaseViewController.h"
#import "MoreTileButton.h"

@interface RBMoreViewController : RBBaseViewController

@property IBOutlet MoreTileButton* btnCatalog;
@property IBOutlet MoreTileButton* btnProfile;
@property IBOutlet MoreTileButton* btnCharacter;
@property IBOutlet MoreTileButton* btnTrade;
@property IBOutlet MoreTileButton* btnForum;
@property IBOutlet MoreTileButton* btnSettings;

@property NSMutableArray* events;
@property IBOutlet MoreTileButton* btnEvent1;
@property IBOutlet MoreTileButton* btnEvent2;
@property IBOutlet MoreTileButton* btnEvent3;
@property IBOutlet MoreTileButton* btnEvent4;
@property IBOutlet UILabel* lblEvents;

-(RBXAnalyticsCustomData) getMostRecentTab;

@end
