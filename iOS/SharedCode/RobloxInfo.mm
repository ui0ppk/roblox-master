//
//  RobloxInfo.m
//  RobloxMobile
//
//  Created by David York on 10/22/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//
#import "RobloxInfo.h"

#import <UIKit/UIDevice.h>
#import <CoreFoundation/CoreFoundation.h>
#import "UserInfo.h"
#import "RobloxWebUtility.h"
#include "FastLog.h"
#include "util/Statistics.h"
#include "v8datamodel/FastLogSettings.h"
#include "iOSSettingsService.h"
#import "RobloxGoogleAnalytics.h"
#include "iOSSettingsService.h"
#include "v8datamodel/DataModel.h"
#include <sys/types.h>
#include <sys/sysctl.h>


#ifndef kCFCoreFoundationVersionNumber_iOS_8_0
    #define kCFCoreFoundationVersionNumber_iOS_8_0 1129.15
#endif

DYNAMIC_FASTFLAGVARIABLE(RequireCuratedGames, false)

// Expose to C/C++
NSString* getBaseUrl() {
    return [RobloxInfo getBaseUrl];
}

NSString* getUserAgentString() {
    return [RobloxInfo getUserAgentString];
}

std::string getFriendlyDeviceName()
{
    return [[RobloxInfo friendlyDeviceName] UTF8String];
}

std::string getDeviceOSVersion()
{
    return [[RobloxInfo deviceOSVersion] UTF8String];
}

int getDeviceMemory()
{
    int size = 0;
    
    NSString *platform = [RobloxInfo deviceType];
    
    if ([platform hasPrefix:@"iPod4"] || [platform hasPrefix:@"iPad1"] || [platform hasPrefix:@"iPhone2"]) // iPhone 3GS
    {
        size = 256;
    }
    else if ([platform hasPrefix:@"iPod5"] || [platform hasPrefix:@"iPad2"] || [platform hasPrefix:@"iPhone3"] || [platform hasPrefix:@"iPhone4"]) // iPhone 4/4S 512MB
    {
        size = 512;
    }
    else if ([platform hasPrefix:@"iPad3"] || [platform hasPrefix:@"iPad4"] || [platform hasPrefix:@"iPhone5"] || [platform hasPrefix:@"iPhone6"] || [platform hasPrefix:@"iPhone7"])
    {
        size = 1024;
    }
    
    return size;
}

bool hasMinMemory(int minMemory)
{
    int size = getDeviceMemory();
    
    if (size == 0)
    {
        return true; // unknown so we assume new device
    }
    else if (minMemory <= size)
    {
        return true;
    }
    
    return false;
}



@implementation RobloxInfo

static NSString * _machine = nil;
static NSString* baseUrl = nil;
static NSString* apiBaseUrl = nil;
static NSString* domainString = nil;
static NSString* wwwBaseUrl = nil;
static NSString* secureBaseUrl = nil;
static NSString* environmentLongName = nil;
static NSString* environmentShortName = nil;
static NSString* baseUrlChangedNotification = @"RBXBaseUrlChangedNotifier";


+(NSString*) getPlistStringFromKey:(NSString*) key
{
    id idFromPlist = [[NSBundle mainBundle] objectForInfoDictionaryKey:key];
    if (!idFromPlist)
    {
        return nil;
    }
    if(![idFromPlist isKindOfClass:[NSString class]])
    {
        return nil;
    }
    return (NSString*) idFromPlist;
}

+(NSNumber*) getPlistNumberFromKey:(NSString*) key
{
    id idFromPlist = [[NSBundle mainBundle] objectForInfoDictionaryKey:key];
    if (!idFromPlist)
    {
        return nil;
    }
    if(![idFromPlist isKindOfClass:[NSNumber class]])
    {
        return nil;
    }
    return (NSNumber*) idFromPlist;
}

