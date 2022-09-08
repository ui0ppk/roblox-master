//
//  LoginScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/20/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import "AppDelegateROMA.h"
#import "LoginScreenController.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "RobloxHUD.h"
#import "RobloxAlert.h"
#import "StoreManager.h"
#import "RobloxTheme.h"
#import "RBResetPasswordViewController.h"
#import "RobloxInfo.h"
#import "Flurry.h"
#import "KeychainItemWrapper.h"
#import "UIView+Position.h"
#import "RobloxWebUtility.h"
#import "iOSSettingsService.h"
#import "RBCaptchaViewController.h"
#import "RBCaptchaV2ViewController.h"
#import "NonRotatableNavigationController.h"
#import "RBXEventReporter.h"
#import "RBValidTextField.h"
#import "SignUpScreenController.h"
#import "RBXFunctions.h"

#define PASSWORD_RESET_URL  @"/Login/ResetPasswordRequest.aspx"
#ifndef kCFCoreFoundationVersionNumber_iOS_8_0
    #define kCFCoreFoundationVersionNumber_iOS_8_0 1129.15
#endif

//---METRICS---
#define LSC_loginSelected           @"LOG IN SCREEN - Log In Selected"
#define LSC_forgotPasswordSelected  @"LOG IN SCREEN - Forgot Password Selected"

@interface LoginScreenController ()
//public properties
@property IBOutlet RBValidTextField* username;
@property IBOutlet RBValidTextField* password;

@end

@implementation LoginScreenController
{
    //private properties
    IBOutlet UILabel*  _loginTitle;
    IBOutlet UILabel*  _notAMemberLabel;
    IBOutlet UIButton* _notAMemberButton;
    IBOutlet UIButton* _forgotPasswordButton;
    IBOutlet UIButton* _loginButton;
    IBOutlet UIButton* _cancelButton;
    IBOutlet UIView* _whiteView;
    
    UITapGestureRecognizer* _touches;
    
    NSString* _segueUsername;
    NSString* _seguePassword;
}

////////////////////////////////////////////////////////////////
#pragma mark - View functions
////////////////////////////////////////////////////////////////
- (void) viewDidLoad {
    [super viewDidLoad];
    
    _touches = [[UITapGestureRecognizer alloc] init];
    [_touches setNumberOfTouchesRequired:1];
    [_touches setNumberOfTapsRequired:1];
    [_touches setDelegate:self];
    [_touches setEnabled:YES];
    [self.view addGestureRecognizer:_touches];
    
    
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [_whiteView.layer setShadowColor:[UIColor blackColor].CGColor];
        [_whiteView.layer setShadowOpacity:0.4];
        [_whiteView.layer setShadowRadius:2.0];
        [_whiteView.layer setShadowOffset:CGSizeMake(0.0, 0.5)];
    }
    
    __weak LoginScreenController* weakself = self;
    
    // Stylize elements
    [_username setTitle:NSLocalizedString(@"UsernameWord", nil)];
    [_username setNextResponder:_password];
    if (_segueUsername != nil && _segueUsername.length > 0)
    {
        [_username setText:_segueUsername];
        _segueUsername = nil;
    }
    else if ([UserInfo CurrentPlayer].username != nil)
        [_username setText:[UserInfo CurrentPlayer].username];
    
    [_password setTitle:NSLocalizedString(@"PasswordWord", nil)];
    [_password setProtectedTextEntry:YES];
    [_password setExitOnEnterBlock:^{
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakself login:weakself.password];
        });
    }];
    if (_seguePassword != nil && _seguePassword.length > 0)
    {
        [_password setText:_seguePassword];
        _seguePassword = nil;
    }
    //else if ([[LoginManager sharedInstance] getRememberPassword])
    //    [_password setText:[UserInfo CurrentPlayer].password];
    
    [_loginTitle setText:[NSLocalizedString(@"LoginWord", nil) uppercaseString]];
    [_loginButton setTitle:NSLocalizedString(@"LoginTitle", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalSubmitButton:_loginButton];
    
    [_cancelButton setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalCancelButton:_cancelButton];
    
    [_notAMemberLabel setFont:[RobloxTheme fontBodySmall]];
    [_notAMemberLabel setText:NSLocalizedString(@"NotAMemberPhrase", nil)];
    [_notAMemberButton setTitle:NSLocalizedString(@"SignupWord", nil) forState:UIControlStateNormal];
    [_notAMemberButton.titleLabel setFont:[RobloxTheme fontBodySmall]];
    [_notAMemberButton addTarget:self action:@selector(didPressSignUp) forControlEvents:UIControlEventTouchUpInside];
    
    [_forgotPasswordButton setTitle:NSLocalizedString(@"Forgot Password?", nil) forState:UIControlStateNormal];
    [_forgotPasswordButton.titleLabel setFont:[RobloxTheme fontBodySmall]];
    [_forgotPasswordButton addTarget:self action:@selector(didPressForgotPassword) forControlEvents:UIControlEventTouchUpInside];
    
    if ([self isForgotPasswordLinkEnabled])
        _forgotPasswordButton.hidden = NO;
    else
        _forgotPasswordButton.hidden = ![RobloxInfo thisDeviceIsATablet];
}
- (void) viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self resignFirstResponder];
    
    [[LoginManager sharedInstance] setRememberPassword:YES];
}
- (void) viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    BOOL isPreiOS8 = NSFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_8_0;
    if (isPreiOS8 && [RobloxInfo thisDeviceIsATablet])
    {
        self.view.superview.bounds =  CGRectMake(0, 0, 540, 296);
    }
}

