//
//  NativeSearchNavItem.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 11/10/14.
//  Modified from code by Ariel Lichtin.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "NativeSearchNavItem.h"
#import "GameSearchResultCell.h"
#import "UserSearchResultCell.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "RBMobileWebViewController.h"
#import "UIViewController+Helpers.h"
#import "FastLog.h"
#import "RBXEventReporter.h"
#import "NSDictionary+Parsing.h"
#import "UIView+Position.h"

#import <UIKit/UIKit.h>

//flags
FASTFLAGVARIABLE(NativeSearchFastResultsEnabled, false)
FASTINTVARIABLE(NativeSearchFastResultsWaitTime, 1000)
#define FAST_RESULT_TIMER_INTERVAL 500.0f
#define NUM_TO_DISPLAY_PHONE 2
#define NUM_TO_DISPLAY_TABLET 5

@interface NativeSearchNavItem () <UISearchBarDelegate, UICollectionViewDelegate, UICollectionViewDataSource>

@end

@implementation NativeSearchNavItem
{
    CGRect _baseFrame;
    UISearchBar* _searchBar;
    SearchResultType _searchType;
    BOOL _compactMode;
    
    // Parent ViewController
    __weak UIViewController* _containerController;
    UIView* _customView;
    UIButton* _iconButton;
    UILabel* _lblAreYouSearching;
    UILabel* _lblForMoreSuggestions;
    
    // Navigation items that are hidden while the search bar is active
    NSArray* _navLeftItems;
    NSArray* _navRightItems;
    NSString* _navTitle;
    
    UICollectionView* _collectionView;
    
    SearchStatus _status;
    
    NSNumber* _keyboardHeight;
    
    NSArray* _searchData; //could be games, users, etc.
    
    bool _searchCompleted; //used to check if we've already fired a search
    int _timeBeforeSearch; //a counter for searching
    NSTimer* _searchWaitTimer; //a timer for counting how long to wait
}

