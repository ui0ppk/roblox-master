//
//  RBProfileViewController.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/6/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBProfileViewController.h"
#import "RobloxTheme.h"
#import "LoginManager.h"
#import "WelcomeScreenController.h"
#import "RBPurchaseViewController.h"
#import "RBAccountManagerViewController.h"
#import "RobloxImageView.h"
#import "UIAlertView+Blocks.h"
#import "RobloxData.h"
#import "UserInfo.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "Flurry.h"
#import "RBbarButtonMenu.h"
#import "UIScrollView+Auto.h"
#import "UIView+Position.h"
#import "RobloxHUD.h"
#import "GameSortHorizontalViewController.h"
#import "GameThumbnailCell.h"
#import "RBPlayerThumbnailCell.h"
#import "RBFullFriendListScreenController.h"
#import "RBWebGamePreviewScreenController.h"
#import "RBSingleSortScreenController.h"
#import "RBMessageComposeScreenController.H"
#import "GameBadgeViewController.h"

#define ROBLOXBADGECELL_ID  @"RobloxBadgeCell"
#define PLAYERBADGECELL_ID  @"PlayerBadgeCell"
#define FRIENDCELL_ID       @"FriendCell"
#define BADGECELL_SUPP      @"BadgeCellSupp"
#define FRIENDCELL_SUPP     @"FriendCellSupp"

#define GAME_ITEM_SIZE CGSizeMake(225, 155)
#define GAME_THUMBNAIL_SIZE CGSizeMake(420, 230)
#define PROFILE_IMAGE_SIZE CGSizeMake(352, 352)
#define FRIEND_AVATAR_SIZE CGSizeMake(110, 110)
#define BADGE_SIZE CGSizeMake(110, 110) // Player badges are fixed sized, so the request

#define MAX_ELEMENTS_PER_REQUEST 20

//---METRICS---
#define PVC_didPressBuyRobux        @"PROFILE SCREEN - Did Press Buy Robux"
#define PVC_didPressBuildersClub    @"PROFILE SCREEN - Did Press Builders Club"
#define PVC_didPressEditAccount     @"PROFILE SCREEN - Did Press Edit Account"
#define PVC_didPressLogOut          @"PROFILE SCREEN - Did Press Log Out"

#pragma mark -
#pragma mark Collection View Cells

//---------------------------------------------------------------------------
@interface RBXPlayerBadgeCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet RobloxImageView* badgeImage;
@property (strong, nonatomic) RBXBadgeInfo* badgeInfo;

@end

@implementation RBXPlayerBadgeCell

- (void)setBadgeInfo:(RBXBadgeInfo *)badgeInfo
{
    _badgeInfo = badgeInfo;
    
    self.badgeImage.animateInOptions = RBXImageViewAnimateInAlways;
    [self.badgeImage loadBadgeWithURL:badgeInfo.imageURL withSize:BADGE_SIZE completion:nil];
}

@end

@interface RBXRobloxBadgeCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet RobloxImageView* badgeImage;
@property (strong, nonatomic) RBXBadgeInfo* badgeInfo;

@end

@implementation RBXRobloxBadgeCell

- (void)setBadgeInfo:(RBXBadgeInfo *)badgeInfo
{
    _badgeInfo = badgeInfo;
    
    self.badgeImage.animateInOptions = RBXImageViewAnimateInAlways;
    [self.badgeImage loadBadgeWithURL:badgeInfo.imageURL withSize:BADGE_SIZE completion:nil];
}

@end
//---------------------------------------------------------------------------

#pragma mark -
#pragma mark Profile View Controller

@interface RBProfileViewController () <UIActionSheetDelegate>

@end

