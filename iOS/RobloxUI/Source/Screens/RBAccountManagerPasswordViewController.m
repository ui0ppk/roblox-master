//
//  RBAccountManagerPasswordViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/26/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBAccountManagerPasswordViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxHUD.h"
#import "RobloxNotifications.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "RobloxData.h"
#import "UIView+Position.h"
#import "SignUpVerifier.h"
#import "RobloxAlert.h"
#import "RBCaptchaViewController.h"
#import "RBCaptchaV2ViewController.h"
#import "NonRotatableNavigationController.h"
#import "RBXFunctions.h"
#import "RobloxNotifications.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 540, 344)

@interface RBAccountManagerPasswordViewController ()

@end


//-------------CHANGE PASSWORD SCREEN------------
@implementation RBAccountManagerPasswordViewController
{
    NSString* usernameCopy;
    UITapGestureRecognizer* _touches;
    bool accountHasPassword;
}

- (void)viewDidLoad
{
    //configure the modal popup superclass
    [self shouldAddCloseButton:NO];
    [self shouldApplyModalTheme:NO];
    [super viewDidLoad];
    
    accountHasPassword = [[UserInfo CurrentPlayer] userHasSetPassword] || ![RBXFunctions isEmptyString:[UserInfo CurrentPlayer].password];
    
    
    _touches = [[UITapGestureRecognizer alloc] init];
    [_touches setNumberOfTouchesRequired:1];
    [_touches setNumberOfTapsRequired:1];
    [_touches setDelegate:self];
    [_touches setEnabled:YES];
    [self.view addGestureRecognizer:_touches];
    
    //NSString* titleText = NSLocalizedString(@"ChangePasswordWord", nil);
    [_lblTitle setText:accountHasPassword ? NSLocalizedString(@"ChangePasswordWord", nil) : NSLocalizedString(@"AddPassword", nil)];
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [RobloxTheme applyShadowToView:_whiteView];
        
        //[_lblTitle setText:titleText];
    }
    
    __weak RBAccountManagerPasswordViewController* weakSelf = self;
    
    [_txtPasswordNew setTitle:NSLocalizedString(@"PasswordNewWord", nil)];
    [_txtPasswordNew setHint:NSLocalizedString(@"PasswordRequirements", nil)];
    [_txtPasswordNew setProtectedTextEntry:YES];
    [_txtPasswordNew setNextResponder:_txtPasswordConfirm];
    [_txtPasswordNew setValidationBlock:^{
        [[SignupVerifier sharedInstance] checkIfValidPassword:weakSelf.txtPasswordNew.text
                                                 withUsername:[UserInfo CurrentPlayer].username
                                                   completion:^(BOOL success, NSString *validMessage)
         {
             if (success)
                 [weakSelf.txtPasswordNew markAsValid];
             else
             {
                 [weakSelf.txtPasswordNew markAsInvalid];
                 [weakSelf.txtPasswordNew showError:validMessage];
             }
         }];
        
        if (weakSelf.txtPasswordConfirm.text.length > 0)
        {
            [[SignupVerifier sharedInstance] checkIfPasswordsMatch:weakSelf.txtPasswordNew.text
                                                  withVerification:weakSelf.txtPasswordConfirm.text
                                                        completion:^(BOOL passwordsMatch, NSString *matchMessage)
             {
                 if (passwordsMatch)
                     [weakSelf.txtPasswordConfirm markAsValid];
                 else
                 {
                     [weakSelf.txtPasswordConfirm markAsInvalid];
                     [weakSelf.txtPasswordConfirm showError:matchMessage];
                 }
             }];
        }
    }];
    
    [_txtPasswordConfirm setTitle:NSLocalizedString(@"PasswordConfirmWord", nil)];
    [_txtPasswordConfirm setHint:NSLocalizedString(@"PasswordConfirmPhrase", nil)];
    [_txtPasswordConfirm setProtectedTextEntry:YES];
    [_txtPasswordConfirm setNextResponder:_txtPasswordOld];
    [_txtPasswordConfirm setValidationBlock:^{
        [[SignupVerifier sharedInstance] checkIfPasswordsMatch:weakSelf.txtPasswordNew.text
                                              withVerification:weakSelf.txtPasswordConfirm.text
                                                    completion:^(BOOL passwordsMatch, NSString *message)
         {
             if (passwordsMatch)
                 [weakSelf.txtPasswordConfirm markAsValid];
             else
             {
                 [weakSelf.txtPasswordConfirm markAsInvalid];
                 [weakSelf.txtPasswordConfirm showError:message];
             }
         }];
    }];
    
    [_txtPasswordOld setTitle:NSLocalizedString(@"PasswordCurrentWord", nil)];
    [_txtPasswordOld setHint:NSLocalizedString(@"PasswordCurrentPhrase", nil)];
    [_txtPasswordOld setProtectedTextEntry:YES];
    [_txtPasswordOld setValidationBlock:^{
        if (weakSelf.txtPasswordOld.text.length >= 1)
            [weakSelf.txtPasswordOld markAsValid];
    }];
    //[_txtPasswordOld setExitOnEnterBlock:^{
    //    [weakSelf savePasswordChange:weakSelf.txtPasswordOld];
    //}];
    [_txtPasswordOld setHidden:!accountHasPassword];
    
    [_btnSave setTitle:NSLocalizedString(@"SaveWord",nil) forState:UIControlStateNormal ];
    [RobloxTheme applyToModalSubmitButton:_btnSave];
    
    [_btnCancel setTitle:NSLocalizedString(@"CancelWord",nil) forState:UIControlStateNormal ];
    [RobloxTheme applyToModalCancelButton:_btnCancel];
    
    usernameCopy = [NSString stringWithFormat:@"%@", [UserInfo CurrentPlayer].username];
}


