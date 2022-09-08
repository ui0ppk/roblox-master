//
//  RBProfileViewController.mm
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
#import "RobloxGoogleAnalytics.h"
#import "RBGameViewController.h"
#import "iOSSettingsService.h"
#import "RobloxWebUtility.h"
#import "RBXEventReporter.h"

#define ROBLOXBADGECELL_ID  @"RobloxBadgeCell"
#define PLAYERBADGECELL_ID  @"PlayerBadgeCell"
#define FRIENDCELL_ID       @"FriendCell"
#define FOLLOWINGCELL_ID    @"FollowingCell"
#define FOLLOWERCELL_ID     @"FollowerCell"
#define BADGECELL_SUPP      @"BadgeCellSupp"
#define FRIENDCELL_SUPP     @"FriendCellSupp"

#define GAME_ITEM_SIZE CGSizeMake(225, 155)
#define GAME_THUMBNAIL_SIZE CGSizeMake(420, 230)
#define PROFILE_IMAGE_SIZE CGSizeMake(352, 352)
#define FRIEND_AVATAR_SIZE CGSizeMake(110, 110)
#define BADGE_SIZE CGSizeMake(110, 110) // Player badges are fixed sized, so the request

#define MAX_ELEMENTS_PER_REQUEST 20

//---METRICS---
#define PVC_openRobux           @"PROFILE SCREEN - Open Robux"
#define PVC_openBuildersClub    @"PROFILE SCREEN - Open Builders Club"
#define PVC_openSettings        @"PROFILE SCREEN - Open Settings"
#define PVC_openLogout          @"PROFILE SCREEN - Open Logout"
#define PVC_catalogSearch       @"PROFILE SCREEN - User Search"
#define PVC_openExternalLink    @"PROFILE SCREEN - Open External Link"
#define PVC_openProfile         @"PROFILE SCREEN - Open Profile"
#define PVC_openGameDetail      @"PROFILE SCREEN - Open Game Detail"
#define PVC_launchGame          @"PROFILE SCREEN - Launch Game"

#define MAX_PLAYER_FRIENDS 200
#define ERROR_PLAYER_FRIENDS -1

#pragma mark -
#pragma mark Collection View Cells


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


#pragma mark
#pragma mark ActionSheets
@interface RBFriendshipActionSheet : UIActionSheet <UIActionSheetDelegate>
typedef enum
{
    SHEET_TAG_FRIEND = 1,
    SHEET_TAG_PENDING_REQUEST = 2
} SHEET_TAGS;

@property int sheetIdTag;
@property RBProfileViewController* profile;
-(id) initFriendSheetFromProfile:(RBProfileViewController*)profilePage;
-(id) initFriendRequestSheetFromProfile:(RBProfileViewController*)profilePage;
@end

@implementation RBFriendshipActionSheet

//CONSTRUCTORS
-(id) initFriendSheetFromProfile:(RBProfileViewController*)profilePage
{
    //the user is currently a friend, display the current friend options
    self = [super initWithTitle:nil
                       delegate:self
              cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
         destructiveButtonTitle:nil
              otherButtonTitles:NSLocalizedString(@"RemoveFriendPhrase", nil), nil];
    
    _sheetIdTag = SHEET_TAG_FRIEND;
    
    _profile = profilePage;
    return self;
}
-(id) initFriendRequestSheetFromProfile:(RBProfileViewController*)profilePage
{
    _sheetIdTag = SHEET_TAG_PENDING_REQUEST;
    _profile = profilePage;
    return self;
}


//DELEGATE FUNCTIONS
- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
    switch (_sheetIdTag)
    {
        case SHEET_TAG_FRIEND:
            if (buttonIndex == 0) //Remove friend
            {
                [self actionSheetRemoveFriend];
            }
            else //Cancel
            {
            }
            break;
            
            
        case SHEET_TAG_PENDING_REQUEST:
            if (buttonIndex == 0) //Accept Friend Request
            {
                
            }
            else if (buttonIndex == 1) //Reject Friend Request
            {
                
            }
            else //Cancel
            {
            }
            break;
    }
}

//BUTTON FUNCTIONS
-(void) actionSheetRemoveFriend
{
    // Remove friend
    [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
    
    [RobloxData unfriend:_profile.userId completion:^(BOOL success)
     {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            [RobloxHUD hideSpinner:YES];
            if(success) { [self updateProfileFriendshipButtonWithStatus:RBXFriendshipStatusNonFriends]; }
            else
            {
                [RobloxHUD showMessage:@"Error - Could Not Remove Friend"];
            }
        });
     }];
}
-(void) actionSheetAcceptFriendRequest
{
    // TO DO : FIGURE OUT A WAY TO GET THE INVITATION ID
    // Accept Friend Request
//    [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
//    
//    [RobloxData acceptFriendRequest:<#(NSUInteger)#> withTargetUserID:_profile.userId completion:^(BOOL success)
//    {
//         dispatch_async(dispatch_get_main_queue(), ^
//                        {
//                            [RobloxHUD hideSpinner:YES];
//                            if(success) { [self updateProfileFriendshipButtonWithStatus:RBXFriendshipStatusFriends]; }
//                            else
//                            {
//                                [RobloxHUD showMessage:@"Error - Could Not Accept Friend Request"];
//                            }
//                        });
//     }];
}
-(void) actionSheetRejectFriendRequest
{
    // TO DO : FIGURE OUT A WAY TO GET THE INVITATION ID
    // Remove best friend
//    [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
//    
//    [RobloxData declineFriendRequest:<#(NSUInteger)#> withTargetUserID:_profile.userId completion:^(BOOL success)
//     {
//         dispatch_async(dispatch_get_main_queue(), ^
//                        {
//                            [RobloxHUD hideSpinner:YES];
//                            if(success) { [self updateProfileFriendshipButtonWithStatus:RBXFriendshipStatusNonFriends]; }
//                            else
//                            {
//                                [RobloxHUD showMessage:@"Error"];
//                            }
//                        });
//     }];
}
-(void) updateProfileFriendshipButtonWithStatus:(RBXFriendshipStatus)aNewStatus
{
    _profile.profile.friendshipStatus = aNewStatus;
    [_profile updateFriendshipButton];
}
@end



