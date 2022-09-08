//
//  RobloxWebUtilities.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 3/26/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "iOSSettingsService.h"
enum EnumButtons { ButtonGames = 10,
                   ButtonCatalog,
                   ButtonInventory,
                   ButtonBuildersClub,
                   ButtonProfile,
                   ButtonMessages,
                   TextFieldSearch,
                   ButtonGroups,
                   ButtonLeaderboards };

// the key we use to get settings
#define IOS_CLIENT_APP_SETTINGS_STRING  "iOSAppSettings"
#define IOS_CLIENT_SETTINGS_API_KEY "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79"

@class StandardOutMessage;

@interface RobloxWebUtility : NSObject
{
    iOSSettingsService cachediOSSettings;
    NSDate *lastSettingsRequestTime;

    BOOL bUpdating;
    RBX::mutex updateLock;

    std::string clientSettingsData;
    std::string iOSAppSettingsData;
}

@property (nonatomic, readonly) BOOL bAppSettingsInitialized;

- (iOSSettingsService*) getCachediOSSettings;
- (void) updateAllClientSettingsWithCompletion:(void(^)())handler;
- (void) updateAllClientSettingsWithReporting:(BOOL)shouldReport withCompletion:(void(^)())handler;
- (void) writeUpdatedSettings;

+ (instancetype) sharedInstance;

@end
