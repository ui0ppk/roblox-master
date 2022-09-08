//
//  WKWebView+RBXWKWebView.m
//  RobloxMobile
//
//  Created by Ashish Jain on 9/30/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "WKWebView+RBXWKWebView.h"

#import "LoginManager.h"
#import "RBXFunctions.h"
#import "RBHybridBridge.h"
#import "RobloxInfo.h"

#import <objc/runtime.h>

@implementation WKWebView (RBXWKWebView)

- (void) setDelegateController:(id)delegate
{
    [self setNavigationDelegate:delegate];
    [self setUIDelegate:delegate];
}

- (NSURLRequest *) request
{
    return objc_getAssociatedObject(self, @selector(request));
}

- (void) setRequest:(NSURLRequest *)request
{
    objc_setAssociatedObject(self, @selector(request), request, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

+ (void) load
{
    static dispatch_once_t onceToken;
    
    // We want to make sure this is only done once!
    dispatch_once(&onceToken, ^{
        Class class = [self class];
        
        // Get the representation of the method names to swizzle.
        SEL originalSelector = @selector(loadRequest:);
        SEL swizzledSelector = @selector(altLoadRequest:);
        
        // Get references to the methods to swizzle.
        Method originalMethod = class_getInstanceMethod(class, originalSelector);
        Method swizzledMethod = class_getInstanceMethod(class, swizzledSelector);
        
        // Attempt to add the new method in place of the old method.
        BOOL didAddMethod = class_addMethod(class, originalSelector, method_getImplementation(swizzledMethod), method_getTypeEncoding(swizzledMethod));
        
        // If we succeeded, put the old method in place of the new method.
        if (didAddMethod) {
            class_replaceMethod(class, swizzledSelector, method_getImplementation(originalMethod), method_getTypeEncoding(originalMethod));
        } else {
            // Otherwise, just swap their implementations.
            method_exchangeImplementations(originalMethod, swizzledMethod);
        }
    });
}

// 2015.1026 ajain - This requires a little mental juggling.  We do a swizzle (replace implementations) of
// the loadRequest method with altLoadRequest.  That way, every time another class calls loadRequest on a WKWebView object, it'll
// actually execute the altLoadRequest method.

// Example: Outside class calls [[WKWebView instance] loadRequest] --> executes
// altLoadRequest implementation (custom code below) --> calls
// [self altLoadRequest:mutableRequest] --> executes
// original loadRequest implementation (apple wkwebview code).

// The method swizzling is why calling altLoadRequest is NOT a recursive method call.

- (void) altLoadRequest:(NSURLRequest *)request
{
    NSArray *cookies = [LoginManager cookies];
    
    NSDictionary *cookiesDict = [NSHTTPCookie requestHeaderFieldsWithCookies:cookies];
    
    NSMutableDictionary *requestHeaders = [NSMutableDictionary dictionaryWithDictionary:request.allHTTPHeaderFields];
    [requestHeaders addEntriesFromDictionary:cookiesDict];
    
    NSMutableURLRequest *mutableRequest = request.mutableCopy;
    [mutableRequest setAllHTTPHeaderFields:requestHeaders];
    
    [self setRequest:mutableRequest];
    
    // 2015.1026 ajain - Since we swizzled with loadRequest, this will actually call the original loadRequest.
    [self altLoadRequest:mutableRequest];
}

- (BOOL) canGoBackInHistory
{
    BOOL val = self.canGoBack;
    return val;
}

- (BOOL) canGoForwardInHistory
{
    BOOL val = self.canGoForward;
    return val;
}

- (void) goBackInHistory
{
    if ([self canGoBack]) {
        [self goBack];
    }
}

- (void) goForwardInHistory
{
    if ([self canGoForward]) {
        [self goForward];
    }
}

- (void) enableScrolling:(BOOL)enable
{
    self.scrollView.scrollEnabled = enable;
}

- (void) enableMultipleTouch:(BOOL)enable
{
    self.multipleTouchEnabled = enable;
}

@end
