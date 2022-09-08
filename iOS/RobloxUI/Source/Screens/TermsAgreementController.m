//
//  TermsAgreementController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/22/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "TermsAgreementController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"

@interface TermsAgreementController ()

@end

@implementation TermsAgreementController
{
    IBOutlet UIWebView *_webView;
    IBOutlet UINavigationBar *_navBar;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [RobloxTheme applyToModalPopupNavBar:_navBar];
    
    NSURL *url = [NSURL URLWithString:self.url];
    NSURLRequest *requestObj = [NSURLRequest requestWithURL:url];
    [_webView loadRequest:requestObj];
    [_webView setScalesPageToFit:YES];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [_webView stopLoading];
}


- (IBAction)closeButtonTouched:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}
@end
