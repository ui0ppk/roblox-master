//
//  LoadingScreenViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/27/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import "AppDelegateROMA.h"

#import "ABTestManager.h"
#import "LoadingScreenViewController.h"
#import "LoginManager.h"
#import "RBActivityIndicatorView.h"
#import "RBXFunctions.h"
#import "RobloxData.h"
#import "RobloxCachedFlags.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "RobloxTheme.h"
#import "RobloxWebUtility.h"
#import "SessionReporter.h"
#import "StoreManager.h"
#import "UpgradeCheckHelper.h"
#import "UserInfo.h"

#import <AdColony/AdColony.h>

DYNAMIC_FASTSTRING(AdColonyAppId);
DYNAMIC_FASTSTRING(AdColonyZoneId);

///TO DO: MIGRATE ALL INITIALIZATION CODE THAT DOES NOT REQUIRE KNOWLEDGE
///         OF THE APP STATE OUT OF THE APP DELEGATE AND INTO THIS SCREEN

@interface LoadingScreenViewController ()

@property IBOutlet UILabel* lblMessage;
@property IBOutlet RBActivityIndicatorView* loadingSpinner;
@property IBOutlet UIImageView* imgLogo;

@property (nonatomic) int completedTasks;

@property (nonatomic) BOOL initializingDevice;

@property (nonatomic) NSDate* initializationStartTime;

@end

@implementation LoadingScreenViewController

//Life Cycle Functions
- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [_lblMessage setText:NSLocalizedString(@"LoadingWord", nil)];
    [_lblMessage setHidden:YES];
    
    [_loadingSpinner setHidden:YES];
}

-(void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    [RBXFunctions dispatchOnMainThread:^
    {
        [_loadingSpinner setHidden:NO];
        [_loadingSpinner startAnimating];
        [_lblMessage setHidden:NO];
    }];

    [self initializeDevice];
}

