//
//  FriendsRequestsDetailController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/19/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "FriendsRequestsDetailController.h"
#import "RobloxImageView.h"
#import "RobloxData.h"
#import "UserInfo.h"
#import "RobloxHUD.h"
#import "RobloxTheme.h"
#import "RobloxNotifications.h"
#import "RBProfileViewController.h"
#import "Flurry.h"
#include "RBActivityIndicatorView.h"
#import "UIView+Position.h"

#define AVATAR_SIZE CGSizeMake(110, 110)
#define ITEMS_PER_REQUEST 1000
#define SPINNER_ICON_FRAME CGRectMake(0, 0, 32, 32)

//---METRICS---
#define FRDC_acceptFriendRequest    @"FRIEND REQUEST SCREEN - Accept Friend Request"
#define FRDC_declineFriendRequest   @"FRIEND REQUEST SCREEN - Decline Friend Reqeust"
#define FRDC_refreshList            @"FRIEND REQUEST SCREEN - Refresh Requests List"
#define FRDC_openFriendProfile      @"FRIEND REQUEST SCREEN - Open Friend Request Profile"

#pragma mark FriendRequestTableCell
@interface FriendRequestTableCell : UITableViewCell
@property (strong, nonatomic) RBXFriendInfo* friendInfo;
@property (strong, nonatomic) IBOutlet RobloxImageView *friendAvatar;
@property (strong, nonatomic) IBOutlet UILabel *friendNameLabel;
@property (strong, nonatomic) IBOutlet UILabel *friendPointsLabel;
@property (strong, nonatomic) IBOutlet UIButton *acceptButton;
@property (strong, nonatomic) IBOutlet UIButton *doNotAcceptButton;
@end

@implementation FriendRequestTableCell
- (void)setFriendInfo:(RBXFriendInfo *)friendInfo
{
    _friendInfo = friendInfo;
    
    // Stylize items
    self.friendNameLabel.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:14];
    self.friendNameLabel.textColor = [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f];
    
    self.friendPointsLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    self.friendPointsLabel.textColor = [UIColor colorWithWhite:(0x34/255.0f) alpha:1.0f];
    self.friendPointsLabel.hidden = YES;
    
    self.friendNameLabel.text = friendInfo.username;
    [self.friendAvatar loadAvatarForUserID:[friendInfo.userID integerValue] prefetchedURL:friendInfo.avatarURL urlIsFinal:friendInfo.avatarIsFinal withSize:AVATAR_SIZE completion:nil];
}
@end

#pragma mark -
#pragma mark FriendsRequestsDetailController

@interface FriendsRequestsDetailController () <UITableViewDelegate, UITableViewDataSource>

@end

@implementation FriendsRequestsDetailController
{
    IBOutlet UITableView* _requestsTable;
    IBOutlet UILabel* _noRequestsLabel;
    UIRefreshControl* _refreshIndicator;
    RBActivityIndicatorView* _loadingSpinner;
    BOOL _initialized;
    NSMutableArray* _requests;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    _requests = [NSMutableArray array];
    
    _initialized = NO;
    _requestsTable.backgroundColor = [UIColor clearColor];
    _requestsTable.delegate = self;
    _requestsTable.dataSource = self;
    _requestsTable.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
    
    _refreshIndicator = [[UIRefreshControl alloc] init];
    [_refreshIndicator addTarget:self action:@selector(refreshRequests:) forControlEvents:UIControlEventValueChanged];
    [_requestsTable addSubview:_refreshIndicator];
    
    _loadingSpinner = [[RBActivityIndicatorView alloc] initWithFrame:SPINNER_ICON_FRAME];
    [self.view addSubview:_loadingSpinner];
    
    _noRequestsLabel.text = NSLocalizedString(@"NoNewRequests", nil);
    _noRequestsLabel.textAlignment = NSTextAlignmentCenter;
    _noRequestsLabel.hidden = YES;
}

- (void) viewDidLayoutSubviews
{
    //center the label
    [_noRequestsLabel centerInFrame:self.view.frame];
    
    //center the loading spinner
    [_loadingSpinner centerInFrame:self.view.frame];
}