+(NSString*)getDeviceType
{
    NSString* model = [RobloxInfo deviceType];
    if ([model rangeOfString:@"iPad"].location != NSNotFound) return @"iPad";
    if ([model rangeOfString:@"iPhone"].location != NSNotFound) return @"iPhone";
    if ([model rangeOfString:@"iPod"].location != NSNotFound) return @"iPod";
    return @"Unknown";
}

// Get model number (-1 for unknown)
+(int) getDeviceModelNumber
{
    NSString* model = [RobloxInfo deviceType];
    int modelNumber = -1;
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        NSRange range = [model rangeOfString:@"iPad"];
        if (range.location != NSNotFound)
        {
            char chNum = [model characterAtIndex:(range.location+4)];
            modelNumber = atoi(&chNum);
        }
    }
    else
    {
        NSRange range;
        range = [model rangeOfString:@"iPod"];
        if (range.location != NSNotFound)
        {
            char chNum = [model characterAtIndex:(range.location+4)];
            modelNumber = atoi(&chNum);
        }
        else
        {
            range = [model rangeOfString:@"iPhone"];
            if (range.location != NSNotFound)
            {
                char chNum = [model characterAtIndex:(range.location+6)];
                modelNumber = atoi(&chNum);
            }
        }
    }
    return modelNumber;
}

+(BOOL) thisDeviceIsATablet
{
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
}

+(BOOL) isTestSite
{
    NSString* url = getBaseUrl();
    if ([url rangeOfString:@".robloxlabs.com"].location != NSNotFound)
        return YES;
    else
        return NO;
}



+ (NSString *) deviceType
{
    if (_machine == nil)
    {
        size_t size;
    
        // Set 'oldp' parameter to NULL to get the size of the data
        // returned so we can allocate appropriate amount of space
        sysctlbyname("hw.machine", NULL, &size, NULL, 0);
        
        // Allocate the space to store name
        char *name = (char*)malloc(size);
        
        // Get the platform name
        sysctlbyname("hw.machine", name, &size, NULL, 0);
        
        // Place name into a string
        _machine = [NSString stringWithUTF8String:name];
        
        // Done with this
        free(name);
    }
    
    return _machine;
}

+(NSString*) deviceOSVersion
{
    return [[UIDevice currentDevice] systemVersion];
}
+(BOOL) isDeviceOSVersionPreiOS8
{
    return NSFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_8_0;
}

