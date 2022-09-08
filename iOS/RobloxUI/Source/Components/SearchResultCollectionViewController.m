//
//
//  RobloxMobile
//
//  Created by Kyler Mulherin on 1/16/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "SearchResultCollectionViewController.h"
#import "RobloxNotifications.h"
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"
#import "RBXEventReporter.h"

//Result cells
#import "GameThumbnailCell.h"
#import "RBSearchUserCell.h"

#define SEARCH_RESULT_BUFFER_ZONE 10
#define SEARCH_RESULT_INITIAL_TOTAL 40

@implementation SearchResultCollectionViewController
{
    UILabel* lblNoResults;
}

-(id) initWithKeyword:(NSString *)keyword andSearchType:(SearchResultType)searchResultType
{
    _searchKeyword = keyword;
    _searchResultType = searchResultType;
    _searchResults = [[NSMutableArray alloc] init];
    
    switch (_searchResultType)
    {
        case SearchResultGames:     _collectionViewCellXibName = [GameThumbnailCell getNibName];    break;
        case SearchResultCatalog:   break;
        case SearchResultGroups:    break;
        case SearchResultUsers:     _collectionViewCellXibName = [RBSearchUserCell getNibName];     break;
    }
    
    _searchResults = [[NSMutableArray alloc] init];
    _resultBufferZone = SEARCH_RESULT_BUFFER_ZONE;
    _resultsPerSearch = SEARCH_RESULT_INITIAL_TOTAL;
    
    return self;
}


- (void) clearCollectionView
{
    [_searchResults removeAllObjects];
    [_collectionView reloadData];
}
- (void) loadSearchForKeywords:(NSString*)keywords
{
    if(_searchKeyword == nil || ![_searchKeyword isEqualToString:keywords])
    {
        _searchKeyword = keywords;
        
        [self clearCollectionView];
    }
    
    //this does stuff and then calls asyncRequestItemsForCollectionView...
    [_collectionView loadElementsAsync];
}


-(void)viewDidLoad
{
    UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
    [flowLayout setScrollDirection:UICollectionViewScrollDirectionVertical];
    
    
    switch (_searchResultType)
    {
        case SearchResultGames:
        {
            NSInteger margin = 6;
            CGSize cellSize = [GameThumbnailCell getCellSize];
            float width = [RobloxInfo thisDeviceIsATablet] ? MAX(self.view.height, self.view.width) : self.view.width;
            NSInteger numberOfCells = width / cellSize.width ;
            NSInteger contentWidth = (numberOfCells * cellSize.width) + (margin * (numberOfCells-1));
            NSInteger edgeInsets = (width - contentWidth) / 2;
            
            [flowLayout setItemSize:cellSize];
            [flowLayout setMinimumLineSpacing:6.0f]; //32.0f];
            [flowLayout setMinimumInteritemSpacing:0.0f];
            [flowLayout setSectionInset:UIEdgeInsetsMake(margin,edgeInsets,margin,edgeInsets)];
            [RobloxTheme applyTheme:RBXThemeGame toViewController:self quickly:YES];
        }  break;
        case SearchResultCatalog:   break;
        case SearchResultGroups:    break;
        case SearchResultUsers:
        {
            [flowLayout setMinimumLineSpacing:32.0f];
            [flowLayout setSectionInset:UIEdgeInsetsMake(20, 20, 20, 20)];
            [flowLayout setItemSize:[RBSearchUserCell getCellSize]];
            [RobloxTheme applyTheme:RBXThemeSocial toViewController:self quickly:YES];
        } break;
    }
    
    
    _collectionView = [[RBInfiniteCollectionView alloc] initWithFrame:CGRectZero collectionViewLayout:flowLayout];
    _collectionView.backgroundView = nil;
    _collectionView.backgroundColor = [UIColor clearColor];
    _collectionView.infiniteDelegate = self;
    [_collectionView registerNib:[UINib nibWithNibName:_collectionViewCellXibName bundle:[NSBundle mainBundle]] forCellWithReuseIdentifier:@"ReuseCell"];
    
    [self.view addSubview:_collectionView];
    
    //add a title to the search results
    NSString* truncatedKeyword = _searchKeyword.length > 12 ? [[_searchKeyword substringToIndex:9] stringByAppendingString:@"..."] : _searchKeyword;
    NSString* pageTitle = [NSString stringWithFormat:@"%@ : %@", NSLocalizedString(@"SearchResultsPhrase", nil), truncatedKeyword];
    self.navigationItem.title = pageTitle;
    [self addSearchIconWithSearchType:_searchResultType andFlurryEvent:nil];
    
    //[RobloxTheme applyToGamesNavBar:self.navigationController.navigationBar];
    self.view.backgroundColor = [RobloxTheme lightBackground];;
    
    lblNoResults = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 300, 300)];
    [lblNoResults setFont:[RobloxTheme fontBodyLargeBold]];
    [lblNoResults setTextColor:[UIColor blackColor]];
    [lblNoResults setTextAlignment:NSTextAlignmentCenter];
    [lblNoResults setText:NSLocalizedString(@"NoResultsFoundWord", nil)];
    [lblNoResults setHidden:YES];
    [self.view addSubview:lblNoResults];
    
    [self clearCollectionView];
}
-(void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextSearchResults];
    
    [self loadSearchForKeywords:_searchKeyword];
}
-(void) viewWillLayoutSubviews
{
    [_collectionView setFrame:self.view.bounds];
    [lblNoResults centerInFrame:_collectionView.frame];
}


