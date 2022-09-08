//
//  SignUpScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/20/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>

#import "SignUpScreenController.h"
#import "TermsAgreementController.h"
#import "RobloxHUD.h"
#import "RobloxAlert.h"
#import "SignupVerifier.h"
#import "UserInfo.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxNotifications.h"
#import "LoginManager.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "LoginScreenController.h"
#import "Flurry.h"
#import "UIView+Position.h"
#import "UIAlertView+Blocks.h"
#import "RobloxData.h"
#import "NonRotatableNavigationController.h"
#import "RBCaptchaViewController.h"
#import "RBXEventReporter.h"
#import "SocialSignUpViewController.h"
#import "RobloxNotifications.h"
#import "RBXFunctions.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 540, 540)
#define MAX_USERNAME_LENGTH 20
#ifndef kCFCoreFoundationVersionNumber_iOS_8_0
    #define kCFCoreFoundationVersionNumber_iOS_8_0 1129.15
#endif

//---METRICS---
#define SUSC_didPressLogin          @"SIGN UP SCREEN - Log In Pressed While Guest"
#define SUSC_userNameSelected       @"SIGN UP SCREEN - User Name Selected"
#define SUSC_emailSelected          @"SIGN UP SCREEN - Email Selected"
#define SUSC_passwordSelected       @"SIGN UP SCREEN - Password Selected"
#define SUSC_birthdaySelected       @"SIGN UP SCREEN - Birthday Button Pressed"
#define SUSC_maleButtonSelected     @"SIGN UP SCREEN - Male Button Pressed"
#define SUSC_femaleButtonSelected   @"SIGN UP SCREEN - Female Button Pressed"
#define SUSC_createAccountSelected  @"SIGN UP SCREEN - Sign Up Button Pressed"

@interface SignUpScreenController ()

@end

@implementation SignUpScreenController
{
    IBOutlet UILabel* _titleLabel;
    IBOutlet UIButton* _btnCancel;
    IBOutlet UIButton* _btnSignup;
    IBOutlet UIWebView *_finePrint;
    IBOutlet UIButton *loginButton;
    IBOutlet UIView* _whiteView;
    IBOutlet UIButton* _btnSocial;
    IBOutlet UILabel* _lblOr;
    
    UITapGestureRecognizer* _touches;
}

