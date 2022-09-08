//
//  ResetPasswordViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "ResetPasswordViewController.h"
#import "RobloxInfo.h"
#import "UIStyleConverter.h"

#define PASSWORD_RESET_URL  @"Login/ResetPasswordRequest.aspx"

@interface ResetPasswordViewController ()

@end

@implementation ResetPasswordViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UIImage *loadingImage = [UIImage animatedImageNamed:@"loading-" duration:0.6f];
    UIButton *loadingButton = [UIButton buttonWithType:UIButtonTypeCustom];
    loadingButton.bounds = CGRectMake( 0, 0, 20.0, 20.0);
    [loadingButton setImage:loadingImage forState:UIControlStateNormal];
    
    [self.loadingBarItem setCustomView:loadingButton];
    
    NSString* baseURL = [RobloxInfo getBaseUrl];
    baseURL = [baseURL stringByAppendingString:PASSWORD_RESET_URL];
    forgotPwURL = [NSURL URLWithString:baseURL];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [UIStyleConverter convertToBlueNavigationBarStyle: self.navigationController.navigationBar];
    }
}

-(void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    [self.webView loadRequest:[NSURLRequest requestWithURL:forgotPwURL]];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

// UIWebview Delegate

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.loadingBarItem.customView setHidden:YES];
    });
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.loadingBarItem.customView setHidden:NO];
    });

    return YES;
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
