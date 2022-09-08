//
//  RBCatalogMasterController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/3/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBWebMessagesViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "RBXMessagesPollingService.h"
#import "RBXFunctions.h"

#define MS_openRobux        @"MESSAGES SCREEN - Open Robux"
#define MS_openBuildersClub @"MESSAGES SCREEN - Open Builders Club"

@interface RBWebMessagesViewController ()

@end

@implementation RBWebMessagesViewController

- (id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        [self initNotificationPolling];
    }
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.title = NSLocalizedString(@"MessagesWord", nil);
    [self addRobuxIconWithFlurryEvent:MS_openRobux
                 andBCIconWithFlurryEvent:MS_openRobux];
    
    [self setUrl:[[RobloxInfo getBaseUrl] stringByAppendingString:[RobloxInfo thisDeviceIsATablet] ? @"my/messages/#!/inbox" : @"inbox"]];
}
- (void)viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeSocial];
    [super viewWillAppear:animated];
}

#pragma mark
#pragma Notification Polling functions

-(void) initNotificationPolling
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)  name:RBX_NOTIFY_NEW_MESSAGES_TOTAL    object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)  name:RBX_NOTIFY_INBOX_UPDATED        object:nil];
    [self updateBadge];
}

-(void) updateBadge
{
    [RBXFunctions dispatchOnMainThread:^{
        //dispatch the update command to the main thread for instant update
        int totalM = [[RBXMessagesPollingService sharedInstance] totalMessages];
        
        //add the badge to the navigational tab bar
        if ([self navigationController])
            if ([[self navigationController] tabBarItem])
                [self.navigationController.tabBarItem setBadgeValue:((totalM <= 0) ? nil : [NSString stringWithFormat:@"%i",totalM])];
    }];
}
@end
