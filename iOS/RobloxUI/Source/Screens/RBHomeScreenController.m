//
//  RBHomeScreenController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBHomeScreenController.h"
#import "RobloxInfo.h"
#import "RobloxTheme.h"

#define HS_openRobux        @"HOME SCREEN - Open Robux"
#define HS_openBuildersClub @"HOME SCREEN - Open Builders Club"
#define HS_openSettings     @"HOME SCREEN - Open Settings"
#define HS_openLogout       @"HOME SCREEN - Open Logout"
#define HS_homePageLink     @"HOME SCREEN - Home Page Link"
#define HS_openExternalLink @"HOME SCREEN - Open External Link"
#define HS_openProfile      @"HOME SCREEN - Open Profile"
#define HS_openGameDetail   @"HOME SCREEN - Open Game Detail"
#define HS_launchGame       @"HOME SCREEN - Launch Game"

@interface RBHomeScreenController ()

@end

@implementation RBHomeScreenController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.title = NSLocalizedString(@"HomeWord", nil);
    
    self.url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"home"];
    
    [self setViewTheme:RBXThemeSocial];
    
    //if ([RobloxInfo thisDeviceIsATablet])
        [self addRobuxIconWithFlurryEvent:HS_openRobux
                 andBCIconWithFlurryEvent:HS_openBuildersClub];
    
    [self setFlurryGameLaunchEvent:HS_launchGame];
    [self setFlurryPageLoadEvent:HS_homePageLink];
    [self setFlurryEventsForExternalLinkEvent:HS_openExternalLink
                              andWebViewEvent:nil
                          andOpenProfileEvent:HS_openProfile
                       andOpenGameDetailEvent:HS_openGameDetail];
    
    [self addSearchIconWithSearchType:SearchResultUsers andFlurryEvent:nil];
    
    
}

- (void) viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeSocial];
    [super viewWillAppear:animated];
}



@end
