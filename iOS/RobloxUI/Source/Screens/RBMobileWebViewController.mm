//
//  RBMobileWebViewController.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBMobileWebViewController.h"

#import "Flurry.h"
#import "MBProgressHUD.h"
#import "NativeSearchNavItem.h"
#import "NSDictionary+Parsing.h"
#import "RBGameViewController.h"
#import "RBHybridBridge.h"
#import "RBHybridWebView.h"
#import "RBHybridWKWebView.h"
#import "RBXFunctions.h"
#import "RobloxAlert.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxHUD.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "RobloxTheme.h"
#import "StoreManager.h"
#import "UIViewController+Helpers.h"
#import "UIView+Position.h"

DYNAMIC_FASTFLAGVARIABLE(EnableWebKit, false);

@interface RBMobileWebViewController () <UIWebViewDelegate, UIAlertViewDelegate, WKUIDelegate, WKNavigationDelegate, WKScriptMessageHandler>

@property (nonatomic, strong) MBProgressHUD *spinner;
@property (nonatomic, strong) UISearchBar *searchBar;
@property (nonatomic, strong) UIButton *backButton;
@property (nonatomic, strong) UIButton *fwdButton;
@property (nonatomic) BOOL showWholeScreen;

@property (nonatomic, strong) NSString *flurryGameLaunchEvent;
@property (nonatomic, strong) NSString *flurryPageEvent;

@end

@implementation RBMobileWebViewController

- (void) initDefaults
{
    if (self)
    {
        self.showNavButtons = YES;
        self.isPopover = NO;
    }
}
- (id) init
{
    self = [super init];
    [self initDefaults];
    
    return self;
}

