//
//  RBWebGamePreviewScreenController.m
//  RobloxMobile
//
//  Created by Pixeloide on 9/19/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBWebGamePreviewScreenController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UserInfo.h"
#import "RobloxHUD.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxNotifications.h"
#import "LoginManager.h"
#import "RBXFunctions.h"

@interface RBWebGamePreviewScreenController () <UIWebViewDelegate>

@end

@implementation RBWebGamePreviewScreenController

- (void)viewDidLoad
{
    self.showNavButtons = NO;
    
    [super viewDidLoad];
    
    self.navigationItem.title = _gameData.title;
    
    self.navigationItem.hidesBackButton = NO;
    self.navigationItem.leftItemsSupplementBackButton = NO;
    
    UIBarButtonItem* backButton = [[UIBarButtonItem alloc] initWithTitle:@"" style:UIBarButtonItemStyleBordered target:nil action:nil];
    [self.navigationItem setBackBarButtonItem:backButton];
    
    NSString* gameURL = [NSString stringWithFormat:@"%@PlaceItem.aspx?id=%@", [RobloxInfo getWWWBaseUrl], _gameData.placeID];
    self.url = gameURL;
    
    if ([UserInfo CurrentPlayer].userLoggedIn)
        [self addNavBarButtons:nil];
    else
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addNavBarButtons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    
    [self addSearchIconWithSearchType:SearchResultGames andFlurryEvent:nil];
}
- (void)viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeGame];
    [super viewWillAppear:animated];
}

-(void) addNavBarButtons:(NSNotification*)notification
{
    [RBXFunctions dispatchOnMainThread:^{
        [self addRobuxIconWithFlurryEvent:nil
                 andBCIconWithFlurryEvent:nil];
        
        if (notification)
            [self reloadWebPage];
        
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
