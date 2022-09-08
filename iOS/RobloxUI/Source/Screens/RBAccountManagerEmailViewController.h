//
//  RBAccountManagerEmailViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/26/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "RBModalPopUpViewController.h"
#include "RBValidTextField.h"

@interface RBAccountManagerEmailViewController : RBModalPopUpViewController <UIGestureRecognizerDelegate>
@property IBOutlet UILabel*     lblTitle;
@property IBOutlet UILabel*     lblAlertReason;
@property IBOutlet UIImageView* imgAlert;
@property IBOutlet UIButton*    btnSave;
@property IBOutlet UIButton*    btnCancel;

@property IBOutlet RBValidTextField* txtEmail;
@property IBOutlet RBValidTextField* txtPassword;

//phone outlets
@property IBOutlet UIView* whiteView;

-(IBAction)saveEmailChange:(id)sender;
-(IBAction)closeController:(id)sender;
@end
