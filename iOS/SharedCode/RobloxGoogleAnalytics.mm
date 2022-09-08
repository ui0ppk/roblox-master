//
//  RobloxGoogleAnalytics.m
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 11/13/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "RobloxGoogleAnalytics.h"

#ifndef RBX_INTERNAL
#include "iOSSettingsService.h"
#import <GAI.h>
#import <GAIFields.h>
#import <GAITracker.h>
#import <GAILogger.h>
#import <GAITrackedViewController.h>
#import <GAIDictionaryBuilder.h>
#import "RobloxInfo.h"
#import "RobloxWebUtility.h"
static const NSInteger kGANDispatchPeriodSec = 10;
#endif

@implementation RobloxGoogleAnalytics

BOOL initializeDone = NO;



+(void) startup
{
#ifndef RBX_INTERNAL
    if(!initializeDone)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSString* googleAccountString = [RobloxInfo getPlistStringFromKey:@"GoogleAnalyticsAccount"];
            if(!googleAccountString)
            {
                NSLog(@"RobloxGoogleAnalytics cannot initialize due to missing account info");
                return;
            }
            if([googleAccountString length] <= 0)
            {
                NSLog(@"RobloxGoogleAnalytics cannot initialize due to account info having length 0");
                return;
            }
            
            [GAI sharedInstance].dispatchInterval = kGANDispatchPeriodSec;

            [[GAI sharedInstance] trackerWithTrackingId:googleAccountString];
            
            NSNumber* sampleRate = [RobloxInfo getPlistNumberFromKey:@"GoogleAnalyticsSampleRate"];
            if(!sampleRate)
            {
                iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
                sampleRate = [NSNumber numberWithInt:iosSettings->GetValueiOSGoogleAnalyticsSampleRate()];
            }
            
            id<GAITracker> tracker = [[GAI sharedInstance] defaultTracker];
            [tracker set:kGAISampleRate value:[sampleRate stringValue]];
            initializeDone = YES;
        });
    }
#endif
}

+(void) release
{
}

+(void) callBackPageTracking:(NSDictionary*) params {
    [RobloxGoogleAnalytics setPageViewTracking:[params objectForKey:@"url"]];
}


+(void) setPageViewTracking:(NSString*)url
{
#ifndef RBX_INTERNAL
    if(initializeDone) {
        [[[GAI sharedInstance] defaultTracker] set:kGAIScreenName value:url];
        [[[GAI sharedInstance] defaultTracker] send:[[GAIDictionaryBuilder createAppView] build]];
    } else {
        [self performSelector:@selector(callBackPageTracking:) withObject:[NSDictionary dictionaryWithObjectsAndKeys: url, @"url",  nil] afterDelay:2];
    }
#endif
}

+(void) callBackEventTracking:(NSDictionary*) params {
    [RobloxGoogleAnalytics  setEventTracking:[params objectForKey:@"category"]
                            withAction:[params objectForKey:@"action"]
                            withLabel:[params objectForKey:@"label"]
                            withValue:[[params objectForKey:@"value"] intValue]];
}

+(void) setEventTracking:(NSString*)category withAction:(NSString *)action withValue:(NSInteger)value
{
    [self setEventTracking:category withAction:action withLabel:nil withValue:value];
}

+(void) setEventTracking:(NSString*)category withAction:(NSString*)action withLabel:(NSString*)label withValue:(NSInteger)value
{
#ifndef RBX_INTERNAL
    if(initializeDone)
    {
        //NSLog(@"*** GA *** category:%@ action:%@ label:%@ value:%d", category, action, label, value);
        [[[GAI sharedInstance] defaultTracker] send:[[GAIDictionaryBuilder createEventWithCategory:category action:action label:label value:[NSNumber numberWithInt:(int)value]] build]];
    }
    else
    {
        [self performSelector:@selector(callBackEventTracking:)
                withObject:[NSDictionary dictionaryWithObjectsAndKeys: category, @"category", action, @"action", label, @"label", [NSString stringWithFormat:@"%lu", (unsigned long)value], @"value",  nil]
                afterDelay:2];
    }
#endif
}

+(void) callbackCustomVariableTracking:(NSDictionary*) params {
    [RobloxGoogleAnalytics setCustomVariableWithLabel:[params objectForKey:@"label"]
                            withValue:[params objectForKey:@"value"]];
}


+(void) setCustomVariableWithLabel:(NSString*)label withValue:(NSString*)value
{
#ifndef RBX_INTERNAL
    if(initializeDone)
        [[[GAI sharedInstance] defaultTracker] set:label value:value];
    else
        [self performSelector:@selector(callbackCustomVariableTracking:) withObject:[NSDictionary dictionaryWithObjectsAndKeys: label, @"label",  value, @"value", nil] afterDelay:2];
#endif
    
}

+(void) debugCountersPrint
{
    NSUserDefaults * stdUserDefaults = [NSUserDefaults standardUserDefaults];
	[stdUserDefaults synchronize];
    
    // get values
    NSInteger appLaunch = [stdUserDefaults integerForKey:@"debug_appLaunch"];
    NSInteger inApp = [stdUserDefaults integerForKey:@"debug_inApp"];
    NSInteger inGame = [stdUserDefaults integerForKey:@"debug_inGame"];
    NSInteger leaveGame = [stdUserDefaults integerForKey:@"debug_leaveGame"];
    NSInteger tryGameJoin = [stdUserDefaults integerForKey:@"debug_tryGameJoin"];
    NSInteger tryBackground = [stdUserDefaults integerForKey:@"debug_tryBackground"];
    NSInteger tryForeground = [stdUserDefaults integerForKey:@"debug_tryForeground"];
    NSInteger terminated = [stdUserDefaults integerForKey:@"debug_terminated"];
    NSInteger inBackground = [stdUserDefaults integerForKey:@"debug_inBackground"];
    
    // print them
    NSLog(@"=== CRASH STATE DUMP ===");
    NSLog(@"appLaunch %ld", (long)appLaunch);
    NSLog(@"inApp %ld", (long)inApp);
    NSLog(@"inGame %ld", (long)inGame);
    NSLog(@"leaveGame %ld", (long)leaveGame);
    NSLog(@"tryGameJoin %ld", (long)tryGameJoin);
    NSLog(@"tryBackground %ld", (long)tryBackground);
    NSLog(@"tryForeground %ld", (long)tryForeground);
    NSLog(@"terminated %ld", (long)terminated);
    NSLog(@"inBackground %ld", (long)inBackground);
}

+ (void) debugCounterIncrement:(NSString *)label
{
	NSUserDefaults * stdUserDefaults = [NSUserDefaults standardUserDefaults];
    
	[stdUserDefaults synchronize];
	NSString * key = [NSString stringWithFormat:@"debug_%@", label];
	NSInteger value = [stdUserDefaults integerForKey:key];
	value++;
	[stdUserDefaults setInteger:value forKey:key];
	[stdUserDefaults synchronize];
    
    NSLog(@"%@: %ld", key, (long)value);
}

@end
