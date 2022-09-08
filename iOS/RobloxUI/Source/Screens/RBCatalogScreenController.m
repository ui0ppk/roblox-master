//
//  RBCatalogMasterController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/3/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBCatalogScreenController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"

#define CS_openRobux        @"CATALOG SCREEN - Open Robux"
#define CS_openBuildersClub @"CATALOG SCREEN - Open Builders Club"
#define CS_openSettings     @"CATALOG SCREEN - Open Settings"
#define CS_openLogout       @"CATALOG SCREEN - Open Logout"
#define CS_catalogSearch    @"CATALOG SCREEN - Catalog Search"
#define CS_openExternalLink @"CATALOG SCREEN - Open External Link"
#define CS_openProfile      @"CATALOG SCREEN - Open Profile"
#define CS_openGameDetail   @"CATALOG SCREEN - Open Game Detail"
#define CS_launchGame       @"CATALOG SCREEN - Launch Game"

@interface RBCatalogScreenController ()

@end

@implementation RBCatalogScreenController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self setViewTheme:RBXThemeGame];
    
    self.navigationItem.title = NSLocalizedString(@"CatalogWord", nil);
    
    [self setUrl:[[RobloxInfo getBaseUrl] stringByAppendingString:@"catalog/"]]; //???
    
    [self addRobuxIconWithFlurryEvent:CS_openRobux
             andBCIconWithFlurryEvent:CS_openBuildersClub];
    
    [self setFlurryPageLoadEvent:CS_catalogSearch];
    [self setFlurryGameLaunchEvent:CS_launchGame]; //don't know when this would ever happen, but I imagine there are ads all over the place
    [self setFlurryEventsForExternalLinkEvent:CS_openExternalLink
                              andWebViewEvent:nil
                          andOpenProfileEvent:CS_openProfile
                       andOpenGameDetailEvent:CS_openGameDetail];
    
}

- (void)viewWillAppear:(BOOL)animated
{
    [self setViewTheme:RBXThemeGame];
    [super viewWillAppear:animated];
}
@end
