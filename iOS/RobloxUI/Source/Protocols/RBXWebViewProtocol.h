//
//  RBXWebViewProtocol.h
//  RobloxMobile
//
//  Created by Ashish Jain on 9/30/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol RBXWebViewProtocol <NSObject>

@property (nonatomic, strong) NSURLRequest *request;
@property (nonatomic, strong) NSURL *URL;

- (void) setDelegateController:(id)delegate;
- (void) loadRequest:(NSURLRequest *)request;
- (void) evaluateJavaScript:(NSString *)javaScriptString completionHandler:(void(^)(NSError *))completionHandler;

- (BOOL) canGoBackInHistory;
- (BOOL) canGoForwardInHistory;

- (void) goBackInHistory;
- (void) goForwardInHistory;

- (void) enableScrolling:(BOOL)enable;
- (void) enableMultipleTouch:(BOOL)enable;

@end


