//
//  GameSortHorizontalViewController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/28/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameSortHorizontalViewController.h"
#import "GameThumbnailCell.h"
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RobloxNotifications.h"
#import "UIView+Position.h"

#define NUM_GAMES_IN_CONTROLLER 30
#define ITEM_SIZE CGSizeMake(166, 220)
#define THUMBNAIL_SIZE CGSizeMake(420, 230)

@interface GameSortHorizontalViewController ()

@end

@implementation GameSortHorizontalViewController
{
    IBOutlet UILabel *_title;
    IBOutlet UIButton *_seeAllButton;
    IBOutlet UICollectionView* _collectionView;
    
    NSNumber* _sortID;
    NSArray* _games;
    RBXAnalyticsGameLocations _gameLocation;
    RBXAnalyticsContextName _gameContext;
}

- (instancetype)init
{
    self = [super init];
    if(self)
    {
        self.startIndex = 0;
        _gameContext = RBXAContextMain;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    if(_sortTitle != nil)
        _title.text = _sortTitle;
	
	[RobloxTheme applyToGameSortSeeAllButton:_seeAllButton];
	[RobloxTheme applyToGameSortTitle:_title];
	
    [_seeAllButton setTitle:NSLocalizedString(@"SeeAllPhrase", nil) forState:UIControlStateNormal];
    [_seeAllButton setShowsTouchWhenHighlighted:YES];
    
    // Setup the collection view
    [_collectionView registerNib:[UINib nibWithNibName:@"GameThumbnailCell" bundle: nil] forCellWithReuseIdentifier:@"ReuseCell"];
    [_collectionView setBackgroundView:nil];
    [_collectionView setBackgroundColor:[UIColor clearColor]];
    
    UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
    [flowLayout setItemSize:ITEM_SIZE];
    [flowLayout setScrollDirection:UICollectionViewScrollDirectionHorizontal];
    [flowLayout setMinimumLineSpacing:6.0f];
    [flowLayout setSectionInset:UIEdgeInsetsMake(0.0, 29.0, 0.0, 29.0)];
    [_collectionView setCollectionViewLayout:flowLayout];
}

- (void)setSortTitle:(NSString *)sortTitle
{
    _sortTitle = [sortTitle uppercaseString];
    _title.text = _sortTitle;
}

-(void) setSort:(RBXGameSort*)sort
{
    [self setSort:sort andGenre:nil];
}

- (void) setSort:(RBXGameSort *)sort andGenre:(RBXGameGenre *)genre {
    _sortTitle = [sort.title uppercaseString];
    _sortID = sort.sortID;
    _games = nil;
    
    _title.text = _sortTitle;
    [_collectionView reloadData];
    
    [RobloxData fetchGameListWithSortID:sort.sortID genreID:genre.genreID fromIndex:self.startIndex numGames:NUM_GAMES_IN_CONTROLLER thumbSize:THUMBNAIL_SIZE completion:^(NSArray *games)
     {
         dispatch_async(dispatch_get_main_queue(), ^
                        {
                            _games = games;
                            
                            [_collectionView reloadData];
                        });
     }];
}

-(void) setSort:(RBXGameSort*)sort withGames:(NSArray*)games
{
    _sortTitle = [sort.title uppercaseString];
    _sortID = sort.sortID;
    _games = games;
    
    _title.text = _sortTitle;
    [_collectionView reloadData];
}

-(void) setAnalyticsLocation:(RBXAnalyticsGameLocations)location andContext:(RBXAnalyticsContextName)context
{
    _gameLocation = location;
    _gameContext = context;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return _games != nil ? _games.count : 0;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    GameThumbnailCell *cell = [_collectionView dequeueReusableCellWithReuseIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    RBXGameData* gameData = _games[indexPath.row];
    [cell setGameData:gameData];
    
    return cell;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    if(_gameSelectedHandler != nil)
    {
        RBXGameData* gameData = _games[indexPath.row];
        [[RBXEventReporter sharedInstance] reportOpenGameDetailFromSort:[NSNumber numberWithInteger:gameData.placeID.integerValue]
                                                               fromPage:_gameLocation
                                                                 inSort:_sortID
                                                                atIndex:[NSNumber numberWithInteger:indexPath.row]
                                                       totalItemsInSort:[NSNumber numberWithInteger:_games.count]];
        
        _gameSelectedHandler(gameData);
    }
}

- (IBAction)seeAllCategoryTouchUpInside:(id)sender
{
    if (_gameContext)
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSeeAll withContext:_gameContext withCustomDataString:_sortID.stringValue];
    
    if(_seeAllHandler != nil)
    {
        _seeAllHandler(_sortID);
    }
}

@end