//Action functions
- (IBAction)savePasswordChange:(id)sender
{
    //dismiss the keyboard if it is up
    [self resignAllResponders];
    
    if (accountHasPassword)
        [self savePassword];
    else
        [self addPassword];
}
-(void) savePassword
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 1 - check if any required fields are missing
        
        bool hasPasswordNew = (_txtPasswordNew.text.length > 0);
        bool hasPasswordConfirm = (_txtPasswordConfirm.text.length > 0);
        bool hasPasswordOld = (_txtPasswordOld.text.length > 0);
        if (!hasPasswordNew)
        {
            [_txtPasswordNew markAsInvalid];
            [_txtPasswordNew showError:NSLocalizedString(@"PasswordMissing", nil)];
            
            [_txtPasswordNew becomeFirstResponder];
        }
        if (!hasPasswordConfirm)
        {
            [_txtPasswordConfirm markAsInvalid];
            [_txtPasswordConfirm showError:NSLocalizedString(@"PasswordConfirmPhrase", nil)];
            
            if (hasPasswordNew)
                [_txtPasswordConfirm becomeFirstResponder];
        }
        if (!hasPasswordOld)
        {
            [_txtPasswordOld markAsInvalid];
            [_txtPasswordOld showError:NSLocalizedString(@"PasswordCurrentPhrase", nil)];
            
            if (hasPasswordNew && hasPasswordConfirm)
                [_txtPasswordOld becomeFirstResponder];
        }
        
        //do not proceed until we have all three
        if (!hasPasswordNew || !hasPasswordConfirm || !hasPasswordOld)
            return;
        
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 2 - check that the new passwords match
        if (![_txtPasswordNew.text isEqualToString:_txtPasswordConfirm.text])
        {
            [_txtPasswordConfirm markAsInvalid];
            [_txtPasswordConfirm showError:NSLocalizedString(@"VerifyPasswordWrong", nil)];
            
            [_txtPasswordConfirm becomeFirstResponder];
            return;
        }
        
        //-- THESE CHECKS ARE A SECURITY HAZARD --
        //check that the old password is correct
        //if (![[UserInfo CurrentPlayer].password isEqualToString:[_txtUserPasswordCurrent text]])
        //{
        //    [RobloxHUD showMessage:NSLocalizedString(@"VerifyError", nil)];
        //    return;
        //}
        
        //check that the new password is not the same as the old one
        //if ([[UserInfo CurrentPlayer].password isEqualToString:[_txtUserPasswordNew text]])
        //{
        //    [RobloxHUD showMessage:NSLocalizedString(@"ErrorSamePassword", nil)];
        //    return;
        //}
        
        
        
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 3 - check that all fields are validated
        if (!_txtPasswordNew.isValidated)       { [_txtPasswordNew becomeFirstResponder];       return; }
        if (!_txtPasswordConfirm.isValidated)   { [_txtPasswordConfirm becomeFirstResponder];   return; }
        if (!_txtPasswordOld.isValidated)       { [_txtPasswordOld becomeFirstResponder];       return; }
        
        
        //display the loading spinner
        [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"ChangePasswordVerb", nil) dimBackground:YES];
        
        //looks like everything is good, send the request
        [RobloxData changeUserOldPassword:_txtPasswordOld.text
                            toNewPassword:_txtPasswordNew.text
                         withConfirmation:_txtPasswordConfirm.text
                            andCompletion:^(BOOL success, NSString *message)
         {
             if (success)
             {
                 //now we need to log the user out and log back in with the new credentials
                 //do this all under the hood
                 
                 //once we have reset the session by logging the user out and back in with the new credentials,
                 //then we can hide the spinner and alert the user that the password change was a success
                 //display the message to the user

                 [[LoginManager sharedInstance] loginWithUsername:usernameCopy password:_txtPasswordNew.text completionBlock:^(NSError *loginError) {
                     if ([RBXFunctions isEmpty:loginError]) {
                         // login successful
                         [self didCompleteReLogin];
                     } else {
                         // login failure
                         [self didFailReLoginWithError:loginError];
                     }
                 }];
             }
             else
             {
                 //it didn't work for some reason, hide the loading spinner and display the message to the user
                 [RBXFunctions dispatchOnMainThread:^{
                     [RobloxHUD hideSpinner:YES];
                     [RobloxHUD prompt:message withTitle:NSLocalizedString(@"ErrorWord", nil)];
                     [_txtPasswordNew setText:@""];
                     [_txtPasswordConfirm setText:@""];
                     [_txtPasswordOld setText:@""];
                 }];
             }
         }];
    });
}
-(void) addPassword
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 1 - check if any required fields are missing
        
        bool hasPasswordNew = (_txtPasswordNew.text.length > 0);
        bool hasPasswordConfirm = (_txtPasswordConfirm.text.length > 0);
        if (!hasPasswordNew)
        {
            [_txtPasswordNew markAsInvalid];
            [_txtPasswordNew showError:NSLocalizedString(@"PasswordMissing", nil)];
            
            [_txtPasswordNew becomeFirstResponder];
        }
        if (!hasPasswordConfirm)
        {
            [_txtPasswordConfirm markAsInvalid];
            [_txtPasswordConfirm showError:NSLocalizedString(@"PasswordConfirmPhrase", nil)];
            
            if (hasPasswordNew)
                [_txtPasswordConfirm becomeFirstResponder];
        }
        
        //do not proceed until we have all three
        if (!hasPasswordNew || !hasPasswordConfirm)
            return;
        
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 2 - check that the new passwords match
        if (![_txtPasswordNew.text isEqualToString:_txtPasswordConfirm.text])
        {
            [_txtPasswordConfirm markAsInvalid];
            [_txtPasswordConfirm showError:NSLocalizedString(@"VerifyPasswordWrong", nil)];
            
            [_txtPasswordConfirm becomeFirstResponder];
            return;
        }
        
        
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 3 - check that all fields are validated
        if (!_txtPasswordNew.isValidated)       { [_txtPasswordNew becomeFirstResponder];       return; }
        if (!_txtPasswordConfirm.isValidated)   { [_txtPasswordConfirm becomeFirstResponder];   return; }
        
        
        //display the loading spinner
        [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"ChangePasswordVerb", nil) dimBackground:YES];
        
        //looks like everything is good, send the request
        [RobloxData changeUserOldPassword:_txtPasswordConfirm.text
                            toNewPassword:_txtPasswordNew.text
                         withConfirmation:_txtPasswordConfirm.text
                            andCompletion:^(BOOL success, NSString *message)
         {
             if (success)
             {
                 //now we need to log the user out and log back in with the new credentials
                 //do this all under the hood
                 
                 //once we have reset the session by logging the user out and back in with the new credentials,
                 //then we can hide the spinner and alert the user that the password change was a success
                 //display the message to the user
                 
                 [[LoginManager sharedInstance] loginWithUsername:usernameCopy password:_txtPasswordNew.text completionBlock:^(NSError *loginError) {
                     if ([RBXFunctions isEmpty:loginError]) {
                         // login successful
                         [self didCompleteReLogin];
                     } else {
                         // login failure
                         [self didFailReLoginWithError:loginError];
                     }
                 }];
                 
                 [RBXFunctions dispatchOnMainThread:^{
                     accountHasPassword = YES;
                     [_txtPasswordOld setHidden:NO];
                 }];
             }
             else
             {
                 //it didn't work for some reason, hide the loading spinner and display the message to the user
                 [RBXFunctions dispatchOnMainThread:^{
                     [RobloxHUD hideSpinner:YES];
                     [RobloxHUD prompt:message withTitle:NSLocalizedString(@"ErrorWord", nil)];
                     [_txtPasswordNew setText:@""];
                     [_txtPasswordConfirm setText:@""];
                     [_txtPasswordOld setText:@""];
                 }];
             }
         }];
    });
}
- (IBAction)closeController:(id)sender
{
    //[[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextLogin];
    [self.navigationController popViewControllerAnimated:YES];
}

