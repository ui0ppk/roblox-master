//
//  SignUpScreenController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/20/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "NonRotatableViewController.h"
#import "RBValidTextField.h"
#import "RBGenderView.h"
#import "RBBirthdayPicker.h"

@interface SignUpScreenController : NonRotatableViewController <UIGestureRecognizerDelegate>

@property IBOutlet RBValidTextField* username;
@property IBOutlet RBValidTextField* password;
@property IBOutlet RBValidTextField* verify;
@property IBOutlet RBValidTextField* email;
@property IBOutlet RBGenderView* gender;
@property IBOutlet RBBirthdayPicker* birthday;

@end
