//
//  RBFullFriendListScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBFullFriendListScreenController.h"
#import "RobloxTheme.h"
#import "RobloxData.h"
#import "RBPlayerThumbnailCell.h"
#import "RBProfileViewController.h"

#define FRIENDCELL_ID       @"FriendCell"

#define FRIEND_AVATAR_SIZE CGSizeMake(110, 110)
#define ITEM_SIZE CGSizeMake(90, 108)

#define FRIENDS_PER_REQUEST 60
#define START_REQUEST_THRESHOLD 10 // The next request will start when there are 10 elements left

#define FOLLOWERS_PER_PAGE 50

@interface RBFullFriendListScreenController () <UICollectionViewDataSource, UICollectionViewDelegate>

@end

@implementation RBFullFriendListScreenController
{
    NSMutableArray* _friends;
    
    UICollectionView* _collectionView;
    
    BOOL _requestInProgress;
    BOOL _friendsListComplete;
    NSUInteger _numItemsInCollectionView;
}

-(id)init
{
    _listTypeValue = RBListTypeFriends; //unless overwritten, this should be the default
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
	self.view.backgroundColor = [RobloxTheme lightBackground];
	
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    self.title = [self useRelevantTitle];
    
    // Initialize the collection view
    UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
    [flowLayout setItemSize:ITEM_SIZE];
    [flowLayout setScrollDirection:UICollectionViewScrollDirectionVertical];
    [flowLayout setMinimumLineSpacing:42.0f];
    [flowLayout setMinimumInteritemSpacing:6.0f];
    [flowLayout setSectionInset:UIEdgeInsetsMake(24, 35, 24, 35)];
    
    _collectionView = [[UICollectionView alloc] initWithFrame:self.view.bounds collectionViewLayout:flowLayout];
    _collectionView.dataSource = self;
    _collectionView.delegate = self;
    _collectionView.backgroundColor = [UIColor clearColor];
    _collectionView.backgroundView = nil;
    [_collectionView registerNib:[UINib nibWithNibName:@"RBPlayerThumbnailCell" bundle:nil] forCellWithReuseIdentifier:FRIENDCELL_ID];
    
    UIBarButtonItem* backButton = [[UIBarButtonItem alloc] initWithTitle:@"" style:UIBarButtonItemStyleBordered target:nil action:nil];
    [self.navigationItem setBackBarButtonItem:backButton];
    
    [self.view addSubview:_collectionView];
    
    [self clearCollectionView];
    
    [self fetchFriends];
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    
    [_collectionView setFrame:self.view.bounds];
}

- (void) clearCollectionView
{
    _requestInProgress = NO;
    _friendsListComplete = NO;
    _numItemsInCollectionView = 0;
    _friends = [NSMutableArray array];
    [_collectionView reloadData];
}

- (NSString*) useRelevantTitle
{
    NSString* aTitle = @"";
    
    switch (_listTypeValue)
    {
        case RBListTypeFollowing:  { aTitle = [NSString stringWithFormat:NSLocalizedString(@"FollowingTitleFormat", nil)]; }
            break;
            
        case RBListTypeFollowers: { aTitle = [NSString stringWithFormat:NSLocalizedString(@"FollowersTitleFormat", nil), self.playerName]; }
            break;
            
        default: { aTitle = [NSString stringWithFormat:NSLocalizedString(@"FriendsTitleFormat", nil), self.playerName];  }
            break;
    }
    return aTitle;
}

//THIS IS A MISNOMER- THIS IS A CONTEXT SPECIFIC SEARCH
//IT MIGHT FETCH FRIENDS BUT IT CAN ALSO FETCH OTHER THINGS LIKE FOLLOWERS AND FOLLOWING TOO
- (void) fetchFriends
{
    _friends = [NSMutableArray array];
    [_collectionView reloadData];
    
    switch (_listTypeValue)
    {
        case RBListTypeFriends:
        {
            [RobloxData fetchUserFriends:self.playerID
                              friendType:RBXFriendTypeAllFriends
                              startIndex:0
                                numItems:FRIENDS_PER_REQUEST
                              avatarSize:FRIEND_AVATAR_SIZE
                              completion:^(NSUInteger totalFriends, NSArray *friends)
             {
                 if(friends)
                 {
                     [self initializeElementsWithArray:friends];
                 }
             }];
        }
            break;
        case RBListTypeFollowing:
        {
            [RobloxData fetchMyFollowingAtPage:1
                                    avatarSize:FRIEND_AVATAR_SIZE
                                withCompletion:^(bool success, NSArray *following) {
                                    if (following)
                                        [self initializeElementsWithArray:following];
                                }];
        }
            break;
        case RBListTypeFollowers:
        {
            [RobloxData fetchFollowersForUser:_playerID
                                       atPage:1
                                   avatarSize:FRIEND_AVATAR_SIZE
                               withCompletion:^(bool success, NSArray *followers) {
                                   if (followers)
                                       [self initializeElementsWithArray:followers];
                               }];
        }
            break;
    }
}

