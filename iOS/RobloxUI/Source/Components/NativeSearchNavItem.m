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
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "RBMobileWebViewController.h"
#import "UIViewController+Helpers.h"

#import <UIKit/UIKit.h>

#define GAME_THUMBNAIL_SIZE CGSizeMake(110, 110)
#define GAME_ITEM_SIZE CGSizeMake(312, 64)
#define USER_THUMBNAIL_SIZE CGSizeMake(110, 110)
#define USER_ITEM_SIZE CGSizeMake(312, 64)

@interface NativeSearchNavItem () <UISearchBarDelegate, UICollectionViewDelegate, UICollectionViewDataSource>

@end

@implementation NativeSearchNavItem
{
    CGRect _baseFrame;
    UISearchBar* _searchBar;
    SearchType _searchType;
    BOOL _compactMode;
    
    // Parent ViewController
    __weak UIViewController* _containerController;
    UIView* _customView;
    UIButton* _iconButton;
    
    // Navigation items that are hidden while the search bar is active
    NSArray* _navLeftItems;
    NSArray* _navRightItems;
    NSString* _navTitle;
    
    UICollectionView* _collectionView;
    
    SearchStatus _status;
    
    NSArray* _searchData; //could be games, users, etc.
}

- (id)initWithSearchType:(SearchType)searchType
            andContainer:(UIViewController*)target
             compactMode:(BOOL)compactMode
{
    self = [super init];
    if (self)
    {
        _compactMode = compactMode;
        
        UIImage* image = [UIImage imageNamed:@"Search"];
        _iconButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [_iconButton setImage:image forState:UIControlStateNormal];
        [_iconButton setFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
        [_iconButton addTarget:self action:@selector(closeButtonTouched) forControlEvents:UIControlEventTouchUpInside];
        
        _status = SearchStatusClosed;
        _searchType = searchType;
        
        if(_compactMode)
            _baseFrame = _iconButton.frame;
        else
            _baseFrame = CGRectMake(0, 0, 160, 26);
        
        _containerController = target;
        
        // Initialize search bar
        _searchBar = [[UISearchBar alloc] initWithFrame:_baseFrame];
        _searchBar.delegate = self;
        _searchBar.backgroundColor = [UIColor clearColor];
        _searchBar.spellCheckingType = UITextSpellCheckingTypeNo;
        _searchBar.autocorrectionType = UITextAutocorrectionTypeNo;
        _searchBar.autocapitalizationType = UITextAutocapitalizationTypeNone;
        if( [_searchBar respondsToSelector:@selector(setSearchBarStyle:)] )
        {
            _searchBar.searchBarStyle = UISearchBarStyleMinimal;
        }

        _customView = [[UIView alloc] initWithFrame:_baseFrame];
        [_customView addSubview:_iconButton];
        [_customView addSubview:_searchBar];
        [self setCustomView:_customView];
        
        _searchBar.hidden = _compactMode;
        _iconButton.hidden = !_compactMode;
        
        // Initialize game results view
        UICollectionViewFlowLayout* layout = [[UICollectionViewFlowLayout alloc] init];
		layout.minimumLineSpacing = 50.0f;
        layout.scrollDirection = UICollectionViewScrollDirectionVertical;
        layout.sectionInset = UIEdgeInsetsMake(45.0f, 26.0f, 20.0f, 26.0f);
        
        _collectionView = [[UICollectionView alloc] initWithFrame:_containerController.view.bounds collectionViewLayout:(UICollectionViewLayout *)layout];
        _collectionView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.8f];
        _collectionView.delegate = self;
        _collectionView.dataSource = self;
        _collectionView.scrollEnabled = NO;
        
        
        //initialize some context specific items
        if (_searchType == SearchTypeUsers)
        {
            _searchBar.placeholder = NSLocalizedString(@"SearchUsersPhrase", nil);
            layout.itemSize = USER_ITEM_SIZE;
            [_collectionView registerNib:[UINib nibWithNibName:@"UserSearchResultCell" bundle:nil] forCellWithReuseIdentifier:@"ReuseCell"];
        }
        else if (_searchType == SearchTypeGames)
        {
            _searchBar.placeholder = NSLocalizedString(@"SearchGamesPhrase", nil);
            layout.itemSize = GAME_ITEM_SIZE;
            [_collectionView registerNib:[UINib nibWithNibName:@"GameSearchResultCell" bundle:nil] forCellWithReuseIdentifier:@"ReuseCell"];
        }
        
        [RobloxTheme applyToSearchNavItem:_searchBar];
    }
    return self;
}

- (void)openSearch
{
    if(_status == SearchStatusOpen)
        return;
    
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
    
    _collectionView.frame = visibleFrame;
    [_containerController.view addSubview:_collectionView];
}