//Notification functions
- (void) didCompleteReLogin
{
    [RBXFunctions dispatchOnMainThread:^{
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [RobloxHUD hideSpinner:YES];
        [RobloxHUD prompt:NSLocalizedString(@"SuccessChangePassword", nil) withTitle:NSLocalizedString(@"SuccessWord", nil)];
        [self.navigationController popViewControllerAnimated:NO];
    }];
}

- (void) didFailReLoginWithError:(NSError *)reloginError
{
    //This is really unusual if it happens, password change succeeds, but relogin fails?
    NSString* failureReason = reloginError.domain ? reloginError.domain : NSLocalizedString(@"UnknownError", nil);
    if ([failureReason isEqualToString:NSLocalizedString(@"TooManyAttempts", nil)])
    {
        [RBXFunctions dispatchOnMainThread:^{
            //open up a captcha so we can attempt to log in again
            UIViewController* controller;
            if ([LoginManager apiProxyEnabled] == YES) {
                controller = [RBCaptchaV2ViewController CaptchaV2ForLoginWithUsername:[UserInfo CurrentPlayer].username completionHandler:^(NSError *captchaError) {
                    if ([RBXFunctions isEmpty:captchaError]) {
                        [self savePasswordChange:nil];
                    }
                    else
                    {
                        [RobloxHUD prompt:failureReason withTitle:NSLocalizedString(@"ErrorWord", nil)];
                    }
                }];
            } else {
                controller = [RBCaptchaViewController CaptchaWithCompletionHandler:^(bool success, NSString *message) {
                    if (success == YES) {
                        [self savePasswordChange:nil];
                    }
                }];
            }
            NonRotatableNavigationController* navigation = [[NonRotatableNavigationController alloc] initWithRootViewController:controller];
            navigation.modalPresentationStyle = UIModalPresentationFormSheet;
            [self presentViewController:navigation animated:YES completion:nil];
        }];
    }
    else
    {
        //we could not log the player back in. Apologize and log them out
        [RBXFunctions dispatchOnMainThread:^{
            //log the player out
            [[LoginManager sharedInstance] logoutRobloxUser];
            
            [RobloxHUD prompt:NSLocalizedString(@"ErrorCannotReLogin", nil) withTitle:NSLocalizedString(@"ErrorUnknownTitle",nil)];
            
            //dismiss the view
            if ([RobloxInfo thisDeviceIsATablet])
            {
                [(UINavigationController*)self.navigationController.presentingViewController popToRootViewControllerAnimated:YES];
                [self dismissViewControllerAnimated:NO completion:nil];
            }
            else
            {
                [self.tabBarController.navigationController popViewControllerAnimated:YES];
            }
        }];
    }
}

-(void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch* touch = [[event allTouches] anyObject];
    if ([[touch view] isEqual:self.view])
        [self resignAllResponders];

    [super touchesBegan:touches withEvent:event];
}
- (void) resignAllResponders
{
    [self.view endEditing:YES];
    //[_txtPasswordNew resignFirstResponder];
    //[_txtPasswordConfirm resignFirstResponder];
    //[_txtPasswordOld resignFirstResponder];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    UIView* touchedView = touch.view;
    if (touchedView == self.view || touchedView == _whiteView)
        [self resignAllResponders];
    
    return  YES;
}


@end
