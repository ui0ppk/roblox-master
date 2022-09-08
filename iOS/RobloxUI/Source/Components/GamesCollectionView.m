//
//  GamesCollectionView.m
//  RobloxMobile
//
//  Created by alichtin on 6/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GamesCollectionView.h"
#import "GameThumbnailCell.h"
#import "RobloxNotifications.h"
#import "RobloxData.h"
#import "RBInfiniteCollectionView.h"
#import "RobloxTheme.h"


@interface GamesCollectionView () <RBInfiniteCollectionViewDelegate>

@end

@implementation GamesCollectionView
{
	RBInfiniteCollectionView* _collectionView;
    
    NSNumber* _playerID; // Some sorts require the player ID
    NSNumber* _sortID;
    NSNumber* _genreID;
    NSString* _keywords;
    NSMutableArray* _games;
    
    NSUInteger _numItemsInCollectionView;
}

- (instancetype)init
{
    self = [super init];
    if(self)
    {
        _games = [[NSMutableArray alloc] init];
        
        self.backgroundColor = [UIColor clearColor];
        
        UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
        [flowLayout setItemSize:[GameThumbnailCell getCellSize]];
        [flowLayout setScrollDirection:UICollectionViewScrollDirectionVertical];
        [flowLayout setMinimumLineSpacing:6.0f]; //32.0f];
        [flowLayout setMinimumInteritemSpacing:0.0f];
        [flowLayout setSectionInset:UIEdgeInsetsMake(6,0,6,0)]; //(20, 20, 20, 20)];
        
        _collectionView = [[RBInfiniteCollectionView alloc] initWithFrame:CGRectZero collectionViewLayout:flowLayout];
        [_collectionView registerNib:[UINib nibWithNibName:@"GameThumbnailCell" bundle: nil] forCellWithReuseIdentifier:@"ReuseCell"];
        _collectionView.backgroundView = nil;
        _collectionView.backgroundColor = [UIColor clearColor];
        _collectionView.infiniteDelegate = self;
        
        [self addSubview:_collectionView];
        
        [self clearCollectionView];
    }
    return self;
}

- (instancetype) initWithFrame:(CGRect)frame {
    if (self == [super initWithFrame:frame]) {
        // init
        self.backgroundColor = [UIColor clearColor];
        
        UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
        [flowLayout setItemSize:[GameThumbnailCell getCellSize]];
        [flowLayout setScrollDirection:UICollectionViewScrollDirectionVertical];
        [flowLayout setMinimumLineSpacing:6.0f]; //32.0f];
        [flowLayout setMinimumInteritemSpacing:0.0f];
        [flowLayout setSectionInset:UIEdgeInsetsMake(6,0,6,0)]; //(20, 20, 20, 20)];
        
        _collectionView = [[RBInfiniteCollectionView alloc] initWithFrame:CGRectZero collectionViewLayout:flowLayout];
        [_collectionView registerNib:[UINib nibWithNibName:@"GameThumbnailCell" bundle: nil] forCellWithReuseIdentifier:@"ReuseCell"];
        _collectionView.backgroundView = nil;
        _collectionView.backgroundColor = [UIColor clearColor];
        _collectionView.infiniteDelegate = self;
        
        [self addSubview:_collectionView];
        
        [self clearCollectionView];
    }
    
    return self;
}

- (void) setFrame:(CGRect)frame
{
    [super setFrame:frame];
    [_collectionView setFrame:self.bounds];
}

- (void) clearCollectionView
{
    [_games removeAllObjects];
    [_collectionView reloadData];
}

- (void) loadGamesForKeywords:(NSString*)keywords
{
    if(_keywords == nil || ![_keywords isEqualToString:keywords])
    {
        _sortID = nil;
        _keywords = keywords;
        
        [self clearCollectionView];
    }
    
    [_collectionView loadElementsAsync];
}

- (void) loadGamesForSort:(NSNumber *)sortID playerID:(NSNumber *)playerID
{
    [self loadGamesForSort:sortID playerID:playerID genreID:nil];
}

- (void) loadGamesForSort:(NSNumber *)sortID playerID:(NSNumber *)playerID genreID:(NSNumber *)genreID {
    if(_sortID == nil || ![_sortID isEqualToNumber:sortID])
    {
        _sortID = sortID;
        _keywords = nil;
        
        [self clearCollectionView];
    }
    
    if (_genreID == nil || ![_genreID isEqualToNumber:genreID]) {
        _genreID = genreID;
    }

    if (_playerID == nil || ![_playerID isEqualToNumber:playerID]) {
        _playerID = playerID;
    }

    [_collectionView loadElementsAsync];
}

#pragma mark -
#pragma mark Infinite scroll delegates

- (void) asyncRequestItemsForCollectionView:(RBInfiniteCollectionView*)collectionView numItemsToRequest:(NSUInteger)itemsToRequest completionHandler:(void(^)())completionHandler
{
    if(_collectionView.numItems > _games.count)
    {
        void(^block)(NSArray*) = ^(NSArray* games)
        {
            dispatch_async(dispatch_get_main_queue(), ^
                           {
                               [_games addObjectsFromArray:games];
                               completionHandler();
                           });
        };
        
        // If _sortID is set, this collection view displays the games belonging to a sort
        // If _keywords is set, this collection view is used to display search results
        if(_sortID != nil)
            [RobloxData fetchGameListWithSortID:_sortID genreID:_genreID playerID:_playerID fromIndex:_games.count numGames:itemsToRequest thumbSize:[RobloxTheme sizeGameCoverSquare] completion:block];
        else // if(_keywords != nil)
            [RobloxData searchGames:_keywords fromIndex:_games.count numGames:itemsToRequest thumbSize:[RobloxTheme sizeGameCoverSquare] completion:block];
    }
}

- (NSUInteger)numItemsInInfiniteCollectionView:(RBInfiniteCollectionView*)collectionView
{
    return _games.count;
}

- (UICollectionViewCell*)infiniteCollectionView:(RBInfiniteCollectionView*)collectionView cellForItemAtIndexPath:(NSIndexPath*)indexPath;
{
    GameThumbnailCell *cell = [_collectionView dequeueReusableCellWithReuseIdentifier:@"ReuseCell" forIndexPath:indexPath];
    if(indexPath.row < _games.count)
    {
        RBXGameData* gameData = _games[indexPath.row];
        if([gameData isKindOfClass:[NSNull class]])
            [cell setGameData:nil];
        else
            [cell setGameData:gameData];
    }
    else
    {
        [cell setGameData:nil];
    }
    
    return cell;
}

- (void)infiniteCollectionView:(RBInfiniteCollectionView*)collectionView didSelectItemAtIndexPath:(NSIndexPath*)indexPath;
{
    if(indexPath.row < _games.count)
    {
        RBXGameData* gameData = _games[indexPath.row];
        NSDictionary* userInfo = @{ @"gameData" : gameData,
                                    @"gameIndex" : [NSNumber numberWithInteger:indexPath.row],
                                    @"totalGames" : [NSNumber numberWithInteger:_games.count] };
        [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_SELECTED object:nil userInfo:userInfo];
    }
}

@end