- (id) initWithNavButtons:(BOOL)showButtons
{
    self = [self init];
    if (self)
    {
        self.showNavButtons = showButtons;
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    [self initDefaults];
    
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    [self initDefaults];
    
    return self;
}

- (void) initializeViews
{
    if ([RBXFunctions isEmpty:self.rbxWebView])
    {
        if ([self isWebkitEnabled] && NSClassFromString(@"WKWebView")) {
            WKWebViewConfiguration *webviewConfig = [[WKWebViewConfiguration alloc]init];
            WKUserContentController *userController = [[WKUserContentController alloc]init];
            
            /*
             The name of the script message handler "RobloxWKHybrid" is very intentional.  It must match the value in the file
             RobloxHybrid\src\common\bridge.ios.js on the line "window.webkit.messageHandlers.RobloxWKHybrid.postMessage"
            */
            [userController addScriptMessageHandler:self name:@"RobloxWKHybrid"];
            webviewConfig.userContentController = userController;

            self.rbxWebView = [[RBHybridWKWebView alloc] initWithFrame:CGRectZero configuration:webviewConfig];
            
        } else {
            self.rbxWebView = [[RBHybridWebView alloc] init];
        }
        
        [self.view addSubview:self.rbxWebView];
        self.rbxWebView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        self.rbxWebView.multipleTouchEnabled = NO;
        self.rbxWebView.contentMode = UIViewContentModeScaleAspectFit;
        self.rbxWebView.controller = self;
        [self.rbxWebView setDelegateController:self];
        [[RBHybridBridge sharedInstance] registerWebView:self.rbxWebView];
    }
    
    if ([RBXFunctions isEmpty:self.spinner])
    {
        self.spinner = [[MBProgressHUD alloc] initWithView:self.rbxWebView];
        self.spinner.clipsToBounds = NO;
        self.spinner.removeFromSuperViewOnHide = NO;
        self.spinner.dimBackground = NO;
    }
    
    if ([RBXFunctions isEmpty:self.backButton])
    {
        self.backButton = [[UIButton alloc] initWithFrame:CGRectMake(0.0, 0.0, 12.0, 21.0)];
        [self.backButton addTarget:self action:@selector(didPressBack) forControlEvents:UIControlEventTouchUpInside];
        [self.backButton setImage:[UIImage imageNamed:@"Left Arrow Normal"] forState:UIControlStateNormal];
    }
    
    if ([RBXFunctions isEmpty:self.fwdButton])
    {
        self.fwdButton = [[UIButton alloc] initWithFrame:CGRectMake(0.0, 0.0, 12.0, 21.0)];
        [self.fwdButton addTarget:self action:@selector(didPressForward) forControlEvents:UIControlEventTouchUpInside];
        [self.fwdButton setImage:[UIImage imageNamed:@"Right Arrow Normal"] forState:UIControlStateNormal];
    }
    
    self.showWholeScreen = NO;
}

- (void)didPressBack
{
    if (self.rbxWebView.canGoBackInHistory)
    {
        [self.rbxWebView goBackInHistory];
    }
    else
    {
        BOOL isPushedController = self.navigationController.viewControllers.count > 1;
        if (isPushedController)
            [self.navigationController popViewControllerAnimated:YES];
    }
}

- (void)didPressForward
{
    if (self.rbxWebView.canGoForwardInHistory) {
        [self.rbxWebView goForwardInHistory];
    }
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    [self.spinner centerInFrame:self.rbxWebView.frame];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self initializeViews];
    
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    [RobloxTheme applyToGamesNavBar:self.navigationController.navigationBar];
    
    [self.rbxWebView setFrame:self.view.bounds];

    [self.spinner setFrame:self.rbxWebView.frame];
    [self.rbxWebView addSubview:self.spinner];
    [self.spinner hide:NO];
    
    if(self.showNavButtons)
    {
        UIBarButtonItem* space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFixedSpace target:nil action:nil];
        [space setWidth:44.0];
        
        self.navigationItem.leftBarButtonItems = @[[[UIBarButtonItem alloc] initWithCustomView:self.backButton],
                                                   space,
                                                   [[UIBarButtonItem alloc] initWithCustomView:self.fwdButton]];
    }
    else if(self.isPopover)
    {
        UIImage* closeImage = [UIImage imageNamed:@"Close Button"];
        UIButton* closeButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, closeImage.size.width, closeImage.size.height)];
        [closeButton addTarget:self action:@selector(didPressClose) forControlEvents:UIControlEventTouchUpInside];
        [closeButton setImage:closeImage forState:UIControlStateNormal];
        
        self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:closeButton];
    }
    
    [self reloadWebPage];
}

- (void)didPressClose
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    // 2015.0928 ajain - This is a fail-safe so that just in case the webview is deallocated in the viewWillDisappear method
    // on its way to displaying the in-game view, then we re-create the webview.
    if ([RBXFunctions isEmpty:self.rbxWebView]) {
        [self initializeViews];
    }
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    [self updateNavButtonsState];
}

- (void) dealloc
{
    NSLog(@"RBMobileWebViewController dealloc");
}

- (void) setUrl:(NSString *)url
{
    _url = url;
    
    [self loadURL:self.url];
}
- (void) loadURL:(NSString*)url
{
    [self loadURL:url screenURL:YES];
}

- (void) loadURL:(NSString *)url screenURL:(bool)shouldFilter
{
    //a one-off function for exposing access to the webview
    if (![RBXFunctions isEmptyString:url] && ![RBXFunctions isEmpty:self.rbxWebView])
    {
        NSURL* urlFromString = [NSURL URLWithString:url];
        NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:urlFromString];
        [RobloxInfo setDefaultHTTPHeadersForRequest:request];
        
        if (shouldFilter)
        {
            //screen all urls that the webview has been told to load
            RBXWebRequestReturnStatus requestStatus = [self handleWebRequest:urlFromString];
            switch (requestStatus)
            {
                case RBXWebRequestReturnFilter:     {   return;     } break;
                case RBXWebRequestReturnScreenPush: {   return;     } break;
                case RBXWebRequestReturnUnknown:    {   return;     } break;
                case RBXWebRequestReturnWebRequest: {   [self.rbxWebView loadRequest:request]; }    break;
            }
        }
        else
        {
            [self.rbxWebView loadRequest:request];
        }
    }
}
- (void) reloadWebPage
{
    [self loadURL:self.url];
}

