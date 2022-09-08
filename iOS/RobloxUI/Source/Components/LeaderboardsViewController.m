//
//  LeaderboardViewController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "LeaderboardsViewController.h"
#import "LeaderboardCell.h"
#import "RobloxData.h"
#import "UserInfo.h"
#import "RobloxTheme.h"
#import "LeaderboardsViewController.h"
#import "RBProfileViewController.h"
#import "RobloxNotifications.h"
#import "Flurry.h"

#define NUM_ITEMS_PER_REQUEST 20
#define START_REQUEST_THRESHOLD 10 // The next request will start when there are 10 elements left
#define CELL_SIZE CGSizeMake(490, 63)
#define AVATAR_SIZE CGSizeMake(110, 110)

#define LBDC_didPressLeaderboardPlayerItem  @"LEADERBOARDS SCREEN - Leaderboard Player Item Pressed"
#define LBDC_didPressLeaderboardClaneItem   @"LEADERBOARDS SCREEN - Leaderboard Clan Item Pressed"

@interface LeaderboardsViewController () <UITableViewDelegate, UITableViewDataSource>

@end

@implementation LeaderboardsViewController
{
    IBOutlet UIActivityIndicatorView *_playersSpinner;
    IBOutlet UIActivityIndicatorView *_clansSpinner;
    IBOutlet UITableView* _playersTable;
    IBOutlet UITableView* _clansTable;
    IBOutlet UILabel* _playersTitle;
    IBOutlet UILabel* _clansTitle;
    LeaderboardCell* _currentUserPlayerCell;
    LeaderboardCell* _currentUserClanCell;
    
    NSMutableArray* _players;
    NSMutableArray* _clans;
	RBXLeaderboardEntry* _currentUserPlayerEntry;
	RBXLeaderboardEntry* _currentUserClanEntry;
    
    BOOL _initialized;
    NSInteger _lastPlayerRequested;
    NSInteger _lastClanRequested;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateLeaderboards) name:RBX_NOTIFY_DID_LEAVE_GAME object:nil];
    
    // Load the current user player header cell
    {
        _currentUserPlayerCell = [[[NSBundle mainBundle] loadNibNamed:@"LeaderboardCell" owner:nil options:nil] firstObject];
        _currentUserPlayerCell.hidden = YES;
        
        CGRect cellRect = _currentUserPlayerCell.frame;
        cellRect.origin = _playersTable.frame.origin;
        _currentUserPlayerCell.frame = cellRect;
        
        [self applyThemeToLeaderboardUserCell:_currentUserPlayerCell];
        
        [self.view insertSubview:_currentUserPlayerCell aboveSubview:_playersTable];
    }
    
    // Load the current user clan header cell
    {
        _currentUserClanCell = [[[NSBundle mainBundle] loadNibNamed:@"LeaderboardCell" owner:nil options:nil] firstObject];
        _currentUserClanCell.hidden = YES;
        
        CGRect cellRect = _currentUserClanCell.frame;
        cellRect.origin = _clansTable.frame.origin;
        _currentUserClanCell.frame = cellRect;
        
        [self applyThemeToLeaderboardUserCell:_currentUserClanCell];
        
        [self.view insertSubview:_currentUserClanCell aboveSubview:_clansTable];
    }
    
    _playersTitle.text = NSLocalizedString(@"PlayersWord", nil);
    [RobloxTheme applyToTableHeaderTitle:_playersTitle];
    
    _clansTitle.text = NSLocalizedString(@"ClansWord", nil);
    [RobloxTheme applyToTableHeaderTitle:_clansTitle];
    
    [_playersTable registerNib:[UINib nibWithNibName:@"LeaderboardCell" bundle:nil] forCellReuseIdentifier:@"ReuseCell"];
    [_playersTable setAllowsSelection:YES];
    [_playersTable setSeparatorStyle:UITableViewCellSeparatorStyleNone];
    
    [_clansTable registerNib:[UINib nibWithNibName:@"LeaderboardCell" bundle:nil] forCellReuseIdentifier:@"ReuseCell"];
    [_clansTable setAllowsSelection:YES];
    [_clansTable setSeparatorStyle:UITableViewCellSeparatorStyleNone];
    
    _players = [NSMutableArray array];
    _clans = [NSMutableArray array];
    
    _initialized = NO;
    _lastPlayerRequested = 0;
    _lastClanRequested = 0;
}

- (void) updateLeaderboards
{
    if(!_initialized)
    {
        _initialized = YES;
        
        [self fetchInitialPlayers];
        [self fetchInitialClans];
    }
}

