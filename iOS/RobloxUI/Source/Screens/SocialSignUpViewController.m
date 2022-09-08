//
//  SocialSignUpViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/10/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
 #import "SocialSignUpViewController.h"
#import "LoginManager.h"
#import "SignupVerifier.h"
#import "LoginManager.h"
#import "RBXEventReporter.h"
#import "RobloxHUD.h"
#import "RobloxInfo.h"
#import "RobloxTheme.h"
#import "UserInfo.h"
#import "RBActivityIndicatorView.h"
#import "UIView+Position.h"
#import "NSDictionary+Parsing.h"
#import "UIView+Position.h"
#import "RobloxAlert.h"
#import "NonRotatableNavigationController.h"
#import "RBCaptchaViewController.h"
#import "RBCaptchaV2ViewController.h"
#import "RBXFunctions.h"

#ifndef kCFCoreFoundationVersionNumber_iOS_8_0
    #define kCFCoreFoundationVersionNumber_iOS_8_0 1129.15
#endif

@interface SocialSignUpViewController ()
@property (nonatomic, strong) RBActivityIndicatorView* loadingSpinner;

@property (nonatomic, strong) NSString* photoURL;
@property (nonatomic, strong) NSString* username;
@property (nonatomic, strong) NSString* gender;
@property (nonatomic, strong) NSString* email;
@property (nonatomic, strong) NSString* birthday;
@property (nonatomic, strong) NSString* gigyaUID;

@property (nonatomic, strong) UITapGestureRecognizer* touches;

@end

@implementation SocialSignUpViewController