@implementation RBProfileViewController
{
    RBXUserProfileInfo* _profile;
    
    NSMutableArray* _playerBadges;
    NSMutableArray* _robloxBadges;
    NSMutableArray* _friends;
    
    GameSortHorizontalViewController* _recentlyPlayedViewController;
    GameSortHorizontalViewController* _myGamesViewController;
    GameSortHorizontalViewController* _favoriteGamesViewController;
    
    IBOutlet UIView* _headerContainer;
    IBOutlet RobloxImageView* _avatarImageView;
    IBOutlet UILabel* _userNameLabel;
    IBOutlet UILabel* _robuxTitle;
    IBOutlet UILabel* _ticketsTitle;
    IBOutlet UILabel* _friendsTitle;
    IBOutlet UILabel* _robuxLabel;
    IBOutlet UILabel* _ticketsLabel;
    IBOutlet UILabel* _friendsLabel;
    IBOutlet UILabel* _badgesHeaderTitle;
    IBOutlet UILabel* _friendsHeaderTitle;
    IBOutlet UIButton* _seeAllFriendsButton;
    IBOutlet UICollectionView* _badgesCollectionView;
    IBOutlet UICollectionView* _friendsCollectionView;
    IBOutlet UIButton *_friendshipButton;
    IBOutlet UIButton *_sendMessageButton;
    
    IBOutlet UIScrollView* _scrollView;
    IBOutlet UIView* _friendsContainerView;
    IBOutlet UIView* _badgesContainerView;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        self.title = NSLocalizedString(@"Profile", nil);
        
        _playerBadges = [NSMutableArray array];
        _robloxBadges = [NSMutableArray array];
        _friends = [NSMutableArray array];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    // Localize, initialize and stylize controls
    _friendsTitle.text = NSLocalizedString(@"FriendsWord", nil);
    _badgesHeaderTitle.text = [NSLocalizedString(@"BadgesWord", nil) uppercaseString];
    _friendsHeaderTitle.text = [NSLocalizedString(@"FriendsWord", nil) uppercaseString];
    _userNameLabel.text = @"";
    _friendsLabel.text = @"";    
    
    _sendMessageButton.hidden = YES;
    _friendshipButton.hidden = YES;
    
    UIBarButtonItem* backButton = [[UIBarButtonItem alloc] initWithTitle:@"" style:UIBarButtonItemStyleBordered target:nil action:nil];
    [self.navigationItem setBackBarButtonItem:backButton];
    
    // Some controls are not in the view if this user is not the one logged in
    if( [self isSessionUser] )
    {
        _robuxTitle.text = [NSLocalizedString(@"RobuxWord", nil) uppercaseString];
        _ticketsTitle.text = NSLocalizedString(@"TicketsWord", nil);
        _robuxLabel.text = @"";
        _ticketsLabel.text = @"";
        
        [RobloxTheme applyToProfileHeaderSubtitle:_robuxTitle];
        [RobloxTheme applyToProfileHeaderSubtitle:_ticketsTitle];

        [RobloxTheme applyToProfileHeaderValue:_robuxLabel];
        [RobloxTheme applyToProfileHeaderValue:_ticketsLabel];
        
        [self addRobuxIconWithFlurryEvent:PVC_didPressBuyRobux
                 andBCIconWithFlurryEvent:PVC_didPressBuildersClub
               andEditIconWithFlurryEvent:PVC_didPressEditAccount
                 andLogOutWithFlurryEvent:PVC_didPressLogOut];
    }
    else
    {
        [_friendshipButton addTarget:self action:@selector(friendshipButtonTouchUpInside:) forControlEvents:UIControlEventTouchUpInside];
    }
    
    [_friendsCollectionView registerNib:[UINib nibWithNibName:@"RBPlayerThumbnailCell" bundle:nil] forCellWithReuseIdentifier:FRIENDCELL_ID];
    
    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    [RobloxTheme applyToProfileHeaderSubtitle:_friendsTitle];
    [RobloxTheme applyToProfileHeaderValue:_friendsLabel];
    [RobloxTheme applyToProfileHeaderValue:_userNameLabel];
    [RobloxTheme applyToTableHeaderTitle:_badgesHeaderTitle];
    [RobloxTheme applyToTableHeaderTitle:_friendsHeaderTitle];
    [RobloxTheme applyToGameSortSeeAllButton:_seeAllFriendsButton];
    
    [_seeAllFriendsButton setTitle:[NSLocalizedString(@"SeeAllPhrase", nil) uppercaseString] forState:UIControlStateNormal];
    
    [self requestUserInfo];
    
    [self setUpCollectionViews];
    
    // Set header background and border
    _headerContainer.backgroundColor = [UIColor whiteColor];
    CALayer* headerLayer = _headerContainer.layer;
    headerLayer.shadowOpacity = 0.25;
    headerLayer.shadowColor = [UIColor colorWithWhite:(0x6B/255.f) alpha:1.0].CGColor;
    headerLayer.shadowOffset = CGSizeMake(0,0);
    headerLayer.shadowRadius = 2.0;
}

