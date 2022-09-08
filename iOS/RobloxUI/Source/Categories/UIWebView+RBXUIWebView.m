//
//  UIWebView+RBXUIWebView.m
//  RobloxMobile
//
//  Created by Ashish Jain on 9/30/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "UIWebView+RBXUIWebView.h"
#import "RBXFunctions.h"
#import "RBHybridBridge.h"

@implementation UIWebView (RBXUIWebView)

@dynamic URL;

- (void) setDelegateController:(id)delegate
{
    [self setDelegate:delegate];
}

- (NSURL *) URL
{
    return [[self request] URL];
}

- (void) evaluateJavaScript:(NSString *)javaScriptString completionHandler:(void (^)(id, NSError *))completionHandler
{
    NSString *js = [self stringByEvaluatingJavaScriptFromString:javaScriptString];
    
    if (completionHandler) {
        completionHandler(js, nil);
    }
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
