//
//  RBSingleSortScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBSingleSortScreenController.h"
#import "RobloxTheme.h"
#import "RobloxData.h"
#import "RBPlayerThumbnailCell.h"
#import "RobloxNotifications.h"
#import "GamesCollectionView.h"
#import "RBWebGamePreviewScreenController.h"
#import "RBXEventReporter.h"

#define FRIENDCELL_ID       @"FriendCell"

#define FRIEND_AVATAR_SIZE CGSizeMake(110, 110)
#define ITEM_SIZE CGSizeMake(90, 108)

#define FRIENDS_PER_REQUEST 40
#define START_REQUEST_THRESHOLD 10 // The next request will start when there are 10 elements left

@interface RBSingleSortScreenController ()

@end

@implementation RBSingleSortScreenController
{
    GamesCollectionView* _collectionView;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    _collectionView = [[GamesCollectionView alloc] init];
    [self.view addSubview:_collectionView];
    
    [_collectionView loadGamesForSort:self.sortID playerID:self.playerID];
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    [_collectionView setFrame:self.view.bounds];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(showGameDetails:) name:RBX_NOTIFY_GAME_SELECTED object:nil];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) showGameDetails:(NSNotification*) notification
{
    RBXGameData* gameData = [notification.userInfo objectForKey:@"gameData"];
    
    [[RBXEventReporter sharedInstance] reportOpenGameDetailFromSort:[NSNumber numberWithInteger:gameData.placeID.integerValue]
                                                           fromPage:RBXALocationGamesSeeAll
                                                             inSort:_sortID
                                                            atIndex:[notification.userInfo objectForKey:@"gameIndex"]
                                                   totalItemsInSort:[notification.userInfo objectForKey:@"totalGames"]];
    
    RBWebGamePreviewScreenController* controller = [[RBWebGamePreviewScreenController alloc] init];
    controller.gameData = gameData;
    [self.navigationController pushViewController:controller animated:YES];
}

@end