- (void) viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onFriendsUpdated:) name:RBXNotificationFriendsUpdated object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onRobuxUpdated:) name:RBXNotificationRobuxUpdated object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(fetchPointsAndUpdate) name:RBXNotificationDidLeaveGame object:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) onFriendsUpdated:(NSNotification *)notification
{
    NSNumber* userID = [self getUserID];
    [self fetchFriendsForUser:userID];
}

- (void)onRobuxUpdated:(NSNotification*)notification
{
    [self fetchMyProfileAndUpdateRobux:YES];
}

- (BOOL)isSessionUser
{
    return self.userId == nil;
}

- (NSNumber*)getUserID
{
    if([self isSessionUser])
    {
        UserInfo* userInfo = [UserInfo CurrentPlayer];
        return [NSNumber numberWithInt:[userInfo.userId intValue]];
    }
    else
        return self.userId;
}

- (void)fetchMyProfileAndUpdateRobux:(BOOL)updateRobux
{
    [RobloxData fetchMyProfileWithSize:PROFILE_IMAGE_SIZE completion:^(RBXUserProfileInfo *profile)
     {
         _profile = profile;
         dispatch_async(dispatch_get_main_queue(), ^
         {
             if (updateRobux)
             {
                 _robuxLabel.text = [NSString stringWithFormat:@"%d", profile.robux];
             }
             else
             {
                 [self showUserInfo:profile];
             }
         });
     }];
}

- (void)fetchPointsAndUpdate
{
    [RobloxData fetchMyProfileWithSize:PROFILE_IMAGE_SIZE completion:^(RBXUserProfileInfo *profile)
    {
//         dispatch_async(dispatch_get_main_queue(), ^{
//            self.pointsLabel.text = profile.displayPoints;
//         });
     }];
}

- (void)fetchFriendsForUser:(NSNumber *)userID
{
    // ** Request friends **
    _friends = [NSMutableArray array];
    [_friendsCollectionView reloadData];
    
    [RobloxData fetchUserFriends:userID friendType:RBXFriendTypeAllFriends startIndex:0 numItems:MAX_ELEMENTS_PER_REQUEST avatarSize:FRIEND_AVATAR_SIZE completion:^(NSUInteger totalFriends, NSArray *friends)
     {
         if(friends)
         {
             [_friends addObjectsFromArray:friends];
             dispatch_async(dispatch_get_main_queue(), ^
             {
                 _friendsLabel.text = [NSString stringWithFormat:@"%d", totalFriends];
                 [_friendsCollectionView reloadData];
             });
         }
     }];
}

