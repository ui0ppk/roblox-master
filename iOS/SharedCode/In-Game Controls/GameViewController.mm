//
//  GameViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 11/20/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "GameViewController.h"
#import "LoginManager.h"
#import "RobloxInfo.h"
#import "StoreManager.h"
#import "AppDelegate.h"
#import "UIScreen+PortraitBounds.h"
#import "PlaceLauncher.h"
#import "RobloxNotifications.h"

#include "v8datamodel/LoginService.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/AdService.h"
#include "util/SoundService.h"

DYNAMIC_FASTSTRINGVARIABLE(AdColonyAppId, "app02d1db3451cc4b6b97");
DYNAMIC_FASTSTRINGVARIABLE(AdColonyZoneId, "vz073fd9a8cf0c447cbc");

@implementation GameViewController
{
    ControlView* _controlView;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        webviewTweenTime = 0.3;
        
        CGSize screenSize = [[UIScreen mainScreen] portraitBounds].size;
        gameView = [[GameView alloc] initWithFrame:CGRectMake(0, 0, screenSize.height, screenSize.width)];
        self.view = gameView;
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleGoingBackgroundNotification:)
                                                     name:RBX_NOTIFY_GAME_START_LEAVING
                                                   object:nil ];
    }
    return self;
}

-(void) dealloc
{
    if(externalWebView)
    {
        [externalWebView removeFromSuperview];
        externalWebView = nil;
    }
    
    _controlView = nil;
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    gameView = nil;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
    
    NSDictionary* dictionary = [NSDictionary dictionaryWithObjectsAndKeys: [RobloxInfo getUserAgentString], @"UserAgent", nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:dictionary];
}

-(void) resizeGameView
{
    [gameView layoutSubviews];
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

-(BOOL)shouldAutorotate
{
    return YES;
}

#ifdef __IPHONE_9_0
-(UIInterfaceOrientationMask) supportedInterfaceOrientations
#else
-(NSUInteger)supportedInterfaceOrientations
#endif
{
    return UIInterfaceOrientationMaskLandscape;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
    UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
    return (orientation == UIInterfaceOrientationLandscapeRight) ? UIInterfaceOrientationLandscapeRight : UIInterfaceOrientationLandscapeLeft;
}

- (void) addControlView:(ControlView*)controlView
{
    _controlView = controlView;
    
    [self.view addSubview:controlView];
}

-(ControlView*) getControlView
{
    return _controlView;
}

-(void) playVideoAd
{
    NSString* adColonyZoneId = [NSString stringWithUTF8String:DFString::AdColonyZoneId.c_str()];

    if(ControlView* controlView = [self getControlView])
    {
        if(shared_ptr<RBX::Game> game = [controlView getGame])
        {
            if(RBX::DataModel* dm = game->getDataModel().get())
            {
                if(RBX::Soundscape::SoundService* soundService = RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(dm))
                {
                    soundService->muteAllChannels(true);
                }
            }
        }
    }
    
    [AdColony playVideoAdForZone:adColonyZoneId withDelegate:self];
}

- (void) onAdColonyAdStartedInZone:(NSString *)zoneID
{
    if (ControlView* controlView = [self getControlView])
    {
        [controlView setUserInteractionEnabled:false];
        if (RbxInputView* inputView = [controlView getRbxInputView])
        {
            [inputView setUserInteractionEnabled:false];
            [inputView cancelAllTouches];
        }
    }
}

- (void) onAdColonyAdAttemptFinished:(BOOL)shown inZone:(NSString *)zoneID
{
    if(ControlView* controlView = [self getControlView])
    {
        [controlView setUserInteractionEnabled:true];
        if (RbxInputView* inputView = [controlView getRbxInputView])
        {
            [inputView setUserInteractionEnabled:true];
        }

        if(shared_ptr<RBX::Game> game = [controlView getGame])
        {
            if(RBX::DataModel* dm = game->getDataModel().get())
            {
                if(RBX::Soundscape::SoundService* soundService = RBX::ServiceProvider::find<RBX::Soundscape::SoundService>(dm))
                {
                    soundService->muteAllChannels(false);
                }
                if(RBX::AdService* adService = RBX::ServiceProvider::find<RBX::AdService>(dm))
                {
                    adService->videoAdClosed(shown);
                }
            }
        }
        
        if (shown)
        {
            //todo: do some webcall here for revenue sharing/stat tracking
        }
    }
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    id storeManager = GetStoreMgr;
    
    if ([storeManager isKindOfClass:[StoreManager class]])
    {
        if([storeManager checkForInAppPurchases:request navigationType:navigationType])
            return NO;
    }
    
    return YES;
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    if (webView == externalWebView && webViewActivityIndicator)
    {
        [webViewActivityIndicator setHidden:YES];
    }
}

-(void) signalGuiServiceUrlWindowClosedOnDataModel:(RBX::DataModel*) dataModel
{
    if(dataModel)
        if(RBX::GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(dataModel))
            guiService->urlWindowClosed();
}

-(void) closeUrlWindow:(id) sender
{
    if(externalWebView)
    {
        UIWebView* tempWebView = externalWebView;
        externalWebView = nil;
        
        if(ControlView* controlView = [self getControlView])
            if(shared_ptr<RBX::Game> game = [controlView getGame])
            {
                [self signalGuiServiceUrlWindowClosedOnDataModel:game->getDataModel().get()];
            }
        
        CGSize screenSize = [[UIScreen mainScreen] portraitBounds].size;
        screenSize = CGSizeMake(screenSize.height, screenSize.width);
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [UIView animateWithDuration:webviewTweenTime
                                  delay:0
                                options:UIViewAnimationOptionTransitionNone
                             animations:^{
                                 tempWebView.frame = CGRectMake(screenSize.width/2 - tempWebView.frame.size.width/2, screenSize.height, tempWebView.frame.size.width, tempWebView.frame.size.height);
                             }
                             completion:^(BOOL finished) {
                                 [tempWebView removeFromSuperview];
                             }
            ];
        });
    }
}