- (void) fetchMoreFriends
{
    if(!_friendsListComplete && !_requestInProgress && _numItemsInCollectionView > _friends.count)
    {
        _requestInProgress = YES;
        
        switch (_listTypeValue)
        {
            case RBListTypeFriends:
            {
                NSUInteger itemsToRequest = _numItemsInCollectionView - _friends.count;
                [RobloxData fetchUserFriends:self.playerID
                                  friendType:RBXFriendTypeAllFriends
                                  startIndex:_friends.count
                                    numItems:itemsToRequest
                                  avatarSize:FRIEND_AVATAR_SIZE
                                  completion:^(NSUInteger totalFriends, NSArray *friends) {
                                      [self updateVisualElementsWithArray:friends];
                                  }];
            }
                break;
            case RBListTypeFollowing:
            {
                int page = _friends.count / FOLLOWERS_PER_PAGE;
                [RobloxData fetchMyFollowingAtPage:page
                                        avatarSize:FRIEND_AVATAR_SIZE
                                    withCompletion:^(bool success, NSArray *following) {
                                        [self updateVisualElementsWithArray:following];
                                    }];
            }
                break;
            case RBListTypeFollowers:
            {
                int page = _friends.count / FOLLOWERS_PER_PAGE;
                [RobloxData fetchFollowersForUser:_playerID
                                           atPage:page
                                       avatarSize:FRIEND_AVATAR_SIZE
                                   withCompletion:^(bool success, NSArray *followers) {
                                       [self updateVisualElementsWithArray:followers];
                                   }];
            }
                break;
        }
    }
}

-(void) initializeElementsWithArray:(NSArray*)friends
{
    dispatch_async(dispatch_get_main_queue(), ^
                   {
                       _numItemsInCollectionView = friends.count;
                       [_friends addObjectsFromArray:friends];
                       [_collectionView reloadData];
                   });
}
-(void) updateVisualElementsWithArray:(NSArray*)friends
{
    dispatch_async(dispatch_get_main_queue(), ^
    {
       NSInteger rangeFrom = _friends.count;
       NSInteger rangeTo = rangeFrom + friends.count;
       
       [_friends addObjectsFromArray:friends];
       
       // Update visible elements
       NSArray* visibleCells = [_collectionView indexPathsForVisibleItems];
       NSMutableArray* cellsToUpdate = [NSMutableArray array];
       for(NSIndexPath* indexPath in visibleCells)
       {
           BOOL inRange = indexPath.row >= rangeFrom && indexPath.row < rangeTo;
           if(inRange)
               [cellsToUpdate addObject:indexPath];
       }
       [_collectionView reloadItemsAtIndexPaths:cellsToUpdate];
       
        NSUInteger itemsToRequest = _numItemsInCollectionView - _friends.count;
       _friendsListComplete = friends.count < itemsToRequest;
       if(_friendsListComplete)
       {
           // If the  list is already completed,
           // remove the remaining placeholder empty cells
           NSUInteger totalElements = _numItemsInCollectionView;
           
           _numItemsInCollectionView = _friends.count;
           
           [_collectionView performBatchUpdates:^
            {
                NSMutableArray* indexes = [NSMutableArray array];
                for(NSUInteger i = _friends.count; i < totalElements; ++i)
                {
                    [indexes addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                }
                [_collectionView deleteItemsAtIndexPaths:indexes];
            }
                                     completion:nil];
       }
       
       _requestInProgress = NO;
       
       [self fetchMoreFriends];
    });
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return _numItemsInCollectionView;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    RBPlayerThumbnailCell *cell = [_collectionView dequeueReusableCellWithReuseIdentifier:FRIENDCELL_ID forIndexPath:indexPath];
    if(indexPath.row < _friends.count)
    {
        RBXFriendInfo* friendData = _friends[indexPath.row];
        if([friendData isKindOfClass:[NSNull class]])
            [cell setFriendInfo:nil];
        else
            [cell setFriendInfo:friendData];
    }
    else
    {
        [cell setFriendInfo:nil];
    }
    
    return cell;
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    NSArray* indexes = [_collectionView indexPathsForVisibleItems];
    NSIndexPath* maxIndex = nil;
    for(NSIndexPath* index in indexes)
    {
        if(maxIndex == nil || maxIndex.row < index.row)
        {
            maxIndex = index;
        }
    }
    
    if(_friendsListComplete == NO && maxIndex.row + START_REQUEST_THRESHOLD > _numItemsInCollectionView)
    {
        int totalAdded = (_listTypeValue == RBListTypeFriends) ? FRIENDS_PER_REQUEST : FOLLOWERS_PER_PAGE;
        _numItemsInCollectionView += totalAdded;

        [_collectionView performBatchUpdates:^
        {
            NSMutableArray* indexes = [NSMutableArray array];
            for(NSUInteger i = _numItemsInCollectionView - totalAdded; i < _numItemsInCollectionView; ++i)
            {
                [indexes addObject:[NSIndexPath indexPathForRow:i inSection:0]];
            }
            [_collectionView insertItemsAtIndexPaths:indexes];
        }
        completion:nil];

        [self fetchMoreFriends];
    }
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    if(indexPath.row < _friends.count)
    {
        RBProfileViewController *viewController = (RBProfileViewController*) [self.navigationController.storyboard instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
        
        RBXFriendInfo* friendInfo = _friends[indexPath.row];
        viewController.userId = friendInfo.userID;
        
        [self.navigationController pushViewController:viewController animated:YES];
    }
}

@end
