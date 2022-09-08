//
//  SessionReporter.m
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 7/12/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "SessionReporter.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxInfo.h"
#import "PlaceLauncher.h"
#import "UserInfo.h"
#import "iOSSettingsService.h"
#import "RobloxMemoryManager.h"
#import "RobloxNotifications.h"
#import "RBXFunctions.h"
#import "v8datamodel/Stats.h"
#import "NSDictionary+Parsing.h"

#define KEY_PLACE_ID                "PlaceId"
#define KEY_GAME_START_TIME         "GameStartTime"
#define KEY_GAME_END_TIME           "GameEndTime"
#define KEY_APP_CRASH               "CRASHED"
#define KEY_FREE_MEMORY             "FreeMemory"
#define KEY_USED_MEMORY             "UsedMemory"
#define KEY_NUM_MEM_WARNING         "NumMemoryWarning"
#define KEY_LAST_OUT_OF_MEM_TIME    "LastOutOfMemWarningTime"

#define APP_STATUS_SUCCESS      "AppStatusSuccess"
#define APP_STATUS_CRASH        "AppStatusCrash"
#define APP_OUT_OF_MEM_ON_LOAD  "AppStatusOutOfMemoryOnLoad"
#define APP_OUT_OF_MEM_IN_GAME  "AppStatusOutOfMemoryInGame"

DYNAMIC_FASTINTVARIABLE(iOSInfluxHundredthsPercentage, 1000)
DYNAMIC_FASTINTVARIABLE(GenerateOutOfMemoryReportIfBelowMB, 10)
static int calculateSessionDataEveryMilliSeconds = 100;

@implementation SessionReporter

+ (id)sharedInstance
{
    static dispatch_once_t rbxSessionReporterFlagsPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxSessionReporterFlagsPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}



-(id) init
{
    if(self = [super init])
    {
    }
    return self;
}

-(void)pushSessionData: (NSString*) eventType PlaceId:(NSInteger) placeId GamePlayTime:(NSInteger) playTime
{
    RBX::Analytics::InfluxDb::Points points;
    points.addPoint("SessionReport" , [eventType UTF8String]);
    

    [RobloxGoogleAnalytics setEventTracking:@"SessionReport" withAction:eventType withLabel:[NSString stringWithFormat:@"%ld", (long)placeId] withValue:playTime];
    
    NSString *appVersion = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastRunningAppVersion"];
    if (!appVersion)
        appVersion  = [RobloxInfo appVersion];
    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"LastUserLoggedIn"])
    {
        points.addPoint("LastUserLoggedIn" , [[[NSUserDefaults standardUserDefaults] stringForKey:@"LastUserLoggedIn"] UTF8String]);
        RBX::Analytics::setUserId([[NSUserDefaults standardUserDefaults] integerForKey:@"LastUserIDLoggedIn"]);
        
    }
    
    if([eventType isEqualToString:@APP_STATUS_CRASH])
        [RobloxGoogleAnalytics  setEventTracking:@"CrashCountByUser"
                                withAction:appVersion
                                withLabel:[[NSUserDefaults standardUserDefaults] stringForKey:@"LastUserLoggedIn"]
                                withValue:0];
    
   
    NSString *gameStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxGameState"];
    NSString *appStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxAppState"];
    if (gameStateString || appStateString)
    {
        NSString * crashStateString = [[NSUserDefaults standardUserDefaults] stringForKey:(gameStateString ? @"RobloxGameState" : @"RobloxAppState")];

        points.addPoint("GameState" , [crashStateString UTF8String]);
        points.addPoint("FreeMemoryKB" , (int)[[NSUserDefaults standardUserDefaults] integerForKey:@KEY_FREE_MEMORY]);
        points.addPoint("UsedMemoryKB" , (int)[[NSUserDefaults standardUserDefaults] integerForKey:@KEY_USED_MEMORY]);
        
        if (gameStateString)
        {
            RBX::Analytics::setPlaceId(placeId);
            points.addPoint("PlayTime" , (int)playTime);
            points.addPoint("NumMemoryWarning" , (int)[[NSUserDefaults standardUserDefaults] integerForKey:@KEY_NUM_MEM_WARNING]);
            
            // If we had received some memory warnings before crashing
            if ([eventType isEqualToString:@APP_STATUS_CRASH] && [[NSUserDefaults standardUserDefaults] objectForKey:@KEY_LAST_OUT_OF_MEM_TIME])
            {
                if ([[NSUserDefaults standardUserDefaults] objectForKey:@KEY_GAME_END_TIME])
                {
                    int lastMemWarningMSec = 1000 * ([[NSUserDefaults standardUserDefaults] doubleForKey:@KEY_GAME_END_TIME] - [[NSUserDefaults standardUserDefaults] doubleForKey:@KEY_LAST_OUT_OF_MEM_TIME]);
                    points.addPoint("MillSecMemoryWarn" , lastMemWarningMSec);
                }
            }
        }
    }
    
    points.report("iOS-RobloxPlayer-SessionReport", DFInt::iOSInfluxHundredthsPercentage);
    
    if ([eventType isEqualToString:@APP_STATUS_CRASH] && [[NSUserDefaults standardUserDefaults] integerForKey:@KEY_FREE_MEMORY] < (DFInt::GenerateOutOfMemoryReportIfBelowMB * 1024) && gameStateString)
        eventType = [gameStateString isEqualToString:@"inGame"] ?  @APP_OUT_OF_MEM_IN_GAME : @APP_OUT_OF_MEM_ON_LOAD;
    
    NSString* apiUrl = [NSString stringWithFormat:@"%@/game/sessions/report?placeId=%ld&eventType=%@", [RobloxInfo getApiBaseUrl], (long)placeId, eventType];
    NSURL *url = [NSURL URLWithString: apiUrl];
    
    NSMutableURLRequest *theRequest = [NSMutableURLRequest  requestWithURL:url
                                                            cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                            timeoutInterval:60*7];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"POST"];
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *receiptResponseData, NSError *error){/*Do not care for response*/}];
}

