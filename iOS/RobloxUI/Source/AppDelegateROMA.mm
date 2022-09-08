//
//  AppDelegate.m
//  RobloxMobile
//
//  Created by David York on 9/20/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "AppDelegateROMA.h"

#import <Bolts/Bolts.h>
#import <FacebookSDK/FacebookSDK.h>
#import <GigyaSDK/Gigya.h>
#import <GooglePlus/GooglePlus.h>

#import "ABTestManager.h"
#import "CrashReporter.h"
#import "iOSSettingsService.h"
#import "LoginManager.h"
#import "ObjectiveCUtilities.h"
#import "RBXEventReporter.h"
#import "RBXFunctions.h"
#import "RBGameViewController.h"
#import "RBMainNavigationController.h"
#import "RBTabBarController.h"
#import "RBXMessagesPollingService.h"
#import "RBWebGamePreviewScreenController.h"
#import "RobloxAlert.h"
#import "RobloxCachedFlags.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxHUD.h"
#import "RobloxInfo.h"
#import "RobloxMemoryManager.h"
#import "RobloxNotifications.h"
#import "RobloxTheme.h"
#import "RobloxWebUtility.h"
#import "SessionReporter.h"
#import "UpgradeCheckHelper.h"
#import "UserInfo.h"

#include "util/standardout.h"
#include "util/http.h"
#include "util/Analytics.h"

#ifndef _DEBUG
#import <AdSupport/AdSupport.h>
#import <MobileAppTracker/MobileAppTracker.h>

#import "Appirater.h"
#import "Flurry.h"
#import "RobloxGoogleAnalytics.h"
#include "v8datamodel/GuiBuilder.h"
#endif


DYNAMIC_FASTFLAGVARIABLE(CleariOSAppStateOnTerminate, true);


#ifdef _DEBUG
    #import "UIView+RBDebug.h"
#endif

#define mAppUpgradeKey @"AppiOSV"

@interface AppDelegateROMA()
@end

@implementation AppDelegateROMA
{
    long _placeIDFromURL;
    BOOL _launchPlaceID;
}
@synthesize bgTask;

- (id) init
{
    if(self = [super init])
    {
        _placeIDFromURL = 0;
        _launchPlaceID = NO;
    }
    return self;
}


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{   
    //Set the UserAgent for all future web requests
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys: [RobloxInfo getUserAgentString], @"UserAgent", nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:dictionary];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    //initialize the cookie accept policy
    [LoginManager sharedInstance];
    
    //configure the default appearance of UI elements
    [RobloxTheme configureDefaultAppearance];
    
    // Initialize Analytics and Event Reporting
    RBX::Analytics::setAppVersion([[RobloxInfo appVersion] UTF8String]);
    [RobloxGoogleAnalytics setEventTracking:@"DeviceType" withAction:[RobloxInfo friendlyDeviceName] withLabel:[RobloxInfo deviceOSVersion] withValue:0];

#if 0
    [UIView toggleViewMainThreadChecking];
#endif
    
#ifndef _DEBUG
    NSString* FLURRY_KEY = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"FlurryKey"];
    if ([FLURRY_KEY length]) {
        [Flurry startSession:FLURRY_KEY];
    }
    
    // App Rating
    NSString* APPIRATER_ID = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"AppiraterId"];
    if ([APPIRATER_ID length]) {
        [Appirater setAppId:APPIRATER_ID];
        [Appirater setDaysUntilPrompt:4];
        [Appirater setUsesUntilPrompt:8];
        [Appirater setTimeBeforeReminding:4];
        [Appirater appLaunched:YES];
    }
#endif
    
#ifndef _DEBUG
    NSString* initialAppState = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxAppState"];