#pragma mark -
#pragma mark Profile View Controller

@implementation RBProfileViewController
{
    RBXUserPresence* _userPresence;
    
    NSMutableArray* _playerBadges;
    NSMutableArray* _robloxBadges;
    NSMutableArray* _friends;
    NSMutableArray* _followers;
    NSMutableArray* _following;
    int _userFriendCount; //used privately
    
    GameSortHorizontalViewController* _recentlyPlayedViewController;
    GameSortHorizontalViewController* _myGamesViewController;
    GameSortHorizontalViewController* _favoriteGamesViewController;
    
    IBOutlet UIScrollView* _scrollView;
    
    //header elements
    IBOutlet UIView* _headerContainer;
    IBOutlet RobloxImageView* _avatarImageView;
    IBOutlet UIImageView* _userOnlineMarker;
    IBOutlet UILabel* _userNameLabel;
    IBOutlet UILabel* _userStatusLabel;
    IBOutlet UILabel* _robuxTitle;
    IBOutlet UILabel* _ticketsTitle;
    IBOutlet UILabel* _friendsTitle;
    IBOutlet UILabel* _robuxLabel;
    IBOutlet UILabel* _ticketsLabel;
    IBOutlet UILabel* _friendsLabel;
    
    //main buttons
    IBOutlet UIButton *_friendshipButton;
    IBOutlet UIButton *_sendMessageButton;
    IBOutlet UIButton *_followIntoGameButton;
    IBOutlet UIButton *_followUserButton;
    
    //friends elements
    IBOutlet UIView* _friendsContainerView;
    IBOutlet UICollectionView* _friendsCollectionView;
    IBOutlet UILabel* _friendsHeaderTitle;
    IBOutlet UIButton* _seeAllFriendsButton;
    
    //followers elements
    IBOutlet UIView* _followersContainerView;
    IBOutlet UICollectionView* _followersCollectionView;
    IBOutlet UILabel* _followersHeaderTitle;
    IBOutlet UIButton* _seeAllFollowersButton;
    
    //following elements
    IBOutlet UIView* _followingContainerView;
    IBOutlet UICollectionView* _followingCollectionView;
    IBOutlet UILabel* _followingHeaderTitle;
    IBOutlet UIButton* _seeAllFollowingButton;
    
    //badges elements
    IBOutlet UIView* _badgesContainerView;
    IBOutlet UILabel* _badgesHeaderTitle;
    IBOutlet UICollectionView* _badgesCollectionView;
}

#pragma mark -
#pragma mark View Loading
- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        self.title = NSLocalizedString(@"Profile", nil);
    }
    return self;
}
- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _playerBadges = [NSMutableArray array];
    _robloxBadges = [NSMutableArray array];
    _friends = [NSMutableArray array];
    _following = [NSMutableArray array];
    _followers = [NSMutableArray array];
	
    //apply a few local changes
    self.edgesForExtendedLayout = UIRectEdgeNone;
    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    //grab the profile information about the player
    [self requestUserInfo];
    
    // Some controls are not in the view if this user is not the one logged in
    if( [self isSessionUser] )  { [self initPlayerProfile]; }
    else                        { [self initOtherProfile];  }
    
    
    //initialize the screen
    [self initCommonProfileElements];
    
}
- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
}
- (void)viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeCreative];
    [super viewWillAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onFriendsUpdated:) name:RBX_NOTIFY_FRIENDS_UPDATED object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onFollowingUpdated:) name:RBX_NOTIFY_FOLLOWING_UPDATED object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onRobuxUpdated:) name:RBX_NOTIFY_ROBUX_UPDATED object:nil];
    
    //reload the dynamic data, but only if we have already fetched the player's profile information
    if ([self isSessionUser] && _profile)
    {
        //don't lock up the UIThread with this request, push it to a background thread
        //each function has a callback that updates on the UIThread, this is safe
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
        {
            //update the player's following, followers, badges, and friends
            if (_profile.userID)
                [self fetchProfileElementsForUser:_profile.userID];
            
            //update the labels at the top of the screen
            [self showUserInfo:_profile];
        });
    }
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextProfile];
}
- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_FRIENDS_UPDATED object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_FOLLOWING_UPDATED object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_ROBUX_UPDATED object:nil];
}


