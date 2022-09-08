//
//  RBHybridWebViewProtocol.h
//  RobloxMobile
//
//  Created by Ashish Jain on 10/19/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

@class RBHybridCommand;

@protocol RBHybridWebViewProtocol <NSObject>

@property(weak, nonatomic) UIViewController *controller;
@property(nonatomic, readonly) NSString* webviewID;
@property(nonatomic, readonly) NSInteger lastRequestID;

- (void)executeJavascriptCommand:(NSString *)jsonCommand requestID:(NSInteger)requestId;
- (void)executeCallback:(NSString*)callbackID success:(BOOL)success withParams:(NSDictionary*)params;

@end