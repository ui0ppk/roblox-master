//
//  RobloxGoogleAnalytics.h
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 11/13/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RobloxGoogleAnalytics : NSObject
{}

+(void) startup;
+(void) setPageViewTracking:(NSString*)url;
+(void) setEventTracking:(NSString*)category withAction:(NSString*)action withValue:(NSInteger)value;
+(void) setEventTracking:(NSString*)category withAction:(NSString*)action withLabel:(NSString*)label withValue:(NSInteger)value;
+(void) setCustomVariableWithLabel:(NSString*)label withValue:(NSString*)value;

// debug counters
+(void) debugCountersPrint;
+(void) debugCounterIncrement:(NSString *)label;

@end
