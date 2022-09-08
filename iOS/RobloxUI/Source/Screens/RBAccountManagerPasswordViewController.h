//
//  RBAccountManagerPasswordViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/26/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "RBModalPopUpViewController.h"
#include "RBValidTextField.h"

//---------------------Change User Password Screen------------------------------
@interface RBAccountManagerPasswordViewController : RBModalPopUpViewController <UIGestureRecognizerDelegate>

@property IBOutlet RBValidTextField* txtPasswordNew;
@property IBOutlet RBValidTextField* txtPasswordConfirm;
@property IBOutlet RBValidTextField* txtPasswordOld;

@property IBOutlet UIButton*    btnSave;
@property IBOutlet UIButton*    btnCancel;
@property IBOutlet UIView*      whiteView;
@property IBOutlet UILabel*     lblTitle;

-(IBAction)savePasswordChange:(id)sender;
@end
