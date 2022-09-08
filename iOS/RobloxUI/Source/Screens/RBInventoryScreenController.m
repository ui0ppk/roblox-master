//
//  RBInventoryMasterController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/3/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBInventoryScreenController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UserInfo.h"

#define ISC_openRobux           @"INVENTORY SCREEN - Open Robux"
#define ISC_openBuildersClub    @"INVENTORY SCREEN - Open Builders Club"
#define ISC_openSettings        @"INVENTORY SCREEN - Open Settings"
#define ISC_openLogout          @"INVENTORY SCREEN - Open Logout"
#define ISC_inventoryPageLink   @"INVENTORY SCREEN - Inventory Page Link"
#define ISC_openExternalLink    @"INVENTORY SCREEN - Open External Link"
#define ISC_openProfile         @"INVENTORY SCREEN - Open Profile"
#define ISC_openGameDetail      @"INVENTORY SCREEN - Open Game Detail"
#define ISC_launchGame          @"INVENTORY SCREEN - Launch Game"

@interface RBInventoryScreenController ()

@end

@implementation RBInventoryScreenController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self setViewTheme:RBXThemeGame];
    self.navigationItem.title = NSLocalizedString(@"InventoryWord", nil);
    
    self.url = [NSString stringWithFormat:@"%@users/%@/inventory#!/hats", [RobloxInfo getWWWBaseUrl], [UserInfo CurrentPlayer].userId];
    
    if ([RobloxInfo thisDeviceIsATablet])
        [self addRobuxIconWithFlurryEvent:ISC_openRobux
                 andBCIconWithFlurryEvent:ISC_openBuildersClub];
    
    [self setFlurryPageLoadEvent:ISC_inventoryPageLink];
    [self setFlurryGameLaunchEvent:ISC_launchGame];
    [self setFlurryEventsForExternalLinkEvent:ISC_openExternalLink
                              andWebViewEvent:nil
                          andOpenProfileEvent:ISC_openProfile
                       andOpenGameDetailEvent:ISC_openGameDetail];
}
@end