#pragma mark -
#pragma mark Initialization and Layout Methods
- (void) initCommonProfileElements
{
    // Set header background and border
    _headerContainer.backgroundColor = [UIColor whiteColor];
    CALayer* headerLayer = _headerContainer.layer;
    headerLayer.shadowOpacity = 0.25;
    headerLayer.shadowColor = [UIColor colorWithWhite:(0x6B/255.f) alpha:1.0].CGColor;
    headerLayer.shadowOffset = CGSizeMake(0,0);
    headerLayer.shadowRadius = 2.0;
    
    // Localize, initialize and stylize labels and buttons
    //Header Elements
    _userNameLabel.text = @"";
    _userStatusLabel.text = @"";
    _userStatusLabel.hidden = YES; //THIS WILL REMAIN HIDDEN UNTIL FOREVER / IT IS SUPPORTED
    _friendsLabel.text = @"";
    _friendsTitle.text = NSLocalizedString(@"FriendsWord", nil);
    [RobloxTheme applyToProfileHeaderSubtitle:_friendsTitle];
    [RobloxTheme applyToProfileHeaderValue:_friendsLabel];
    [RobloxTheme applyToProfileHeaderValue:_userNameLabel];
    
    //Friends
    [_friendsHeaderTitle setText:[NSLocalizedString(@"FriendsWord", nil) uppercaseString]];
    [_seeAllFriendsButton setTitle:NSLocalizedString(@"SeeAllPhrase", nil) forState:UIControlStateNormal];
    [_seeAllFriendsButton setHidden:YES];
    [RobloxTheme applyToGameSortSeeAllButton:_seeAllFriendsButton];
    [RobloxTheme applyToTableHeaderTitle:_friendsHeaderTitle];
    
    //Following
    [_followingHeaderTitle setText:[NSLocalizedString(@"FollowingWord", nil) uppercaseString]];
    [_seeAllFollowingButton setTitle:NSLocalizedString(@"SeeAllPhrase", nil) forState:UIControlStateNormal];
    [_seeAllFollowingButton setHidden:YES];
    [RobloxTheme applyToGameSortSeeAllButton:_seeAllFollowingButton];
    [RobloxTheme applyToTableHeaderTitle:_followingHeaderTitle];
    _followingContainerView.hidden = NO;
    
    //Followers
    [_followersHeaderTitle setText:[NSLocalizedString(@"FollowersWord", nil) uppercaseString]];
    [_seeAllFollowersButton setTitle:NSLocalizedString(@"SeeAllPhrase", nil) forState:UIControlStateNormal];
    [_seeAllFollowersButton setHidden:YES];
    [RobloxTheme applyToGameSortSeeAllButton:_seeAllFollowersButton];
    [RobloxTheme applyToTableHeaderTitle:_followersHeaderTitle];
    _followersContainerView.hidden = NO;
    
    //Badges
    _badgesHeaderTitle.text = [NSLocalizedString(@"BadgesWord", nil) uppercaseString];
    [RobloxTheme applyToTableHeaderTitle:_badgesHeaderTitle];
    
    
    
    //add navigation bar elements
    UIBarButtonItem* backButton = [[UIBarButtonItem alloc] initWithTitle:@"" style:UIBarButtonItemStyleBordered target:nil action:nil];
    [self.navigationItem setBackBarButtonItem:backButton];
    
    //add the builders club stuff only if the user is logged in
    if ([self isLoggedIn])
        [self addIcons:nil];
    else
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addIcons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    
    [self setFlurryEventsForExternalLinkEvent:nil
                              andWebViewEvent:nil
                          andOpenProfileEvent:PVC_openProfile
                       andOpenGameDetailEvent:PVC_openGameDetail];
    
    //add a search bar to allow players to search for other players
    [self addSearchIconWithSearchType:SearchResultUsers andFlurryEvent:nil];

    //layout the views
    [self setUpCollectionViews];
}
- (void) initPlayerProfile
{
    _robuxTitle.text = [NSLocalizedString(@"RobuxWord", nil) uppercaseString];
    _ticketsTitle.text = NSLocalizedString(@"TicketsWord", nil);
    _robuxLabel.text = @"";
    _ticketsLabel.text = @"";
    
    [RobloxTheme applyToProfileHeaderSubtitle:_robuxTitle];
    [RobloxTheme applyToProfileHeaderSubtitle:_ticketsTitle];
    [RobloxTheme applyToProfileHeaderValue:_robuxLabel];
    [RobloxTheme applyToProfileHeaderValue:_ticketsLabel];
    
    
    //With the new layout, hide the player's friends and recently played as they've been relocated to the Home Screen
    if ([self isNewLayout])
    {
        _friendsContainerView.hidden = YES;
    }
   
}
- (void) initOtherProfile
{
    _sendMessageButton.hidden = YES;
    _friendshipButton.hidden = YES;
    _followIntoGameButton.hidden = YES;
    _followUserButton.hidden = YES;
}
- (void)setUpCollectionViews
{
    __weak RBProfileViewController* weakSelf = self;
    
    NSNumber* userID = [self getUserID];
    
    // My games
    _myGamesViewController = [[GameSortHorizontalViewController alloc] initWithNibName:@"GameSortHorizontalViewController" bundle:nil];
    [_myGamesViewController setAnalyticsLocation:RBXALocationProfile andContext:RBXAContextMain];
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
    [_favoriteGamesViewController setAnalyticsLocation:RBXALocationProfile andContext:RBXAContextMain];
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
    [_friendsCollectionView registerNib:[UINib nibWithNibName:@"RBPlayerThumbnailCell" bundle:nil] forCellWithReuseIdentifier:FRIENDCELL_ID];
    
    // Followers
    if (_followingContainerView)
    {
        _followingCollectionView.dataSource = self;
        _followingCollectionView.delegate = self;
        [_followingCollectionView setContentInset:UIEdgeInsetsMake(0.0, 0.0, 0.0, 0.0)];
        [_followingCollectionView registerNib:[UINib nibWithNibName:@"RBPlayerThumbnailCell" bundle:nil] forCellWithReuseIdentifier:FOLLOWINGCELL_ID];
    }
    
    
    // Following
    _followersCollectionView.dataSource = self;
    _followersCollectionView.delegate = self;
    [_followersCollectionView setContentInset:UIEdgeInsetsMake(0.0, 0.0, 0.0, 0.0)];
    [_followersCollectionView registerNib:[UINib nibWithNibName:@"RBPlayerThumbnailCell" bundle:nil] forCellWithReuseIdentifier:FOLLOWERCELL_ID];
    
    
    [self layoutCollectionViews];
}
- (void)layoutCollectionViews
{
    dispatch_async(dispatch_get_main_queue(), ^
    {
        #define OFFSET_Y 20
        CGFloat lastY = CGRectGetMaxY(_headerContainer.frame) + OFFSET_Y;
        
        if (_friendsContainerView.hidden == NO)
        {
            lastY = CGRectGetMaxY(_friendsContainerView.frame) + OFFSET_Y;
            _seeAllFriendsButton.hidden = NO;
        }
        
        if(_followingContainerView != nil && _followingContainerView.hidden == NO)
        {
            _followingContainerView.y = lastY;
            lastY = CGRectGetMaxY(_followingContainerView.frame) + OFFSET_Y;
            _seeAllFollowingButton.hidden = NO;
        }
        
        if(_followersContainerView.hidden == NO)
        {
            _followersContainerView.y = lastY;
            lastY = CGRectGetMaxY(_followersContainerView.frame) + OFFSET_Y;
            _seeAllFollowersButton.hidden = NO;
        }
        
        if(_favoriteGamesViewController != nil && _favoriteGamesViewController.view.hidden == NO)
        {
            _favoriteGamesViewController.view.y = lastY;
            lastY = CGRectGetMaxY(_favoriteGamesViewController.view.frame) + OFFSET_Y;
        }
        
        if(_myGamesViewController != nil && _myGamesViewController.view.hidden == NO)
        {
            _myGamesViewController.view.y = lastY;
            lastY = CGRectGetMaxY(_myGamesViewController.view.frame) + OFFSET_Y;
        }
        
        
        if(_badgesContainerView.hidden == NO)
        {
            _badgesContainerView.y = lastY;
            
            [_badgesCollectionView setSize:_badgesCollectionView.contentSize];
            
            _badgesContainerView.size = CGRectUnion(_badgesCollectionView.frame, _badgesHeaderTitle.frame).size;
        }
        
        [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
            
    });
}


#pragma mark -
#pragma mark Notification Event Listeners
- (void) onFriendsUpdated:(NSNotification*)notification
{
    NSNumber* userID = [self getUserID];
    [self fetchFriendsForUser:userID];
}
- (void) onFollowingUpdated:(NSNotification*)notification
{
    [self fetchMyFollowing];
}
- (void) onRobuxUpdated:(NSNotification*)notification
{
    [self updateRobux];
}
- (void) addIcons:(NSNotification*) notification
{
    dispatch_async(dispatch_get_main_queue(), ^
    {
        [self addRobuxIconWithFlurryEvent:PVC_openRobux
                 andBCIconWithFlurryEvent:PVC_openBuildersClub];
        
        [self configureUserButtons];
        
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(removeIcons:) name:RBX_NOTIFY_LOGGED_OUT object:nil];
    });
}
-(void) removeIcons:(NSNotification*) notification
{
    dispatch_async(dispatch_get_main_queue(), ^
    {
        [self removeRobuxAndBCIcons];
        
        [self initOtherProfile];
       
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGGED_OUT object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addIcons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    });
}

