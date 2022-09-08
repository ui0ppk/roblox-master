//
//  GameSortResultsScreenController.m
//  RobloxMobile
//
//  Created by alichtin on 5/30/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameSortResultsScreenController.h"
#import "RBWebGamePreviewScreenController.h"
#import "GameSearchResultsScreenController.h"
#import "GamesCollectionView.h"
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RobloxNotifications.h"
#import "ExtendedSegmentedControl.h"
#import "Flurry.h"
#import "RBXEventReporter.h"

#define MAX_SELECTOR_ITEMS 4

//---METRICS---
#define GSRSC_showSort          @"GAME SORT RESULTS SCREEN - Show Sort"
#define GSRSC_showGameDetails   @"GAME SORT RESULTS SCREEN - Show Game Details"
#define GSRSC_showSearchResults @"GAME SORT RESULTS SCREEN - Show Search Results"

@implementation GameSortResultsScreenController
{
	IBOutlet ExtendedSegmentedControl* _sortSelector;
	
	NSArray* _sorts;
	NSMutableArray* _collectionViews;
}

+ (instancetype) gameSortResultsScreenController {
    return [[UIStoryboard storyboardWithName:@"MainStoryboard_iPad" bundle:nil] instantiateViewControllerWithIdentifier:@"GameSortResultsScreenController"];
}

+ (instancetype) gameSortResultsScreenControllerWithSort:(RBXGameSort *)sort genre:(RBXGameGenre *)genre playerID:(NSNumber *)playerID {
    GameSortResultsScreenController *controller = [[UIStoryboard storyboardWithName:@"MainStoryboard_iPad" bundle:nil] instantiateViewControllerWithIdentifier:@"GameSortResultsScreenController"];
    controller.selectedSort = sort.sortID;
    controller.selectedGenre = genre;
    controller.playerID = playerID;
    
    return controller;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
	self.view.backgroundColor = [RobloxTheme lightBackground];
	
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    [self addSearchIconWithSearchType:SearchResultGames andFlurryEvent:nil];
    
	[_sortSelector removeAllSegments];
    [_sortSelector setMaxVisibleItems:MAX_SELECTOR_ITEMS];
    
    [RobloxData fetchAllSorts:^(NSArray *sorts)
     {
         dispatch_async(dispatch_get_main_queue(), ^
                        {
                            _sorts = sorts;
                            [self initSortViews];
                        });
     }];
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    for(GamesCollectionView* collection in _collectionViews)
    {
        [collection setFrame:self.view.bounds];
    }
    
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextGamesSeeAll];
}

- (void) initSortViews
{
    _collectionViews = [NSMutableArray array];
    
	int selectedIndex = -1;
	for(int i = 0; i < _sorts.count; ++i)
	{
		RBXGameSort* sort = _sorts[i];
		
        [_sortSelector insertSegmentWithTitle:sort.title atIndex:i animated:NO];
        
        GamesCollectionView* collection = [[GamesCollectionView alloc] init];
        [_collectionViews addObject:collection];
        
        if([sort.sortID isEqualToNumber:_selectedSort])
        {
			selectedIndex = i;
            [self showSort:selectedIndex];
		}
	}
	
    [_sortSelector setSelectedSegmentIndex:selectedIndex];
}

- (IBAction)sortSelectionChanged:(id)sender
{
    [self showSort:_sortSelector.selectedSegmentIndex];
}

- (void)showSort:(int)sortIndex
{
    [Flurry logEvent:GSRSC_showSort];
    for(int i = 0; i < _collectionViews.count; ++i)
    {
        GamesCollectionView* view = _collectionViews[i];
        [view removeFromSuperview];
    }
    
    RBXGameSort* sort = _sorts[sortIndex];
    GamesCollectionView* selectedView = _collectionViews[sortIndex];
    [self.view addSubview:selectedView];
    [selectedView loadGamesForSort:sort.sortID playerID:self.playerID genreID:self.selectedGenre.genreID];
}


@end