- (void) requestUserInfo
{
    NSNumber* userID = [self getUserID];
    
    // ** Request basic user info **
    // If no user id is set, load the current user info
    if( self.userId == nil )
    {
        [self fetchMyProfileAndUpdateRobux:NO];
    }
    else
    {
        [RobloxData fetchUserProfile:userID avatarSize:PROFILE_IMAGE_SIZE completion:^(RBXUserProfileInfo *profile)
        {
            _profile = profile;
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [self showUserInfo:profile];
            });
        }];
    }
    
    // ** Request badges **
    _playerBadges = [NSMutableArray array];
    [_badgesCollectionView reloadData];
    
    [RobloxData fetchUserBadges:RBXUserBadgeTypePlayer forUser:userID badgeSize:BADGE_SIZE completion:^(NSArray *badges)
    {
        if(badges)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _badgesContainerView.hidden = _badgesContainerView.hidden || badges == nil || badges.count == 0;
                
                [_playerBadges addObjectsFromArray:badges];
                [_badgesCollectionView reloadData];
                
                [self layoutCollectionViews];
            });
        }
    }];
    
    _robloxBadges = [NSMutableArray array];
    [_badgesCollectionView reloadData];
    
    [RobloxData fetchUserBadges:RBXUserBadgeTypeRoblox forUser:userID badgeSize:BADGE_SIZE completion:^(NSArray *badges)
    {
        if(badges)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _badgesContainerView.hidden = _badgesContainerView.hidden || badges == nil || badges.count == 0;
                
                [_robloxBadges addObjectsFromArray:badges];
                [_badgesCollectionView reloadData];
                
                [self layoutCollectionViews];
            });
        }
    }];
    
    [self fetchFriendsForUser:userID];
}

- (void) showUserInfo:(RBXUserProfileInfo*)profile
{
    _userNameLabel.text = profile.username;
    [_avatarImageView loadAvatarForUserID:profile.userID prefetchedURL:profile.avatarURL urlIsFinal:profile.avatarIsFinal withSize:PROFILE_IMAGE_SIZE completion:nil];
    
    if( [self isSessionUser] )
    {
        _robuxLabel.text = [NSString stringWithFormat:@"%d", profile.robux];
        _ticketsLabel.text = [NSString stringWithFormat:@"%d", profile.tickets];
    }
    else
    {
        _sendMessageButton.hidden = NO;
        _friendshipButton.hidden = NO;
        [self updateFrienshipButton];
        
        self.title = [NSString stringWithFormat:NSLocalizedString(@"ProfileTitleFormat", nil), profile.username];
        _myGamesViewController.sortTitle = [NSString stringWithFormat:NSLocalizedString(@"FriendPlacesFormat", nil), _profile.username];
    }
}