- (void) loadData
{
    if(!_initialized)
    {
        _initialized = YES;
        _requestsTable.hidden = YES;
        [_loadingSpinner startAnimating];
        
        UserInfo* userInfo = [UserInfo CurrentPlayer];
        [RobloxData fetchUserFriends:[NSNumber numberWithInteger:[userInfo.userId integerValue]]
                          friendType:RBXFriendTypeFriendRequest
                          startIndex:0
                            numItems:ITEMS_PER_REQUEST
                          avatarSize:AVATAR_SIZE completion:^(NSUInteger totalFriends, NSArray *friends)
        {
            [_requests removeAllObjects];
            [_requests addObjectsFromArray:friends];
            
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [_loadingSpinner stopAnimating];
                _noRequestsLabel.hidden = ([_requests count] > 0);
                _requestsTable.hidden = NO;
                [_requestsTable reloadData];
            });
        }];
    }
}
- (void) refreshRequests:(UIRefreshControl*)refreshControl
{
    if (refreshControl)
        [Flurry logEvent:FRDC_refreshList];
    
    //make a request to pull down the messages from the server
    UserInfo* userInfo = [UserInfo CurrentPlayer];
    [RobloxData fetchUserFriends:[NSNumber numberWithInteger:[userInfo.userId integerValue]]
                      friendType:RBXFriendTypeFriendRequest
                      startIndex:0
                        numItems:ITEMS_PER_REQUEST
                      avatarSize:AVATAR_SIZE completion:^(NSUInteger totalFriends, NSArray *friends)
     {
         //loop through the list of messages and look for differences between the two sets
         //since invitationIDs are sorted chronologically on the server, we can easily check what messages are new, the same, or missing in the new list
         NSMutableArray* mergedRequests = [NSMutableArray arrayWithCapacity:([friends count] + [_requests count])];
         NSMutableArray* setToAdd = [NSMutableArray arrayWithCapacity:[friends count]];
         NSMutableArray* setToRemove = [NSMutableArray arrayWithCapacity:[_requests count]];
         
         int i = 0; int j = 0;
         while (i < friends.count && j < _requests.count)
         {
             RBXFriendInfo* requestNew = friends[i];
             RBXFriendInfo* requestOld = _requests[j];
             if ([requestNew invitationID] > [requestOld invitationID])
             {
                 [setToAdd addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                 [mergedRequests addObject:friends[i]];
                 i++;
             }
             else if ([requestNew invitationID] == [requestOld invitationID])
             {
                 [mergedRequests addObject:friends[i]];
                 i++; j++;
             }
             else
             {
                 [setToRemove addObject:[NSIndexPath indexPathForRow:j inSection:0]];
                 j++;
             }
         }
         
         //handle the remaining messages : add the new ones, remove the old ones
         for (; i < friends.count; i++)
         {
             [setToAdd addObject:[NSIndexPath indexPathForRow:i inSection:0]];
             [mergedRequests addObject:friends[i]];
         }
         for (; j < _requests.count; j++)
         {
             [setToRemove addObject:[NSIndexPath indexPathForRow:j inSection:0]];
         }
         
         //assign the combined list of messages to the table array
         _requests = mergedRequests;
         
         //stop the animation and refresh the table
         dispatch_async(dispatch_get_main_queue(), ^
                        {
                            _noRequestsLabel.hidden = ([_requests count] > 0);
                            
                            if (refreshControl)
                                [refreshControl endRefreshing];
                            
                            if (_initialized)
                            {
                                //animate the change
                                [_requestsTable beginUpdates];
                                [_requestsTable deleteRowsAtIndexPaths:setToRemove withRowAnimation:UITableViewRowAnimationAutomatic];
                                [_requestsTable insertRowsAtIndexPaths:setToAdd withRowAnimation:UITableViewRowAnimationAutomatic];
                                [_requestsTable endUpdates];
                            }
                            else
                            {
                                //or simply
                                [_requestsTable reloadData];
                            }
                        });
     }];
}

#pragma mark -
#pragma mark Actions

-(FriendRequestTableCell*)parentCellForView:(id)theView
{
    id viewSuperView = [theView superview];
    while (viewSuperView != nil)
    {
        if ([viewSuperView isKindOfClass:[FriendRequestTableCell class]])
        {
            return (FriendRequestTableCell*)viewSuperView;
        }
        else
        {
            viewSuperView = [viewSuperView superview];
        }
    }
    return nil;
}

-(void)acceptFriendRequest:(id)sender
{
    [Flurry logEvent:FRDC_acceptFriendRequest];
    
    FriendRequestTableCell* cell = [self parentCellForView:sender];
    NSIndexPath* cellIndex = [_requestsTable indexPathForCell:cell];
    [cell acceptButton].enabled = NO;
    [cell doNotAcceptButton].enabled = NO;
    
    RBXFriendInfo* request = _requests[cellIndex.row];
    NSInteger invitationID = request.invitationID;
    NSInteger targetUserID = request.userID.integerValue;
    
    [RobloxData acceptFriendRequest:invitationID
                   withTargetUserID:targetUserID
                         completion:^(BOOL success)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            if (success)
            {
                [_requests removeObjectAtIndex:cellIndex.row];
                [_requestsTable deleteRowsAtIndexPaths:[NSArray arrayWithObject:cellIndex] withRowAnimation:UITableViewRowAnimationNone];
                
                [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_FRIENDS_UPDATED object:nil];
                
                //update the UI elements
                _noRequestsLabel.hidden = ([_requests count] > 0);
            }
            
            //re-enable the buttons
            [cell acceptButton].enabled = YES;
            [cell doNotAcceptButton].enabled = YES;
        });
    }];
}