//Accessors
- (bool) isWebkitEnabled
{
    return DFFlag::EnableWebKit;
}

//Mutators
- (void) setFlurryGameLaunchEvent:(NSString*)gameLaunchEvent
{
    _flurryGameLaunchEvent = gameLaunchEvent;
}
- (void) setFlurryPageLoadEvent:(NSString*)pageLaunchEvent
{
    _flurryPageEvent = pageLaunchEvent;
}

#pragma mark - Common UIWebView and WKWebView methods

- (void) setToShowWholeScreen:(BOOL)showAll
{
    _showWholeScreen = showAll;
    [self.rbxWebView enableScrolling:YES];
    [self.rbxWebView enableMultipleTouch:YES];
}

- (BOOL) shouldStartNavigationActionWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    [self updateNavButtonsState];
    
    id storeManager = GetStoreMgr;
    if ([storeManager isKindOfClass:[StoreManager class]])
    {
        if([storeManager checkForInAppPurchases:request navigationType:navigationType])
        {
            return NO;
        }
    }
    
    if([self checkForGameLaunch:request])
        return NO;
    
    //Screen all clicked links to ensure that they are safe to be displayed
    if (navigationType == UIWebViewNavigationTypeLinkClicked)
    {
        RBXWebRequestReturnStatus requestStatus = [self handleWebRequest:request.URL];
        switch (requestStatus)
        {
            case (RBXWebRequestReturnWebRequest):
            {
                //looks like a regular Roblox URL
                return YES;
            } break;
            case (RBXWebRequestReturnScreenPush):
            {
                //the url has been handled by another view
                return NO;
            } break;
            case (RBXWebRequestReturnUnknown):
            {
                //not quite sure what domain we've hit here
                [[UIApplication sharedApplication] openURL:request.URL];
                return NO;
            } break;
            case (RBXWebRequestReturnFilter):
            {
                return NO;
            }break;
        }
    }
    return YES;
}

