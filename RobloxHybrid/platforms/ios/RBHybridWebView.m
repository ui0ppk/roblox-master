//
//  RBHybridWebView.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/18/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "NSDictionary+Parsing.h"
#import "RBHybridWebView.h"
#import "RBHybridBridge.h"
#import "RBHybridCommand.h"
#import "RBHybridModule.h"
#import "SessionReporter.h"
#import "UserInfo.h"

#import "RobloxInfo.h"

#import <objc/runtime.h>

#define JS_HYBRID_OBJ @"window.Roblox.Hybrid"

@interface RBHybridWebView ()

@property(nonatomic, readwrite) NSInteger lastRequestID;

@end

@implementation RBHybridWebView

// This is from RBHybridWebViewProtocol
@synthesize controller;
@synthesize webviewID;
@synthesize lastRequestID;

- (instancetype)init
{
    [RBHybridWebView initializeUserAgent];
    
    self = [super init];
    if (self)
    {
        [self initializeWebView];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    [RBHybridWebView initializeUserAgent];
    
    self = [super initWithCoder:coder];
    if (self)
    {
        [self initializeWebView];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    [RBHybridWebView initializeUserAgent];
    
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

- (void) executeJavascriptCommand:(NSString *)jsonCommand requestID:(NSInteger)requestId
{
    /*
     2015.1116 ajain - This code is intentionally the same as the code in RBHybridWKWebView.  I wrote this as a centralized method,
     but as a result of a code review it was determined that duplicating code would be better since in a few months we will stop
     supporting iOS 7 and UIWebView's which will allow for all this code to be removed.
     */
    NSError* parseError = nil;
    NSDictionary* params = [NSJSONSerialization JSONObjectWithData:[jsonCommand dataUsingEncoding:NSUTF8StringEncoding] options:NSJSONReadingMutableContainers error:&parseError];
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
                                                        HTTPResponseCode:202
                                                            responseData:[jsonCommand dataUsingEncoding:NSUTF8StringEncoding]
                                                          additionalData:@{@"JSCommandParseErrorDescUI":parseError.description}];
    }
}

-(void)executeCommand:(RBHybridCommand*)command
{
    command.originWebView = self;
    
    if(command.requestID > self.lastRequestID)
    {
        self.lastRequestID = command.requestID;
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
    
    [self stringByEvaluatingJavaScriptFromString:callbackCommand];
}

- (NSString *) webviewID
{
    NSString *getIDCommand = [NSString stringWithFormat:@"%@.Bridge.getWebViewID()", JS_HYBRID_OBJ];
    NSString *newID = [self stringByEvaluatingJavaScriptFromString:getIDCommand];    
    return newID;
}

- (void)dealloc
{
    NSLog(@"RBHybridWebView dealloc");
    [[RBHybridBridge sharedInstance] unRegisterWebView:self];
}

@end
