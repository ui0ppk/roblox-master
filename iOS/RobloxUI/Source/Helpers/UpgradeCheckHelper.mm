//
//  UpgradeCheckHelper.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 4/9/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "UpgradeCheckHelper.h"
#import "Reachability.h"
#import "RobloxAlert.h"
#import "PlaceLauncher.h"
#import "RobloxInfo.h"


#include "Util/Http.h"
#include "util/standardout.h"

// semi-private methods
@interface UpgradeCheckHelper ()
- (void) makeUpgradeRequest:(NSURLRequest*) request;
- (UIButton*) getAlertViewButton:(NSString*) buttonTitle;
- (void) processCheckForUpdateResponse;
@end

@implementation UpgradeCheckHelper


+ (UpgradeCheckHelper*) getUpgradeCheckHelper
{
    static dispatch_once_t upgradePred = 0;
    __strong static UpgradeCheckHelper* _sharedUpgradeCheckHelper = nil;
    dispatch_once(&upgradePred, ^{ // Need to use GCD for thread-safe allocation of singleton
        _sharedUpgradeCheckHelper = [[self alloc] init];
    });
    return _sharedUpgradeCheckHelper;
}

- (id) init
{
    if(self = [super init])
    {
        upgradeAlertView = [[UIAlertView alloc] init];
        upgradeAlertView.delegate = self;
        
        upgradeResponseData = [[NSMutableData alloc] init];;
        upgradeConnection = nil;
    }
    return self;
}

+ (NSString *) getUpgradeUrl
{
    return [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"mobileapi/check-app-version?appVersion=%@"];
}

- (UIButton*) getAlertViewButton:(NSString*) buttonTitle
{
    for(UIView *view in upgradeAlertView.subviews)
    {
        if([view isKindOfClass:[UIButton class]])
        {
            UIButton* button = (UIButton*)view;
            if([button.currentTitle isEqualToString:buttonTitle])
                return button;
        }
    }
    
    return nil;
}

- (void) makeUpgradeRequest:(NSURLRequest*) request
{
    // reset our response data
    [upgradeResponseData setLength:0];
    
    if(!request)
        NSLog(@"its bloody nil");
    
    upgradeConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self];
}

+ (void) checkForUpdate:(NSString*) appName
{
    NSString* appVersion = (NSString*)[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    if(appVersion == nil)
        return;
    
    Reachability* reachability = [Reachability reachabilityForInternetConnection];
    NetworkStatus remoteStatus = [reachability currentReachabilityStatus];
    
    if(remoteStatus == NotReachable)
    {
        [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"ConnectionError", nil)];
        return;
    }
    
    appVersion = [NSString stringWithFormat:@"%s%@",RBX::Http::urlEncode([appName cStringUsingEncoding:NSUTF8StringEncoding]).c_str(),appVersion];
    NSString *urlString = [NSString stringWithFormat:[UpgradeCheckHelper getUpgradeUrl],appVersion];
    
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    [[UpgradeCheckHelper getUpgradeCheckHelper] makeUpgradeRequest:request];
}

- (NSString*) getLocalizedUpgradeString:(NSString*) localizedKey
{
    NSString* upgradeString = [NSString stringWithFormat:NSLocalizedString(localizedKey, nil), @"ROBLOX"];
    
    id bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
    if (bundleName)
    {
        if ([bundleName isKindOfClass:[NSString class]])
        {
            NSString* bundleString = (NSString*)bundleName;
            upgradeString = [NSString stringWithFormat:NSLocalizedString(localizedKey, nil), bundleString];
        }
    }
    
    return upgradeString;
}

