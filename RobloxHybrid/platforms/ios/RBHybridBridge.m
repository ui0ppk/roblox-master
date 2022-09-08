//
//  RBHybridURLProtocol.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridBridge.h"
#import "RBHybridCommand.h"
#import "RBHybridModuleDialogs.h"
#import "RBHybridModuleSocial.h"
#import "RBHybridModuleGame.h"
#import "RBHybridModuleAnalytics.h"
#import "RBHybridWebView.h"
#import "RBXFunctions.h"
#import "SessionReporter.h"
#import "UserInfo.h"

#import "NSDictionary+Parsing.h"

@interface RBHybridURLProtocol : NSURLProtocol

@end

@implementation RBHybridURLProtocol

+ (BOOL)canInitWithRequest:(NSURLRequest*)request
{
    NSURL* url = [request URL];
    
    if( url.path != nil && [url.path rangeOfString:@"/rbx_native_exec"].location != NSNotFound )
    {
        NSString* webViewID = [request valueForHTTPHeaderField:@"webViewId"];
        RBHybridWebView* webView = [[RBHybridBridge sharedInstance] getWebViewForID:webViewID];
        if(!webView)
        {
            NSLog(@"ERROR: Unable to retrieve webview for ID %@", webViewID);
        }
        else
        {
            NSString* requestIDStr = [request valueForHTTPHeaderField:@"requestId"];
            NSInteger requestID = [requestIDStr integerValue];
            if(requestID > webView.lastRequestID)
            {
                NSString* jsonCommand = [request valueForHTTPHeaderField:@"command"];
                [webView executeJavascriptCommand:jsonCommand requestID:requestID];
            }
        }
        
        return YES;
    }
    
    return NO;
}

+ (NSURLRequest*)canonicalRequestForRequest:(NSURLRequest*)request
{
    return request;
}

- (void)startLoading
{
    // Send a null but successful response immediately
    NSHTTPURLResponse* response = [[NSHTTPURLResponse alloc]
                                   initWithURL:[[self request] URL]
                                   statusCode:200
                                   HTTPVersion:@"HTTP/1.1"
                                   headerFields:@{@"Content-Type":@"text/plain"}];
    [[self client] URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
    [[self client] URLProtocolDidFinishLoading:self];
}

- (void)stopLoading
{
    // Dummy implementation // Not necessary
}

@end

@implementation RBHybridBridge
{
    NSMutableDictionary* _modules;
    NSMutableArray* _webviews;
}

+ (RBHybridBridge*) sharedInstance
{
    static RBHybridBridge* instance;
    
    static dispatch_once_t queue;
    dispatch_once(&queue, ^
    {
        instance = [[RBHybridBridge alloc] init];
    });
    
    return instance;
}

-(instancetype)init
{
    self = [super init];
    if(self)
    {
        [NSURLProtocol registerClass:[RBHybridURLProtocol class]];
        
        _modules = [NSMutableDictionary dictionary];
        _webviews = [NSMutableArray array];
        
        [self initializeModules];
    }
    return self;
}

-(void)initializeModules
{
    [self registerModule:[[RBHybridModuleDialogs alloc] init]];
    [self registerModule:[[RBHybridModuleSocial alloc] init]];
    [self registerModule:[[RBHybridModuleGame alloc] init]];
    [self registerModule:[[RBHybridModuleAnalytics alloc] init]];
}

-(void) registerModule:(RBHybridModule*)module
{
    [_modules setObject:module forKey:module.moduleID];
}

-(void) unRegisterModule:(RBHybridModule*)module
{
    [_modules removeObjectForKey:module.moduleID];
}

-(void)registerWebView:(UIView <RBHybridWebViewProtocol> *)webview
{
    // 2015.1023 ajain - valueWithNonretainedObject adds an object "value reference"
    // without incrementing the retain count of the object, thereby allowing it to be
    // unregistered when the webview's retain count hits 0.
    if (![_webviews containsObject:[NSValue valueWithNonretainedObject:webview]]) {
        [_webviews addObject:[NSValue valueWithNonretainedObject:webview]];
    }
}

-(void)unRegisterWebView:(UIView *)webview
{
    if ([_webviews containsObject:[NSValue valueWithNonretainedObject:webview]]) {
        [_webviews removeObject:[NSValue valueWithNonretainedObject:webview]];
    }
}

-(UIView <RBHybridWebViewProtocol> *)getWebViewForID:(NSString*)webviewID
{
    for(NSValue* webviewPtr in _webviews)
    {
        UIView <RBHybridWebViewProtocol> * webview = [webviewPtr nonretainedObjectValue];
        if (!webview)
        {
            NSLog(@"Webview no longer exists");
        }
        else if(webview.webviewID != nil && [webview.webviewID isEqualToString:webviewID])
        {
            return webview;
        }
    }
   return nil;
}

-(RBHybridModule*)getModuleForID:(NSString*)moduleID
{
    return [_modules objectForKey:moduleID];
}

- (void)dealloc
{
    [NSURLProtocol unregisterClass:[RBHybridURLProtocol class]];
}

@end
