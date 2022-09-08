//
//  RobloxWebUtility.mm
//  RobloxMobile
//
//  Created by Ben Tkacheff on 3/26/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "RobloxWebUtility.h"
#import "RobloxInfo.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxCachedFlags.h"
#import "RobloxNotifications.h"
#import "v8datamodel/Game.h"
#import "RBXFunctions.h"
#import "SessionReporter.h"

// how long (in seconds) we store iosSettingsService
#define SETTINGS_CACHE_MAX_LIFETIME 300

#define CLIENT_SETTINGS_FILENAME        "ClientAppSettings.json"
#define IOS_CLIENT_SETTINGS_FILENAME    "iOSAppSettings.json"

@implementation RobloxWebUtility

+ (instancetype) sharedInstance
{
    static dispatch_once_t onceToken;
    static RobloxWebUtility *sharedRobloxWebUtility = nil;
    dispatch_once(&onceToken, ^{
        sharedRobloxWebUtility = [[self alloc] init];
    });
    
    return sharedRobloxWebUtility;
}

-(void) loadSettingsJSON:(const char *)group toObject:(RBX::FastLogJSON *)dest fromFile:(NSString *)filename
{
    NSLog(@"RobloxWebUtility::loadSettingsJSON group %s filename %@", group, filename);
    
    NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString * documentsDirectory = [paths objectAtIndex:0];
    
    if (documentsDirectory)
    {
        NSString * filePath = [NSString stringWithFormat:@"%@/%@", documentsDirectory, filename];
        NSError * error = 0;
        NSString * appSettingsJSON = [NSString stringWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:&error];
        
        if (error)
        {
            NSLog(@"error loading %@: %@", filename, error);
        }
        else if (appSettingsJSON)
        {
            NSLog(@"loaded successfully");
            
            std::string settingsData([appSettingsJSON cStringUsingEncoding:NSUTF8StringEncoding]);
            LoadClientSettingsFromString(group, settingsData, dest);
        }
        else
        {
            NSLog(@"could not load %@", filename);
        }
    }
}

-(void) saveSettingsJSON:(std::string *)settingsData toFile:(NSString *)filename
{
    NSLog(@"RobloxWebUtility::saveSettingsJSON filename %@", filename);
    
    NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString * documentsDirectory = [paths objectAtIndex:0];
    
    if (documentsDirectory)
    {
        NSString * filePath = [NSString stringWithFormat:@"%@/%@", documentsDirectory, filename];
        NSString * appSettingsJSON = [NSString stringWithUTF8String:settingsData->c_str()];
        
        if (appSettingsJSON)
        {
            NSError * error = nil;
            [appSettingsJSON writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&error];
            
            if (error)
            {
                NSLog(@"FAILED TO WRITE %@ JSON: %@", filename, error);
            }
            else
            {
                NSLog(@"saved successfully");
            }
        }
        else
        {
            NSLog(@"failed to convert settingsData c-string to NSString!");
        }
    }
}

-(id) init
{
    if(self = [super init])
    {
        RBX::Game::globalInit(false);
        
        // set last time a long time ago
        lastSettingsRequestTime = [NSDate dateWithTimeIntervalSince1970:0];
        
        [self loadSettingsJSON:CLIENT_APP_SETTINGS_STRING toObject:&RBX::ClientAppSettings::singleton() fromFile:@CLIENT_SETTINGS_FILENAME];
        [self loadSettingsJSON:IOS_CLIENT_APP_SETTINGS_STRING toObject:&RBX::ClientAppSettings::singleton() fromFile:@IOS_CLIENT_SETTINGS_FILENAME];
        [self loadSettingsJSON:IOS_CLIENT_APP_SETTINGS_STRING toObject:&cachediOSSettings fromFile:@IOS_CLIENT_SETTINGS_FILENAME];
    
        // Reset synchronized flags, they should be set by the server
        FLog::ResetSynchronizedVariablesState();
    }
    return self;
}

- (iOSSettingsService*) getCachediOSSettings
{
    return &cachediOSSettings;
}