////////////////////////////////////////////////////////////////
#pragma mark - Accessors and Mutators
////////////////////////////////////////////////////////////////
- (BOOL) isForgotPasswordLinkEnabled
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    return iOSSettings->GetValueEnableLinkForgottenPassword();
}

-(void) setUsername:(NSString *)username andPassword:(NSString *)password
{
    if (_username)
        [_username setText:username ? username : @""];
    else
        _segueUsername = username;
    
    
    if (_password)
        [_password setText:password ? password : @""];
    else
        _seguePassword = password;
}

- (void) executeDismissalCompletionBlockWithDismissType:(LoginScreenDismissalType)dismissType error:(NSError *)loginError {
    if (nil != self.dismissalCompletionHandler) {
        self.dismissalCompletionHandler(dismissType, loginError);
    }
}

////////////////////////////////////////////////////////////////
#pragma mark - UI Actions
////////////////////////////////////////////////////////////////
- (IBAction)closeController:(id)sender {
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose
                                             withContext:RBXAContextLogin];
    [self dismissViewControllerAnimated:YES completion:nil];
    [self executeDismissalCompletionBlockWithDismissType:LoginScreenDismissalCancelled error:nil];
}

- (IBAction)login:(id)sender {
    
    bool hasUsername = _username.text.length > 0;
    bool hasPassword = _password.text.length > 0;
    if (!hasUsername)
    {
        //[RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"UsernameMissing", nil)];
        [_username showError:NSLocalizedString(@"UsernameMissing", nil)];
        [_username markAsInvalid];
        [_username becomeFirstResponder];
        [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldUsername
                                                           withContext:RBXAContextLogin
                                                             withError:RBXAErrorMissingRequiredField];
    }
    if (!hasPassword)
    {
        //[RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"PasswordMissing", nil)];
        [_password showError:NSLocalizedString(@"PasswordMissing", nil)];
        [_password markAsInvalid];
        
        if (hasUsername)
            [_password becomeFirstResponder];
        
        [[RBXEventReporter sharedInstance] reportFormFieldValidation:RBXAFieldPassword
                                                         withContext:RBXAContextLogin
                                                             withError:RBXAErrorMissingRequiredField];
    }
    
    if (hasUsername && hasPassword)
    {
        [Flurry logEvent:LSC_loginSelected];
        [self showLoggingIn];
        
        [[LoginManager sharedInstance] loginWithUsername:_username.text password:_password.text completionBlock:^(NSError *loginError) {
            if ([RBXFunctions isEmpty:loginError]) {
                // Initialize Store Manager on login, so that any pending transaction from last session are queued
                GetStoreMgr;
                
                //save the username and password to the keychain
                if ([LoginManager sessionLoginEnabled] == NO) {
                    KeychainItemWrapper *keychainItem = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
                    [keychainItem setObject:[UserInfo CurrentPlayer].password forKey:(__bridge id)kSecValueData];
                    [keychainItem setObject:[UserInfo CurrentPlayer].username forKey:(__bridge id)kSecAttrAccount];
                }
                
                //update the UI
                [self stopShowLoggingIn]; // <-- this is sent on the main thread already
                dispatch_async(dispatch_get_main_queue(), ^{
                    //clear out the password
                    _password.text = @"";
                    
                    [self dismissViewControllerAnimated:YES completion:nil];
                    [self executeDismissalCompletionBlockWithDismissType:LoginScreenDismissalLoginSuccess error:nil];
                });
            } else {
                NSString* errorMessage = loginError.domain;
                
                if (!errorMessage || errorMessage.length == 0)
                    errorMessage = NSLocalizedString(@"UnknownLoginError", nil);
                
                [RBXFunctions dispatchOnMainThread:^{
                    [self stopShowLoggingIn];
                    
                    //only clear the password text for specific errors
                    if ([errorMessage isEqualToString:NSLocalizedString(@"InvalidUsernameOrPw", nil)])
                    {
                        _password.text = @"";
                        [_password markAsInvalid];
                        [_password showError:errorMessage];
                        
                        [_password becomeFirstResponder];
                        //[RobloxHUD prompt:errorMessage withTitle:NSLocalizedString(@"ErrorWord",nil)];
                    }
                    else if ([errorMessage isEqualToString:NSLocalizedString(@"TooManyAttempts", nil)])
                    {
                        //open up a captcha so we can attempt to log in again
                        NonRotatableNavigationController* navigation = [LoginManager CaptchaForLoginWithUsername:_username.text
                                                                                                 andV1Completion:^(bool success, NSString *message)
                        {
                            [RBXFunctions dispatchOnMainThread:^
                            {
                                if (success == YES)
                                {
                                    [self login:nil];
                                }
                            }];
                            
                        }
                                                                                                 andV2Completion:^(NSError *captchaError)
                        {
                            [RBXFunctions dispatchOnMainThread:^
                            {
                                if ([RBXFunctions isEmpty:captchaError])
                                {
                                    [self login:nil];
                                }
                                else
                                {
                                    [RobloxHUD prompt:errorMessage withTitle:NSLocalizedString(@"ErrorWord", nil)];
                                    [self executeDismissalCompletionBlockWithDismissType:LoginScreenDismissalLoginFailed error:loginError];
                                }
                            }];
                        }];
                        [self presentViewController:navigation animated:YES completion:nil];
                    }
                    else
                    {
                        [RobloxHUD prompt:errorMessage withTitle:NSLocalizedString(@"ErrorWord", nil)];
                    }
                    
                    //Why is this completion block being told to execute? The screen isn't being dismissed - Kyler 1/8/2015
                    //[self executeDismissalCompletionBlockWithDismissType:LoginScreenDismissalLoginFailed error:loginError];
                }];
            }
        }];
    }
}