//Game Launcher Stuff
-(BOOL) checkForGameLaunch:(NSURLRequest *)request
{
    NSURL *url = request.URL;
    NSString *requestUrlString = url.absoluteString;
    
    // check that we are trying to launch a game
    if (    [requestUrlString rangeOfString:@"/games/start?"].location           == NSNotFound
        &&  [requestUrlString rangeOfString:@"robloxmobile://placeID="].location == NSNotFound)
        return NO;
    
    NSArray* components = [requestUrlString componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"=&?/"]];
    NSString* placeId;
    RBXGameLaunchParams* params;
    
    // Parse out our args and launch a game if we find either placeid or userID
    for (int i = 0; i < components.count; i++)
    {
        NSString* component = [components objectAtIndex:i];
        component = [component lowercaseString];
        
        if ([component isEqualToString:@"placeid"])
        {
            placeId = [components objectAtIndex:(i+1)];
            if (params)
            {
                //if params is already initialized, it means that for some reason,
                // the placeid component was not the first parameter parsed.
                //So, simply update the placeholder targetId with the real one.
                params.targetId = placeId.integerValue;
            }
            else
            {
                //if params is not initialized, create a default for joining a place
                params = [RBXGameLaunchParams InitParamsForJoinPlace:placeId.integerValue];
            }
            continue;
        }
        else if ([component isEqualToString:@"userid"])
        {
            //if there is a userid parameter, then there won't be other parameters. Grab it and escape
            NSString* userId = [components objectAtIndex:(i+1)];
            params = [RBXGameLaunchParams InitParamsForFollowUser:userId.integerValue];
            break;
        }
        else if ([component isEqualToString:@"gameinstanceid"])
        {
            NSString* gameInstanceId = [components objectAtIndex:(i+1)];
            if (placeId)
            {
                //if the placeId value has already been parsed, we have all the necessary parameters
                // to initialize our Launch Parameters
                params = [RBXGameLaunchParams InitParamsForJoinGameInstance:placeId.integerValue withInstanceID:gameInstanceId];
                break;
            }
            else
            {
                //if placeId has not yet been parsed for some reason, create the launch parameters
                // with a placeholder value. We will parse out the proper placeId value later.
                params = [RBXGameLaunchParams InitParamsForJoinGameInstance:-1 withInstanceID:gameInstanceId];
                continue;
            }
        }
        else if ([component isEqualToString:@"accesscode"])
        {
            NSString* accessCode = [components objectAtIndex:(i+1)];
            if (placeId)
            {
                //if the placeId value has already been parsed, we have all the necessary parameters
                // to initialize our Launch Parameters
                params = [RBXGameLaunchParams InitParamsForJoinPrivateServer:placeId.integerValue withAccessCode:accessCode];
                break;
            }
            else
            {
                //if placeId has not yet been parsed for some reason, create the launch parameters
                // with a placeholder value. We will parse out the proper placeId value later.
                params = [RBXGameLaunchParams InitParamsForJoinPrivateServer:-1 withAccessCode:accessCode];
                continue;
            }
        }
    }
    
    //check if we have valid args to start a game, if not, exit
    if (params)
    {
        RBMobileWebViewController __weak *weakSelf = self;
        
        if (params.joinRequestType == JOIN_GAME_REQUEST_USERID)
        {
            //we need the placeID to check if we can play a game, use the userID to fetch the user presence to fetch the gameID
            [RobloxData fetchPresenceForUser:[NSNumber numberWithInt:params.targetId] withCompletion:^(RBXUserPresence* presence)
             {
                 //[self checkPlayable:presence.gameID.integerValue withLaunchParameters:params];
                 [RBXFunctions dispatchOnMainThread:^{
                     RBGameViewController* controller = [[RBGameViewController alloc] initWithLaunchParams:params];
                     [weakSelf presentViewController:controller animated:NO completion:nil];
                 }];
             }];
        }
        else
        {
            //[self checkPlayable:placeId.integerValue withLaunchParameters:params];
            [RBXFunctions dispatchOnMainThread:^{
                RBGameViewController* controller = [[RBGameViewController alloc] initWithLaunchParams:params];
                [weakSelf presentViewController:controller animated:NO completion:nil];
            }];
        }
        
        return YES;
    }
    
    return NO;
}

-(void) checkPlayable:(int)placeId withLaunchParameters:(RBXGameLaunchParams*)launchParams
{
    //check if the game is playable
    NSString* isPlayableString = [NSString stringWithFormat:@"%@/game/is-playable?placeId=%i", [RobloxInfo getApiBaseUrl], placeId];
    
    NSURL *playableURL = [NSURL URLWithString: isPlayableString];
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:playableURL
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60*7];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"GET"];
    
    RBMobileWebViewController __weak *weakSelf = self;
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *data, NSError *error)
     {
         [weakSelf processPlayableData:response playData:data playError:error withLaunchParams:launchParams];
     }];
}
-(void) processPlayableData:(NSURLResponse*)response playData:(NSData*)data playError:(NSError*)error withLaunchParams:(RBXGameLaunchParams*)parameters
{
    [RBXFunctions dispatchOnMainThread:^{
        NSHTTPURLResponse* urlResponse = ( NSHTTPURLResponse*) response;
        if ([urlResponse statusCode] == 200) // successful response
        {
            
            NSDictionary* dict = [[NSDictionary alloc] init];
            NSError* error = nil;
            dict = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&error];
            if (!error) // No Error processsing json data from our servers
            {
                NSNumber * n = [dict valueForKey:@"isPlayable"];
                BOOL success = [n boolValue];
                
                if (success)
                {
                    RBGameViewController* controller = [[RBGameViewController alloc] initWithLaunchParams:parameters];
                    [self presentViewController:controller animated:NO completion:nil];
                    if (self.flurryGameLaunchEvent)
                        [Flurry logEvent:self.flurryGameLaunchEvent];
                    return;
                }
                else
                {
                    [RobloxHUD showMessage:NSLocalizedString(@"GameNotPlayable", nil)];
                }
            }
        }
        
        [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"ConnectionErrorGameRestricted", nil)];
    }];
    
}