- (void) viewDidLoad {
    [super viewDidLoad];
    
    _touches = [[UITapGestureRecognizer alloc] init];
    [_touches setNumberOfTouchesRequired:1];
    [_touches setNumberOfTapsRequired:1];
    [_touches setDelegate:self];
    [_touches setEnabled:YES];
    [self.view addGestureRecognizer:_touches];
    
    
    self.username = [[UserInfo CurrentPlayer] GigyaName];
    self.email =    [[UserInfo CurrentPlayer] userEmail];
    self.gigyaUID = [[UserInfo CurrentPlayer] GigyaUID];
    self.gender =   [[UserInfo CurrentPlayer] GigyaGender];
    self.birthday = [[UserInfo CurrentPlayer] birthday];
    self.photoURL = [[UserInfo CurrentPlayer] GigyaPhotoURL];
    
    if (![RobloxInfo thisDeviceIsATablet])
        [RobloxTheme applyShadowToView:_whiteView];
    
    self.loadingSpinner = [[RBActivityIndicatorView alloc] initWithFrame:CGRectMake(0, 0, 30, 30)];
    [self.loadingSpinner setHidden:YES];
    [self.view addSubview:_loadingSpinner];
    [self.loadingSpinner startAnimating];
    
    //initialize the shadow on the container
    _shadowContainer.backgroundColor = [UIColor clearColor];
    [RobloxTheme applyShadowToView:_shadowContainer];
    [_shadowContainer.layer setShadowOffset:CGSizeMake(0.0, 5)];
    
    //localize the UI
    [_lblAlmostDone setText:[NSString stringWithFormat:@"%@, %@", self.username, [NSLocalizedString(@"YouAreAlmostDonePhrase", nil) lowercaseString]]];
    [_lblAlmostDone setAdjustsFontSizeToFitWidth:YES];
    [_lblAlmostDone setFont:[RobloxInfo thisDeviceIsATablet] ? [RobloxTheme fontH3] : [RobloxTheme fontBodyLarge]];
    [_lblAlmostDone setTextColor:[RobloxTheme colorGray1]];
    
    [_lblSelectUserName setText:NSLocalizedString(@"SelectUserNameWord", nil)];
    [_lblSelectUserName setFont:[RobloxInfo thisDeviceIsATablet] ? [RobloxTheme fontH3] : [RobloxTheme fontBodyLarge]];
    [_lblSelectUserName setTextColor:[RobloxTheme colorGray1]];
    
    [_lblAlreadyHaveAccount setText:NSLocalizedString(@"AlreadyHaveARobloxAccountPhrase", nil)];
    [_lblAlreadyHaveAccount setFont:[RobloxTheme fontBodySmall]];
    [_lblAlreadyHaveAccount setTextColor:[RobloxInfo thisDeviceIsATablet] ? [RobloxTheme colorGray2] : [RobloxTheme colorGray1]];
    
    [_lblNameDescription setText:NSLocalizedString(@"SelectUserNameDescriptionPhrase", nil)];
    [_lblNameDescription setFont:[RobloxInfo thisDeviceIsATablet] ? [RobloxTheme fontBody] : [RobloxTheme fontBodySmall]];
    [_lblNameDescription setTextColor:[RobloxTheme colorGray2]];
    
    [_btnAccept setTitle:NSLocalizedString(@"SignupWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalSubmitButton:_btnAccept];
    
    [_btnCancel setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalCancelButton:_btnCancel];
    
    
    __weak SocialSignUpViewController* weakSelf = self;
    [_txtUsername setTitle:NSLocalizedString(@"UsernameWord", nil)];
    [_txtUsername setHint:NSLocalizedString(@"UsernameRequirements", nil)];
    [_txtUsername setNextResponder:nil];
    [_txtUsername setValidationBlock:^{
        [[SignupVerifier sharedInstance] checkIfValidUsername:weakSelf.txtUsername.text completion:^(BOOL success, NSString *validMessage)
         {
             if (success)
                 [weakSelf.txtUsername markAsValid];
             else
             {
                 [weakSelf.txtUsername markAsInvalid];
                 
                 //handle bad username
                 if ([validMessage isEqualToString:NSLocalizedString(@"UsernameCommon", nil)])
                 {
                     [[SignupVerifier sharedInstance] getAlternateUsername:weakSelf.txtUsername.text
                                                                completion:^(BOOL success, NSString *alternateMessage)
                      {
                          if (success)
                          {
                              [RobloxHUD prompt:[NSString stringWithFormat:NSLocalizedString(@"UsernameTaken", nil), alternateMessage]
                                      withTitle:NSLocalizedString(@"UsernameTakenTitle", nil)
                                           onOK:^
                               {
                                   [weakSelf.txtUsername setText:alternateMessage];
                                   [weakSelf.txtUsername markAsValid];
                                   [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonPopUpOkay withContext:RBXAContextSocialSignup];
                               }
                                       onCancel:^
                               {
                                   [weakSelf.txtUsername markAsInvalid];
                                   [weakSelf.txtUsername showError:validMessage];
                                   [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonPopUpCancel withContext:RBXAContextSocialSignup];
                               }];
                          }
                          else
                          {
                              [weakSelf.txtUsername markAsInvalid];
                              [weakSelf.txtUsername showError:validMessage];
                          }
                      }];
                 }
                 else
                 {
                     [weakSelf.txtUsername markAsInvalid];
                     [weakSelf.txtUsername showError:validMessage];
                 }
             }
         }];
    }];
    
    
    //load the user's profile picture
    [_shadowContainer setHidden:YES];
    [_imgIdentity loadBadgeWithURL:self.photoURL
                          withSize:_imgIdentity.size
                        completion:^
     {
         dispatch_async(dispatch_get_main_queue(), ^{
             //stop animating the loading spinner
             [_loadingSpinner stopAnimating];
             [_loadingSpinner setHidden:YES];
             [_shadowContainer setHidden:NO];
         });
     }];
    
    //pre-load a username
    _txtUsername.hidden = YES;
    [[SignupVerifier sharedInstance] getAlternateUsername:self.username
                                               completion:^(BOOL success, NSString *message)
     {
         dispatch_async(dispatch_get_main_queue(), ^{
             _txtUsername.hidden = NO;
             if (success)
                 [_txtUsername setText:message];
         });
     }];
}
- (void) viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    
    BOOL isPreiOS8 = NSFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_8_0;
    if (isPreiOS8 && [RobloxInfo thisDeviceIsATablet])
    {
        self.view.superview.bounds =  CGRectMake(0, 0, 540, 540);
    }
    
    
    [self.loadingSpinner centerInFrame:_shadowContainer.frame];
    
    //make a circular image
    CGFloat centerX = self.view.frame.size.width * 0.5;
    CGFloat halfHeight = _shadowContainer.height * 0.5;
    [_shadowContainer setFrame:CGRectMake(centerX - halfHeight, _shadowContainer.y, _shadowContainer.height, _shadowContainer.height)];
    UIBezierPath* bpath = [UIBezierPath bezierPathWithRoundedRect:_imgIdentity.bounds cornerRadius:(_imgIdentity.width * 0.5)];
    CAShapeLayer* circularMask = [CAShapeLayer layer];
    circularMask.path = bpath.CGPath;
    [_imgIdentity.layer setMask:circularMask];
    
    _shadowContainer.layer.shadowPath = bpath.CGPath;
}

- (IBAction) createAccount:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSignup withContext:RBXAContextSocialSignup];
    
    [_btnAccept setHidden:YES];
    [_btnCancel setHidden:YES];
    [_loadingSpinner setHidden:NO];
    [_loadingSpinner startAnimating];
    
    [[LoginManager sharedInstance] doSocialSignupWithUsername:_txtUsername.text
                                                      gigyaID:self.gigyaUID
                                                     birthday:self.birthday
                                                       gender:self.gender
                                                        email:self.email.length > 0 ? self.email : nil
                                                   completion:^(bool success, NSString *message)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            [_btnAccept setHidden:NO];
            [_btnCancel setHidden:NO];
            [_loadingSpinner setHidden:YES];
            [_loadingSpinner stopAnimating];
            
            if (success)
            {
                //TODO : play welcome animation
                
                
                //dismiss the view and go into the app
                [self dismissViewControllerAnimated:YES completion:nil];
            }
            else
            {
                if ([message isEqualToString:NSLocalizedString(@"SocialSignupErrorCaptchaNeeded", nil)])
                {
                    //open up a captcha so we can attempt to sign up
                    NonRotatableNavigationController* navigation = [LoginManager CaptchaForSocialSignupWithUsername:_txtUsername.text
                                                                                                    andV1Completion:^(bool success, NSString *message)
                    {
                        [RBXFunctions dispatchOnMainThread:^
                         {
                             if (success)
                             {
                                 //successfully solved captcha, retry the account creation
                                 [self createAccount:nil];
                             }
                         }];
                    }
                                                                                                    andV2Completion:^(NSError *captchaError)
                    {
                        [RBXFunctions dispatchOnMainThread:^
                         {
                             if ([RBXFunctions isEmpty:captchaError])
                             {
                                 //successfully solved captcha, retry the account creation
                                 [self createAccount:nil];
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
                {
                    //otherwise print the message out to the user
                    [RobloxAlert RobloxAlertWithMessage:message];
                }
            }
        });
    }];
}
- (IBAction) cancelSignUp:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextSocialSignup];
    [[LoginManager sharedInstance] doSocialLogout];
    [self dismissViewControllerAnimated:YES completion:nil];
}

//Delegate functions
- (BOOL)disablesAutomaticKeyboardDismissal
{
    return NO;
}
- (void) resignAllResponders
{
    //[_txtUsername resignFirstResponder];
    [self.view endEditing:YES];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    //NSLog(@"GestureRecognizer : %@", touch);
    UIView* touchedView = touch.view;
    if (touchedView == self.view  || touchedView == _whiteView)
    {
        [self resignAllResponders];
        return  YES;
    }
    return NO;
}

@end
