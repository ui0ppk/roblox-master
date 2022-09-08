//
//  RBCaptchaViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 6/8/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBCaptchaViewController.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"
#import "RBXEventReporter.h"
#import "RBXFunctions.h"

@interface RBCaptchaViewController ()

@property (nonatomic, retain) UIWebView* webview;
@property (nonatomic, retain) NSString* captchaURLString;
@property (nonatomic, retain) NSString* captchaSolvedURLString;

@property (nonatomic) BOOL solved;

@end


@implementation RBCaptchaViewController
{
    void* _captchaContext;
}

+ (instancetype) CaptchaWithCompletionHandler:(AttemptedCompletionHandler)completionHandler
{
    RBCaptchaViewController *vc = [[RBCaptchaViewController alloc] init];
    
    vc.solved = NO;
    if (nil != completionHandler) {
        vc.attemptedCompletionHandler = completionHandler;
    }
    
    return vc;
}

// View Lifecycle functions
- (void) viewDidLoad
{
    [self shouldAddCloseButton:YES];
    [self shouldApplyModalTheme:YES];
    
    //disable the tap recognizer for versions higher than iOS7
    if (floor(NSFoundationVersionNumber) >= NSFoundationVersionNumber_iOS_7_1)
        [self disableTapRecognizer];
    
    [super viewDidLoad];
    
    self.view.clipsToBounds = YES;
    self.view.backgroundColor = [UIColor whiteColor];
    
    _captchaURLString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"mobile-captcha"];
    _captchaSolvedURLString = [_captchaURLString stringByAppendingString:@"-solved"];
    
    [self.navigationController.navigationItem setTitle:NSLocalizedString(@"CaptchaWord", nil)];
    [RobloxTheme applyToModalPopupNavBar:self.navigationController.navigationBar];
    
    
    _webview = [[UIWebView alloc] initWithFrame:self.view.frame];
    [_webview setDelegate:self];
    [_webview loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:_captchaURLString]]];
    [_webview setScalesPageToFit:NO];
    _webview.scrollView.scrollEnabled = NO;
    _webview.scrollView.pagingEnabled = NO;
    [self.view addSubview:_webview];
    
    [_webview.scrollView addObserver:self forKeyPath:@"contentSize" options:0 context:_captchaContext];
    
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextCaptcha];
}
- (void) viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    [_webview setSize:self.view.frame.size];
}
- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self becomeFirstResponder];
}
- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [self resignFirstResponder];
    [_webview stopLoading];
    [_webview.scrollView removeObserver:self forKeyPath:@"contentSize"];
    
    if (!self.solved)
    {
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose
                                                 withContext:RBXAContextCaptcha];
        if (nil != self.attemptedCompletionHandler) {
            self.attemptedCompletionHandler(NO, nil);
        }
    }
}
- (void) dealloc
{
    [_webview setDelegate:nil];
}



// Override functions
-(void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == _captchaContext)
    {
        //make it so that the view scrolls if and ONLY if it needs to. Scrolling looks dumb -Kyler
        _webview.scrollView.scrollEnabled = (_webview.scrollView.contentSize.height > self.view.frame.size.height);
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


// Delegate Functions
-(void) webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldCaptcha
                                                     withContext:RBXAContextCaptcha
                                                       withError:RBXAErrorMiscWebErrors];
    [self dismissViewControllerAnimated:NO completion:nil];
}
-(void) webViewDidFinishLoad:(UIWebView *)webView
{
    NSString* url = webView.request.URL.absoluteString;
    
    if ([url isEqualToString:_captchaSolvedURLString])
    {
        //The successful response event reporting is handled by the web
        if (nil != self.attemptedCompletionHandler) {
            self.attemptedCompletionHandler(YES, nil);
        }
        
        //mark the captcha as solved and go back to whatever we were doing
        self.solved = YES;
        [self dismissViewControllerAnimated:NO completion:nil];
    }
}


@end
