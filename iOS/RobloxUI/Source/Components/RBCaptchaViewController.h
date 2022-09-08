//
//  RBCaptchaViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 6/8/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBModalPopUpViewController.h"

typedef void(^AttemptedCompletionHandler)(bool success, NSString *message);

@interface RBCaptchaViewController : RBModalPopUpViewController <UIWebViewDelegate>

@property (nonatomic, copy) AttemptedCompletionHandler attemptedCompletionHandler;

// Constructors
+ (instancetype) CaptchaWithCompletionHandler:(AttemptedCompletionHandler)completionHandler;

@end
