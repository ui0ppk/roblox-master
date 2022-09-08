//
//  RBHybridCommand.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RBHybridWebView.h"
#import "RBHybridWebViewProtocol.h"

@interface RBHybridCommand : NSObject

@property(nonatomic) NSInteger requestID;
@property(strong, nonatomic) NSString* moduleID;
@property(strong, nonatomic) NSString* functionName;
@property(strong, nonatomic) NSDictionary* params;
@property(strong, nonatomic) NSString* callbackID;

@property(weak, nonatomic) UIView <RBHybridWebViewProtocol> *originWebView;

-(void)executeCallback:(BOOL)success;
-(void)executeCallback:(BOOL)success withParams:(NSDictionary*)params;

@end