#pragma mark -
#pragma mark Property Accessors
- (BOOL)isSessionUser
{
    return self.userId == nil;
}
- (BOOL)isLoggedIn
{
    return [UserInfo CurrentPlayer].userLoggedIn;
}
- (BOOL)isNewLayout
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    return !iOSSettings->GetValueEnableFriendsOnProfile();
}
- (NSNumber*)getUserID
{
    if([self isSessionUser])
    {
        UserInfo* userInfo = [UserInfo CurrentPlayer];
        return userInfo.userId;
    }
    else
        return self.userId;
}


#pragma mark -
#pragma mark Data Requests and Fetches
- (void) requestUserInfo
{
    // ** Request basic user info **
    // If no user id is set, load the current user info
    if( [self isSessionUser])
    {
        [RobloxData fetchMyProfileWithSize:PROFILE_IMAGE_SIZE completion:^(RBXUserProfileInfo *profile)
         {
             _profile = profile;
             dispatch_async(dispatch_get_main_queue(), ^
            {
                [self showUserInfo:profile];
            });
             
             [self fetchProfileElementsForUser:_profile.userID];
         }];
    }
    else
    {
        NSNumber* userID = [self getUserID];
        [RobloxData fetchUserProfile:userID avatarSize:PROFILE_IMAGE_SIZE completion:^(RBXUserProfileInfo *profile)
         {
             _profile = profile;
             dispatch_async(dispatch_get_main_queue(), ^
            {
                [self showUserInfo:profile];
            });
             
             [self fetchProfileElementsForUser:_profile.userID];
         }];
    }
}
- (void)updateRobux
{
    [RobloxData fetchMyProfileWithSize:PROFILE_IMAGE_SIZE completion:^(RBXUserProfileInfo *profile)
     {
         dispatch_async(dispatch_get_main_queue(), ^
        {
                _robuxLabel.text = [NSString stringWithFormat:@"%lu", (unsigned long)profile.robux];
        });
     }];
}
- (void)fetchProfileElementsForUser:(NSNumber*)userID
{
    if ([self isSessionUser])
    {
        // ** Request the player's Following **
        [self fetchMyFollowing];
    }
    else
    {
        // ** Request User Presence **
        [self fetchUserPresenceForUser:userID];
    }
    
    // ** Request Badges
    [self fetchBadgesForUser:userID];
    
    // ** Request Friends **
    [self fetchFriendsForUser:userID];
    
    // ** Request Followers **
    [self fetchFollowersForUser:userID];
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
                 _friendsLabel.text = [NSString stringWithFormat:@"%lu", (unsigned long)totalFriends];
                 
                 //check for reasons to hide the friends view
                 if (_friends.count == 0)
                 {
                     //-the user has no friends
                     _friendsContainerView.hidden = YES;
                 }
                 else if ([self isSessionUser] && [self isNewLayout])
                 {
                     //-the user is on the Profile page (not Other page) and the newLayout flag is activated
                     _friendsContainerView.hidden = YES;
                 }
                 else if ([self isLoggedIn] && [userID isEqualToNumber:[UserInfo CurrentPlayer].userId])
                 {
                     //user is logged in and looking at themself on a different page
                     _friendsContainerView.hidden = YES;
                 }
                 else
                 {
                     _friendsContainerView.hidden = NO;
                     [_friendsCollectionView reloadData];
                 }
                 
                 [self layoutCollectionViews];
             });
         }
         
         //if the user id of the user we just searched is not the player, then we need to know how many friends the user has
         if ([self isLoggedIn])
         {
             //it is important to make sure the user is logged in, as the fetch requires an authenticated user
             //check how many friends the user has
             if (!userID || ![UserInfo CurrentPlayer].userId)
             {
                 // if for some reason there has been some egregious error, add an error value and walk away
                 _userFriendCount = ERROR_PLAYER_FRIENDS;
                 return;
             }
             
             
             if ([[UserInfo CurrentPlayer].userId isEqualToNumber:userID])
                 _userFriendCount = (int)[_friends count];
             else
             {
                 //also get the count for how many friends the user currently has
                 [RobloxData fetchFriendCount:[UserInfo CurrentPlayer].userId
                               withCompletion:^(BOOL success, NSString *message, int count)
                 {
                        //if this call was unsuccessful, prevent the user from sending any friend requests
                        // by setting the friend count to the max allowed count
                        _userFriendCount = success ? count : ERROR_PLAYER_FRIENDS;
                 }];
             }
         }
         
     }];
    
    
}
- (void)fetchFollowersForUser:(NSNumber*)userID
{
    //_followers
    [RobloxData fetchFollowersForUser:userID
                               atPage:1
                           avatarSize:FRIEND_AVATAR_SIZE
                       withCompletion:^(bool success, NSArray *followers)
     {
         dispatch_async(dispatch_get_main_queue(), ^
         {
             if (!success || followers.count == 0)
             {
                 //the user has no followers, hide the followers setion
                 _followersContainerView.hidden = YES;
             }
             else
             {
                 if (_followers) { [_followers removeAllObjects]; }
                 
                 _followers = [NSMutableArray arrayWithArray:followers];
                 [_followersCollectionView reloadData];
             }
         });
         [self layoutCollectionViews];
     }];
}
- (void)fetchMyFollowing
{
    //_following
    [RobloxData fetchMyFollowingAtPage:1
                            avatarSize:FRIEND_AVATAR_SIZE
                        withCompletion:^(bool success, NSArray *following)
     {
         dispatch_async(dispatch_get_main_queue(), ^
         {
             if (!success || following.count == 0)
             {
                  _followingContainerView.hidden = YES;
             }
             else
             {
                 if (_following) { [_following removeAllObjects]; }
                 
                 _following = [NSMutableArray arrayWithArray:following];
                 [_followingCollectionView reloadData];
                 
             }
         });
         [self layoutCollectionViews];
     }];
}
- (void)fetchBadgesForUser:(NSNumber*)userID
{
    // ** Request badges **
    _playerBadges = [NSMutableArray array];
    [RobloxData fetchUserBadges:RBXUserBadgeTypePlayer forUser:userID badgeSize:BADGE_SIZE completion:^(NSArray *badges)
    {
         dispatch_async(dispatch_get_main_queue(), ^
         {
             if(badges)
                 [_playerBadges addObjectsFromArray:badges];
             else
                 [_playerBadges removeAllObjects];
             
             [_badgesCollectionView reloadData];
             _badgesContainerView.hidden = _robloxBadges.count == 0 && _playerBadges.count == 0;
             [self layoutCollectionViews];
         });
     }];
    
    _robloxBadges = [NSMutableArray array];
    [RobloxData fetchUserBadges:RBXUserBadgeTypeRoblox forUser:userID badgeSize:BADGE_SIZE completion:^(NSArray *badges)
    {
         dispatch_async(dispatch_get_main_queue(), ^
         {
             if(badges)
                [_robloxBadges addObjectsFromArray:badges];
             else
                 [_robloxBadges removeAllObjects];
            
             [_badgesCollectionView reloadData];
             _badgesContainerView.hidden = _robloxBadges.count == 0 && _playerBadges.count == 0;
             [self layoutCollectionViews];
         });
     }];
}
- (void)fetchUserPresenceForUser:(NSNumber*)userID
{
    [RobloxData fetchPresenceForUser:userID withCompletion:^(RBXUserPresence* presence)
     {
         if (presence)
         {
             dispatch_async(dispatch_get_main_queue(), ^
            {
                _userPresence = presence;
                [self showUserOnlineStatus];
            });
         }
     }];
}