- (BOOL) getPlayData: (NSInteger&) placeId PlayTimeSeconds: (NSInteger&) gamePlayTime CalculateNow: (BOOL) endNow
{    
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@KEY_PLACE_ID])
    {
        if ([[NSUserDefaults standardUserDefaults] objectForKey:@KEY_GAME_START_TIME])
        {
            if ([[NSUserDefaults standardUserDefaults] objectForKey:@KEY_GAME_END_TIME])
                gamePlayTime = [[NSUserDefaults standardUserDefaults] doubleForKey:@KEY_GAME_END_TIME] - [[NSUserDefaults standardUserDefaults] doubleForKey:@KEY_GAME_START_TIME];
            else if (endNow)
                gamePlayTime = [[NSDate date] timeIntervalSince1970] - [[NSUserDefaults standardUserDefaults] doubleForKey:@KEY_GAME_START_TIME];
            
            placeId = [[NSUserDefaults standardUserDefaults] integerForKey:@KEY_PLACE_ID];
        }
        return YES;
    }
    return NO;
}

-(void) callTimerFn
{
    if([[PlaceLauncher sharedInstance] getIsCurrentlyPlayingGame])
    {
        [self reportSessionFor:GAME_RUNNING];
        [self performSelector:@selector(callTimerFn) withObject:nil afterDelay:((NSTimeInterval)calculateSessionDataEveryMilliSeconds/1000)];
    }
}

-(void) reportSessionFor:(ApplicationState) appState
{
    [self reportSessionFor:appState PlaceId:0];
}