- (id)initWithSearchType:(SearchResultType)searchType
            andContainer:(UIViewController*)target
             compactMode:(BOOL)compactMode
{
    self = [super init];
    if (self)
    {
        //initialize some properties
        _compactMode = compactMode;
        _status = SearchStatusClosed;
        _searchType = searchType;
        _containerController = target;
        _keyboardHeight = nil;
        
        
        //Initialize the nav-bar icon
        UIImage* image = [UIImage imageNamed:@"Icon Search White"];
        _iconButton = [UIButton buttonWithType:UIButtonTypeCustom];
        _iconButton.hidden = !_compactMode;
        [_iconButton setImage:image forState:UIControlStateNormal];
        [_iconButton setFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
        [_iconButton addTarget:self action:@selector(openSearch) forControlEvents:UIControlEventTouchUpInside];
        
        // Initialize the base frame
        _baseFrame = _compactMode ? _iconButton.frame : CGRectMake(0, 0, 160, 26);
        
        // Initialize search bar
        _searchBar = [[UISearchBar alloc] initWithFrame:_baseFrame];
        _searchBar.hidden = _compactMode;
        _searchBar.delegate = self;
        _searchBar.backgroundColor = [UIColor clearColor];
        _searchBar.spellCheckingType = UITextSpellCheckingTypeNo;
        _searchBar.autocorrectionType = UITextAutocorrectionTypeNo;
        _searchBar.autocapitalizationType = UITextAutocapitalizationTypeNone;
        if( [_searchBar respondsToSelector:@selector(setSearchBarStyle:)] )
            _searchBar.searchBarStyle = UISearchBarStyleMinimal;
        

        
        
        // Initialize the custom nav-bar view
        _customView = [[UIView alloc] initWithFrame:_baseFrame];
        [_customView addSubview:_iconButton];
        [_customView addSubview:_searchBar];
        [self setCustomView:_customView];
        
        
        //initialize some layout properties
        float edgeInsetSidesAmount = [RobloxInfo thisDeviceIsATablet] ? 26.0f : 4.0f;
        UICollectionViewFlowLayout* layout = [[UICollectionViewFlowLayout alloc] init];
        layout.minimumLineSpacing = [RobloxInfo thisDeviceIsATablet] ? 50.0f : 20.0f;
        layout.minimumInteritemSpacing = [RobloxInfo thisDeviceIsATablet] ? 10.0f : 0.0f;
        layout.scrollDirection = UICollectionViewScrollDirectionVertical;
        layout.sectionInset = UIEdgeInsetsMake(45.0f, edgeInsetSidesAmount, 20.0f, edgeInsetSidesAmount);
        
        //initialize some context specific items
        NSString* cellTypeName;
        
        switch (_searchType)
        {
            case (SearchResultUsers) :
            {
                _searchBar.placeholder = NSLocalizedString(@"SearchUsersPhrase", nil);
                cellTypeName = [UserSearchResultCell getNibName];
                layout.itemSize = [UserSearchResultCell getCellSize];
            } break;
                
            case (SearchResultGames) :
            {
                _searchBar.placeholder = NSLocalizedString(@"SearchGamesPhrase", nil);
                layout.itemSize = [GameSearchResultCell getCellSize];
                cellTypeName = [GameSearchResultCell getNibName];
            } break;
                
            case (SearchResultGroups) : {} break;
                
            case (SearchResultCatalog) : {} break;
        }
        if (layout.itemSize.width != 0)
        {
            CGSize cellSize = layout.itemSize;
            NSInteger numberOfCells = _containerController.view.frame.size.width / cellSize.width ;
            NSInteger contentWidth = numberOfCells * cellSize.width;
            NSInteger edgeInsets = (_containerController.view.frame.size.width - contentWidth) / (numberOfCells + 1);
            layout.sectionInset = UIEdgeInsetsMake(45.0f,edgeInsets,layout.minimumLineSpacing,edgeInsets);
        }
        
        // Initialize results view
        _collectionView = [[UICollectionView alloc] initWithFrame:_containerController.view.bounds collectionViewLayout:layout];
        _collectionView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.8f];
        _collectionView.delegate = self;
        _collectionView.dataSource = self;
        _collectionView.scrollEnabled = NO;
        if (cellTypeName)
            [_collectionView registerNib:[UINib nibWithNibName:cellTypeName bundle:nil] forCellWithReuseIdentifier:@"ReuseCell"];
        
        
        
        
        //Initialize the helper text
        int labelHeight = 20;
        UIFont* fontToUse = [RobloxInfo thisDeviceIsATablet] ? [RobloxTheme fontBody] : [RobloxTheme fontBodySmall];
        _lblAreYouSearching = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, _collectionView.frame.size.width, labelHeight)];
        [_lblAreYouSearching setFont:fontToUse];
        [_lblAreYouSearching setTextColor:[RobloxTheme colorGray4]];
        [_lblAreYouSearching setText:NSLocalizedString(@"SearchSuggestionPart1", nil)];
        [_lblAreYouSearching setHidden:YES];
        
        _lblForMoreSuggestions = [[UILabel alloc] initWithFrame:CGRectMake(0, _collectionView.frame.size.height - labelHeight, _collectionView.frame.size.width, labelHeight)];
        [_lblForMoreSuggestions setFont:fontToUse];
        [_lblForMoreSuggestions setTextColor:[RobloxTheme colorGray4]];
        [_lblForMoreSuggestions setText:NSLocalizedString(@"SearchSuggestionPart2", nil)];
        [_lblForMoreSuggestions setTextAlignment:NSTextAlignmentRight];
        [_lblForMoreSuggestions setHidden:YES];
        
        
        [RobloxTheme applyToSearchNavItem:_searchBar];
    }
    return self;
}

//flags
- (BOOL) isFastResultsEnabled
{
    return FFlag::NativeSearchFastResultsEnabled;
}
- (int) getSearchWaitTime
{
    return FInt::NativeSearchFastResultsWaitTime;
}