- (void) fetchInitialPlayers
{
	// Hide the tables and show the spinner
	[_currentUserPlayerCell setHidden:YES];
	[_playersSpinner setHidden:NO];
	[_playersSpinner startAnimating];
	[_playersTable setHidden:YES];
	
	// Group the full leaderboard and user info request
	dispatch_group_t group = dispatch_group_create();
	
	// If a user is logged in, fetch the current user LB entry
	UserInfo* currentPlayer = [UserInfo CurrentPlayer];
	if(currentPlayer.userLoggedIn)
	{
		dispatch_group_enter(group);
		[RobloxData fetchUserLeaderboardInfo:RBXLeaderboardsTargetUser
                                      userID:currentPlayer.userId
                               distributorID:self.distributorID
                                  timeFilter:self.timeFilter
                                  avatarSize:AVATAR_SIZE
                                  completion:^(RBXLeaderboardEntry *userEntry)
                                  {
                                      _currentUserPlayerEntry = userEntry;
                                      dispatch_group_leave(group);
                                  }];
	}
	
	// Fetch the first N leaderboard entries
	dispatch_group_enter(group);
	[RobloxData fetchLeaderboardsFor:RBXLeaderboardsTargetUser
                          timeFilter:self.timeFilter
                       distributorID:self.distributorID
                          startIndex:0
                            numItems:NUM_ITEMS_PER_REQUEST
                           lastEntry:nil
                          avatarSize:AVATAR_SIZE
						  completion:^(NSArray *leaderboard)
						  {
                              if(leaderboard != nil)
                              {
                                  [_players addObjectsFromArray:leaderboard];
                              }
                              dispatch_group_leave(group);
						  }];
	
	// Execute the following block when both operations are complete.
	dispatch_group_notify(group, dispatch_get_main_queue(), ^
    {
		if(_currentUserPlayerEntry != nil)
		{
			// Show the user entry and resize the table
			[_currentUserPlayerCell showPlayerInfo:_currentUserPlayerEntry];
			
			_currentUserPlayerCell.hidden = NO;
			
			CGRect rect = _playersTable.frame;
			rect.origin.y += _currentUserPlayerCell.frame.size.height;
			rect.size.height -= _currentUserPlayerCell.frame.size.height;
			_playersTable.frame = rect;
		}
		
		// Show the table, hide the spinner
		[_playersTable reloadData];
        [_playersTable setHidden:NO];
        [_playersSpinner stopAnimating];
        [_playersSpinner setHidden:YES];
	});
}

- (void) fetchInitialClans
{
	// Hide the tables and show the spinner
	[_currentUserClanCell setHidden:YES];
	[_clansSpinner setHidden:NO];
	[_clansSpinner startAnimating];
	[_clansTable setHidden:YES];
	
	// Group the full leaderboard and user info request
	dispatch_group_t group = dispatch_group_create();
	
	// If a user is logged in, fetch the current user LB entry
	UserInfo* currentPlayer = [UserInfo CurrentPlayer];
	if(currentPlayer.userLoggedIn)
	{
		dispatch_group_enter(group);
		[RobloxData fetchUserLeaderboardInfo:RBXLeaderboardsTargetClan
                                      userID:currentPlayer.userId
                               distributorID:self.distributorID
                                  timeFilter:self.timeFilter
                                  avatarSize:AVATAR_SIZE
                                  completion:^(RBXLeaderboardEntry *userEntry)
                                  {
                                      _currentUserClanEntry = userEntry;
                                      dispatch_group_leave(group);
                                  }];
	}
	
	// Fetch the first N leaderboard entries
	dispatch_group_enter(group);
	[RobloxData fetchLeaderboardsFor:RBXLeaderboardsTargetClan
                          timeFilter:self.timeFilter
                       distributorID:self.distributorID
                          startIndex:0
                            numItems:NUM_ITEMS_PER_REQUEST
                           lastEntry:nil
                          avatarSize:AVATAR_SIZE
						  completion:^(NSArray *leaderboard)
						  {
                              if(leaderboard != nil)
                              {
                                  [_clans addObjectsFromArray:leaderboard];
                              }
                              dispatch_group_leave(group);
						  }];
	
	// Execute the following block when both operations are complete.
	dispatch_group_notify(group, dispatch_get_main_queue(), ^
    {
		if(_currentUserClanEntry != nil)
		{
			// Show the user entry and resize the table
			[_currentUserClanCell showPlayerInfo:_currentUserClanEntry];
			
			_currentUserClanCell.hidden = NO;
			
			CGRect rect = _clansTable.frame;
			rect.origin.y += _currentUserClanCell.frame.size.height;
			rect.size.height -= _currentUserClanCell.frame.size.height;
			_clansTable.frame = rect;
		}
		
		// Show the table, hide the spinner
		[_clansTable reloadData];
        [_clansTable setHidden:NO];
        [_clansSpinner stopAnimating];
        [_clansSpinner setHidden:YES];
        
        if(self.onLeaderboardsLoaded)
        {
            BOOL hasLeaderboards = _players != nil && _players.count > 0;
            self.onLeaderboardsLoaded(hasLeaderboards);
        }
	});
}

