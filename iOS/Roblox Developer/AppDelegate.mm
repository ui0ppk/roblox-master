//
//  AppDelegate.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 10/9/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "AppDelegate.h"
#import "PlaceLauncher.h"
#import "UserInfo.h"
#import "RobloxInfo.h"
#import "RobloxMemoryManager.h"
#import "UpgradeCheckHelper.h"
#import "CrashReporter.h"
#import "SessionReporter.h"
#import "KeychainItemWrapper.h"
#import "LoginManager.h"
#import "ABTestManager.h"
#import "RobloxNotifications.h"

#include "v8datamodel/GuiBuilder.h"
#include "util/standardout.h"
#include "util/http.h"


@implementation AppDelegate

@synthesize bgTask;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    leavingGame = NO;
    
    [LoginManager sharedInstance];
    
    //we need to call this on start up, so save a browser tracker
    //NSString* browserTracker = [RobloxData initializeBrowserTracker];
    //[[NSUserDefaults standardUserDefaults] setObject:browserTracker forKey:BROWSER_TRACKER_KEY];
    //[[NSUserDefaults standardUserDefaults] synchronize];
    
    // this makes sure everyone is on the right minimum version of the app
    [UpgradeCheckHelper checkForUpdate:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"]];
    
    // Register the preference defaults early.
    NSString * defaultKeys[2] = { @"warnings_preference", @"wifionly_preference" };
    NSNumber * defaultValues[2] = { [NSNumber numberWithBool:YES], [NSNumber numberWithBool:NO] };
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjects:(id *)defaultValues forKeys:(id *)defaultKeys count:2];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    [CrashReporter sharedInstance];
    [[SessionReporter sharedInstance] reportSessionFor:APPLICATION_FRESH_START];
    
    KeychainItemWrapper *keychainItem = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
    if (keychainItem)
    {
        NSString *password = [keychainItem objectForKey:(__bridge id)kSecValueData];
        NSString* username = [keychainItem objectForKey:(__bridge id)kSecAttrAccount];

        [UserInfo CurrentPlayer].username = username;
        [UserInfo CurrentPlayer].password = password;
    }
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(gotDidLeaveGameNotification:) name:RBX_NOTIFY_GAME_DID_LEAVE object:nil ];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(gotStartLeaveGameNotification:) name:RBX_NOTIFY_GAME_START_LEAVING object:nil ];
    
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@HAS_SHOWN_KEY];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    
    [[LoginManager sharedInstance] setRememberPassword:true];
    
    [[ABTestManager sharedInstance] fetchExperimentsForBrowserTracker];
    
    return YES;
}

-(void) gotDidLeaveGameNotification:(NSNotification *)aNotification
{
    leavingGame = NO;
    [[UIApplication sharedApplication] setStatusBarHidden:NO];
}

-(void) gotStartLeaveGameNotification:(NSNotification *)aNotification
{
    leavingGame = YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    
    [[PlaceLauncher sharedInstance] disableViewBecauseGoingToBackground];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    [[NSUserDefaults standardUserDefaults] setObject:@"tryBackground" forKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    
    [[PlaceLauncher sharedInstance] leaveGame];
    
    [[SessionReporter sharedInstance] reportSessionFor:APPLICATION_BACKGROUND];

    [[LoginManager sharedInstance] processBackground];
    
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxGameState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    
    [[PlaceLauncher sharedInstance] clearCachedContent];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // this makes sure everyone is on the right minimum version of the app
    [UpgradeCheckHelper checkForUpdate:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"]];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [[NSUserDefaults standardUserDefaults] setObject:@"tryForeground" forKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    RobloxWebUtility * robloxWebUtility = [RobloxWebUtility sharedInstance];
    if (![robloxWebUtility bAppSettingsInitialized])
    {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW,0), ^{
            [robloxWebUtility updateAllClientSettingsWithCompletion:nil];
        });
    }
    
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    
    [[PlaceLauncher sharedInstance] enableViewBecauseGoingToForeground];
    
    [[SessionReporter sharedInstance] reportSessionFor:APPLICATION_ACTIVE];
    
    [[NSUserDefaults standardUserDefaults] setObject:@"inApp" forKey:@"RobloxAppState"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) ? UIInterfaceOrientationMaskAll : UIInterfaceOrientationMaskLandscape;
}

@end