//Accessors
-(RBXAnalyticsCustomData) getDataForSortType
{
    RBXAnalyticsCustomData searchDataType = RBXACustomGames;
    switch (_searchType)
    {
        case SearchResultGames: searchDataType = RBXACustomGames; break;
        case SearchResultUsers: searchDataType = RBXACustomUsers; break;
        case SearchResultCatalog: searchDataType = RBXACustomCatalog; break;
        case SearchResultGroups: break;
    }
    return searchDataType;
}


//Navigation Bar functions
- (void) openSearch
{
    if(_status == SearchStatusOpen)
        return;
    
    //add the observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardOpened:) name:UIKeyboardDidShowNotification object:nil];
    
    
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSearchOpen
                                             withContext:RBXAContextMain
                                          withCustomData:[self getDataForSortType]];
    
    _searchBar.hidden = NO;
    _iconButton.hidden = YES;
    _status = SearchStatusOpen;

    [_searchBar setShowsCancelButton:YES];
    
    CGRect visibleFrame = _containerController.view.bounds;
	CGFloat windowWidth = visibleFrame.size.width;
    
    // Animate the search bar in
    [UIView animateWithDuration:0.3f
        animations:^
        {
            // Make space in the navigation bar for the search bar
            _containerController.navigationItem.hidesBackButton = YES;
            _navLeftItems = _containerController.navigationItem.leftBarButtonItems;
            _navRightItems = _containerController.navigationItem.rightBarButtonItems;
            _navTitle = _containerController.navigationItem.title;
            _containerController.navigationItem.titleView.hidden = YES;
            _containerController.navigationItem.title = @"";
            
            [_containerController.navigationItem setLeftBarButtonItems:[NSArray array]];
            [_containerController.navigationItem setRightBarButtonItems:[NSArray arrayWithObject:self]];
            
            _customView.frame = CGRectMake(0, 0, windowWidth - 30.0f, _baseFrame.size.height);
            _searchBar.frame = CGRectMake(0, 0, windowWidth - 30.0f, _baseFrame.size.height);
        }
        completion:^(BOOL finished)
        {
            [_searchBar becomeFirstResponder];
        }];
    
    //only display the results if the flag is on
    if ([self isFastResultsEnabled])
    {
        _collectionView.frame = visibleFrame;
        [_containerController.view addSubview:_collectionView];
        [_containerController.view addSubview:_lblAreYouSearching];
        [_containerController.view addSubview:_lblForMoreSuggestions];
    }
}
- (void) closeSearch
{
    if(_status == SearchStatusClosed)
        return;
    
    _status = SearchStatusClosed;
    [_searchBar resignFirstResponder];
    [self deactivateWaitTimer];
    
    [UIView animateWithDuration:0.3f
        animations:^
        {
            _customView.frame = _baseFrame;
            _searchBar.frame = _baseFrame;
            [_searchBar setShowsCancelButton:NO animated:YES];
        }
        completion:^(BOOL finished)
        {
            // Restore the navigation bar
            [_containerController.navigationItem setHidesBackButton: NO];
            [_containerController.navigationItem setLeftBarButtonItems:_navLeftItems animated:YES];
            [_containerController.navigationItem setRightBarButtonItems:_navRightItems animated:YES];
            [_containerController.navigationItem setTitle:_navTitle];
            _containerController.navigationItem.titleView.hidden = NO;
            
            // Animate and release the focus from the search bar
            [_searchBar endEditing:YES];
            [_searchBar resignFirstResponder];
            
            _searchBar.hidden = _compactMode;
            _iconButton.hidden = !_compactMode;
            
            
        }
     ];
    
    [_collectionView removeFromSuperview];
    [_lblAreYouSearching removeFromSuperview];
    [_lblForMoreSuggestions removeFromSuperview];
    
    //remove the notification listener
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}