-(void)declineFriendRequest:(id)sender
{
    [Flurry logEvent:FRDC_declineFriendRequest];
    
    FriendRequestTableCell* cell = [self parentCellForView:sender];
    NSIndexPath* cellIndex = [_requestsTable indexPathForCell:cell];
    [cell acceptButton].enabled = NO;
    [cell doNotAcceptButton].enabled = NO;
    
    RBXFriendInfo* request = _requests[cellIndex.row];
    NSInteger invitationID = request.invitationID;
    NSInteger targetUserID = request.userID.integerValue;
    
    [RobloxData declineFriendRequest:invitationID
                    withTargetUserID:targetUserID
                          completion:^(BOOL success)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            if(success)
            {
                [_requests removeObjectAtIndex:cellIndex.row];
                [_requestsTable deleteRowsAtIndexPaths:[NSArray arrayWithObject:cellIndex] withRowAnimation:UITableViewRowAnimationNone];
                
                [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_FRIENDS_UPDATED object:nil];
                
                //update the UI elements
                _noRequestsLabel.hidden = ([_requests count] > 0);
            }
            
            //re-enable the buttons
            [cell acceptButton].enabled = YES;
            [cell doNotAcceptButton].enabled = YES;
        });
    }];
}

#pragma mark -
#pragma mark UITableView delegate

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _requests.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString* CellIdentifier = @"ReuseCell";
    FriendRequestTableCell* cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier forIndexPath:indexPath];
    RBXFriendInfo* friendInfo = _requests[indexPath.row];
    cell.friendInfo = friendInfo;
    [cell.acceptButton addTarget:self action:@selector(acceptFriendRequest:) forControlEvents:UIControlEventTouchUpInside];
    [cell.acceptButton setEnabled:YES];
    [cell.doNotAcceptButton addTarget:self action:@selector(declineFriendRequest:) forControlEvents:UIControlEventTouchUpInside];
    [cell.doNotAcceptButton setEnabled:YES];
    cell.backgroundColor = [UIColor clearColor];
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [Flurry logEvent:FRDC_openFriendProfile];
    RBXFriendInfo* friendInfo = _requests[indexPath.row];
    
    RBProfileViewController *viewController = [self.storyboard instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
    viewController.userId = friendInfo.userID;
    [self.navigationController pushViewController:viewController animated:YES];
    
}

@end