-(void) postAnalyticPayloadFromContext:(RBXAnalyticsContextName)context
                                result:(RBXAnalyticsResult)result
                              rbxError:(RBXAnalyticsErrorName)rbxError
                          startingTime:(NSDate*)startTime
                     attemptedUsername:(NSString*)username
                            URLRequest:(NSURLRequest*)request
                      HTTPResponseCode:(NSInteger)responseCode
                          responseData:(NSData*)responseData
                        additionalData:(NSDictionary*)additionalData
{
    [RBXFunctions dispatchOnBackgroundThread:^{
        
        //NOTE - REQUIRE ALL INPUT INTO THE DICTIONARY TO BE STRINGS
        NSMutableDictionary<NSString*, NSString*> *analyticsPayload = [NSMutableDictionary dictionary];
        
        //NSTimeInterval is a measurement given in seconds, be sure to cast it to milliseconds
        NSTimeInterval responseTime = MAX(0, ([[NSDate date] timeIntervalSinceDate:startTime] * 1000));
        [analyticsPayload setObject:[NSString stringWithFormat:@"%d", (int)responseTime] forKey:@"responseTimeMs"];
        
        //Keep track of the type of action we are doing
        if (context == RBXAContextLogin || context == RBXAContextSocialLogin || context == RBXAContextAppLaunch)
        {
            //NOTE - REQUIRES ADDITIONAL DATA
            NSString* loginType = @"manual";
            bool performingAutoLogin = [additionalData boolValueForKey:@"isAutoLogin" withDefault:NO];
            if      (context == RBXAContextSocialLogin && performingAutoLogin == YES)  loginType = @"socialAuto";
            else if (context == RBXAContextSocialLogin && performingAutoLogin == NO)   loginType = @"social";
            else if (context != RBXAContextSocialLogin && performingAutoLogin == YES)  loginType = @"auto";
            [analyticsPayload setObject:loginType forKey:@"loginType"];
        }
        else if (context == RBXAContextSocialLogin || context == RBXAContextSignup)
        {
            NSString* signupType = (context == RBXAContextSignup) ? @"regular" : @"social";
            [analyticsPayload setObject:signupType forKey:@"signupType"];
        }
        else if (context == RBXAContextSettingsSocial)
        {
            // NOTE - REQUIRES ADDITIONAL DATA
            //save what social network we are connecting to
            NSString* socialNetworkName = [additionalData stringForKey:@"provider" withDefault:nil];
            if (socialNetworkName)
                [analyticsPayload setObject:socialNetworkName forKey:@"provider"];
        }
        
        if (result == RBXAResultFailure) {
            
            //Failure reason
            [analyticsPayload setObject:[[RBXEventReporter sharedInstance] nameForError:rbxError] forKey:@"Status"];
            
            //Request URL
            if (![RBXFunctions isEmpty:request.URL.description])
                [analyticsPayload setObject:request.URL.description forKey:@"requestUrl"];
            
            //Response body as a string, but clean it up so it reports properly
            NSString *responseBody = [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding];
            responseBody = [responseBody stringByReplacingOccurrencesOfString:@"\'" withString:@"\\\'"];
            responseBody = [responseBody stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
            responseBody = [responseBody stringByReplacingOccurrencesOfString:@"\n" withString:@" "]; //replace new line with space
            responseBody = [responseBody stringByReplacingOccurrencesOfString:@"\r" withString:@" "]; //replace line return with space
            if (![RBXFunctions isEmpty:responseBody])
                [analyticsPayload setObject:responseBody forKey:@"responseBody"];
            
            // Username <- simply pulling it from the UserInfo object is not accurate
            if (![RBXFunctions isEmpty:username])
                [analyticsPayload setObject:username forKey:@"username"];
            
            // Browser Tracker Id
            if (![RBXFunctions isEmpty:[RobloxData browserTrackerId]])
                [analyticsPayload setObject:[RobloxData browserTrackerId] forKey:@"browserTrackerId"];
            
            // App Version
            if (![RBXFunctions isEmpty:[RobloxInfo appVersion]])
                [analyticsPayload setObject:[RobloxInfo appVersion] forKey:@"appVersion"];
            
            // Device Type
            if (![RBXFunctions isEmpty:[RobloxInfo deviceType]])
                [analyticsPayload setObject:[RobloxInfo deviceType] forKey:@"deviceType"];
            
            // OS Version
            if (![RBXFunctions isEmpty:[RobloxInfo deviceOSVersion]])
                [analyticsPayload setObject:[RobloxInfo deviceOSVersion] forKey:@"deviceOSVersion"];
            
            //HTTP Response Code
            if (responseCode)
                [analyticsPayload setObject:[NSString stringWithFormat:@"%ld", (long)responseCode] forKey:@"httpResponseCode"];
        }
        
        // Post analytics information
        [self reportSessionForContext:context
                               result:result
                            errorName:rbxError
                         responseCode:responseCode
                                 data:analyticsPayload];
    }];
}

-(void) postStartupPayloadForEvent:(NSString*)requestName
                    completionTime:(NSTimeInterval)timeSeconds
{
    [RBXFunctions dispatchOnBackgroundThread:^{
        NSNumber* timeNumber = [NSNumber numberWithInt:(int)(timeSeconds * 1000.0)];
        
        [self sendSessionDataToInfluxSeries:@"AppStartupTimeiOS"
                                       data:@{@"requestName":requestName,
                                              @"completionTime":timeNumber.stringValue}];
        
    }];
}
-(void) postAutoLoginFailurePayload:(NSTimeInterval)loginTimestamp
                   cookieExpiration:(NSTimeInterval)actualExpirationTimestamp
                 expectedExpiration:(NSTimeInterval)expectedExpirationTimestamp
{
    [RBXFunctions dispatchOnBackgroundThread:^{
        
        //create a number formatter to handle the conversion from timeInterval to string
        NSNumberFormatter* nf = [[NSNumberFormatter alloc] init];
        [nf setMaximumFractionDigits:0];
        [nf setRoundingMode:NSNumberFormatterRoundHalfEven];
        
        //convert NSTimeInterval from seconds to milliseconds
        NSNumber* numLogin = [NSNumber numberWithDouble:(double)loginTimestamp * 1000.0] ;
        NSNumber* numActual = [NSNumber numberWithDouble:(double)actualExpirationTimestamp * 1000.0] ;
        NSNumber* numExpected = [NSNumber numberWithDouble:(double)expectedExpirationTimestamp * 1000.0];
        
        //report the data to Influx
        [self sendSessionDataToInfluxSeries:@"AutoLoginFailures"
                                       data:@{@"initialLoginTimestamp":[nf stringFromNumber:numLogin],
                                              @"cookieExpirationTimestamp":[nf stringFromNumber:numActual],
                                              @"expectedCookieExpirationTimestamp":[nf stringFromNumber:numExpected]}];
        
    }];
}


-(void) sendSessionDataToInfluxSeries:(NSString*)seriesName data:(NSDictionary<NSString*, NSString*>*)dataDictionary
{
    // Post result to Influx
    RBX::Analytics::InfluxDb::Points analyticsPoints;

    for (NSString *key in dataDictionary) {
        NSString* obj = (NSString*)[dataDictionary objectForKey:key];
        //NSLog(@"Session Data [%s] = %@", [key UTF8String], obj);
        analyticsPoints.addPoint(std::string([key UTF8String]).c_str(), [obj UTF8String]);
    }
    analyticsPoints.report(std::string([seriesName UTF8String]), 10000, true);
}

-(void) reportSessionForContext:(RBXAnalyticsContextName)context result:(RBXAnalyticsResult)result errorName:(RBXAnalyticsErrorName)errorName responseCode:(NSInteger)responseCode data:(NSDictionary *)dataDictionary
{
    //Log In Documentation Reference :
    //Sign Up Documentation Reference : https://docs.google.com/document/d/1q2T4--wtMgDtv8Nnv9xJm3V0rJLeXB0hn1GpjnNnEJk/edit?ts=5627cc46
    
    /*
    here are the counter names for diag:
        diag: iOS-AppLogin-Success
        diag: iOS-AppLogin-Failure
        diag: iOS-SocialLogin-Success
        diag: iOS-SocialLogin-Failure
        diag: iOS-AppSignup-Success
        diag: iOS-AppSignup-Failure
        diag: iOS-SocialSignup-Success
        diag: iOS-SocialSignup-Failure
        value will be 1 for each report
    */
    
    //sanitize some inputs
    if (result == RBXAResultFailure) {
        if ([RBXFunctions isEmpty:@(errorName)]) {
            errorName = RBXAErrorNoError;
        }
    }

    NSString* GAContextName = nil;
    switch (context) {
            
        //login metrics
        case RBXAContextLogin:
        {
            // Post result to Google Analytics
            GAContextName = [[RBXEventReporter sharedInstance] nameForContext:context]; //"login"
            
            //Diag
            if      (result == RBXAResultSuccess) {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-AppLogin-Success", 1);
                
            }
            else if (result == RBXAResultFailure) {
                 RBX::Analytics::EphemeralCounter::reportCounter("iOS-AppLogin-Failure", 1);

                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"LoginFailureiOS" data:dataDictionary];
            }
        } break;
            
        // Social Login metrics
        case RBXAContextSocialLogin:
        {
            // Post result to Google Analytics
            GAContextName = [[RBXEventReporter sharedInstance] nameForContext:context]; //"socialLogin"
            //Diag
            if      (result == RBXAResultSuccess) {
                 RBX::Analytics::EphemeralCounter::reportCounter("iOS-SocialLogin-Success", 1);
            }
            else if (result == RBXAResultFailure) {
                 RBX::Analytics::EphemeralCounter::reportCounter("iOS-SocialLogin-Failure", 1);

                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"LoginFailureiOS" data:dataDictionary];
            }
        } break;

        // Automatic login success metrics
        case RBXAContextAppLaunch:
        {
            // Post result to Google Analytics
            GAContextName = [[RBXEventReporter sharedInstance] nameForContext:context]; //"appLaunch"
            
            // Diag
            if (responseCode == 999)
            {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-inAppBrowserTrackerChanged-Success", 1);
                
                //Nothing is being done with the Data dictionary ??
            }
        } break;
         
            
        //Signup metrics
        case RBXAContextSignup:
        {
            // Post result to Google Analytics
            GAContextName = @"SignupAttempt";
            
            //Diag
            if      (result == RBXAResultSuccess) {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-AppSignup-Success", 1);
            }
            else if (result == RBXAResultFailure) {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-AppSignup-Failure", 1);
                
                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"SignupFailureiOS" data:dataDictionary];
            }
            
        } break;
            
        //Social Signup metrics
        case RBXAContextSocialSignup:
        {
            // Post result to Google Analytics
            GAContextName = @"SocialSignupAttempt";
            
            //Diag
            if      (result == RBXAResultSuccess) {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-SocialSignup-Success", 1);
            }
            else if (result == RBXAResultFailure) {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-SocialSignup-Failure", 1);
                
                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"SignupFailureiOS" data:dataDictionary];
            }
            
        } break;
            
        // Social Connect / Disconnect metrics
        case RBXAContextSettingsSocialConnect:
        {
            GAContextName = @"SocialConnectAttempt";

            if (result == RBXAResultFailure)
            {
                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"SocialConnectFailureiOS" data:dataDictionary];
            }
        } break;
            
        case RBXAContextSettingsSocialDisconnect:
        {
            GAContextName = @"SocialDisconnectAttempt";
            if (result == RBXAResultFailure)
            {
                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"SocialDisconnectFailureiOS" data:dataDictionary];
            }
            
        } break;
            
            // Application Did Enter Foreground
        case RBXAContextAppEnterForeground:
        {
            // Post result to Google Analytics
            GAContextName = [[RBXEventReporter sharedInstance] nameForContext:context];
            
            //Diag
            if (result == RBXAResultFailure) {
                RBX::Analytics::EphemeralCounter::reportCounter("iOS-AppEnterForeground-Failure", 1);
                
                // Post additional details to Influx
                [self sendSessionDataToInfluxSeries:@"AppEnterForegroundFailureiOS" data:dataDictionary];
            }
        } break;

            
        //for all other cases, do nothing
        default:
            break;
    }
    
    
    //Report to GA
    if (GAContextName)
    {
        NSString* GAReportString = (result == RBXAResultSuccess) ? @"Success" : [[RBXEventReporter sharedInstance] nameForError:errorName];
        [RobloxGoogleAnalytics setEventTracking:GAContextName
                                     withAction:GAReportString
                                      withLabel:[NSString stringWithFormat:@"%ld", (long)responseCode]
                                      withValue:0];
    }
    
}

