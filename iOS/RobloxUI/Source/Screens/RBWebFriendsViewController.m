//
//  RBWebFriendsViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/10/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBWebFriendsViewController.h"
#import "RobloxInfo.h"
#import "RobloxTheme.h"
#import "RobloxNotifications.h"
#import "UITabBarItem+CustomBadge.h"
#import "RBXMessagesPollingService.h"

#define FS_openRobux                @"FRIENDS SCREEN - Open Robux"
#define FS_openBuildersClub         @"FRIENDS SCREEN - Open Builders Club"

@interface RBWebFriendsViewController ()
@property int badgeNumber;
@property NSString* friendsURL;
@end

@implementation RBWebFriendsViewController
-(id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        _badgeNumber = 0;
        [self initNotificationPolling];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _friendsURL = [NSString stringWithFormat:@"%@users/%@/friends", [RobloxInfo getWWWBaseUrl], [UserInfo CurrentPlayer].userId];
    
    [self setViewTheme:RBXThemeSocial];
    
    self.navigationItem.title = NSLocalizedString(@"FriendsWord", nil);
    
    [self addRobuxIconWithFlurryEvent:FS_openRobux
             andBCIconWithFlurryEvent:FS_openBuildersClub];
    
    
}

- (void) viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeSocial];
    
    //auto-navigate to the friends request section if you have any outstanding requests
    NSString* currentURL = [_friendsURL stringByAppendingString:(_badgeNumber <= 0 ? @"#!/friends" : @"#!/friend-requests")];
    [self setUrl:currentURL];
    [super viewWillAppear:animated];
}

-(void) initNotificationPolling
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)  name:RBX_NOTIFY_NEW_MESSAGES_TOTAL        object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)  name:RBX_NOTIFY_NEW_FRIEND_REQUESTS_TOTAL object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)  name:RBX_NOTIFY_FRIEND_REQUESTS_UPDATED  object:nil];
    [self updateBadge];
}
-(void) updateBadge
{
    dispatch_async(dispatch_get_main_queue(), ^
   {
       //dispatch the update command to the main thread for instant update
       _badgeNumber = [[RBXMessagesPollingService sharedInstance] totalFriendRequests];
       if ([self navigationController])
           if ([[self navigationController] tabBarItem])
           {
               //this should be cleaned up at some point to use a RobloxTheme constant. Roblox Theme needs to be cleaned up.
               [self.navigationController.tabBarItem setBadgeValue:(_badgeNumber <= 0 ? nil : [NSString stringWithFormat:@"%i",_badgeNumber])];
           }
   });
}
@end