//Other functions
- (void) showSuggestionText:(BOOL)isShown
{
    _lblAreYouSearching.hidden = !isShown;
    _lblForMoreSuggestions.hidden = !isShown;
}
- (void) searchForResultsWithText:(NSString*)searchText
{
    //This number clearly doesn't matter as the returned amount is constant. GG Web Team.
    int numToSearch = [RobloxInfo thisDeviceIsATablet] ? NUM_TO_DISPLAY_TABLET: NUM_TO_DISPLAY_PHONE;
    
    switch(_searchType)
    {
        case (SearchResultGames):
        {
            [RobloxData searchGames:searchText
                          fromIndex:0
                           numGames:numToSearch
                          thumbSize:[RobloxTheme sizeGameCoverRectangle]
                         completion:^(NSArray *games)
             {
                 dispatch_async(dispatch_get_main_queue(), ^
                {
                    if([searchText isEqualToString:_searchBar.text] )
                    {
                        _searchData = games;
                        [_collectionView reloadData];
                        [self showSuggestionText:(games.count > 0)];
                    }
                });
             }];
        } break;
            
        case (SearchResultUsers) :
        {
            [RobloxData searchUsers:searchText
                          fromIndex:0
                           numUsers:numToSearch
                          thumbSize:[RobloxTheme sizeProfilePictureMedium]
                         completion:^(NSArray *users)
             {
                 dispatch_async(dispatch_get_main_queue(), ^
                {
                    if([searchText isEqualToString:_searchBar.text] )
                    {
                        _searchData = users;
                        [_collectionView reloadData];
                        [self showSuggestionText:(users.count > 0)];
                    }
                });
             }];
        } break;
            
        case (SearchResultCatalog) : {} break;
        case (SearchResultGroups) : {} break;
    }
}
- (void) initWaitTimer
{
    if (_searchWaitTimer)
        return;
    
    _searchCompleted = NO;
    _searchWaitTimer = [NSTimer timerWithTimeInterval:(FAST_RESULT_TIMER_INTERVAL / 1000.0f)
                                               target:self
                                             selector:@selector(ticTimer)
                                             userInfo:nil
                                              repeats:YES];
}
- (void) activateWaitTimer
{
    if (![_searchWaitTimer isValid])
    {
        [self initWaitTimer];
        [[NSRunLoop mainRunLoop] addTimer:_searchWaitTimer forMode:NSDefaultRunLoopMode];
    }
}
- (void) deactivateWaitTimer
{
    if ([_searchWaitTimer isValid])
    {
        [_searchWaitTimer invalidate];
        _searchWaitTimer = nil;
    }
}
- (void) ticTimer
{
    if (_searchCompleted)
        return;
    
    _timeBeforeSearch -= FAST_RESULT_TIMER_INTERVAL;
    
    if (_timeBeforeSearch <= 0)
    {
        [self searchForResultsWithText:_searchBar.text];
        _searchCompleted = YES;
    }
}


//Notification functions
-(void) keyboardOpened:(NSNotification*)notification
{
    //NSLog(@"Keyboard Opened : %@", notification.userInfo);
    if (notification && notification.userInfo)
    {
        //get the height of the keyboard
        NSDictionary* info = notification.userInfo;
        CGRect keyboardFrame = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
        if ([RobloxInfo thisDeviceIsATablet])
            _keyboardHeight = MIN([NSNumber numberWithFloat:keyboardFrame.size.height], [NSNumber numberWithFloat:keyboardFrame.size.width]);
        else
            _keyboardHeight = [NSNumber numberWithFloat:keyboardFrame.size.height];
        
        //move the helper text to the top of the label
        if (_keyboardHeight)
            dispatch_async(dispatch_get_main_queue(), ^{
                float navBarTopOffset = _containerController.view.bottom - (_keyboardHeight.floatValue + _lblForMoreSuggestions.height + _lblForMoreSuggestions.height);
                
                [_lblForMoreSuggestions setY:navBarTopOffset];
                [_lblForMoreSuggestions setRight:_containerController.view.right];
            });
        
    }
}


