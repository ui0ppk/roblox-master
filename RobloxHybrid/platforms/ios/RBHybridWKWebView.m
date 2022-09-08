//
//  RBHybridWKWebView.m
//  RobloxMobile
//
//  Created by Ashish Jain on 10/16/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import "RBHybridWKWebView.h"

#import "NSDictionary+Parsing.h"
#import "RBHybridBridge.h"
#import "RBHybridCommand.h"
#import "RBHybridModule.h"
#import "SessionReporter.h"
#import "UserInfo.h"

#import "RobloxInfo.h"
#import <objc/runtime.h>

#define JS_HYBRID_OBJ @"window.Roblox.Hybrid"

@implementation RBHybridWKWebView

// Implemented for RBHybridWebViewProtocol

@synthesize webviewID;
@synthesize controller;
@synthesize lastRequestID;

- (instancetype)init
{
    [RBHybridWKWebView initializeUserAgent];
    
    self = [super init];
    if (self)
    {
        [self initializeWebView];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    [RBHybridWKWebView initializeUserAgent];
    
    self = [super initWithCoder:coder];
    if (self)
    {
        [self initializeWebView];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    [RBHybridWKWebView initializeUserAgent];
    
    self = [super initWithFrame:frame];
    if (self)
    {
        [self initializeWebView];
    }
    return self;
}

+(void)initializeUserAgent
{
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObject:[RobloxInfo getUserAgentString] forKey:@"UserAgent"]];
}

-(void)initializeWebView
{
    [[RBHybridBridge sharedInstance] registerWebView:self];
    self.scrollView.decelerationRate = UIScrollViewDecelerationRateNormal;
}

- (void) executeJavascriptCommand:(NSString *)jsCommand requestID:(NSInteger)requestId
{
    /*
     2015.1116 ajain - This code is intentionally the same as the code in RBHybridWebView.  I wrote this as a centralized method, 
     but as a result of a code review it was determined that duplicating code would be better since in a few months we will stop
     supporting iOS 7 and UIWebView's which will allows for alI wrote this as a centralized method,
     but as a result of a code review it was determined that duplicating code would be better since in a few months we will stopl the RBHybridWebView code to be removed.
     */
    NSError* parseError = nil;
    NSDictionary* params = [NSJSONSerialization JSONObjectWithData:[jsCommand dataUsingEncoding:NSUTF8StringEncoding] options:NSJSONReadingMutableContainers error:&parseError];
    if(parseError == nil)
    {
        RBHybridCommand* command = [[RBHybridCommand alloc] init];
        command.requestID = requestId;
        command.moduleID = [params stringForKey:@"moduleID"];
        command.functionName = [params stringForKey:@"functionName"];
        command.params = [params dictionaryForKey:@"params"];
        command.callbackID = [params stringForKey:@"callbackID"];
        
        [self executeCommand:command];
    }
    else
    {
        // 2015.1116 ajain - Something is wrong with the javascript command.  Report back.
        [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextHybrid
                                                                  result:RBXAResultFailure
                                                                rbxError:RBXAErrorJSONParseFailure
                                                            startingTime:[NSDate date]
                                                       attemptedUsername:[UserInfo CurrentPlayer].username
                                                              URLRequest:nil
                                                        HTTPResponseCode:201
                                                            responseData:[jsCommand dataUsingEncoding:NSUTF8StringEncoding]
                                                          additionalData:@{@"JSCommandParseErrorDescWK":parseError.description}];
    }
}

-(void)executeCommand:(RBHybridCommand*)command
{
    command.originWebView = self;
    
    if(command.requestID > self.lastRequestID)
    {
        lastRequestID = command.requestID;
    }
    
    RBHybridModule* module = [[RBHybridBridge sharedInstance] getModuleForID:command.moduleID];
    if(module != nil)
    {
        [module executeCommand:command];
    }
}

-(void)executeCallback:(NSString*)callbackID success:(BOOL)success withParams:(NSDictionary*)params
{
    NSError* error;
    NSString* paramsString = @"{}";
    if(params != nil)
    {
        NSData* paramsData = [NSJSONSerialization dataWithJSONObject:params options:NSJSONWritingPrettyPrinted error:&error];
        paramsString = [[NSString alloc] initWithData:paramsData encoding:NSUTF8StringEncoding];
    }
    
    NSString* callbackCommand =
    [NSString stringWithFormat:@"%@.Bridge.nativeCallback('%@', %@, %@);",
     JS_HYBRID_OBJ,
     callbackID,
     success ? @"true" : @"false",
     paramsString];
    
    [self evaluateJavaScript:callbackCommand completionHandler:nil];
}

- (NSString*) webviewID
{
    /*
     The new implementation of javascript callbacks for WebKit makes this property unnecessary.
     This method is here simply to serve as a placeholder for purposes of paralleling the RBHybridWebView implementation.
    */
    return nil;
}

- (void)dealloc
{
    NSLog(@"RBHybridWKWebView dealloc");
    [[RBHybridBridge sharedInstance] unRegisterWebView:self];
}

@end