#endif
    
    // detect crashing in this function
    [[NSUserDefaults standardUserDefaults] synchronize];
    NSString * launchState = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxLaunchState"];
    if (launchState)
    {
        [[NSUserDefaults standardUserDefaults] setObject:@"appLaunch" forKey:@"RobloxAppState"];
    }
    
    [[NSUserDefaults standardUserDefaults] setObject:@"appLaunch" forKey:@"RobloxLaunchState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // Register the preference defaults early.
    NSString * defaultKeys[2] = { @"warnings_preference", @"wifionly_preference" };
    NSNumber * defaultValues[2] = { [NSNumber numberWithBool:YES], [NSNumber numberWithBool:NO] };
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjects:(id *)defaultValues forKeys:(id *)defaultKeys count:2];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];

    [CrashReporter sharedInstance];
    [[SessionReporter sharedInstance] reportSessionFor:APPLICATION_FRESH_START];
    [RobloxGoogleAnalytics debugCountersPrint];
    
#ifndef _DEBUG
    // Mobile App Tracking integration
    [MobileAppTracker initializeWithMATAdvertiserId:@"18714"
                                   MATConversionKey:@"4316dbf38e776530b30b954d3786bd41"];
    [MobileAppTracker setAppleAdvertisingIdentifier:[[ASIdentifierManager sharedManager] advertisingIdentifier]
                         advertisingTrackingEnabled:[[ASIdentifierManager sharedManager] isAdvertisingTrackingEnabled]];
    
    // For existing users prior to MAT SDK implementation, call setExistingUser:YES before measureSession.
    // Otherwise, existing users will be counted as new installs the first time they run your app.
    BOOL isExistingUser = initialAppState != nil;
    [MobileAppTracker setExistingUser:isExistingUser];
    
    NSNumber* GuiBuilderDisplayStatsIndex = [RobloxInfo getPlistNumberFromKey:@"GuiBuilderDisplayStatsIndex"];
    if(GuiBuilderDisplayStatsIndex)
    {
        RBX::GuiBuilder::setDebugDisplay(static_cast<RBX::GuiBuilder::Display>([GuiBuilderDisplayStatsIndex intValue]));
    }