//Task Functions
-(void) initializeDevice
{
    ///USE THIS FUNCTION TO PERFORM ANY ACTIONS AND CONVEY MESSAGES TO THE USER
    _initializationStartTime = [NSDate date];

    
    //STEP 1) we need to call this on start up, to create/save a browser tracker
    [self updateMessage:NSLocalizedString(@"LoadingMessageInitDevice", nil)];
    NSDate* reportingTime = [NSDate date];
    [RobloxData initializeBrowserTrackerWithCompletion:^(bool success, NSString* browserTracker)
    {
        //keep track of how long it took to initalize the browser tracker, we cannot report it yet
        NSTimeInterval initializeTime = [[NSDate date] timeIntervalSinceDate:reportingTime];
        
        //Configure the analytics
        [[RBXEventReporter sharedInstance] reportAppLaunch:RBXAContextAppLaunch]; //this is timestamped to measure the time from launch to "ready"
        
        
        // Update all client settings
        // NOTE - analytic reporting for flag loading is already handled within this function
        [self updateAllClientSettingsWithCompletion:^
        {
            //now that we have client flags, we can send reports out send out analytic reports
            [[SessionReporter sharedInstance] postStartupPayloadForEvent:@"deviceInitialize" completionTime:initializeTime];
            
            //initialize some flag settings
            [self initCrashlytics];
            [self initAdColony];
            
            //fetch the AB Tests (legacy synchronous operation)
            [self initABTests];
            
            //last of all
            [self doTaskAutoLogin];
        }];
    }];
}
-(void) updateAllClientSettingsWithCompletion:(void(^)(void))handler
{
    [self updateMessage:NSLocalizedString(@"LoadingMessageInitFlags", nil)];
    
    //Code Migrated from AppDelegateROMA - 2/25/2016
    [RBXFunctions dispatchOnMainThread:^
    {
        RobloxWebUtility * robloxWebUtility = [RobloxWebUtility sharedInstance];
        
        //Make sure that the base URL has been set
        //NOTE- This must be initialized or else the WebUtility will crash
        [RobloxInfo getBaseUrl];
        
        //update the settings
        [robloxWebUtility updateAllClientSettingsWithReporting:YES withCompletion:handler];
    }];
}
-(void) initCrashlytics
{
    //Code Migrated from AppDelegateROMA - 2/25/2016
    [self updateMessage:NSLocalizedString(@"LoadingMessageInitCrashlytics", nil)];
    
    [RBXFunctions dispatchOnMainThread:^
    {
        iOSSettingsService* iss = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
        [[RobloxCachedFlags sharedInstance] setInt:@"CrashlyticsPercentage" withValue:iss->GetValueCrashlyticsPercentage()];
        [[RobloxCachedFlags sharedInstance] sync];
    }];
    
}
-(void) initAdColony
{
    //Code Migrated from AppDelegateROMA - 2/25/2016
    [self updateMessage:NSLocalizedString(@"LoadingMessageInitAdColony", nil)];
    
    [RBXFunctions dispatchOnMainThread:^
    {
        NSString* adColonyAppId = [NSString stringWithUTF8String:DFString::AdColonyAppId.c_str()];
        NSString* adColonyZoneId = [NSString stringWithUTF8String:DFString::AdColonyZoneId.c_str()];
        
        [AdColony configureWithAppID:adColonyAppId zoneIDs:@[adColonyZoneId] delegate:nil logging:YES];
        
    }];
}
-(void) initABTests
{
    // initalize the AB Test - this may take a bit - it is a synchronous request
    [self updateMessage:NSLocalizedString(@"LoadingMessageInitExperiments", nil)];
    
    NSDate* fetchingExperimentsStart = [NSDate date];
    [[ABTestManager sharedInstance] fetchExperimentsForBrowserTracker];
    NSTimeInterval fetchExperimentsTime = [[NSDate date] timeIntervalSinceDate:fetchingExperimentsStart];
    
    //report how long it took to initialize the AB Tests
    [[SessionReporter sharedInstance] postStartupPayloadForEvent:@"fetchABTestExperiments" completionTime:fetchExperimentsTime];
}
-(void) doTaskAutoLogin
{
    //attempt to automatically log in
    NSString* message;
    if ([[LoginManager sharedInstance] hasLoginCredentials] || [[LoginManager sharedInstance] hasSocialLoginCredentials] || [LoginManager sessionLoginEnabled])
        message = NSLocalizedString(@"LoadingMessageAutoLogin", nil);
    
    if (message)
    {
        [self updateMessage:message];
        
        NSDate* autoLoginTaskStart = [NSDate date];
        [[LoginManager sharedInstance] processStartupAutoLogin:^(NSError *loginError)
        {
            if ([LoginManager sessionLoginEnabled])
            {
                //report how long a session login took
                [[SessionReporter sharedInstance] postStartupPayloadForEvent:@"fetchUserInfo" completionTime:[[NSDate date] timeIntervalSinceDate:autoLoginTaskStart]];
            }
            
            
            //check for a successful login
            if ([RBXFunctions isEmpty:loginError])
            {
                // Initialize Store Manager on login, so that any pending transaction from last session are queued
                GetStoreMgr;
            }
            
            
            //regarless if we are successfully logged in or not, go to the Welcome Screen
            //The Welcome Screen will decide what screen to display
            [self goToWelcomeScreen];
        }];
    }
    else
    {
        
        //the app has no stored credentials and session login is not enabled, escape and go to the Welcome Screen
        [self goToWelcomeScreen];
    }
}


//Helper Functions
-(void) updateMessage:(NSString*)message
{
    [RBXFunctions dispatchOnMainThread:^{ [_lblMessage setText:message]; }];
}

//Navigation Functions
-(void) goToWelcomeScreen
{
    //report the total time for startup
    [[SessionReporter sharedInstance] postStartupPayloadForEvent:@"startupFinished" completionTime:[[NSDate date] timeIntervalSinceDate:_initializationStartTime]];
    
    //We are done here, let's move on
    //if (_lblMessage.text == nil || _lblMessage.text.length == 0)
    //    [_lblMessage setText:NSLocalizedString(@"LoadingMessageComplete", nil)];
    
    [RBXFunctions dispatchOnMainThread:^{
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [self performSegueWithIdentifier:@"ShowWelcomeScreen" sender:self];
    }];
}

@end