+(NSString*) appVersion
{
    return [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
}

static NSString* robloxDeviceName = nil;
+(NSString*) friendlyDeviceName
{
    //This website has been immensely useful : http://www.everymac.com/ultimate-mac-lookup/
    
    if (!robloxDeviceName)
    {
        //fetch the platform name
        NSString *platform = [RobloxInfo deviceType];
        
        //make a default name
        robloxDeviceName = [NSString stringWithFormat:@"Unknown-%@",platform];
        
        //try to find the real name
        NSString* firstFour = [platform substringToIndex:(platform.length >= 4) ? 4 : platform.length]; //for faster parsing
        if ([firstFour isEqualToString:@"iPho"])
        {
            if      ([platform isEqualToString:@"iPhone1,1"])    robloxDeviceName =  @"iPhone 1G";
            else if ([platform isEqualToString:@"iPhone1,2"])    robloxDeviceName =  @"iPhone 3G";
            else if ([platform isEqualToString:@"iPhone2,1"])    robloxDeviceName =  @"iPhone 3GS";
            else if ([platform isEqualToString:@"iPhone3,1"])    robloxDeviceName =  @"iPhone 4";
            else if ([platform isEqualToString:@"iPhone3,2"])    robloxDeviceName =  @"iPhone 4";
            else if ([platform isEqualToString:@"iPhone3,3"])    robloxDeviceName =  @"iPhone 4"; //(Verizon)
            else if ([platform isEqualToString:@"iPhone4,1"])    robloxDeviceName =  @"iPhone 4S";
            else if ([platform isEqualToString:@"iPhone5,1"])    robloxDeviceName =  @"iPhone 5"; //(GSM)
            else if ([platform isEqualToString:@"iPhone5,2"])    robloxDeviceName =  @"iPhone 5"; //(GSM+CDMA)
            else if ([platform isEqualToString:@"iPhone5,3"])    robloxDeviceName =  @"iPhone 5C"; //(GSM)
            else if ([platform isEqualToString:@"iPhone5,4"])    robloxDeviceName =  @"iPhone 5C"; //(Global)
            else if ([platform isEqualToString:@"iPhone6,1"])    robloxDeviceName =  @"iPhone 5S"; //(GSM)
            else if ([platform isEqualToString:@"iPhone6,2"])    robloxDeviceName =  @"iPhone 5S"; //(Global)
            else if ([platform isEqualToString:@"iPhone7,1"])    robloxDeviceName =  @"iPhone 6 Plus";
            else if ([platform isEqualToString:@"iPhone7,2"])    robloxDeviceName =  @"iPhone 6";
            else if ([platform isEqualToString:@"iPhone8,1"])    robloxDeviceName =  @"iPhone 6S";
            else if ([platform isEqualToString:@"iPhone8,2"])    robloxDeviceName =  @"iPhone 6S Plus";
        }
        else if ([firstFour isEqualToString:@"iPod"])
        {
            if      ([platform isEqualToString:@"iPod1,1"])      robloxDeviceName =  @"iPod Touch (1 Gen)";
            else if ([platform isEqualToString:@"iPod2,1"])      robloxDeviceName =  @"iPod Touch (2 Gen)";
            else if ([platform isEqualToString:@"iPod3,1"])      robloxDeviceName =  @"iPod Touch (3 Gen)";
            else if ([platform isEqualToString:@"iPod4,1"])      robloxDeviceName =  @"iPod Touch (4 Gen)";
            else if ([platform isEqualToString:@"iPod5,1"])      robloxDeviceName =  @"iPod Touch (5 Gen)";
            else if ([platform isEqualToString:@"iPod7,1"])      robloxDeviceName =  @"iPod Touch (6 Gen)";
        }
        else if ([firstFour isEqualToString:@"iPad"])
        {
            if      ([platform isEqualToString:@"iPad1,1"])      robloxDeviceName =  @"iPad 1";
            else if ([platform isEqualToString:@"iPad2,1"])      robloxDeviceName =  @"iPad 2"; //(WiFi)
            else if ([platform isEqualToString:@"iPad2,2"])      robloxDeviceName =  @"iPad 2"; //(GSM)
            else if ([platform isEqualToString:@"iPad2,3"])      robloxDeviceName =  @"iPad 2"; //(CDMA)
            else if ([platform isEqualToString:@"iPad2,4"])      robloxDeviceName =  @"iPad 2"; //(WiFi)
            else if ([platform isEqualToString:@"iPad2,5"])      robloxDeviceName =  @"iPad Mini 1"; //(WiFi)
            else if ([platform isEqualToString:@"iPad2,6"])      robloxDeviceName =  @"iPad Mini 1"; //(GSM)
            else if ([platform isEqualToString:@"iPad2,7"])      robloxDeviceName =  @"iPad Mini 1"; //(GSM+CDMA)
            else if ([platform isEqualToString:@"iPad3,1"])      robloxDeviceName =  @"iPad 3"; //(WiFi)
            else if ([platform isEqualToString:@"iPad3,2"])      robloxDeviceName =  @"iPad 3"; //(GSM+CDMA)
            else if ([platform isEqualToString:@"iPad3,3"])      robloxDeviceName =  @"iPad 3"; //(GSM)
            else if ([platform isEqualToString:@"iPad3,4"])      robloxDeviceName =  @"iPad 4"; //(WiFi)
            else if ([platform isEqualToString:@"iPad3,5"])      robloxDeviceName =  @"iPad 4"; //(GSM)
            else if ([platform isEqualToString:@"iPad3,6"])      robloxDeviceName =  @"iPad 4"; //(GSM+CDMA)
            else if ([platform isEqualToString:@"iPad4,1"])      robloxDeviceName =  @"iPad Air"; //(WiFi)
            else if ([platform isEqualToString:@"iPad4,2"])      robloxDeviceName =  @"iPad Air"; //(GSM+CDMA)
            else if ([platform isEqualToString:@"iPad4,4"])      robloxDeviceName =  @"iPad Mini 2"; //Retina (WiFi)
            else if ([platform isEqualToString:@"iPad4,5"])      robloxDeviceName =  @"iPad Mini 2"; //Retina (GSM+CDMA)
            else if ([platform isEqualToString:@"iPad4,6"])      robloxDeviceName =  @"iPad Mini 2"; //Retina (China)
            else if ([platform isEqualToString:@"iPad4,7"])      robloxDeviceName =  @"iPad Mini 3"; //(WiFi)
            else if ([platform isEqualToString:@"iPad4,8"])      robloxDeviceName =  @"iPad Mini 3"; //(GSM+CDMA)
            else if ([platform isEqualToString:@"iPad4,9"])      robloxDeviceName =  @"iPad Mini 3"; //(Wifi+China)
            else if ([platform isEqualToString:@"iPad5,1"])      robloxDeviceName =  @"iPad Mini 4"; //(Wifi Only)
            else if ([platform isEqualToString:@"iPad5,2"])      robloxDeviceName =  @"iPad Mini 4"; //(Wifi+Cellular)
            else if ([platform isEqualToString:@"iPad5,3"])      robloxDeviceName =  @"iPad Air 2"; //(WiFi)
            else if ([platform isEqualToString:@"iPad5,4"])      robloxDeviceName =  @"iPad Air 2"; //(GSM+CDMA)
        }
        else if ([firstFour isEqualToString:@"Watc"])
        {
            if      ([platform isEqualToString:@"Watch1,1"])     robloxDeviceName =  @"iWatch"; //38 mm
            else if ([platform isEqualToString:@"Watch1,2"])     robloxDeviceName =  @"iWatch"; //42 mm
        }
        else
        {
            if      ([platform isEqualToString:@"i386"])         robloxDeviceName = @"Simulator 32 bit intel";
            else if ([platform isEqualToString:@"x86_64"])       robloxDeviceName = @"Simulator 64 bit intel";
        }
    }
    
    return robloxDeviceName;
    
    
#if 0
    NSString* userFriendlyDeviceName = @"Unknown";
    NSString* urlString;
    urlString = [[RobloxInfo getBaseUrl] stringByAppendingString:@"mobileapi/friendly-device-name"];
    
    NSURL *url = [NSURL URLWithString: urlString];
    
    
    NSMutableURLRequest *theRequest = [NSMutableURLRequest  requestWithURL:url
                                                               cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                           timeoutInterval:60*7];

    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    
    [theRequest setHTTPMethod:@"GET"];
    
    NSData *receivedData = [NSURLConnection sendSynchronousRequest:theRequest returningResponse:nil error:NULL];
	if (receivedData)
        userFriendlyDeviceName = [[[NSString alloc] initWithData:receivedData encoding:NSUTF8StringEncoding] autorelease];
    
    return userFriendlyDeviceName;
#endif
}

static NSString* userAgentString = nil;
+(NSString*) getUserAgentString
{
    if (!userAgentString)
    {
        id userAgentName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"RbxUserAgent"];
        userAgentString = [NSString stringWithFormat:@"Mozilla/5.0 (%@; %@; CPU iPhone OS %@ like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Mobile/9B176 ROBLOX iOS App %@ %@Hybrid",
                            [UIDevice currentDevice].model,
                            [self deviceType],
                            [[UIDevice currentDevice] systemVersion],
                            [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
                            (userAgentName ? [(NSString*)userAgentName stringByAppendingString:@" "] : @"")];
    }
    return userAgentString;
}

+(void) setDefaultHTTPHeadersForRequest:(NSMutableURLRequest*)request
{
    [request setValue:getUserAgentString() forHTTPHeaderField:@"User-Agent"];
    
    BOOL requireCuratedGames = [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"RbxRequireCuratedGames"] boolValue];
    if(DFFlag::RequireCuratedGames && requireCuratedGames == YES)
    {
        [request setValue:@"true" forHTTPHeaderField:@"require-curated-games"];
    }
}



