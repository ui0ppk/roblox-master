//
//  CrashReporter.m
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 6/25/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "CrashReporter.h"
#import "RobloxCachedFlags.h"
#import "RobloxWebUtility.h"
#import "util/standardout.h"
#import "ObjectiveCUtilities.h"
#include "FastLog.h"
#import "RobloxGoogleAnalytics.h"
#import <Crashlytics/Crashlytics.h>

// DO NOT RELEASE WITH TESTING ENABLED FOR APP STORE SUBMIT
//#define ROBLOX_TESTING 1

@implementation CrashReporter

bool crashlyticsActive = false;

#define CHANNEL_OUTPUT 1
#define CHANNEL_LOGGING 2

static void fastLogMesage(FLog::Channel id, const char* message)
{
    if(id == CHANNEL_OUTPUT)
    {
        printf("FLog%02d: %s\n", id, message);
    }
    else if (id == CHANNEL_LOGGING)
    {
        if(crashlyticsActive)
        {
            CLS_LOG("%@", [NSString stringWithUTF8String:message]);
        }
        // Always do the NSLog
        NSLog(@"%s", message);
    }
}

-(NSString*) activeCrashReporterString
{
    if(crashlyticsActive)
        return @"Crashlytics";
    else
        return @"None";
}


+ (CrashReporter*)sharedInstance
{
    static dispatch_once_t rbxCrashReporterFlagsPred = 0;
    __strong static CrashReporter* _sharedObject = nil;
    dispatch_once(&rbxCrashReporterFlagsPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(void) setupCrashlytics
{
#ifndef _DEBUG
    if(!crashlyticsActive)
    {
        crashlyticsActive = true;
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH,0), ^{
            
            id crashlyticsKey = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CrashlyticsKey"];
            if (!crashlyticsKey)
            {
                NSLog(@"Crash Reporter cannot initialize Crashlytics due to missing API key");
                return;
            }
            if(![crashlyticsKey isKindOfClass:[NSString class]])
            {
                NSLog(@"Crash Reporter not initializing Crashlytics because plist entry is not a string");
                return;
            }
            NSString* crashlyticsString = (NSString*) crashlyticsKey;
            if([crashlyticsString length] <= 0)
            {
                NSLog(@"Crash Reporter cannot initialize Crashlytics due to API key having length 0");
                return;
            }
            
            [Crashlytics startWithAPIKey:crashlyticsString];
            
            NSLog(@"Crash Reporter initialized: Crashlytics");
            [[NSUserDefaults standardUserDefaults] setObject:@"Crashlytics" forKey:@"CrashReporterSDK"];
            [[NSUserDefaults standardUserDefaults] synchronize];
        
            
        });
    }
#endif

}

- (void) setupFastLogConnection
{
    FLog::SetExternalLogFunc(fastLogMesage);
    messageOutConnection = RBX::StandardOut::singleton()->messageOut.connect(boostFuncFromSelector_1< const RBX::StandardOutMessage& >
                                                                             (@selector(tryLogMessage:),self) );
}


- (void) setup
{
    [[RobloxCachedFlags sharedInstance] sync];
    
    
    NSString *gameStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxGameState"];
    NSString *appStateString = [[NSUserDefaults standardUserDefaults] stringForKey:@"RobloxAppState"];
    
    // clear out if we cleanly exited
    if ([appStateString isEqualToString:@"inBackground"] || [appStateString isEqualToString:@"terminated"] )
    {
        appStateString = nil;
    }
    
    if (gameStateString || appStateString)
    {
        NSString *crashReporterSDK = [[NSUserDefaults standardUserDefaults] stringForKey:@"CrashReporterSDK"];
        if (crashReporterSDK)
        {
            if([crashReporterSDK rangeOfString:@"Crashlytics"].location != NSNotFound)
                [self setupCrashlytics];
            
            [self setupFastLogConnection];
            return;
        }
    }
    
    
    srand (time(NULL));
    int rnd = (rand() % 100) + 1;
    
    NSInteger pctCrashlytics = 0;
    BOOL bCrashlytics = [[RobloxCachedFlags sharedInstance] getInt:@"CrashlyticsPercentage" withValue:&pctCrashlytics];

    if (bCrashlytics && pctCrashlytics > 0)
    {
        if (rnd <= pctCrashlytics)
        {
            [self setupCrashlytics];
        }
    }
    
    [self setupFastLogConnection];
}

-(id) init
{
    if(self = [super init])
    {
        [self setup];
    }
    return self;
}
                      
- (void) tryLogMessage:(const RBX::StandardOutMessage&) message
{
    
    NSString* logString = [NSString stringWithCString:message.message.c_str() encoding:NSUTF8StringEncoding];
    if([logString isEqualToString:@""])
        return;
    
    RBX::MessageType messageType = message.type;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        
        iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
        int tfLoggingLevel = iosSettings->GetValueCrashLoggingLevel();
        
        if ( messageType >= tfLoggingLevel )
        {
            if (crashlyticsActive)
            {
                CLS_LOG("%@", logString);
            }
        }
        
        // Always do the NSLog
        NSLog(@"%@", logString);

    });
}

- (void) logStringKeyValue:(NSString*) key withValue:(NSString*) value
{
    if (crashlyticsActive)
        [[Crashlytics sharedInstance] setObjectValue:value forKey:key];
}

- (void) logBoolKeyValue:(NSString*) key withValue:(BOOL) value
{
    if (crashlyticsActive)
        [[Crashlytics sharedInstance]setIntValue:value forKey:key];
}

- (void) logIntKeyValue:(NSString*) key withValue:(int) value
{
    if (crashlyticsActive)
        [[Crashlytics sharedInstance]setIntValue:value forKey:key];
}

- (void) logFloatKeyValue:(NSString*) key withValue:(float) value
{
    if (crashlyticsActive)
        [[Crashlytics sharedInstance]setIntValue:value forKey:key];
}


@end