////////////////////////////////////////////////////////////////
// View delegates
////////////////////////////////////////////////////////////////
- (void)viewDidLoad
{
    [super viewDidLoad];
    [self.view.layer setMasksToBounds:NO];
    [self.view.layer setShadowColor:[UIColor blackColor].CGColor];
    [self.view.layer setShadowOpacity:0.6];
    [self.view.layer setShadowRadius:1.0];
    [self.view.layer setShadowOffset:CGSizeMake(0.0, 1.0)];
    
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
    
    // Fine print
    NSString *htmlFile = [[NSBundle mainBundle] pathForResource:@"SignUpDisclamer" ofType:@"html" inDirectory:nil];
    if(htmlFile)
    {
        NSString* htmlContents = [NSString stringWithContentsOfFile:htmlFile encoding:NSUTF8StringEncoding error:NULL];
        htmlContents = [htmlContents stringByReplacingOccurrencesOfString:@"textPlaceholder" withString:NSLocalizedString(@"SignupFinePrint", nil)];
        [_finePrint loadData:[htmlContents dataUsingEncoding:NSUTF8StringEncoding] MIMEType:@"text/html" textEncodingName:@"UTF-8" baseURL:[NSURL URLWithString:@""]];
    }
    
    __weak SignUpScreenController* weakSelf = self;
    
    //Initialize the input views
    //NSLocalizedString([RobloxInfo thisDeviceIsATablet] ? @"UsernameRequirements" : @"UsernameRequirementsShort", nil)]
    [_username setTitle:NSLocalizedString(@"UsernameWord", nil)];
    [_username setHint:NSLocalizedString(@"UsernameRequirements", nil)];
    [_username setNextResponder:_password];
    [_username setValidationBlock:^{
        [[SignupVerifier sharedInstance] checkIfValidUsername:weakSelf.username.text completion:^(BOOL success, NSString *validMessage)
        {
            if (success)
                [weakSelf.username markAsValid];
            else
            {
                [weakSelf.username markAsInvalid];
                
                //handle bad username
                if ([validMessage isEqualToString:NSLocalizedString(@"UsernameCommon", nil)])
                {
                    [[SignupVerifier sharedInstance] getAlternateUsername:weakSelf.username.text
                                                               completion:^(BOOL success, NSString *alternateMessage)
                     {
                         if (success)
                         {
                             [RobloxHUD prompt:[NSString stringWithFormat:NSLocalizedString(@"UsernameTaken", nil), alternateMessage]
                                     withTitle:NSLocalizedString(@"UsernameTakenTitle", nil)
                                          onOK:^
                              {
                                  [weakSelf.username setText:alternateMessage];
                                  [weakSelf.username markAsValid];
                                  [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonPopUpOkay withContext:RBXAContextSignup];
                              }
                                      onCancel:^
                              {
                                  [weakSelf.username markAsInvalid];
                                  [weakSelf.username showError:validMessage];
                                  [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonPopUpCancel withContext:RBXAContextSignup];
                              }];
                         }
                         else
                         {
                             [weakSelf.username markAsInvalid];
                             [weakSelf.username showError:validMessage];
                         }
                     }];
                }
                else
                {
                    [weakSelf.username markAsInvalid];
                    [weakSelf.username showError:validMessage];
                }
            }
        }];
    }];
    
    //NSLocalizedString([RobloxInfo thisDeviceIsATablet] ? @"PasswordRequirements" : @"PasswordRequirementsShort"
    [_password setTitle:NSLocalizedString(@"PasswordWord", nil)];
    [_password setHint:NSLocalizedString(@"PasswordRequirements", nil)];
    [_password setNextResponder:_verify];
    [_password setProtectedTextEntry:YES];
    [_password setValidationBlock:^{
        [[SignupVerifier sharedInstance] checkIfValidPassword:weakSelf.password.text
                                                 withUsername:weakSelf.username.text
                                                   completion:^(BOOL success, NSString *validMessage)
        {
            if (success)
                [weakSelf.password markAsValid];
            else
            {
                [weakSelf.password markAsInvalid];
                [weakSelf.password showError:validMessage];
            }
        }];
        
        if (weakSelf.verify.text.length > 0)
        {
            [[SignupVerifier sharedInstance] checkIfPasswordsMatch:weakSelf.password.text
                                                  withVerification:weakSelf.verify.text
                                                        completion:^(BOOL passwordsMatch, NSString *matchMessage)
            {
                if (passwordsMatch)
                    [weakSelf.verify markAsValid];
                else
                {
                    [weakSelf.verify markAsInvalid];
                    [weakSelf.verify showError:matchMessage];
                }
            }];
        }
    }];
    
    [_verify setTitle:NSLocalizedString(@"VerifyWord", nil)];
    [_verify setNextResponder:_email];
    [_verify setProtectedTextEntry:YES];
    [_verify setValidationBlock:^{
        [[SignupVerifier sharedInstance] checkIfPasswordsMatch:weakSelf.password.text
                                              withVerification:weakSelf.verify.text
                                                    completion:^(BOOL passwordsMatch, NSString *message)
        {
            if (passwordsMatch)
                [weakSelf.verify markAsValid];
            else
            {
                [weakSelf.verify markAsInvalid];
                [weakSelf.verify showError:message];
            }
        }];
    }];
    
    if (_email)
    {
        [_email setTitle:NSLocalizedString(@"EmailWord", nil)];
        [_email setHint:NSLocalizedString(@"EmailRequirements", nil)];
        [_email setNextResponder:nil];
        [_email setKeyboardType:UIKeyboardTypeEmailAddress];
        [_email setValidationBlock:^{
            [[SignupVerifier sharedInstance] checkIfValidEmail:weakSelf.email.text
                                                    completion:^(BOOL success, NSString *message)
             {
                 if (success)
                     [weakSelf.email markAsValid];
                 else
                 {
                     [weakSelf.email markAsInvalid];
                     [weakSelf.email showError:message];
                 }
             }];
        }];
    }
    
    [_gender setTouchBlock:^{
        [weakSelf resignAllResponders];
    }];
    
    [_birthday setValidationBlock:^{
        [weakSelf resignAllResponders];
        
        //check locally if the user is under 13
        if ([weakSelf.birthday userUnder13])
        {
            //check if the user has already entered anything in the email field
            if ([weakSelf.email.text length] > 0)
            {
                NSString* confirmMessage = [NSString stringWithFormat:NSLocalizedString(@"ConfirmEmailUnder13Message", nil), weakSelf.email.text];
                UIAlertView* confirm = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"ConfirmEmailUnder13",nil)
                                                                  message:confirmMessage
                                                                 delegate:nil
                                                        cancelButtonTitle:NSLocalizedString(@"NoWord", nil)
                                                        otherButtonTitles:NSLocalizedString(@"YesWord", nil), nil];
                [confirm showWithCompletion:^(UIAlertView *alertView, NSInteger buttonIndex)
                {
                    if (![alertView isEqual:confirm])
                        return;
    
                    //NO - remove the provided email
                    if (buttonIndex == 0)
                    {
                        [weakSelf.email setText:@""];
                        [weakSelf.birthday resignFirstResponder];
                        [weakSelf.email setTitle:NSLocalizedString(@"EmailUnder13Word", nil)];
                        [weakSelf.email forceUpdate];
                        return;
                    }
    
                    //YES - Do Nothing
                    if (buttonIndex == 1)
                        return;
                }];
            }
            else
            {
                [weakSelf.email setTitle:NSLocalizedString(@"EmailUnder13Word", nil)];
            }
        }
        else
        {
            [weakSelf.email setTitle:NSLocalizedString(@"EmailWord", nil)];
        }
    }];
    
    
    //Localize some strings
    [_titleLabel setText:[[NSLocalizedString(@"SignupWord", nil) uppercaseString] stringByReplacingOccurrencesOfString:@" " withString:@""]];
    
    [_btnSignup setTitle:NSLocalizedString(@"SignupWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalSubmitButton:_btnSignup];
    
    [_btnCancel setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalCancelButton:_btnCancel];
    
    if (_lblOr)
    {
        [_lblOr setText:NSLocalizedString(@"OrWord", nil)];
        [_lblOr setFont:[RobloxTheme fontBodySmall]];
        [_lblOr setTextColor:[RobloxTheme colorGray2]];
    }
    
    if (_btnSocial)
    {
        [_btnSocial setTitle:NSLocalizedString(@"SignInSocialWord", nil) forState:UIControlStateNormal];
        [RobloxTheme applyToFacebookButton:_btnSocial];
    }
    
    [loginButton setTitle:NSLocalizedString(@"LoginWord", nil) forState:UIControlStateNormal];
    [loginButton.titleLabel setFont:[RobloxTheme fontBody]];
    [loginButton setHidden:NO];
}

- (BOOL)disablesAutomaticKeyboardDismissal
{
    return NO;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextSignup];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_finePrint stopLoading];
}