- (void) updateNavButtonsState
{
    BOOL isPushedController = self.navigationController.viewControllers.count > 1;
    self.backButton.enabled = self.rbxWebView.canGoBackInHistory || isPushedController;
    self.fwdButton.enabled = self.rbxWebView.canGoForwardInHistory;
}

#pragma mark - WKWebView Delegate Methods
- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    if ([message.body isKindOfClass:[NSDictionary class]]) {
        NSDictionary *messageBody = (NSDictionary *)message.body;
        
        NSString *jsCommand = [messageBody stringForKey:@"command"];
        NSNumber *requestId = [messageBody numberForKey:@"requestId"];
        
        [self.rbxWebView executeJavascriptCommand:jsCommand requestID:requestId.integerValue];
    }
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)())completionHandler
{
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:message
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:[UIAlertAction actionWithTitle:@"OK"
                                                        style:UIAlertActionStyleCancel
                                                      handler:^(UIAlertAction *action) {
                                                          completionHandler();
                                                      }]];
    
    [self presentViewController:alertController animated:YES completion:nil];
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationResponse:(WKNavigationResponse *)navigationResponse decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler
{
    if (nil != decisionHandler) {
        decisionHandler(WKNavigationResponsePolicyAllow);
    }
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    BOOL allowNavigationAction = [self shouldStartNavigationActionWithRequest:navigationAction.request navigationType:navigationAction.navigationType];
    if (nil != decisionHandler) {
        if (allowNavigationAction) {
            decisionHandler(WKNavigationActionPolicyAllow);
        } else {
            decisionHandler(WKNavigationActionPolicyCancel);
        }
    }
}

- (BOOL) shouldStartDecidePolicy: (NSURLRequest *) request
{
    return YES;
}

- (void) webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
{
    [RBXFunctions dispatchOnMainThread:^{
        [self updateNavButtonsState];
        [self.spinner show:YES];
    }];
    
    if (self.flurryPageEvent)
        [Flurry logEvent:self.flurryPageEvent];
}

- (void) webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    [RBXFunctions dispatchOnMainThread:^{
        [self updateNavButtonsState];
        [self.spinner hide:YES];
    }];
    
    //scale the page to fit the view it is shoved into
    if (self.showWholeScreen)
    {
        CGSize contentSize = webView.scrollView.contentSize;
        CGSize viewSize = self.view.bounds.size;
        
        float rw = viewSize.width / contentSize.width;
        
        webView.scrollView.minimumZoomScale = rw;
        //webView.scrollView.maximumZoomScale = rw;
        webView.scrollView.zoomScale = rw;
    }
}

- (void) webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
    [self updateNavButtonsState];
    
    [self.spinner hide:YES];    
}

#pragma mark - UIWebView Delegate Methods
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    BOOL val = [self shouldStartNavigationActionWithRequest:request navigationType:navigationType];
    return val;
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
    [self updateNavButtonsState];
    
    if (self.flurryPageEvent)
        [Flurry logEvent:self.flurryPageEvent];
    
    [self.spinner show:YES];
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    [self updateNavButtonsState];
    [self.spinner hide:YES];
    
    //scale the page to fit the view it is shoved into
    if (self.showWholeScreen)
    {
        CGSize contentSize = webView.scrollView.contentSize;
        CGSize viewSize = self.view.bounds.size;
        
        float rw = viewSize.width / contentSize.width;
        
        webView.scrollView.minimumZoomScale = rw;
        //webView.scrollView.maximumZoomScale = rw;
        webView.scrollView.zoomScale = rw;
    }
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    [self updateNavButtonsState];
    
    [self.spinner hide:YES];
}

@end
