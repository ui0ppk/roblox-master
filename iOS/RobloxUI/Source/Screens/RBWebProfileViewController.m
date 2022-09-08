//
//  RBCatalogMasterController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/3/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBWebProfileViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "RobloxNotifications.h"
#import "RBXFunctions.h"

@interface RBWebProfileViewController ()

@end

@implementation RBWebProfileViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.title = NSLocalizedString(@"ProfileWord", nil);
    
    
    NSString* profileURL = [NSString stringWithFormat:@"%@users/%@/profile", [RobloxInfo getWWWBaseUrl], _userId.stringValue];
    [self loadURL:profileURL screenURL:NO];
    
    if ([[UserInfo CurrentPlayer] userLoggedIn])
        [self addNavBarButtons:nil];
    else
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addNavBarButtons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    
    [self addSearchIconWithSearchType:SearchResultUsers andFlurryEvent:nil];
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
