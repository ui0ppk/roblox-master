//
//  RBModalPopUpViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 10/15/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "RobloxTheme.h"

@interface RBModalPopUpViewController : UIViewController <UIGestureRecognizerDelegate>

-(void) disableTapRecognizer;
-(void) shouldAddCloseButton:(BOOL)shouldAdd;
-(void) shouldApplyModalTheme:(BOOL)shouldApply;
@end