//USING THE RBXUSERPROFILE INFO, UPDATE ALL THE SCREEN ELEMENTS
- (void) showUserInfo:(RBXUserProfileInfo*)profile
{
    _userNameLabel.text = profile.username;
    //_userStatusLabel.text = profile.status;
    //_userStatusLabel.hidden = [profile.status length] <= 0;
    [_avatarImageView loadAvatarForUserID:profile.userID.intValue prefetchedURL:profile.avatarURL urlIsFinal:profile.avatarIsFinal withSize:PROFILE_IMAGE_SIZE completion:nil];
    
    if( [self isSessionUser] )
    {
        _robuxLabel.text = [NSString stringWithFormat:@"%lu", (unsigned long)profile.robux];
        _ticketsLabel.text = [NSString stringWithFormat:@"%lu", (unsigned long)profile.tickets];
    }
    else
    {
        //before adding the buttons the the Other's Profile page
        // check that the current player is not looking at their own profile and that they are not guests
        NSNumber* playerID = [UserInfo CurrentPlayer].userId == nil ? [NSNumber numberWithInt:-1] : [UserInfo CurrentPlayer].userId;
        BOOL check1 = ([profile.userID isEqualToNumber:playerID] == NO);
        BOOL check2 = ([self isLoggedIn]);
        if (check1 && check2)
        {
            [self configureUserButtons];
        }
        self.title = [NSString stringWithFormat:NSLocalizedString(@"ProfileTitleFormat", nil), profile.username];
        _myGamesViewController.sortTitle = [NSString stringWithFormat:NSLocalizedString(@"FriendPlacesFormat", nil), _profile.username];
        
        //if we are on the Other's Profile page and we are looking at our own profile,
        // hide the friends list if the new layout is active, because their friends list is on the home page
        if ([UserInfo CurrentPlayer].userId && _profile.userID && [_profile.userID isEqualToNumber:[UserInfo CurrentPlayer].userId] && [self isNewLayout])
        {
            _friendsContainerView.hidden = YES;
            [self layoutCollectionViews];
        }
    }
}
- (void) configureUserButtons
{
    int visibleButtons = 4;
    
    //make sure we have not navigated to the current user's public profile
 
    // We need to hide the message button and adjust the position of other buttons
    // if we are looking at the Roblox profile
    bool isRoblox = ([self.userId intValue] == 1);
    
    if (isRoblox)
    {
        _sendMessageButton.hidden = YES;
        visibleButtons = 4;
    }
    else
        _sendMessageButton.hidden = NO;
    
    if(_profile.friendshipStatus == RBXFriendshipStatusNonFriends)
    {
        [_sendMessageButton setImage:[UIImage imageNamed:@"Send Message Disabled"] forState:UIControlStateDisabled];
        _sendMessageButton.enabled = false;
    }
    
    _friendshipButton.hidden = NO;
    _followIntoGameButton.hidden = NO;
    _followUserButton.hidden = NO;
    
    //set up the follow into game button
    [_followIntoGameButton setImage:[UIImage imageNamed:@"Join Friend Game"] forState:UIControlStateNormal];
    [_followIntoGameButton setImage:[UIImage imageNamed:@"Join Friend Down"] forState:UIControlStateSelected];
    [_followIntoGameButton setImage:[UIImage imageNamed:@"Join Friend Disabled"] forState:UIControlStateDisabled];
    
    //set up the friendship button with all of its states
    [self updateFriendshipButton];
    
    //update the follow user button
    [self updateFollowButton];
    
    
    //update the layout of the buttons based on what is visible
    float origin = CGRectGetMaxX(_avatarImageView.frame);
    float screenSpace = CGRectGetMinX(_friendsTitle.frame) - origin;
    float buttonSpace = screenSpace / visibleButtons;
    
    //position the buttons
    int index = 0;
    if (!isRoblox)
        [_sendMessageButton setX:(origin + (buttonSpace * index++))];
    
    [_friendshipButton setX:(origin + (buttonSpace * index++))];
    [_followUserButton setX:(origin + (buttonSpace * index++))];
    [_followIntoGameButton setX:(origin + (buttonSpace * index))];
}
- (void) showUserOnlineStatus
{
    //update the interface with values corresponding to the user's online status
    bool isPlayerOnline = _userPresence.isOnline;
    bool lastPlaceExists = _userPresence.placeID != nil;
    _followIntoGameButton.enabled = (isPlayerOnline && lastPlaceExists);
    //_followIntoGameButton.hidden = !_followIntoGameButton.enabled;
    
    //TODO: Display when the player was last online
    
    
    //TODO: Display a green or red dot next to avatar to indicate if the player is online
    NSString* imageName = _userPresence.isOnline ? @"User Online" : @"User Offline";
    [_userOnlineMarker setImage:[UIImage imageNamed:imageName]];
    
    //TODO: Display what game the user was last in
}