- (void) viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    BOOL isPreiOS8 = NSFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_8_0;
    if (isPreiOS8 && [RobloxInfo thisDeviceIsATablet])
        self.view.superview.bounds = DEFAULT_VIEW_SIZE;
    
    if (_btnSocial)
        [RobloxTheme applyToFacebookButton:_btnSocial];
    
}

- (IBAction)closeController:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextSignup];
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([segue.identifier isEqualToString:@"FinePrintSegue"])
    {
        TermsAgreementController *controller = (TermsAgreementController *)segue.destinationViewController;
        controller.url = sender;
    }
}

- (IBAction)didPressLogin:(id)sender
{
    [Flurry logEvent:SUSC_didPressLogin];
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonLogin withContext:RBXAContextSignup];
    
    UIViewController *presenter = self.presentingViewController;
    UIStoryboard* storyboard = [UIStoryboard storyboardWithName:[RobloxInfo getStoryboardName] bundle:nil];
    LoginScreenController* controller = (LoginScreenController*)[storyboard instantiateViewControllerWithIdentifier:@"LoginScreenController"];
    
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
}

- (IBAction)didPressGigyaSignup:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSocialSignIn withContext:RBXAContextSignup];
    [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"AuthenticatingWithFacebookWord", nil)
                      dimBackground:YES];
    [[LoginManager sharedInstance] doSocialLoginFromController:self
                                                   forProvider:[LoginManager ProviderNameFacebook]
                                                withCompletion:^(bool success, NSString *message)
     {
         if (success)
         {
             if ([message isEqualToString:@"newUser"])
             {
                 dispatch_async(dispatch_get_main_queue(), ^{
                     UIViewController *presenter = self.presentingViewController;
                     [self dismissViewControllerAnimated:YES completion:^{
                         //Show social sign up controller
                         UIStoryboard* storyboard = [UIStoryboard storyboardWithName:[RobloxInfo getStoryboardName] bundle:nil];
                         SocialSignUpViewController* controller = [storyboard instantiateViewControllerWithIdentifier:@"SocialSignUpViewController"];
                         [controller setModalPresentationStyle:UIModalPresentationFormSheet];
                         [presenter presentViewController:controller animated:YES completion:nil];
                     }];
                 });
             }
         }
         else
         {
             if (message)
                 [RobloxAlert RobloxAlertWithMessage:message];
             [[LoginManager sharedInstance] doSocialLogout];
         }
     }];
    
}