+(NSString*) getBaseUrl
{
    if (baseUrl == nil) {
        if ([RobloxInfo thisDeviceIsATablet]) {
            baseUrl = (NSString*)[[[NSBundle mainBundle] infoDictionary] objectForKey:@"RbxBaseUrl"];
        } else {
            baseUrl = (NSString*)[[[NSBundle mainBundle] infoDictionary] objectForKey:@"RbxBaseMobileUrl"];
        }
        
        if (![baseUrl hasSuffix:@"/"]) {
            NSMutableString *newBaseUrl = [[NSMutableString alloc] initWithString:baseUrl];
            [newBaseUrl appendString:@"/"];
            baseUrl = newBaseUrl;
        }

        [RobloxInfo setBaseUrl:baseUrl];
    }
    return baseUrl;
}

+(NSString*) getStoryboardName
{
    return [[[NSBundle mainBundle] infoDictionary] objectForKey:@"UIMainStoryboardFile"];
}

+(NSString*) getSecureBaseUrl
{
    if (secureBaseUrl == nil)
    {
        NSString* url = [self getWWWBaseUrl];
        if (![url isEqualToString:@""])
        {
            NSRange separator = [url rangeOfString:@":"];
            
            NSString *Right = [url substringWithRange:NSMakeRange(separator.location , url.length - separator.location - 1)];
            url = [NSString stringWithFormat:@"https%@", Right];
            secureBaseUrl = url;
        }
    }
    return secureBaseUrl;
}