- (void)closeSearch
{
    if(_status == SearchStatusClosed)
        return;
    
    _status = SearchStatusClosed;

	[_searchBar resignFirstResponder];

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
}
- (void) closeSearchFast
{
    if(_status == SearchStatusClosed)
        return;
    
    _status = SearchStatusClosed;
    
    [_searchBar resignFirstResponder];

    // Restore the navigation bar
    [_containerController.navigationItem setHidesBackButton: NO];
    [_containerController.navigationItem setLeftBarButtonItems:_navLeftItems animated:YES];
    [_containerController.navigationItem setRightBarButtonItems:_navRightItems animated:YES];
    [_containerController.navigationItem setTitle:_navTitle];
    _containerController.navigationItem.titleView.hidden = NO;
    
    // Animate and release the focus from the search bar
    _customView.frame = _baseFrame;
    _searchBar.frame = _baseFrame;
    [_searchBar setShowsCancelButton:NO animated:YES];
    [_searchBar endEditing:YES];
    [_searchBar resignFirstResponder];
    
    _searchBar.hidden = _compactMode;
    _iconButton.hidden = !_compactMode;
}

- (void)closeButtonTouched
{
    [self openSearch];
}

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar
{
	[self openSearch];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
    [self closeSearch];
    
	NSDictionary* userInfo = [NSDictionary dictionaryWithObject:_searchBar.text forKey:@"keywords"];
    if (_searchType == SearchTypeGames)
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:RBXNotificationSearchGames object:nil userInfo:userInfo];
    }
    else if (_searchType == SearchTypeUsers)
    {
        //Open a webview with the results
        CGRect currentScreenRect = CGRectMake(_containerController.view.frame.origin.x, _containerController.view.frame.origin.y,
                                              _containerController.view.frame.size.width, _containerController.view.frame.size.height);
        RBMobileWebViewController* results = [[RBMobileWebViewController alloc] initWithNavButtons:NO];
        [results.view setFrame:currentScreenRect];
        
        NSString* endpointFormat = [RobloxInfo thisDeviceIsATablet] ? @"%@/users/search?keyword=%@" : @"%@people?search=%@";
        [results setUrl:[NSString stringWithFormat:endpointFormat, [RobloxInfo getBaseUrl], _searchBar.text]];
        
        [self closeSearchFast];
        [_containerController.navigationController pushViewController:results animated:YES];
    }
}

- (void)searchBarCancelButtonClicked:(UISearchBar *) searchBar
{
    searchBar.text = nil;
	[self closeSearch];
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
    if(searchText.length == 0)
    {
        _searchData = nil;
        [_collectionView reloadData];
    }
    else
    {
        if      (_searchType == SearchTypeGames)
        {
            [RobloxData searchGames:searchText fromIndex:0 numGames:9 thumbSize:GAME_THUMBNAIL_SIZE completion:^(NSArray *games)
             {
                 dispatch_async(dispatch_get_main_queue(), ^
                                {
                                    if([searchText length] && [searchText isEqualToString:searchBar.text] )
                                    {
                                        _searchData = games;
                                        [_collectionView reloadData];
                                    }
                                });
             }];
        }
        else if (_searchType == SearchTypeUsers)
        {
            /*[RobloxData searchUsers:searchText fromIndex:0 numGames:9 thumbSize:USER_THUMBNAIL_SIZE completion:^(NSArray *users)
             {
                 dispatch_async(dispatch_get_main_queue(), ^
                                {
                                    if([searchText length] && [searchText isEqualToString:searchBar.text] )
                                    {
                                        _searchData = users;
                                        [_collectionView reloadData];
                                    }
                                });
             }];*/
        }
        
        
    }
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar
{
    [self closeSearch];
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return _searchData ? _searchData.count : 0;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    //grab the generic cell
    UICollectionViewCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    if (_searchType == SearchTypeGames)
    {
        RBXGameData* gameData = _searchData[indexPath.row];
        [(GameSearchResultCell*)cell setGameData:gameData];
    }
    else if (_searchType == SearchTypeUsers)
    {
        
    }
    return cell;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
	[self closeSearch];
	
    //[collectionView deselectItemAtIndexPath:indexPath animated:YES];
    if (_searchType == SearchTypeGames)
    {
        RBXGameData* gameData = _searchData[indexPath.row];
        NSDictionary* userInfo = [NSDictionary dictionaryWithObject:gameData forKey:@"gameData"];
        [[NSNotificationCenter defaultCenter] postNotificationName:RBXNotificationGameSelected object:nil userInfo:userInfo];
    }
    else if (_searchType == SearchTypeUsers)
    {
        
    }
}

@end
