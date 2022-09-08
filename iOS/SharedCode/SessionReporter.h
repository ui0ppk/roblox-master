//
//  SessionReporter.h
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 7/12/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "RBXEventReporter.h"

@interface SessionReporter : NSObject

typedef enum
{	APPLICATION_ACTIVE,
    APPLICATION_BACKGROUND,
    GAME_RUNNING,
    ENTER_GAME,
    EXIT_GAME,
    OUT_MEMORY_ON_LOAD,
    OUT_MEMORY_IN_GAME,
    APPLICATION_FRESH_START
}	ApplicationState;


+(id) sharedInstance;
-(void) reportSessionForContext:(RBXAnalyticsContextName)context result:(RBXAnalyticsResult)result errorName:(RBXAnalyticsErrorName)errorName responseCode:(NSInteger)responseCode data:(NSDictionary *)dataDictionary;
-(void) reportSessionFor:(ApplicationState) appState PlaceId:(NSInteger) placeId;
-(void) reportSessionFor:(ApplicationState) appState;
-(void) postAnalyticPayloadFromContext:(RBXAnalyticsContextName)context
                                result:(RBXAnalyticsResult)result
                              rbxError:(RBXAnalyticsErrorName)rbxError
                          startingTime:(NSDate*)startTime
                     attemptedUsername:(NSString*)username
                            URLRequest:(NSURLRequest*)request
                      HTTPResponseCode:(NSInteger)responseCode
                          responseData:(NSData*)responseData
                        additionalData:(NSDictionary*)additionalData;
-(void) postStartupPayloadForEvent:(NSString*)requestName
                    completionTime:(NSTimeInterval)timeMS;
-(void) postAutoLoginFailurePayload:(NSTimeInterval)loginTimestamp
                   cookieExpiration:(NSTimeInterval)actualExpirationTimestamp
                 expectedExpiration:(NSTimeInterval)expectedExpirationTimestamp;
@end
