//
//  RBCaptchaV2ViewController.h
//  RobloxMobile
//
//  Created by Ashish Jain on 12/17/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import "RBModalPopUpViewController.h"

typedef void(^CaptchaCompletionHandler)(NSError *captchaError);

@interface RBCaptchaV2ViewController : RBModalPopUpViewController <UIGestureRecognizerDelegate, UITextFieldDelegate>

+ (instancetype) CaptchaV2ForLoginWithUsername:(NSString*)username
                             completionHandler:(CaptchaCompletionHandler)completionHandler;

+ (instancetype) CaptchaV2ForSignupWithUsername:(NSString *)username
                                  andCompletion:(CaptchaCompletionHandler)completionHandler;

+ (instancetype) CaptchaV2ForSocialSignupWithUsername:(NSString *)username
                                        andCompletion:(CaptchaCompletionHandler)completionHandler;
@end
