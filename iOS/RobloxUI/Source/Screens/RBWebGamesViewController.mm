//
//  RBCatalogMasterController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/3/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBWebGamesViewController.h"
#import "RobloxInfo.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "FastLog.h"
#import "RobloxNotifications.h"
#import "RBXFunctions.h"

DYNAMIC_FASTFLAGVARIABLE(UseNewWebGamesPage, true);

@interface RBWebGamesViewController ()

@end

@implementation RBWebGamesViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.title = NSLocalizedString(@"GameWord", nil);
    
    [self setViewTheme:RBXThemeGame];
    
    if ([UserInfo CurrentPlayer].userLoggedIn)
        [self addNavBarButtons:nil];
    else
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addNavBarButtons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    
    
    [self addSearchIconWithSearchType:SearchResultGames andFlurryEvent:nil];
    
    if (DFFlag::UseNewWebGamesPage)
        [self setUrl:[[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"games"]];
    else
        [self setUrl:[[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"games/list"]];
    
    
    
}

-(void) viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeGame];
    [super viewWillAppear:animated];

}

-(void) addNavBarButtons:(NSNotification*)notification
{
    [RBXFunctions dispatchOnMainThread:^{
        [self addRobuxIconWithFlurryEvent:nil
                 andBCIconWithFlurryEvent:nil];
        
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(removeIcons:) name:RBX_NOTIFY_LOGGED_OUT object:nil];
    }];
}

-(void) removeIcons:(NSNotification*) notification
{
    [RBXFunctions dispatchOnMainThread:^{
        [self removeRobuxAndBCIcons];
        
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGGED_OUT object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addNavBarButtons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    }];
}
@end
