//
//  RBResetPasswordControllerViewController.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/26/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBResetPasswordViewController.h"
#import "RobloxInfo.h"

@interface RBResetPasswordViewController ()

@end

@implementation RBResetPasswordViewController

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch {
    return NO;
}

@end