- (void)setUpCollectionViews
{
    __weak RBProfileViewController* weakSelf = self;
    
    NSNumber* userID = [self getUserID];

    // Recently played games
    if( [self isSessionUser] )
    {
        _recentlyPlayedViewController = [[GameSortHorizontalViewController alloc] initWithNibName:@"GameSortHorizontalViewController" bundle:nil];
        _recentlyPlayedViewController.gameSelectedHandler = ^(RBXGameData* gameData) { [weakSelf showGameDetails:gameData]; };
        _recentlyPlayedViewController.seeAllHandler = ^(NSNumber* sortID) { [weakSelf seeAllGamesForSort:sortID]; };
        [self addChildViewController:_recentlyPlayedViewController];
        [_scrollView addSubview:_recentlyPlayedViewController.view];
        
        [RobloxData fetchGameList:[RBXGameSort RecentlyPlayedGamesSort].sortID
                        fromIndex:0
                         numGames:MAX_ELEMENTS_PER_REQUEST
                        thumbSize:GAME_THUMBNAIL_SIZE
                       completion:^(NSArray *games)
                       {
                            dispatch_async(dispatch_get_main_queue(), ^
                            {
                                _recentlyPlayedViewController.view.hidden = games == nil || games.count == 0;
                                [_recentlyPlayedViewController setSort:[RBXGameSort RecentlyPlayedGamesSort] withGames:games];
                                [self layoutCollectionViews];
                            });
                       }];
    }

    // My games
    _myGamesViewController = [[GameSortHorizontalViewController alloc] initWithNibName:@"GameSortHorizontalViewController" bundle:nil];
    _myGamesViewController.gameSelectedHandler = ^(RBXGameData* gameData) { [weakSelf showGameDetails:gameData]; };
    _myGamesViewController.seeAllHandler = ^(NSNumber* sortID) { [weakSelf seeAllGamesForSort:sortID]; };
    [self addChildViewController:_myGamesViewController];
    [_scrollView addSubview:_myGamesViewController.view];

    [RobloxData
        fetchUserPlaces:userID
        startIndex:0
        numGames:MAX_ELEMENTS_PER_REQUEST
        thumbSize:GAME_THUMBNAIL_SIZE
        completion:^(NSArray *games)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _myGamesViewController.view.hidden = games == nil || games.count == 0;
                [_myGamesViewController setSort:[RBXGameSort MyGamesSort] withGames:games];
                
                if( _profile != nil && ![self isSessionUser] )
                {
                    _myGamesViewController.sortTitle = [NSString stringWithFormat:NSLocalizedString(@"FriendPlacesFormat", nil), _profile.username];
                }
            });
        }];

    // Favorite games
    _favoriteGamesViewController = [[GameSortHorizontalViewController alloc] initWithNibName:@"GameSortHorizontalViewController" bundle:nil];
    _favoriteGamesViewController.gameSelectedHandler = ^(RBXGameData* gameData) { [weakSelf showGameDetails:gameData]; };
    _favoriteGamesViewController.seeAllHandler = ^(NSNumber* sortID) { [weakSelf seeAllGamesForSort:sortID]; };
    [self addChildViewController:_favoriteGamesViewController];
    [_scrollView addSubview:_favoriteGamesViewController.view];
    
    [RobloxData
        fetchUserFavoriteGames:userID
        startIndex:0
        numGames:MAX_ELEMENTS_PER_REQUEST
        thumbSize:GAME_THUMBNAIL_SIZE
        completion:^(NSArray *games)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _favoriteGamesViewController.view.hidden = games == nil || games.count == 0;
                [_favoriteGamesViewController setSort:[RBXGameSort FavoriteGamesSort] withGames:games];
                [self layoutCollectionViews];
            });
        }];
    
    // Badges
    _badgesCollectionView.dataSource = self;
    _badgesCollectionView.delegate = self;
    [_badgesCollectionView setContentInset:UIEdgeInsetsMake(0.0, 0.0, 0.0, 0.0)];
    
    // Friends
    _friendsCollectionView.dataSource = self;
    _friendsCollectionView.delegate = self;
    [_friendsCollectionView setContentInset:UIEdgeInsetsMake(0.0, 0.0, 0.0, 0.0)];
    
    [self layoutCollectionViews];
}

- (void)layoutCollectionViews
{
    #define OFFSET_Y 20
    
    CGFloat lastY = CGRectGetMaxY(_friendsContainerView.frame) + OFFSET_Y;
    
    if(_recentlyPlayedViewController != nil && _recentlyPlayedViewController.view.hidden == NO)
    {
        _recentlyPlayedViewController.view.y = lastY;
        lastY = CGRectGetMaxY(_recentlyPlayedViewController.view.frame) + OFFSET_Y;
    }
    
    if(_myGamesViewController != nil && _myGamesViewController.view.hidden == NO)
    {
        _myGamesViewController.view.y = lastY;
        lastY = CGRectGetMaxY(_myGamesViewController.view.frame) + OFFSET_Y;
    }
    
    if(_favoriteGamesViewController != nil && _favoriteGamesViewController.view.hidden == NO)
    {
        _favoriteGamesViewController.view.y = lastY;
        lastY = CGRectGetMaxY(_favoriteGamesViewController.view.frame) + OFFSET_Y;
    }
    
    if(_badgesContainerView.hidden == NO)
    {
        _badgesContainerView.y = lastY;
        
        _badgesCollectionView.size = _badgesCollectionView.contentSize;
        
        _badgesContainerView.size = CGRectUnion(_badgesCollectionView.frame, _badgesHeaderTitle.frame).size;
    }
    
    [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
}

- (IBAction)didPressSendMessage:(id)sender
{
    [self performSegueWithIdentifier:@"composeMessage" sender:nil];
}

- (IBAction)didPressFriendshipButton:(id)sender
{
    
}


- (IBAction)didPressSeeAllFriends:(id)sender
{
    RBFullFriendListScreenController* controller = [[RBFullFriendListScreenController alloc] init];
    controller.playerID = _profile.userID;
    controller.playerName = _profile.username;
    [self.navigationController pushViewController:controller animated:YES];
}

- (void) showGameDetails:(RBXGameData*)gameData
{
    RBWebGamePreviewScreenController* controller = [[RBWebGamePreviewScreenController alloc] init];
    controller.gameData = gameData;
    [self.navigationController pushViewController:controller animated:YES];
}

- (void) seeAllGamesForSort:(NSNumber*)sortID
{
    RBSingleSortScreenController* controller = [[RBSingleSortScreenController alloc] init];
    controller.sortID = sortID;
    controller.playerID = [self getUserID];
    [self.navigationController pushViewController:controller animated:YES];
}

#pragma mark -
#pragma mark Collection View Delegate Methods

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    if (collectionView == _badgesCollectionView) {
        if (_robloxBadges.count && _playerBadges.count) {
            return 2;
        } else {
            return 1;
        }
    }
    
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    if(collectionView == _badgesCollectionView)
    {
        switch (section) {
            case 0:
                return _robloxBadges.count;
                break;
            case 1:
                return _playerBadges.count;
                break;
            default:
                break;
        }
    }
    else if(collectionView == _friendsCollectionView)
    {
        return _friends.count;
    }
    
    return 0;
}