#pragma mark -
#pragma mark User Interface Actions
//Main Buttons
- (IBAction)didPressSendMessage:(id)sender
{
    [self performSegueWithIdentifier:@"composeMessage" sender:nil];
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMessage withContext:RBXAContextProfile];
}
- (IBAction)didPressFollowIntoGame:(id)sender
{
    //before we follow into the game, check if the player is still online and in game
    if (_userId)
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonJoinInGame
                                                 withContext:RBXAContextProfile
                                        withCustomDataString:_userId.stringValue];
        
        [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"ProfileCheckUserPresenceWord", nil) dimBackground:YES];
        [RobloxData fetchPresenceForUser:_userId withCompletion:^(RBXUserPresence *presence)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _userPresence = presence;
                [RobloxHUD hideSpinner:YES];
            
                //check if the player is still online and still in a game
                if (presence.isOnline && presence.placeID != nil)
                {
                    //if they are, attempt to follow them into the game
                    RBGameViewController* controller = [[RBGameViewController alloc] initWithLaunchParams:[RBXGameLaunchParams InitParamsForFollowUser:[_profile.userID integerValue]]];
                    [self presentViewController:controller animated:NO completion:nil];
                }
                else
                {
                    //looks like they are no longer online, let's update the user presence
                    [self showUserOnlineStatus];
                    
                    //alert the user that this won't work
                    UIAlertView* alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"GenericFailedGameFollowErrorTitle", nil)
                                                                    message:NSLocalizedString(@"GenericFailedGameFollowErrorMessage", nil)
                                                                   delegate:nil
                                                          cancelButtonTitle:NSLocalizedString(@"OkayWord", nil)
                                                          otherButtonTitles:nil];
                    [alert show];
                }
            });
        }];
    }
}