- (void)didPressForgotPassword {
    [Flurry logEvent:LSC_forgotPasswordSelected];
    NSString* baseURL = [RobloxInfo getWWWBaseUrl];
    baseURL = [baseURL stringByAppendingString:PASSWORD_RESET_URL];
    
    NSURL* url = [NSURL URLWithString:baseURL];
    
    RBResetPasswordViewController* controller = [[RBResetPasswordViewController alloc] initWithURL:url andTitle:NSLocalizedString(@"Reset Password", nil)];
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:controller];
    UIViewController *presenter = self.presentingViewController;
    
    [self dismissViewControllerAnimated:NO completion:nil];
    [self executeDismissalCompletionBlockWithDismissType:LoginScreenDismissalCancelled error:nil];
    [presenter presentViewController:navigation animated:YES completion:nil];
}
- (void)didPressSignUp{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSignup withContext:RBXAContextLogin];
    
    NSString* controllerName;
    if ([[LoginManager sharedInstance] isFacebookEnabled])
        controllerName = @"SignUpScreenControllerWithSocial";
    else if ([LoginManager apiProxyEnabled])
        controllerName = @"SignUpAPIScreenController";
    else
        controllerName = @"SignUpScreenController";
    
    UIViewController *presenter = self.presentingViewController;
    UIStoryboard* storyboard = [UIStoryboard storyboardWithName:[RobloxInfo getStoryboardName] bundle:nil];
    SignUpScreenController* controller = (SignUpScreenController*)[storyboard instantiateViewControllerWithIdentifier:controllerName];
    
    BOOL isPreiOS8 = NSFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_8_0;
    if (isPreiOS8 && [RobloxInfo thisDeviceIsATablet])
    {
        [self dismissViewControllerAnimated:YES completion:^{
            [presenter presentViewController:controller animated:YES completion:nil];
        }];
    }
    else
    {
        [self dismissViewControllerAnimated:YES completion:nil];
        [presenter presentViewController:controller animated:YES completion:nil];
    }
    
    [self executeDismissalCompletionBlockWithDismissType:LoginScreenDismissalCancelled error:nil];
}

////////////////////////////////////////////////////////////////
#pragma mark - Delegate Functions
////////////////////////////////////////////////////////////////
- (BOOL)disablesAutomaticKeyboardDismissal {
    return NO;
}
-(void) resignAllResponders
{
    [self.view endEditing:YES];
    //RBValidTextField* activeField;
    //if      ([_username isEditing]) activeField = _username;
    //else if ([_password isEditing]) activeField = _password;
    //
    //if (activeField)
    //    [activeField resignFirstResponder];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    UIView* touchedView = touch.view;
    if (touchedView == self.view || touchedView == _whiteView)
        [self resignAllResponders];
    
    return  YES;
}

////////////////////////////////////////////////////////////////
// Login
////////////////////////////////////////////////////////////////
- (void) showLoggingIn
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [_username endEditing:YES];
        [_password endEditing:YES];
        
        [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"LoggingIn", nil) dimBackground:YES];
    });
}
- (void) stopShowLoggingIn
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [RobloxHUD hideSpinner:NO];
    });
}



@end