- (void) fetchMorePlayers
{
    _lastPlayerRequested = _players.count;
    [RobloxData fetchLeaderboardsFor:RBXLeaderboardsTargetUser
                          timeFilter:self.timeFilter
                       distributorID:self.distributorID
                          startIndex:_players.count
                            numItems:NUM_ITEMS_PER_REQUEST
                           lastEntry:[_players lastObject]
                          avatarSize:AVATAR_SIZE
                          completion:^(NSArray *leaderboard)
                          {
                              if(leaderboard != nil)
                              {
                                  [_players addObjectsFromArray:leaderboard];
                                  dispatch_async(dispatch_get_main_queue(), ^
                                  {
                                      NSMutableArray* items = [NSMutableArray array];
                                      for(int i = _players.count - leaderboard.count; i < _players.count; ++i)
                                      {
                                          [items addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                                      }
                                      [_playersTable insertRowsAtIndexPaths:items withRowAnimation:UITableViewRowAnimationNone];
                                  });
                              }
                          }];
}

- (void) fetchMoreClans
{
    _lastClanRequested = _clans.count;
    [RobloxData fetchLeaderboardsFor:RBXLeaderboardsTargetClan
                          timeFilter:self.timeFilter
                       distributorID:self.distributorID
                          startIndex:_clans.count
                            numItems:NUM_ITEMS_PER_REQUEST
                           lastEntry:[_clans lastObject]
                          avatarSize:AVATAR_SIZE
                          completion:^(NSArray *leaderboard)
                          {
                              if(leaderboard != nil)
                              {
                                  [_clans addObjectsFromArray:leaderboard];
                                  dispatch_async(dispatch_get_main_queue(), ^
                                  {
                                      NSMutableArray* items = [NSMutableArray array];
                                      for(int i = _clans.count - leaderboard.count; i < _clans.count; ++i)
                                      {
                                          [items addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                                      }
                                      [_clansTable insertRowsAtIndexPaths:items withRowAnimation:UITableViewRowAnimationNone];
                                  });
                              }
                          }];
}

#pragma odd style functions
-(void) applyThemeToLeaderboardUserCell:(LeaderboardCell*)cell
{
    cell.titleLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    cell.titleLabel.textColor = [UIColor colorWithRed:(0x41/255.0f) green:(0x63/255.0f) blue:(0x99/255.0f) alpha:1.0f];
    
    cell.subtitleLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:12];
    cell.subtitleLabel.textColor = [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f];
    
    cell.rankLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:13];
    cell.rankLabel.textColor = [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f];
    
    cell.pointsLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    cell.pointsLabel.textColor = [UIColor colorWithRed:(0x41/255.0f) green:(0x63/255.0f) blue:(0x99/255.0f) alpha:1.0f];
    
    // Add a separator line for this "header" cell
    UIImageView* separator = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"Separator"]];
    CGRect frame = cell.contentView.bounds;
    frame.origin.y = frame.size.height - 1;
    frame.size.height = 1;
    separator.frame = frame;
    [cell.contentView addSubview:separator];
}


#pragma Delegate functions
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    NSArray* elements = tableView == _playersTable ? _players : _clans;
    return elements != nil ? elements.count : 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    LeaderboardCell* cell = [tableView dequeueReusableCellWithIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    if([tableView isEqual:_playersTable])
    {
        [cell showPlayerInfo:_players[indexPath.row]];
        
        if(_lastPlayerRequested < _players.count && indexPath.row + START_REQUEST_THRESHOLD > _players.count)
            [self fetchMorePlayers];
    }
    else if([tableView isEqual:_clansTable])
    {
        [cell showClanInfo:_clans[indexPath.row]];
        
        if(_lastClanRequested < _clans.count && indexPath.row + START_REQUEST_THRESHOLD > _clans.count)
            [self fetchMoreClans];
    }
    
    //[RobloxTheme applyToLeaderboardCell:cell];
    cell.titleLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    cell.titleLabel.textColor = [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f];
    
    cell.subtitleLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:12];
    cell.subtitleLabel.textColor = [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f];
    
    cell.rankLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:13];
    cell.rankLabel.textColor = [UIColor colorWithWhite:(0x97/255.0f) alpha:1.0f];
    
    cell.pointsLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    cell.pointsLabel.textColor = [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f];
    
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    RBProfileViewController *viewController = (RBProfileViewController*)
    [self.storyboard instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
    
    if ([tableView isEqual:_playersTable]) {
        [Flurry logEvent:LBDC_didPressLeaderboardPlayerItem];
        RBXLeaderboardEntry* player = _players[indexPath.row];
        viewController.userId = @(player.userID);
    } else if ([tableView isEqual:_clansTable]) {
        [Flurry logEvent:LBDC_didPressLeaderboardClaneItem];
        RBXLeaderboardEntry* player = _clans[indexPath.row];
        viewController.userId = @(player.userID);
    }
    [self.navigationController pushViewController:viewController animated:YES];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return CELL_SIZE.height;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