- (void) processCheckForUpdateResponse
{
    // grab info from connection
    NSError *e = nil;
    NSDictionary *jsonDictionary = [NSJSONSerialization JSONObjectWithData: upgradeResponseData options: NSJSONReadingAllowFragments error: &e];
    if (jsonDictionary == nil || e)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error parsing JSON in AppDelegate: %s",[[e localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding]);
        return;
    }
    
    // parse thru the dictionaries to get the proper responses
    NSDictionary* responseDictionary;
    if([jsonDictionary objectForKey:@"data"] != [NSNull null])
        responseDictionary = (NSDictionary*)[jsonDictionary valueForKey:@"data"];
    else
        return;
    
    NSString* upgradeAction = nil;
    if([responseDictionary objectForKey:@"UpgradeAction"] != [NSNull null])
        upgradeAction = [responseDictionary objectForKey:@"UpgradeAction"];
    else
        return;
    
    NSString* upgradeString = nil;
    if([responseDictionary objectForKey:@"Message"] != [NSNull null])
        upgradeString = [responseDictionary objectForKey:@"Message"];
    
    // post message to user
    if ( [upgradeAction isEqualToString:@"Recommended"] == TRUE )
    {
        //[[PlaceLauncher sharedInstance] leaveGame];
        if (upgradeString == nil)
        {
            upgradeString = [self getLocalizedUpgradeString:@"RecommendUpgradeBody"];
        }
        
        upgradeAlertView.title = NSLocalizedString(@"RecommendUpgradeTitle", nil);
        upgradeAlertView.message = upgradeString;
        
        BOOL addIgnoreButton = YES;
        UIButton* button = [self getAlertViewButton:NSLocalizedString(@"IgnoreButtonText", nil)];
        if(button != nil)
            addIgnoreButton = NO;
        
        if(addIgnoreButton)
            [upgradeAlertView addButtonWithTitle:NSLocalizedString(@"IgnoreButtonText", nil)];
        
        [upgradeAlertView addButtonWithTitle:NSLocalizedString(@"UpgradeButtonText", nil)];
        button = [self getAlertViewButton:NSLocalizedString(@"UpgradeButtonText", nil)];
        if(button != nil)
        {
            button.enabled = YES;
            button.highlighted = YES;
        }
        
        dispatch_async(dispatch_get_main_queue(),^{
                [upgradeAlertView show];
            });
    }
    else if ( [upgradeAction isEqualToString:@"Required"] == TRUE )
    {
        //[[PlaceLauncher sharedInstance] leaveGame];
        if (upgradeString == nil)
        {
            upgradeString = [self getLocalizedUpgradeString:@"RequireUpgradeBody"];
        }
        
        upgradeAlertView.title = NSLocalizedString(@"RequireUpgradeTitle", nil);
        upgradeAlertView.message = upgradeString;
        
        
        [upgradeAlertView addButtonWithTitle:NSLocalizedString(@"UpgradeButtonText", nil)];
        
        dispatch_async(dispatch_get_main_queue(),^{
            [upgradeAlertView show];
        });
    }
}

// NSURLConnectionDataDelegate methods
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    if(connection == upgradeConnection)
        [upgradeResponseData appendData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    if(connection == upgradeConnection)
    {
        upgradeConnection = nil;
        
        [self processCheckForUpdateResponse];
    }
}

// UIAlertViewDelegate methods
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if ( upgradeAlertView == alertView)
    {
        if ([[alertView buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"UpgradeButtonText", nil)])
        {
            id appID = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"AppiraterId"];
            if(appID && [appID isKindOfClass:[NSString class]])
            {
                NSString* url = [NSString stringWithFormat:@"itms-apps://itunes.apple.com/app/id%@", appID];
                [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
            }
            else
            {
                id bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
                if (bundleName)
                {
                    if ([bundleName isKindOfClass:[NSString class]])
                    {
                        NSString* bundleString = (NSString*)bundleName;
                        bundleString = [bundleString stringByReplacingOccurrencesOfString:@" " withString:@""];
                        
                        // bundle name "ROBLOX" does not correspond to app store url, needs mobile
                        if ([bundleString isEqualToString:@"ROBLOX"])
                            bundleString = [bundleString stringByAppendingString:@"mobile"];
                        
                        NSString* url = [@"itms-apps://itunes.com/apps/" stringByAppendingString:bundleString];
                        
                        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
                    }
                }
            }
   
        }
    }
}

@end