//See All Buttons
- (IBAction)didPressSeeAllFriends:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSeeAll withContext:RBXAContextProfile withCustomDataString:@"friends"];
    RBFullFriendListScreenController* controller = [[RBFullFriendListScreenController alloc] init];
    controller.playerID = _profile.userID;
    controller.playerName = _profile.username;
    controller.listTypeValue = RBListTypeFriends;
    controller.title = [NSString stringWithFormat:NSLocalizedString(@"UserFriends", nil), _profile.username];
    [self.navigationController pushViewController:controller animated:YES];
}
- (IBAction)didPressSeeAllFollowers:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSeeAll withContext:RBXAContextProfile withCustomDataString:@"followers"];
    RBFullFriendListScreenController* controller = [[RBFullFriendListScreenController alloc] init];
    controller.playerID = _profile.userID;
    controller.playerName = _profile.username;
    controller.listTypeValue = RBListTypeFollowers;
    controller.title = [NSString stringWithFormat:NSLocalizedString(@"UserFollowers", nil), _profile.username];
    [self.navigationController pushViewController:controller animated:YES];
}
- (IBAction)didPressSeeAllFollowing:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSeeAll withContext:RBXAContextProfile withCustomDataString:@"following"];
    RBFullFriendListScreenController* controller = [[RBFullFriendListScreenController alloc] init];
    controller.playerID = _profile.userID;
    controller.playerName = _profile.username;
    controller.listTypeValue = RBListTypeFollowing;
    controller.title = [NSString stringWithFormat:NSLocalizedString(@"UserFollowing", nil), _profile.username];
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
        int numSections = 0;
        if (_robloxBadges.count > 0) numSections++;
        if (_playerBadges.count > 0) numSections++;
        
        return numSections;
    }
    
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    if(collectionView == _badgesCollectionView)
    {
        switch (section)
        {
            case 0:
                if (_robloxBadges.count > 0)
                    return _robloxBadges.count;
                else
                    //there is the possibility that the player has no roblox badges, but they have player badges, so just shift the sections
                    return _playerBadges.count;
                break;
            case 1:
                return _playerBadges.count;
                break;
            default:
                return 0;
                break;
        }
    }
    else if(collectionView == _friendsCollectionView)
    {
        return _friends.count;
    }
    else if (collectionView == _followersCollectionView)
    {
        return _followers.count;
    }
    else if (collectionView == _followingCollectionView)
    {
        return _following.count;
    }
    return 0;
}

- (UICollectionViewCell*)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if ([collectionView isEqual:_badgesCollectionView])
    {
        switch (indexPath.section)
        {
            case 0:
            {
                if (_robloxBadges.count > 0)
                {
                    RBXRobloxBadgeCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:ROBLOXBADGECELL_ID forIndexPath:indexPath];
                    cell.badgeInfo = _robloxBadges[indexPath.row];
                    return cell;
                }
                
                //there is the possibility that the player has no roblox badges, but they have player badges, so just shift the sections
                RBXPlayerBadgeCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:PLAYERBADGECELL_ID forIndexPath:indexPath];
                cell.badgeInfo = _playerBadges[indexPath.row];
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
    else if ([collectionView isEqual:_friendsCollectionView])
    {
        RBPlayerThumbnailCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:FRIENDCELL_ID forIndexPath:indexPath];
        cell.friendInfo = _friends[indexPath.row];
        return cell;
    }
    else if ([collectionView isEqual:_followingCollectionView])
    {
        RBPlayerThumbnailCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:FOLLOWINGCELL_ID forIndexPath:indexPath];
        cell.friendInfo = _following[indexPath.row];
        return cell;
    }
    else if ([collectionView isEqual:_followersCollectionView])
    {
        RBPlayerThumbnailCell* cell = [collectionView dequeueReusableCellWithReuseIdentifier:FOLLOWERCELL_ID forIndexPath:indexPath];
        cell.friendInfo = _followers[indexPath.row];
        return cell;
    }
    
    return nil;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    [collectionView deselectItemAtIndexPath:indexPath animated:YES];
    
    if ([collectionView isEqual:_friendsCollectionView])
    {
        RBXFriendInfo* friendInfo = _friends[indexPath.row];
        [self pushProfileControllerWithUserID:friendInfo.userID];
    }
    else if ([collectionView isEqual:_followersCollectionView])
    {
        RBXFriendInfo* friendInfo = _followers[indexPath.row];
        [self pushProfileControllerWithUserID:friendInfo.userID];
    }
    else if ([collectionView isEqual:_followingCollectionView])
    {
        RBXFriendInfo* friendInfo = _following[indexPath.row];
        [self pushProfileControllerWithUserID:friendInfo.userID];
    }
    else if ([collectionView isEqual:_badgesCollectionView])
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonBadge withContext:RBXAContextProfile];
        RBXBadgeInfo* badge;
        if (indexPath.section == 0)
            //There is a possibility that there are no roblox badges, but there might be player badges
            badge = ((_robloxBadges.count) ? _robloxBadges[indexPath.row] : _playerBadges[indexPath.row]);
        else
            badge = _playerBadges[indexPath.row];
        
        if (!badge) return;
        GameBadgeViewController* badgePopup = [[GameBadgeViewController alloc] initWithBadgeInfo:badge];
        [self.navigationController presentViewController:badgePopup animated:YES completion:nil];
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

