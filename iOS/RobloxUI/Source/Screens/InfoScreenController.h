//
//  InfoScreenController.h
//  RobloxMobile
//
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "NonRotatableViewController.h"

@interface InfoScreenController : NonRotatableViewController <UIWebViewDelegate>
@property (retain, nonatomic) IBOutlet UITextView *txtDeviceInfo;
@property (retain, nonatomic) IBOutlet UITextView *txtRobloxInfo;
@end