//Text Editing Delegate Functions
- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar
{
	[self openSearch];
}
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSubmit withContext:RBXAContextSearch withCustomData:[self getDataForSortType]];
    [self closeSearch];
    
	NSDictionary* userInfo = [NSDictionary dictionaryWithObject:_searchBar.text forKey:@"keywords"];
    
    switch (_searchType)
    {
        case (SearchResultGames):   [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_SEARCH_GAMES object:nil userInfo:userInfo]; break;
        case (SearchResultCatalog): break;
        case (SearchResultGroups):  break;
        case (SearchResultUsers):   [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_SEARCH_USERS object:nil userInfo:userInfo]; break;
    }
}
- (void)searchBarCancelButtonClicked:(UISearchBar *) searchBar
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextMain withCustomData:[self getDataForSortType]];
    searchBar.text = nil;
	[self closeSearch];
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
    if (![self isFastResultsEnabled]) return;
    
    [self showSuggestionText:NO];
    
    if(searchText.length == 0)
    {
        _searchData = nil;
        [_collectionView reloadData];
        return;
    }
    
    NSString* lastCharacter = [searchText substringFromIndex:(searchText.length-1)];
    if ([lastCharacter isEqualToString:@" "])
    {
        if (!_searchCompleted)
        {
            [self searchForResultsWithText:searchText];
            _searchCompleted = YES;
        }
    }
    else
    {
        _searchCompleted = NO;
        
        //reset the timer
        _timeBeforeSearch = [self getSearchWaitTime];
        [self activateWaitTimer];
    }
}
- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar
{
    [self closeSearch];
}

//Collection view delegate functions
- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    int numItems = 0;
    if (_searchData)
        numItems = MIN(_searchData.count, [RobloxInfo thisDeviceIsATablet] ? NUM_TO_DISPLAY_TABLET : NUM_TO_DISPLAY_PHONE);
    
    return numItems;
}
- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    //grab the generic cell
    UICollectionViewCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    switch(_searchType)
    {
        case (SearchResultGames):
        {
            RBXGameData* gameData = _searchData[indexPath.row];
            [(GameSearchResultCell*)cell setGameData:gameData];
        } break;
            
        case (SearchResultCatalog):
        {
            
        } break;
            
        case (SearchResultGroups):
        {
            
        } break;
            
        case (SearchResultUsers):
        {
            RBXUserSearchInfo* searchData = _searchData[indexPath.row];
            [(UserSearchResultCell*)cell setInfo:searchData];
        } break;
    }
    
    return cell;
}
- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
	[self closeSearch];
	
    //[collectionView deselectItemAtIndexPath:indexPath animated:YES];
    switch(_searchType)
    {
        case (SearchResultGames):
        {
            RBXGameData* gameData = _searchData[indexPath.row];
            if (!gameData)
                return;
            [[RBXEventReporter sharedInstance] reportOpenGameDetailFromSearch:[NSNumber numberWithInteger:gameData.placeID.integerValue]
                                                                     fromPage:RBXALocationGameSearch
                                                                      atIndex:[NSNumber numberWithInteger:indexPath.row]
                                                             totalItemsInSort:[NSNumber numberWithInteger:_searchData.count]];
            NSDictionary* userInfo = @{ @"gameData" : gameData,
                                        @"gameIndex" : [NSNumber numberWithInteger:indexPath.row],
                                        @"totalGames" : [NSNumber numberWithInteger:_searchData.count] };
            [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_SELECTED object:nil userInfo:userInfo];
        } break;
            
        case (SearchResultCatalog):
        {
            
        } break;
            
        case (SearchResultGroups):
        {
            
        } break;
            
        case (SearchResultUsers):
        {
            RBXUserSearchInfo* userData = _searchData[indexPath.row];
            NSDictionary* userInfo = [NSDictionary dictionaryWithObject:userData forKey:@"userData"];
            [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_USER_SELECTED object:nil userInfo:userInfo];
        } break;
    }
}

@end
