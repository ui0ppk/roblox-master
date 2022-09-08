//
//  RBPurchaseViewContrlller.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBPurchaseViewContrlller.h"
#import "StoreManager.h"
#import "RobloxHUD.h"
#import "RobloxTheme.h"

@interface RBPurchaseViewContrlller ()
@property (nonatomic, strong) UIWebView* purchaseWebView;
@property (nonatomic, strong) UITapGestureRecognizer* gestureRecognizer;
@property (nonatomic, strong) NSURL* url;

- (void)didTapOutside:(UIGestureRecognizer*)sender;
- (void)didPressClose;
@end

@implementation RBPurchaseViewContrlller

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
    }
    return self;
}

- (id)initWithURL:(NSURL*)url andTitle:(NSString*)title {
    self = [super init];
    if (self) {
        self.url = url;
        self.title = title;
    }
    
    return self;
}

- (void)loadView {
    [super loadView];
    
    self.purchaseWebView = [[UIWebView alloc] init];
    self.purchaseWebView.delegate = self;
    self.purchaseWebView.frame = CGRectMake(self.view.frame.origin.x, self.view.frame.origin.y, self.view.frame.size.width, self.view.frame.size.height);
    self.purchaseWebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.purchaseWebView.scalesPageToFit = NO;
    self.purchaseWebView.multipleTouchEnabled = NO;
    self.purchaseWebView.scrollView.scrollEnabled = NO;
    self.purchaseWebView.scrollView.bounces = NO;
    
    [RobloxTheme applyToModalPopupNavBar:self.navigationController.navigationBar];
    [self.view addSubview:self.purchaseWebView];
    
    NSURLRequest* request = [NSURLRequest requestWithURL:self.url];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.purchaseWebView loadRequest:request];
    });
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    
    UIButton* close = [RobloxTheme applyCloseButtonToUINavigationItem:self.navigationItem];
    [close addTarget:self action:@selector(didPressClose) forControlEvents:UIControlEventTouchUpInside];
    
    self.gestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(didTapOutside:)];
    [self.gestureRecognizer setNumberOfTapsRequired:1];
    [self.gestureRecognizer setCancelsTouchesInView:NO];
    self.gestureRecognizer.delegate = self;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self.view.window addGestureRecognizer:self.gestureRecognizer];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self.view.window removeGestureRecognizer:self.gestureRecognizer];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void)webViewDidStartLoad:(UIWebView *)webView {
    [RobloxHUD showSpinnerForView:self.view withLabel:nil dimBackground:NO];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    [RobloxHUD hideSpinner:YES];
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error {
    [RobloxHUD hideSpinner:YES];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    id storeManager = GetStoreMgr;
    if ([storeManager isKindOfClass:[StoreManager class]]) {
        if([storeManager checkForInAppPurchases:request navigationType:navigationType]) {
            return NO;
        }
    }
    
    return YES;
}

//// so that we can touch inside the uiwebview
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

- (void)didPressClose {
    [self dismissViewControllerAnimated:YES completion:^{
    }];
}

- (void)didTapOutside:(UIGestureRecognizer*)sender {
    if (sender.state == UIGestureRecognizerStateEnded) {
        CGPoint location = [sender locationInView:nil];
        if (![self.navigationController.view pointInside:[self.navigationController.view convertPoint:location fromView:self.view.window] withEvent:nil]) {
            [self dismissViewControllerAnimated:YES completion:^{
            }];
        }
    }
}

@end
