//
//  GameViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 11/20/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "GameView.h"
#include <string>
#import "ControlView.h"
#import <AdColony/AdColony.h>

@interface GameViewController : UIViewController <UIWebViewDelegate, AdColonyAdDelegate>
{
    GameView* gameView;
    UIWebView* externalWebView;
    UIButton* closeWebviewButton;
    UIActivityIndicatorView* webViewActivityIndicator;

    CGFloat webviewTweenTime;
}

-(void) playVideoAd;

-(void) resizeGameView;
-(void) openUrlWindow:(std::string) url;
-(void) closeUrlWindow;
-(void) closeUrlWindow:(id) sender;

- (void) addControlView:(ControlView*)controlView;
-(ControlView*) getControlView;

- (void) onAdColonyAdStartedInZone:(NSString *)zoneID;
- (void) onAdColonyAdAttemptFinished:(BOOL)shown inZone:(NSString *)zoneID;

@end