-(void) closeUrlWindow
{
    [self closeUrlWindow:nil];
}

-(void) openUrlWindow:(std::string) url
{
    if(externalWebView)
        return;
    
    CGSize screenSize = [[UIScreen mainScreen] portraitBounds].size;
    // switch so dimensions are in landscape
    screenSize = CGSizeMake(screenSize.height, screenSize.width);

    int webviewWidth = 660;
    int webviewHeight = 400;
    if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
    {
        webviewWidth = screenSize.width;
        webviewHeight = screenSize.height;
    }
    
    if (externalWebView == nil)
    {
        int closeButtonSize = 22;
        int closeButtonOffset = 5;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            externalWebView = [[UIWebView alloc] initWithFrame:CGRectMake(screenSize.width/2 - webviewWidth/2, screenSize.height, webviewWidth, webviewHeight)];
            externalWebView.delegate = self;
            externalWebView.userInteractionEnabled = YES;
            externalWebView.scalesPageToFit = NO;
            
            webViewActivityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
            [webViewActivityIndicator setHidden:NO];
            [webViewActivityIndicator startAnimating];
            [webViewActivityIndicator setFrame:CGRectMake(externalWebView.frame.size.width/2 - webViewActivityIndicator.frame.size.width/2,
                                                          externalWebView.frame.size.height/2 - webViewActivityIndicator.frame.size.height/2,
                                                          webViewActivityIndicator.frame.size.width,
                                                          webViewActivityIndicator.frame.size.height)];
            
            closeWebviewButton = [[UIButton alloc] initWithFrame:CGRectMake(externalWebView.frame.size.width - closeButtonSize - closeButtonOffset, closeButtonOffset, closeButtonSize, closeButtonSize)];
            [closeWebviewButton setImage:[UIImage imageNamed:@"Clear.png"] forState:UIControlStateNormal];
            [closeWebviewButton addTarget:self action:@selector(closeUrlWindow:) forControlEvents:UIControlEventTouchUpInside];

            [externalWebView addSubview:closeWebviewButton];
            [externalWebView addSubview:webViewActivityIndicator];
            
            if(ControlView* controlView = [self getControlView])
                [controlView addSubview:externalWebView];
        });
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [externalWebView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]]]];
        [UIView animateWithDuration:webviewTweenTime animations:^{
            externalWebView.frame = CGRectMake(screenSize.width/2 - webviewWidth/2, screenSize.height/2 - webviewHeight/2,  externalWebView.frame.size.width,  externalWebView.frame.size.height);
        }];
    });
}


-(void) handleGoingBackgroundNotification:(NSNotification*) leaveGameNotification
{
    if ([AdColony videoAdCurrentlyRunning])
        [AdColony cancelAd];
}
@end