////////////////////////////////////////////////////////////////
// Input fields
////////////////////////////////////////////////////////////////
- (void) resignAllResponders
{
    [self.view endEditing:YES];
    
    //RBValidTextField* activeField;
    //if      ([_username isEditing]) activeField = _username;
    //else if ([_password isEditing]) activeField = _password;
    //else if ([_verify   isEditing]) activeField = _verify;
    //else if ([_email    isEditing]) activeField = _email;
    //
    //if (activeField)
    //    [activeField resignFirstResponder];
}


- (IBAction)createAccount:(id)sender
{
    [Flurry logEvent:SUSC_createAccountSelected];
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSubmit withContext:RBXAContextSignup];
    //[self resignAllResponders];
    
    //do signup checks
    [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"SigningUp", nil) dimBackground:YES];
    
    ////////////////////////////////////////////////////////////////////
    //check the bools
    bool canLogin = YES; //Why is this called canLogin and not canSignup?
    
    if (!_username.isValidated)
    {
        canLogin = NO;
        if (_username.text.length == 0)
        {
            [_username showError:NSLocalizedString(@"UsernameMissing", nil)];
            [_username markAsInvalid];
        }
    }
    
    if (!_password.isValidated)
    {
        canLogin = NO;
        if (_password.text.length == 0)
        {
            [_password markAsInvalid];
            [_password showError:NSLocalizedString(@"PasswordMissing", nil)];
        }
    }

    if (!_verify.isValidated)
    {
        canLogin = NO;
        if (_verify.text.length == 0)
        {
            [_verify markAsInvalid];
            [_verify showError:NSLocalizedString(@"VerifyMissing", nil)];
        }
    }
    
    //with API proxy or Social signup, the email field may not exist
    if (_email && _email.text.length > 0 && !_email.isValidated)
    {
        canLogin = NO;
    }

    if (![_birthday isValid])
    {
        canLogin = NO;
        [_birthday markAsInvalid];
    }



    
    if (canLogin)
    {
        [self executeSignup];
    }
    else
    {
        [RobloxHUD hideSpinner:YES];
    }
}