+(NSString*) getApiBaseUrl
{
    if (apiBaseUrl == nil)
    {
        NSString* url = getBaseUrl();
        
        if(![url isEqualToString:@""])
        {
            NSRange separator = [url rangeOfString:@"."];
            
            NSString *Right = [url substringWithRange:NSMakeRange(separator.location , url.length - separator.location - 1)];
            url = [NSString stringWithFormat:@"https://api%@", Right];
            apiBaseUrl = url;
        }
    }
    return apiBaseUrl;
}

+(NSString*) getDomainString
{
    if (domainString == nil)
    {
        NSString* url = getBaseUrl();
        
        if(![url isEqualToString:@""])
        {
            url = [url stringByReplacingOccurrencesOfString:@"http://" withString:@""];
            NSRange separator = [url rangeOfString:@"."];
            
            url = [url substringWithRange:NSMakeRange(separator.location , url.length - separator.location - 1)];

            url = [url stringByReplacingOccurrencesOfString:@"/" withString:@""];

            domainString = url;
        }
    }
    return domainString;
    
}

+(NSString*) getWWWBaseUrl
{
    if (wwwBaseUrl == nil)
    {
        NSString* url = getBaseUrl();
        
        NSRange separator = [url rangeOfString:@"."];
        //keep the '/' at the end, do not truncate last character
        NSString* right = [url substringWithRange:NSMakeRange(separator.location, url.length - separator.location)];
        
        url = [NSString stringWithFormat:@"http://www%@", right];
        
        wwwBaseUrl = url;
    }
    
    return wwwBaseUrl;
}



+(NSString*) getBaseUrlChangedNotification
{
    return baseUrlChangedNotification;
}

