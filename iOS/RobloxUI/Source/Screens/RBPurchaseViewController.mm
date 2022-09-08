//
//  RBPurchaseViewController.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBPurchaseViewController.h"
#import "StoreManager.h"
#import "RobloxHUD.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"
#import "RBXEventReporter.h"
#import "UserInfo.h"

@interface RBPurchaseViewController ()
@property (nonatomic, strong) UIWebView* purchaseWebView;
@property (nonatomic, strong) NSURL* url;
@end

@implementation RBPurchaseViewController
{
    RBXAnalyticsContextName purchasingContext;
}


- (id)initWithURL:(NSURL*)url andTitle:(NSString*)title {
    self = [super init];
    if (self) {
        self.url = url;
        self.title = title;
    }
    
    return self;
}
- (id)initWithRobuxPurchasing
{
    purchasingContext = RBXAContextPurchasingRobux;
    NSString* url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"mobile-app-upgrades/native-ios/robux"];
    NSString* titleString = [NSString stringWithFormat:@"%@ : R$%@", NSLocalizedString(@"CurrentRobuxBalanceWord", nil), [UserInfo CurrentPlayer].Robux];
    
    self = [self initWithURL:[NSURL URLWithString:url] andTitle:titleString];
    return self;
}
- (id)initWithBCPurchasing
{
    purchasingContext = RBXAContextPurchasingBC;
    NSString* url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"mobile-app-upgrades/native-ios/bc"];
    //NSString* BCLevelKey = @"NBCWord";
    //[UserInfo CurrentPlayer].bcMember
    NSString* titleString = NSLocalizedString(@"Builders Club", nil); //[NSString stringWithFormat:@"%@ : %@", NSLocalizedString(@"CurrentBuildersClubWord", nil), NSLocalizedString(BCLevelKey, nil)];
    
    self = [self initWithURL:[NSURL URLWithString:url] andTitle:titleString];
    return self;
}



- (void)loadView {
    [super loadView];
    
    self.purchaseWebView = [[UIWebView alloc] init];
    self.purchaseWebView.delegate = self;
    self.purchaseWebView.frame = self.view.bounds;
    self.purchaseWebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.purchaseWebView.scalesPageToFit = YES;
    self.purchaseWebView.multipleTouchEnabled = NO;
    self.purchaseWebView.scrollView.scrollEnabled = YES;
    self.purchaseWebView.scrollView.bounces = NO;
    
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
    
    //pesky phone problem
    if (![RobloxInfo thisDeviceIsATablet])
        [self disableTapRecognizer];
}
- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:purchasingContext];
}


- (void)webViewDidStartLoad:(UIWebView *)webView {
    [RobloxHUD showSpinnerWithLabel:nil dimBackground:NO];
    
    if (![RobloxInfo thisDeviceIsATablet])
        webView.hidden = YES;
}

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    [RobloxHUD hideSpinner:YES];
    
    //scale the page to fit the view it is shoved into
    if (![RobloxInfo thisDeviceIsATablet])
    {
        CGSize contentSize = webView.scrollView.contentSize;
        CGSize viewSize = self.view.bounds.size;
        
        float rw = viewSize.width / contentSize.width;
        
        webView.scrollView.minimumZoomScale = rw;
        webView.scrollView.maximumZoomScale = rw;
        webView.scrollView.zoomScale = rw;
        webView.hidden = NO;
    }
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error {
    [RobloxHUD hideSpinner:YES];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    id storeManager = GetStoreMgr;
    if ([storeManager isKindOfClass:[StoreManager class]]) {
        if([storeManager checkForInAppPurchases:request navigationType:navigationType]) {
            NSString* url = [NSString stringWithFormat:@"%@", request.URL];
            NSArray* urlParts = [url componentsSeparatedByString:@"?id="];
            if (urlParts.count >= 1)
            {
                NSString* purchaseID = [[(NSString*)urlParts[1] componentsSeparatedByString:@"."] lastObject];
                [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSubmit withContext:purchasingContext withCustomDataString:purchaseID];
            }

            return NO;
        }
    }
    
    return YES;
}
@end