- (UICollectionViewCell*)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if (collectionView == _badgesCollectionView)
    {
        switch (indexPath.section)
        {
            case 0:
            {
                RBXRobloxBadgeCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:ROBLOXBADGECELL_ID forIndexPath:indexPath];
                cell.badgeInfo = _robloxBadges[indexPath.row];
                
                return cell;
            }
            break;
            case 1:
            {
                RBXPlayerBadgeCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:PLAYERBADGECELL_ID forIndexPath:indexPath];
                cell.badgeInfo = _playerBadges[indexPath.row];
                
                return cell;
            }
            break;
            default:
                break;
        }
    }
    else if (collectionView == _friendsCollectionView)
    {
        RBPlayerThumbnailCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:FRIENDCELL_ID forIndexPath:indexPath];
        cell.friendInfo = _friends[indexPath.row];
        return cell;
    }
    
    return nil;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    [collectionView deselectItemAtIndexPath:indexPath animated:YES];
    
    if ([collectionView isEqual:_friendsCollectionView])
    {
        RBProfileViewController *viewController = (RBProfileViewController*) [self.storyboard instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
        
        RBXFriendInfo* friendInfo = _friends[indexPath.row];
        viewController.userId = friendInfo.userID;
        
        [self.navigationController pushViewController:viewController animated:YES];
    }
    else if ([collectionView isEqual:_badgesCollectionView])
    {
        /*RBXBadgeInfo* badge = (indexPath.section == 0) ? _robloxBadges[indexPath.row] : _playerBadges[indexPath.row];
        if (!badge) return;
        GameBadgeViewController* badgePopup = [[GameBadgeViewController alloc] initWithBadgeInfo:badge];
        [self.navigationController presentViewController:badgePopup animated:YES completion:nil];*/
    }
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if( [segue.identifier isEqualToString:@"composeMessage"] )
    {
        RBMessageComposeScreenController* controller = segue.destinationViewController;
        [controller sendMessageToUser:_profile.userID name:_profile.username];
    }
}

#pragma mark -
#pragma mark Friendship

- (void)updateFrienshipButton
{
    switch(_profile.friendshipStatus)
    {
        case RBXFriendshipStatusNonFriends:
            [_friendshipButton setImage:[UIImage imageNamed:@"Friend Request Button"] forState:UIControlStateNormal];
            break;
        case RBXFriendshipStatusRequestSent:
            [_friendshipButton setImage:[UIImage imageNamed:@"Friend Request Sent"] forState:UIControlStateNormal];
            break;
        case RBXFriendshipStatusRequestReceived:
            [_friendshipButton setImage:[UIImage imageNamed:@"Friend Request Confirm"] forState:UIControlStateNormal];
            break;
        case RBXFriendshipStatusFriends:
            [_friendshipButton setImage:[UIImage imageNamed:@"Friend Button"] forState:UIControlStateNormal];
            break;
        case RBXFriendshipStatusBestFriends:
            [_friendshipButton setImage:[UIImage imageNamed:@"Best Friends Button"] forState:UIControlStateNormal];
            break;
    }
}