- (void)updateFriendshipButton
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
            [_friendshipButton setImage:[UIImage imageNamed:@"Friend Button"] forState:UIControlStateNormal];
            break;
    }
}

- (IBAction)friendshipButtonTouchUpInside:(id)sender
{
    switch (_profile.friendshipStatus)
    {
        case RBXFriendshipStatusNonFriends:
        {
            [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonFriendRequest withContext:RBXAContextProfile withCustomDataString:@"makeFriend"];
            //check if the user has exceeded 200 friends
            if (_userFriendCount >= MAX_PLAYER_FRIENDS)
            {
                //display an error message and quit
                [RobloxHUD showMessage:NSLocalizedString(@"MaxFriendCountError", nil)];
                return;
            }
            else if (_userFriendCount == ERROR_PLAYER_FRIENDS)
            {
                //display an error message and quit
                [RobloxHUD showMessage:NSLocalizedString(@"GenericFailedFriendRequestError", nil)];
                return;
            }
            
            [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
            [RobloxData requestFrienshipWithUserID:_profile.userID completion:^(BOOL success)
            {
                [RobloxHUD hideSpinner:YES];
                if(success)
                {
                    _profile.friendshipStatus = RBXFriendshipStatusRequestSent;
                    [self updateFriendshipButton];
                }
                else
                {
                    [RobloxHUD showMessage:NSLocalizedString(@"GenericFailedFriendRequestError", nil)];
                }
            }];
            break;
        }
        case RBXFriendshipStatusRequestSent:
        {
            [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonFriendRequest withContext:RBXAContextProfile withCustomDataString:@"requestAlreadySent"];
            // Nothing to do
            break;
        }
        case RBXFriendshipStatusFriends:
        {
            [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonFriendRequest withContext:RBXAContextProfile withCustomDataString:@"alreadyFriends"];
            RBFriendshipActionSheet* actionSheet = [[RBFriendshipActionSheet alloc] initFriendSheetFromProfile:self];
            [actionSheet showFromRect:_friendshipButton.frame inView:_friendshipButton.superview animated:YES];
            break;
        }
        case RBXFriendshipStatusBestFriends:
        {
            RBFriendshipActionSheet* actionSheet = [[RBFriendshipActionSheet alloc] initFriendSheetFromProfile:self];
            [actionSheet showFromRect:_friendshipButton.frame inView:_friendshipButton.superview animated:YES];
            break;
        }
        case RBXFriendshipStatusRequestReceived:
        {
            [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonFriendRequest withContext:RBXAContextProfile withCustomDataString:@"attemptConfirmRequest"];
            // Nothing to do
            //TO DO : Offer Dialog Box to Accept / Reject Friend Requests
            //RBFriendshipActionSheet* actionSheet = [[RBFriendshipActionSheet alloc] initFriendRequestSheetFromProfile:self];
            //[actionSheet showFromRect:_friendshipButton.frame inView:_friendshipButton.superview animated:YES];
            break;
        }
    }
}


#pragma mark Followers
- (void)updateFollowButton
{
    NSString* imageName = (_profile.isFollowing) ? @"Following User Button" : @"Follow User Button";
    [_followUserButton setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
}

- (IBAction)didPressFollowUser:(id)sender
{
    [RobloxHUD showSpinnerWithLabel:nil dimBackground:YES];
    if (_profile.isFollowing)
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonFollow withContext:RBXAContextProfile withCustomDataString:@"unfollow"];
        
        //we should not follow anymore
        [RobloxData unfollowUser:_profile.userID completion:^(BOOL success) {
            if (success)
                dispatch_async(dispatch_get_main_queue(), ^{
                    _profile.isFollowing = NO;
                    [self updateFollowButton];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_FOLLOWING_UPDATED object:nil];
                });
            
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                [RobloxHUD hideSpinner:YES];
                [self fetchFollowersForUser:_profile.userID];
            });
            
        }];
    }
    else
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonFollow withContext:RBXAContextProfile withCustomDataString:@"follow"];
        
        //let's follow this cool dude
        [RobloxData followUser:_profile.userID completion:^(BOOL success) {
            if (success)
                dispatch_async(dispatch_get_main_queue(), ^{
                    _profile.isFollowing = YES;
                    [self updateFollowButton];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_FOLLOWING_UPDATED object:nil];
                });
            
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                [RobloxHUD hideSpinner:YES];
                [self fetchFollowersForUser:_profile.userID];
            });
        }];
    }
    
    
}
@end





