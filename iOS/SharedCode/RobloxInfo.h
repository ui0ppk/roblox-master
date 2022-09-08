//
//  RobloxInfo.h
//  RobloxMobile
//
//  Created by David York on 10/22/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RobloxInfo : NSObject
{
}

+(BOOL) thisDeviceIsATablet;
+(BOOL) isTestSite;
+(BOOL) isDeviceOSVersionPreiOS8;
+(NSString*) getUserAgentString;
+(NSString*) deviceType;
+(NSString*) getBaseUrl;
+(void) setBaseUrl:(NSString*)url;
+(NSString*) getApiBaseUrl;
+(NSString*) getDomainString;
+(NSString*) getWWWBaseUrl;
+(NSString*) getSecureBaseUrl;
+(NSString*) getStoryboardName;
+(NSString*) deviceOSVersion;
+(NSString*) appVersion;
+(NSString*) friendlyDeviceName;
+(NSString*) searchUrl;
+(NSString*) getEnvironmentName:(bool)shortVersion;
+(NSString*) getPlistStringFromKey:(NSString*) key;
+(NSNumber*) getPlistNumberFromKey:(NSString*) key;
+(NSString *) gameGenres;

+(NSString*) getBaseUrlChangedNotification;

+(void) reportMaxMemoryUsedForPlaceID:(NSInteger)placeID;

+(void) setDefaultHTTPHeadersForRequest:(NSMutableURLRequest*)request;

@end

// Expose to C/C++
NSString* getBaseUrl();
NSString* getUserAgentString();
int getDeviceMemory();
bool hasMinMemory(int minMemory);