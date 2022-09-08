//
//  RBMobileWebViewController.h
//  RobloxMobile
//
//  Created by Christian Hresko on 6/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RBBaseViewController.h"
#import "RBXWebViewProtocol.h"
#import "RBHybridWebViewProtocol.h"

@interface RBMobileWebViewController : RBBaseViewController

@property (nonatomic, strong) UIView <RBXWebViewProtocol, RBHybridWebViewProtocol> *rbxWebView;
@property(strong, nonatomic) NSString* url;
@property(nonatomic) BOOL showNavButtons;
@property(nonatomic) BOOL isPopover;
@property(nonatomic) int placeID;

- (id) initWithNavButtons:(BOOL)showButtons;
- (void) loadURL:(NSString*)url;
- (void) loadURL:(NSString*)url screenURL:(bool)shouldFilter;
- (void) reloadWebPage;
- (void) setToShowWholeScreen:(BOOL)showAll;

//flurry events
- (void) setFlurryGameLaunchEvent:(NSString*)gameLaunchEvent;
- (void) setFlurryPageLoadEvent:(NSString*)pageLaunchEvent;

@end