- (void) executeSignup
{
    [[SignupVerifier sharedInstance] signUpWithUsername:self.username.text
                                               password:self.password.text
                                            birthString:self.birthday.playerBirthday
                                                 gender:self.gender.playerGender
                                                  email:(self.email ? self.email.text : @"")
                                        completionBlock:^(NSError *signUpError) {
                                            if ([RBXFunctions isEmpty:signUpError]) {
                                                //Successful sign up. Log the user into their new account
                                                [RBXFunctions dispatchOnMainThread:^{
                                                    
                                                    [UserInfo CurrentPlayer].username = _username.text;
                                                    [UserInfo CurrentPlayer].password = _password.text;
                                                    
                                                    [[LoginManager sharedInstance] loginWithUsername:_username.text password:_password.text completionBlock:^(NSError *loginError) {
                                                        [RobloxHUD hideSpinner:YES];
                                                        
                                                        if ([RBXFunctions isEmpty:loginError]) {
                                                            [RBXFunctions dispatchOnMainThread:^{
                                                                [RobloxHUD hideSpinner:NO];
                                                                [self dismissViewControllerAnimated:YES completion:^{
                                                                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_SIGNUP_COMPLETED object:self userInfo:nil];
                                                                }];
                                                            }];
                                                        } else {
                                                            [RBXFunctions dispatchOnMainThread:^{
                                                                [RobloxHUD hideSpinner:NO];
                                                            }];
                                                        }
                                                    }];
                                                }];
                                            } else {
                                                //Something failed when signing up
                                                [RBXFunctions dispatchOnMainThread:^{
                                                    [RobloxHUD hideSpinner:YES];
                                                    NSString *reason = signUpError.domain;
                                                    
                                                    if ([reason isEqualToString:NSLocalizedString(@"TooManyAttempts", nil)])
                                                    {
                                                        //open up a captcha so we can attempt to sign up again
                                                        NonRotatableNavigationController* navigation = [LoginManager CaptchaForSignupWithUsername:_username.text
                                                                                                                                  andV1Completion:^(bool success, NSString *message)
                                                        {
                                                            [RBXFunctions dispatchOnMainThread:^
                                                            {
                                                                if (success == YES)
                                                                {
                                                                    [self executeSignup];
                                                                }
                                                            }];
                                                        }
                                                                                                                                  andV2Completion:^(NSError *captchaError)
                                                        {
                                                            [RBXFunctions dispatchOnMainThread:^
                                                            {
                                                                if ([RBXFunctions isEmpty:captchaError])
                                                                {
                                                                    //captcha was successful, try again
                                                                    [self executeSignup];
                                                                }
                                                                else
                                                                {
                                                                    [RobloxHUD prompt:captchaError.domain withTitle:NSLocalizedString(@"ErrorWord", nil)];
                                                                }
                                                            }];
                                                        }];
                                                        [self presentViewController:navigation animated:YES completion:nil];
                                                    }
                                                    else
                                                        [RobloxAlert RobloxAlertWithMessage:reason];
                                                }];
                                            }
                                        }];
}

//Delegate functions
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    //NSLog(@"GestureRecognizer : %@", touch);
    UIView* touchedView = touch.view;
    if (touchedView == self.view  || touchedView == _gender || touchedView == _whiteView)
    {
        [self resignAllResponders];
        return  YES;
    }
    return NO;
}

////////////////////////////////////////////////////////////////
// Fine print
////////////////////////////////////////////////////////////////
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    NSString* urlRequestString = [[request URL] absoluteString];
    NSRange finePrintInit = [urlRequestString rangeOfString:@"file"];
    if(finePrintInit.location != NSNotFound)
        return YES;
    
    [self performSegueWithIdentifier:@"FinePrintSegue" sender:urlRequestString];
    //[[UIApplication sharedApplication] openURL:request.URL];
    
    return NO;
}

@end