- (void)friendshipButtonTouchUpInside:(id)sender
{
    switch (_profile.friendshipStatus)
    {
        case RBXFriendshipStatusNonFriends:
        {
            [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
            [RobloxData sendFriendRequest:_profile.userID completion:^(BOOL success)
            {
                [RobloxHUD hideSpinner:YES];
                if(success)
                {
                    _profile.friendshipStatus = RBXFriendshipStatusRequestSent;
                    [self updateFrienshipButton];
                }
                else
                {
                    [RobloxHUD showMessage:@"Error"];
                }
            }];
            break;
        }
        case RBXFriendshipStatusRequestSent:
        {
            // Nothing to do
            break;
        }
        case RBXFriendshipStatusFriends:
        {
            // Show friend options
            UIActionSheet* actionSheet = [[UIActionSheet alloc]
                                                    initWithTitle:nil
                                                         delegate:self
                                                cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
                                           destructiveButtonTitle:nil
                                                otherButtonTitles:NSLocalizedString(@"MakeBestFriendPhrase", nil),
                                                                  NSLocalizedString(@"RemoveFriendPhrase", nil), nil];
            [actionSheet showFromRect:_friendshipButton.frame inView:_friendshipButton.superview animated:YES];
            break;
        }
        case RBXFriendshipStatusBestFriends:
        {
            // Show friend options
            UIActionSheet* actionSheet = [[UIActionSheet alloc]
                                                    initWithTitle:nil
                                                         delegate:self
                                                cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
                                           destructiveButtonTitle:nil
                                                otherButtonTitles:NSLocalizedString(@"RemoveBestFriendPhrase", nil),
                                                                  NSLocalizedString(@"RemoveFriendPhrase", nil), nil];
            [actionSheet showFromRect:_friendshipButton.frame inView:_friendshipButton.superview animated:YES];
            break;
        }
        case RBXFriendshipStatusRequestReceived:
        {
            // Nothing to do
            break;
        }
    }
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if(buttonIndex == 0)
    {
        [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
        
        if(_profile.friendshipStatus == RBXFriendshipStatusFriends)
        {
            // Add best friend
            [RobloxData makeBestFriend:_profile.userID completion:^(BOOL success)
            {
                dispatch_async(dispatch_get_main_queue(), ^
                {
                    [RobloxHUD hideSpinner:YES];
                    if(success)
                    {
                        _profile.friendshipStatus = RBXFriendshipStatusBestFriends;
                        [self updateFrienshipButton];
                    }
                    else
                    {
                        [RobloxHUD showMessage:@"Error"];
                    }
                });
            }];
        }
        else // if(_profile.friendshipStatus == RBXFriendshipStatusBestFriends)
        {
            // Remove best friend
            [RobloxData removeBestFriend:_profile.userID completion:^(BOOL success)
            {
                dispatch_async(dispatch_get_main_queue(), ^
                {
                    [RobloxHUD hideSpinner:YES];
                    if(success)
                    {
                        _profile.friendshipStatus = RBXFriendshipStatusFriends;
                        [self updateFrienshipButton];
                    }
                    else
                    {
                        [RobloxHUD showMessage:@"Error"];
                    }
                });
            }];
        }
    }
    else if(buttonIndex == 1)
    {
        // Remove friend
        [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
        
        [RobloxData removeFriend:_profile.userID completion:^(BOOL success)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [RobloxHUD hideSpinner:YES];
                if(success)
                {
                    _profile.friendshipStatus = RBXFriendshipStatusNonFriends;
                    [self updateFrienshipButton];
                }
                else
                {
                    [RobloxHUD showMessage:@"Error"];
                }
            });
        }];
    }
}

@end