+(void) setBaseUrl:(NSString*)url
{
    baseUrl = url;
    if (![url hasSuffix:@"/"]) {
        url = [url stringByAppendingString:@"/"];
    }
    SetBaseURL([url UTF8String]);

    [[NSNotificationCenter defaultCenter] postNotificationName:baseUrlChangedNotification object:self userInfo:nil];
    [RobloxGoogleAnalytics startup];
    
    //clear all the urls that have dependencies on the base url, they will be recreated the next time they are requested
    apiBaseUrl = nil;
    domainString = nil;
    wwwBaseUrl = nil;
    secureBaseUrl = nil;
    environmentShortName = nil;
    environmentLongName = nil;
}


+(NSString*) getEnvironmentName:(bool)shortVersion
{
    if (environmentLongName == nil || environmentShortName == nil)
    {
        //parse out the environment name from the baseURL
        NSString* url = [self getBaseUrl];
        NSString* right = [url substringFromIndex:[url rangeOfString:@"."].location + 1];
        NSString* environment = [right substringToIndex:[right rangeOfString:@"."].location];
        if (environment)
        {
            environmentLongName = environment;

            //NOTE - the function [NSString containsString:] is unsupported in versions of iOS before iOS 8
            if ([environment isEqualToString:@"roblox"])
                environmentShortName = @"PROD";
            else if ([environment rangeOfString:@"sitetest"].location != NSNotFound)
                environmentShortName = [NSString stringWithFormat:@"ST%@", [environment substringFromIndex:environment.length-1]];
            else if ([environment rangeOfString:@"gametest"].location != NSNotFound)
                environmentShortName = [NSString stringWithFormat:@"GT%@", [environment substringFromIndex:environment.length-1]];
            else
                environmentShortName = @"UNKNOWN";
        }
        else
        {
            environmentLongName = @"UNKNOWN";
            environmentShortName = @"UNKNOWN";
        }
    }
    
    //NSLog(@"Long Environment Name = %@", environmentLongName);
    //NSLog(@"Short Environment Name = %@", environmentShortName);
    if (shortVersion && environmentShortName)
        return environmentShortName;
    else if (!shortVersion && environmentLongName)
        return environmentLongName;

    //if all else fails
    return @"UNKNOWN";
}

+(NSString*) searchUrl
{
    const char* searchEndpointLog;
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    
    if ([RobloxInfo thisDeviceIsATablet] == YES)
        searchEndpointLog = iosSettings->GetValueSearchEndpointIPad();
    else
        searchEndpointLog = iosSettings->GetValueSearchEndpointIPhone();
    
    return [NSString stringWithUTF8String:searchEndpointLog];
}

+(NSString *) gameGenres
{
    const char* genres;
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    
    genres = iosSettings->GetValueRBXGameGenres();
    
    return [NSString stringWithUTF8String:genres];
}

+(void) reportMaxMemoryUsedForPlaceID:(NSInteger)placeID
{
    NSLog(@"reportMaxMemoryUsedForPlaceID %ld", (long)placeID);
    [[NSUserDefaults standardUserDefaults] synchronize];
    NSInteger maxMemoryUsed = [[NSUserDefaults standardUserDefaults] integerForKey:@"MaxMemoryUsed"];
    long megaBytes = maxMemoryUsed/(1024*1024);
    
    NSLog(@"maxMemoryUsed: %ld megaBytes: %ld", (long)maxMemoryUsed, megaBytes);
    
    if (megaBytes > 0)
    {
        NSString * placeIDString = [NSString stringWithFormat:@"%ld", (long)placeID];
        NSString * deviceTypeString = [RobloxInfo deviceType];
        NSLog(@"MAX MEMORY REPORT: deviceType:%@ place:%@ MB:%ld", deviceTypeString, placeIDString, megaBytes);

        [RobloxGoogleAnalytics      setEventTracking:@"MaxMemoryUsage"
                                          withAction:deviceTypeString
                                           withLabel:placeIDString
                                           withValue:megaBytes];
    }
}

@end
