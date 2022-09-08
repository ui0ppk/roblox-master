//
//  ResetPasswordViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ResetPasswordViewController : UIViewController <UIWebViewDelegate>
{
    NSURL* forgotPwURL;
}

@property (retain, nonatomic) IBOutlet UIWebView *webView;
@property (retain, nonatomic) IBOutlet UIBarButtonItem *loadingBarItem;

@end
