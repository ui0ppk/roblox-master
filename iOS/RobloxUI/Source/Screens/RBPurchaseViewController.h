//
//  RBPurchaseViewController.h
//  RobloxMobile
//
//  Created by Christian Hresko on 6/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "RBModalPopUpViewController.h"

// TODO - make this a subclass of a common Modal UIViewController
@interface RBPurchaseViewController : RBModalPopUpViewController <UIWebViewDelegate>

- (id)initWithURL:(NSURL*)url andTitle:(NSString*)title;
- (id)initWithBCPurchasing;
- (id)initWithRobuxPurchasing;
@end