-(RBXAnalyticsCustomData) getDataForSortType
{
    RBXAnalyticsCustomData searchDataType = RBXACustomGames;
    switch (_searchResultType)
    {
        case SearchResultGames: searchDataType = RBXACustomGames; break;
        case SearchResultUsers: searchDataType = RBXACustomUsers; break;
        case SearchResultCatalog: searchDataType = RBXACustomCatalog; break;
        case SearchResultGroups: break;
    }
    return searchDataType;
}


#pragma mark -
#pragma mark Infinite scroll delegates

- (void) asyncRequestItemsForCollectionView:(RBInfiniteCollectionView*)collectionView numItemsToRequest:(NSUInteger)itemsToRequest completionHandler:(void(^)())completionHandler
{
    if(_collectionView.numItems > _searchResults.count)
    {
        //completion block to add the elements from
        void(^block)(NSArray*) = ^(NSArray* results)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [_searchResults addObjectsFromArray:results];
                lblNoResults.hidden = ([_searchResults count] > 0);
                completionHandler();
            });
        };
        
        
        
        switch (_searchResultType)
        {
            case SearchResultGames:
                [RobloxData searchGames:_searchKeyword fromIndex:_searchResults.count numGames:itemsToRequest thumbSize:[RobloxTheme sizeGameCoverRectangleLarge] completion:block];
                break;
                
            case SearchResultUsers:
                [RobloxData searchUsers:_searchKeyword fromIndex:_searchResults.count numUsers:itemsToRequest thumbSize:[RobloxTheme sizeProfilePictureLarge] completion:block];
                break;
                
            case SearchResultCatalog:
                
                break;
                
            case SearchResultGroups:
                
                break;
        }
        
        
    }
}

- (NSUInteger)numItemsInInfiniteCollectionView:(RBInfiniteCollectionView*)collectionView
{
    return _searchResults.count;
}

- (UICollectionViewCell*)infiniteCollectionView:(RBInfiniteCollectionView*)collectionView cellForItemAtIndexPath:(NSIndexPath*)indexPath
{
    UICollectionViewCell* cell = [_collectionView dequeueReusableCellWithReuseIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    bool validIndex = (indexPath.row < _searchResults.count);
    
    switch (_searchResultType)
    {
        case SearchResultGames:
        {
            RBXGameData* gameData = (validIndex) ? _searchResults[indexPath.row] : nil;
            [(GameThumbnailCell*)cell setGameData:gameData];
        }
        break;
        
        case SearchResultUsers:
        {
            RBXUserSearchInfo* aSearchCell = (validIndex) ? _searchResults[indexPath.row] : nil;
            [(RBSearchUserCell*)cell setInfo:aSearchCell];
        }
            break;
        case SearchResultCatalog:
            break;
        case SearchResultGroups:
            break;
    }
    
    return cell;
}

- (void)infiniteCollectionView:(RBInfiniteCollectionView*)collectionView didSelectItemAtIndexPath:(NSIndexPath*)indexPath
{
    if(indexPath.row < _searchResults.count)
    {
        switch (_searchResultType)
        {
            case SearchResultGames:
            {
                RBXGameData* gameData = (RBXGameData*)_searchResults[indexPath.row];
                
                [[RBXEventReporter sharedInstance] reportOpenGameDetailFromSearch:[NSNumber numberWithInteger:gameData.placeID.integerValue]
                                                                         fromPage:RBXALocationGameSearch
                                                                          atIndex:[NSNumber numberWithInteger:indexPath.row]
                                                                 totalItemsInSort:[NSNumber numberWithInteger:_searchResults.count]];
                
                [self pushGameDetailWithGameData:gameData];
            } break;
                
            case SearchResultUsers:
            {
                RBXUserSearchInfo* cell = _searchResults[indexPath.row];
                [self pushProfileControllerWithUserID:cell.userId];
            } break;
                
            case SearchResultGroups:
            {
                
            } break;
                
            case SearchResultCatalog:
            {
                
            } break;
        }
    }
}

@end