#endif
    
    // this makes sure everyone is on the right minimum version of the app
    [UpgradeCheckHelper checkForUpdate:mAppUpgradeKey];
    
    //initialize the notification polling service
    [[RBXMessagesPollingService sharedInstance] beginPolling];
    
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxLaunchState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    //initialize Gigya
    NSString* gigyaKey = [RobloxInfo getPlistStringFromKey:@"GigyaAPIKey"];
    [Gigya initWithAPIKey:gigyaKey application:application launchOptions:launchOptions];
    
    return YES;
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationWillResignActive begin");

    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    
    [[PlaceLauncher sharedInstance] disableViewBecauseGoingToBackground];
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationWillResignActive end");
    
    //pause the notification polling
    [[RBXMessagesPollingService sharedInstance] pausePolling];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    [[NSUserDefaults standardUserDefaults] setObject:@"tryBackground" forKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationDidEnterBackground begin");

    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    
    [[PlaceLauncher sharedInstance] leaveGame];
    
    [[SessionReporter sharedInstance] reportSessionFor:APPLICATION_BACKGROUND];
    

    [RobloxGoogleAnalytics setPageViewTracking:@"RobloxApp/EnterBackGround"];

    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationDidEnterBackground end");
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxGameState"];

    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    [[PlaceLauncher sharedInstance] applicationDidReceiveMemoryWarning];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationWillEnterForeground begin");

#ifndef _DEBUG
    [Appirater appEnteredForeground:YES];
#endif
    
    // this makes sure everyone is on the right minimum version of the app
    [UpgradeCheckHelper checkForUpdate:mAppUpgradeKey];
    [RobloxGoogleAnalytics setPageViewTracking:@"RobloxApp/EnterForeGround"];
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationWillEnterForeground end");
    
    // Session Management - Make a call to getUserAccountInfoForContext to verify that the ROBLOSECURITY cookie is still valid
    if ([LoginManager sessionLoginEnabled] == YES) {
        [[LoginManager sharedInstance] getUserAccountInfoForContext:RBXAContextAppEnterForeground completionHandler:^(NSError *authCookieValidationError) {
            if ([RBXFunctions isEmpty:authCookieValidationError]) {
                // No error; ROBLOSECURITY cookie is still valid; user is still logged in
                [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
            } else {
                // Error; ROBLOSECURITY cookie is invalid; user is logged out
                [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
                [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGGED_OUT object:@{@"object":authCookieValidationError}];
            }
        }];
    }
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [[NSUserDefaults standardUserDefaults] setObject:@"tryForeground" forKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    [FBAppCall handleDidBecomeActive];
    
    //init the facebook app id
    //bool isTestEnvironment = ([[RobloxInfo getEnvironmentName:YES] rangeOfString:@"PROD"].location == NSNotFound);
    //NSString* facebookKeyName =  isTestEnvironment ? @"FacebookTestingAppID" : @"FacebookAppID";
    //NSString* facebookDisplayKeyName = isTestEnvironment ? @"FacebookTestingDisplayName" : @"FacebookDisplayName";
    [FBSettings setDefaultAppID:[RobloxInfo getPlistStringFromKey:@"FacebookAppID"]];
    //[FBSettings setDefaultDisplayName:[RobloxInfo getPlistStringFromKey:facebookDisplayKeyName]];
    [FBAppEvents activateApp];
    [RBXFunctions dispatchOnMainThread:^{
        [[RobloxWebUtility sharedInstance] updateAllClientSettingsWithCompletion:nil];
    }];
    
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationDidBecomeActive begin");
    
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    
    [[PlaceLauncher sharedInstance] enableViewBecauseGoingToForeground];
    [[SessionReporter sharedInstance] reportSessionFor:APPLICATION_ACTIVE];

    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "AppDelegate applicationDidBecomeActive end");
    
    [[NSUserDefaults standardUserDefaults] setObject:@"inApp" forKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] synchronize];

#ifndef _DEBUG
    // Mobile App Tracking integration
    [MobileAppTracker measureSession];
#endif
    
    //reactivate the notification polling if it is off
    [[RBXMessagesPollingService sharedInstance] beginPolling];
    
    // Launch a place from an external source
    if( _placeIDFromURL != 0 )
    {
        [self launchPlace:_placeIDFromURL launch:_launchPlaceID];
        _placeIDFromURL = 0;
        _launchPlaceID = NO;
    }
    
    //set up Gigya
    [Gigya handleDidBecomeActive];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    NSLog(@"RobloxGameState: %@", [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxGameState"]);
    NSLog(@"RobloxAppState: %@", [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxAppState"]);

    if (DFFlag::CleariOSAppStateOnTerminate) // if we don't remove this key, the app will think we crashed every time we launch
    {
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxAppState"];
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxGameState"];
    }
    else
        [[NSUserDefaults standardUserDefaults] setObject:@"terminated" forKey:@"RobloxAppState"];

    [[NSUserDefaults standardUserDefaults] synchronize];
    
    [[PlaceLauncher sharedInstance] leaveGame];
    
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    [[LoginManager sharedInstance] applicationWillTerminate];
    [RobloxGoogleAnalytics setPageViewTracking:@"RobloxApp/Exit"];
    
    //kill the notification polling
    [[RBXMessagesPollingService sharedInstance] clean];
    
    //Report to the server that the app is logging the user out
    if ([[UserInfo CurrentPlayer] userLoggedIn])
        [[RBXEventReporter sharedInstance] reportAppClose:RBXAContextMain];
}

- (BOOL) application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    NSLog(@"AppDelegate::openURL URL:\t%@\n"
          "From source:\t%@\n"
          "With annotation:%@",
          url, sourceApplication, annotation);
    
    BOOL bHandled = NO;

    //report the protocol launch
    [[RBXEventReporter sharedInstance] reportAppLaunch:RBXAContextProtocolLaunch];
    
    NSString * urlString = [url absoluteString];
    if ([urlString hasPrefix:@"robloxmobile"])
    {
        NSArray* components = [urlString componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"=&?/"]];
    
        for (int i=0; i<components.count; i++)
        {
            NSString* component = [components objectAtIndex:i];
            component = [component lowercaseString];
            
            if ([component isEqualToString:@"placeid"])
            {
                _placeIDFromURL = [((NSString*)[components objectAtIndex:(i+1)]) longLongValue];
                _launchPlaceID = YES;
                NSLog(@"setting placeID:%ld", _placeIDFromURL);
                bHandled = YES;
            }
        }
    }
    
    //attempt to open the URL with Gigya
    if (!bHandled)
        bHandled = [Gigya handleOpenURL:url application:application sourceApplication:sourceApplication annotation:annotation];
    
    //attempt to open the URL with Facebook
    if(!bHandled)
    {
        bHandled = [FBAppCall handleOpenURL:url sourceApplication:sourceApplication fallbackHandler:^(FBAppCall *call)
        {
            BFURL *parsedUrl = [BFURL URLWithInboundURL:url sourceApplication:sourceApplication];
            if ([parsedUrl appLinkData])
            {
                NSURL *targetUrl = [parsedUrl targetURL];
                NSArray* components = [[targetUrl absoluteString] componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"=&?/"]];
                for(int i = 0; i < components.count-1; i++)
                {
                    NSString* component = [components objectAtIndex:i];
                    component = [component lowercaseString];
                    
                    if([component isEqualToString:@"games"])
                    {
                        _placeIDFromURL = [((NSString*)[components objectAtIndex:(i+1)]) longLongValue];
                        _launchPlaceID = NO;
                        NSLog(@"setting placeID:%ld", _placeIDFromURL);
                    }
                }
            }
        }];
    }

    //attempt to open the URL with GPP? Google+?
    if(!bHandled)
        bHandled = [GPPURLHandler handleURL:url sourceApplication:sourceApplication annotation:annotation];
    
    
#ifndef _DEBUG
    // Mobile App Tracking integration
    [MobileAppTracker applicationDidOpenURL:[url absoluteString] sourceApplication:sourceApplication];
#endif

    return bHandled;
}

- (void)checkMemoryConstraints:(long)placeId;
{
    if (!hasMinMemory(512))
    {
        [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"UnsupportedDevicePlayError", nil)];
        return;
    }
    
    // log it to GA
    NSString * placeIDString = [NSString stringWithFormat:@"%ld", placeId];
    NSString * deviceTypeString = [RobloxInfo deviceType];
    [RobloxGoogleAnalytics setEventTracking:@"LaunchFromURL" withAction:placeIDString withLabel:deviceTypeString withValue:0];
}

- (void)launchPlace:(long)placeID launch:(BOOL)launchPlace
{
    [RobloxHUD showSpinnerWithLabel:@"" dimBackground:YES];
    
    NSString* strPlaceID = [NSString stringWithFormat:@"%ld", placeID];
    [RobloxData fetchGameDetails:strPlaceID completion:^(RBXGameData *gameData)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            [RobloxHUD hideSpinner:YES];
            
            if(gameData == nil)
            {
                /// There is no Game Details Page on Mobile
                if(![RobloxInfo thisDeviceIsATablet])
                {
                    UINavigationController* rootController = (UINavigationController*) [UIApplication sharedApplication].keyWindow.rootViewController;
                    
                    RBGameViewController* gameControler = [[RBGameViewController alloc] initWithLaunchParams:[RBXGameLaunchParams InitParamsForJoinPlace:gameData.placeID.integerValue]];
                    [rootController presentViewController:gameControler animated:NO completion:nil];
                }
                else
                    [RobloxHUD showMessage:NSLocalizedString(@"UnknownError", nil)];
                
                [RobloxHUD hideSpinner:NO];
            }
            else
            {
                UINavigationController* rootController = (UINavigationController*) [UIApplication sharedApplication].keyWindow.rootViewController;
                
                RBWebGamePreviewScreenController* gamePreviewController = [[RBWebGamePreviewScreenController alloc] init];
                gamePreviewController.isPopover = YES;
                gamePreviewController.gameData = gameData;
                gamePreviewController.showNavButtons = NO;
                
                
                UINavigationController* container = [[UINavigationController alloc] initWithRootViewController:gamePreviewController];
                container.modalPresentationStyle = UIModalPresentationFormSheet;
                
                [rootController presentViewController:container animated:YES completion:^
                {
                    if(launchPlace)
                    {
                        RBGameViewController* gameControler = [[RBGameViewController alloc] initWithLaunchParams:[RBXGameLaunchParams InitParamsForJoinPlace:gameData.placeID.integerValue]];
                        [gamePreviewController presentViewController:gameControler animated:NO completion:nil];
                    }
                }];
            }
        });
    }];
}

@end