- (void) writeUpdatedSettings
{
    NSLog(@"RobloxWebUtility::writeUpdatedSettings - begin bUpdating:%d", bUpdating);
    
    if (bUpdating)
    {
        // make sure we are not in game
        NSString * gameState = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxGameState"];
        if (gameState && ![gameState isEqualToString:@"tryGameJoin"]) // we are not joining/ingame/leaving
        {
            NSLog(@"RobloxGameState: %@ - not updating", gameState);
        }
        else
        {
            NSLog(@"updating...");
            LoadClientSettingsFromString(CLIENT_APP_SETTINGS_STRING, clientSettingsData, &RBX::ClientAppSettings::singleton());
            LoadClientSettingsFromString(IOS_CLIENT_APP_SETTINGS_STRING, iOSAppSettingsData, &RBX::ClientAppSettings::singleton());
            LoadClientSettingsFromString(IOS_CLIENT_APP_SETTINGS_STRING, iOSAppSettingsData, &cachediOSSettings);
    
            bUpdating = false;
        }
    }

    NSLog(@"RobloxWebUtility::writeUpdatedSettings - end");
}

- (void) updateAllClientSettingsWithReporting:(BOOL)shouldReport withCompletion:(void(^)())handler;
{
    NSLog(@"RobloxWebUtility::updateAllClientSettingsWithReport:%d - begin ", shouldReport);
    NSLog(@"- fetching settings from %@ using baseURL %@", [RobloxInfo getEnvironmentName:NO], [RobloxInfo getBaseUrl]);
    {
        RBX::mutex::scoped_lock lock(updateLock);
        
        if (bUpdating)
        {
            NSLog(@"already updating.");
            return;
        }
        
        
        bUpdating = true;
    }
    
    NSDate* reportingStartTime;
    NSTimeInterval clientFetchTime;
    NSTimeInterval appFetchTime;
    
    
    clientSettingsData.assign("");
    iOSAppSettingsData.assign("");
    
    // Set our fast logs settings
    reportingStartTime = [NSDate date];
    FetchClientSettingsData(CLIENT_APP_SETTINGS_STRING, CLIENT_SETTINGS_API_KEY, &clientSettingsData);
    clientFetchTime = [[NSDate date] timeIntervalSinceDate:reportingStartTime];
    reportingStartTime = [NSDate date];
    FetchClientSettingsData(IOS_CLIENT_APP_SETTINGS_STRING, IOS_CLIENT_SETTINGS_API_KEY, &iOSAppSettingsData);
    appFetchTime = [[NSDate date] timeIntervalSinceDate:reportingStartTime];
    
    //report how long it takes to complete these calls
    if (shouldReport)
    {
        [[SessionReporter sharedInstance] postStartupPayloadForEvent:@"fetchClientSettings" completionTime:clientFetchTime];
        [[SessionReporter sharedInstance] postStartupPayloadForEvent:@"fetchAppSettings" completionTime:appFetchTime];
    }
    
    
    lastSettingsRequestTime = [NSDate date];
    
    [[RobloxCachedFlags sharedInstance] setInt:@"CrashlyticsPercentage" withValue:cachediOSSettings.GetValueCrashlyticsPercentage()];
    
    // debug print
    NSLog(@"CrashlyticsPercentage: %d", cachediOSSettings.GetValueCrashlyticsPercentage());
    
    [[RobloxCachedFlags sharedInstance] sync];
    
    _bAppSettingsInitialized = true;
    
    [self saveSettingsJSON:&clientSettingsData toFile:@CLIENT_SETTINGS_FILENAME];
    [self saveSettingsJSON:&iOSAppSettingsData toFile:@IOS_CLIENT_SETTINGS_FILENAME];
    
    // write them in the main thread to be safe
    [RBXFunctions dispatchOnMainThread:^{
        [self writeUpdatedSettings];

        [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_FFLAGS_UPDATED object:nil];
        if (![RBXFunctions isEmpty:handler])
            handler();
    }];
    
    NSLog(@"RobloxWebUtility::updateAllClientSettingsWithWrite - end");
}

- (void) updateAllClientSettingsWithCompletion:(void(^)(void))handler
{
    [self updateAllClientSettingsWithReporting:NO withCompletion:handler];
}

@end
