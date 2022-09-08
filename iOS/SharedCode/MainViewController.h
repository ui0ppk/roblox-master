//
//  MainViewController.h
//  IOSClient
//
//  Created by Ben Tkacheff on 9/20/12.
//  Copyright (c) 2012 tolkring@gmail.com. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "RobloxView.h"

@class GameViewController;

@interface MainViewController : UIViewController
{
    UIWindow* ogreWindow;
    UIView* ogreView;
    GameViewController* ogreViewController;
    RobloxView* rbxView;
    
    UIViewController* lastNonGameController;
}

+ (id)sharedInstance;

-(void) switchView:(UIView*) newView;
-(void) addSubview:(UIView*) view;

-(UIWindow*) getOgreWindow;
-(void) setOgreWindow:(UIWindow*) value;

-(UIView*) getOgreView;
-(void) setOgreView:(UIView*) value;

-(GameViewController*) getOgreViewController;
-(void) setOgreViewController:(GameViewController*) value;

-(void) setRobloxView:(RobloxView*) newRbxView;
-(RobloxView*) getRobloxView;

-(void) setLastNonGameController:(UIViewController*) lastController;
-(UIViewController*) getLastNonGameController;

-(void) debugPrint;

@end