-(void) reportSessionFor:(ApplicationState) appState PlaceId:(NSInteger) idPlace
{
    
    switch (appState)
    {
        case APPLICATION_FRESH_START:
        {
            
            NSString *gameStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxGameState"];
            NSString *appStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxAppState"];
    
            if (gameStateString || appStateString)
            {
                NSString * crashStateString = [[NSUserDefaults standardUserDefaults] stringForKey:(gameStateString ? @"RobloxGameState" : @"RobloxAppState")];
                
                [RobloxGoogleAnalytics debugCounterIncrement:crashStateString];
                
                // create "terminated_gameStateString" string if needed
                if (appStateString && [appStateString isEqualToString:@"terminated"])
                {
                    if (gameStateString)
                        crashStateString = [NSString stringWithFormat:@"terminated_%@", gameStateString];
                }
                
                NSString *appVersion = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastRunningAppVersion"];
                if (!appVersion)
                    appVersion  = [RobloxInfo appVersion];
                
                [RobloxGoogleAnalytics  setEventTracking:@"CrashState"
                                        withAction:appVersion
                                        withLabel:crashStateString
                                        withValue:0];
                
                // MemState reporting
                NSString *memMgrStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxMemMgrState"];
                if (memMgrStateString)
                {
                    
                    [RobloxGoogleAnalytics  setEventTracking:@"RobloxMemMgrState"
                                                  withAction:appVersion
                                                   withLabel:memMgrStateString
                                                   withValue:0];
                    
                    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxMemMgrState"];
                    
                    if ([memMgrStateString isEqualToString:@"Bouncer"])
                        [[RobloxMemoryManager sharedInstance] memBouncerCrashed];
                }
                
                if (gameStateString)
                {
                    NSInteger placeId, gamePlayTime = 0;
                    BOOL hasPlayData = [self getPlayData:placeId PlayTimeSeconds:gamePlayTime CalculateNow:NO];
                    
                    if (hasPlayData)
                    {
                        // Do not report to Stats if the Player did not crash, we do similar on Mac/PC/Android
                        // App Crash will not be considered into MTBF
                        [self pushSessionData:@APP_STATUS_CRASH PlaceId:placeId GamePlayTime:gamePlayTime];
                        RBX::Analytics::EphemeralCounter::reportCounter("iOS-ROBLOXPlayer-Crash", 1, true);
                        RBX::Analytics::EphemeralCounter::reportCounter("ROBLOXPlayer-Crash", 1, true);
                    }
                }
                else
                    [self pushSessionData:@APP_STATUS_CRASH PlaceId:0 GamePlayTime:0];

                
                [RobloxGoogleAnalytics setEventTracking:@APP_STATUS_CRASH withAction:@"UserAgent" withLabel:[RobloxInfo getUserAgentString] withValue:0];
                
                
                
                
            }
            
            
            gameStateString = nil;
            appStateString = nil;
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxGameState"];
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxAppState"];
            [[NSUserDefaults standardUserDefaults] synchronize];
            
            [[NSUserDefaults standardUserDefaults] setObject:[RobloxInfo appVersion] forKey:@"LastRunningAppVersion"];
            
            // Since we have reported the last session data time to clear it up.
            [self clearSession];
            [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(callTimerFn) object:nil];
            
            break;
        }
            
        case APPLICATION_ACTIVE:
        {
            NSInteger placeId, gamePlayTime = 0;
            BOOL hasPlayData = [self getPlayData:placeId PlayTimeSeconds:gamePlayTime CalculateNow:NO];
            
            
            if(hasPlayData)
            {
                NSString* eventType = [NSString stringWithFormat:@"%s", APP_STATUS_SUCCESS];
                [self pushSessionData:eventType PlaceId:placeId GamePlayTime:gamePlayTime];
            }
            
            // Since we have reported the last session data time to clear it up.
            [self clearSession];
            
            //Scenario where user was in game, double clicked the home buton & then came back quickly and still in Game.
            NSString *gameStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxGameState"];
            if([gameStateString isEqualToString:@"inGame"])
            {
                [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_GAME_START_TIME];
                [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_GAME_END_TIME];
                [[NSUserDefaults standardUserDefaults] setInteger:placeId forKey:@KEY_PLACE_ID];
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
            
            break;
        }
            
        case APPLICATION_BACKGROUND:
        {
            NSInteger placeId, gamePlayTime = 0;
            BOOL hasPlayData = [self getPlayData:placeId PlayTimeSeconds:gamePlayTime CalculateNow:YES];
            
            // Report if we have gamePlayTime available
            // This will be a successful exit case & would never be a crash case
            // Report State, report successState state only if placeId is not zero & gamePlayTime is not zero
            if(hasPlayData && gamePlayTime)
                [self pushSessionData:@APP_STATUS_SUCCESS PlaceId:placeId GamePlayTime:gamePlayTime];
            
            
            [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(callTimerFn) object:nil];
            
            // Since we have reported the last session data time to clear it up.
            [self clearSession];
            
            break;
        }
            
        case GAME_RUNNING:
        {
            if([[NSUserDefaults standardUserDefaults] objectForKey:@KEY_GAME_START_TIME])
            {
                int kiloBytesFree = freeMemory()/1024.0f;
                int kiloBytesUsed = usedMemory()/1024.0f;
                
                [[NSUserDefaults standardUserDefaults] setInteger:kiloBytesFree forKey:@KEY_FREE_MEMORY];
                [[NSUserDefaults standardUserDefaults] setInteger:kiloBytesUsed forKey:@KEY_USED_MEMORY];
                [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_GAME_END_TIME];
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
            break;
        }
            
        case ENTER_GAME:
        {
            int kiloBytesFree = freeMemory()/1024.0f;
            int kiloBytesUsed = usedMemory()/1024.0f;
            
            [[NSUserDefaults standardUserDefaults] setInteger:kiloBytesFree forKey:@KEY_FREE_MEMORY];
            [[NSUserDefaults standardUserDefaults] setInteger:kiloBytesUsed forKey:@KEY_USED_MEMORY];
            
            [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_GAME_START_TIME];
            [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_GAME_END_TIME];
            [[NSUserDefaults standardUserDefaults] setInteger:idPlace forKey:@KEY_PLACE_ID];
            [[NSUserDefaults standardUserDefaults] synchronize];
            
            
            iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
            calculateSessionDataEveryMilliSeconds = iosSettings->GetValueCalculateSessionReportEveryMilliSeconds();
            
            
            // Keep recording Game End Time, as will be useful in the event of Crash
            dispatch_sync(dispatch_get_main_queue(), ^{
            [self performSelector:@selector(callTimerFn) withObject:nil afterDelay:((NSTimeInterval)calculateSessionDataEveryMilliSeconds/1000)];
            });
            break;
        }
        
        case EXIT_GAME:
        case OUT_MEMORY_ON_LOAD:
        case OUT_MEMORY_IN_GAME:
        {
            if([[NSUserDefaults standardUserDefaults] objectForKey:@KEY_GAME_START_TIME])
            {
                int kiloBytesFree = freeMemory()/1024.0f;
                int kiloBytesUsed = usedMemory()/1024.0f;
                
                [[NSUserDefaults standardUserDefaults] setInteger:kiloBytesFree forKey:@KEY_FREE_MEMORY];
                [[NSUserDefaults standardUserDefaults] setInteger:kiloBytesUsed forKey:@KEY_USED_MEMORY];
                [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_GAME_END_TIME];
            }
            
            if (appState != EXIT_GAME) // It is not a Exit, but a memory warning & we are continuing to play the Game
            {
                int numOutOfMemWarning = [[NSUserDefaults standardUserDefaults] integerForKey:@KEY_NUM_MEM_WARNING];
                [[NSUserDefaults standardUserDefaults] setInteger:++numOutOfMemWarning forKey:@KEY_NUM_MEM_WARNING];
                [[NSUserDefaults standardUserDefaults] setDouble:[[NSDate date] timeIntervalSince1970] forKey:@KEY_LAST_OUT_OF_MEM_TIME];
            }
            [[NSUserDefaults standardUserDefaults] synchronize];

            
            
            NSInteger placeId = 0;
            NSInteger gamePlayTime = 0;
            BOOL hasPlayData = [self getPlayData:placeId PlayTimeSeconds:gamePlayTime CalculateNow:YES];
            
            
            if (appState == EXIT_GAME) // It is a Clean Game Exit, Not a memory warning
            {
                if (hasPlayData)
                {
                    [RobloxInfo reportMaxMemoryUsedForPlaceID:placeId];
                    [self pushSessionData:@APP_STATUS_SUCCESS PlaceId:placeId GamePlayTime:gamePlayTime];
                }
                
                [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_DID_LEAVE_GAME object:nil];
                [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(callTimerFn) object:nil];
                [self clearSession];
            }
            break;
        }
            
        default:
            break;
    }
    
}

-(void) clearSession
{
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_GAME_START_TIME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_GAME_END_TIME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_PLACE_ID];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_FREE_MEMORY];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_USED_MEMORY];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_NUM_MEM_WARNING];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@KEY_LAST_OUT_OF_MEM_TIME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"ExceptionName"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"ExceptionDesc"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"ExceptionReason"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"ExceptionUserInfo"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"ExceptionCallStackTop"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}



@end
