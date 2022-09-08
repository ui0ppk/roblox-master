//
//  TestAccountSigninView.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 7/22/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBXFunctions.h"
#import "TestAccountSigninViewController.h"
#import "LoginManager.h"
#import "UserInfo.h"
#import "RobloxAlert.h"
#import "PlaceLauncher.h"
#import "UIStyleConverter.h"
#import "RobloxInfo.h"
#import "ResetPasswordViewController.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxNotifications.h"

@interface TestAccountSigninViewController ()

@end

@implementation TestAccountSigninViewController

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self == [super initWithCoder:aDecoder])
    {
        // initialize here
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [UIStyleConverter convertToBoldTextFieldStyle:self.username];
    [UIStyleConverter convertToBoldTextFieldStyle:self.password];
    [UIStyleConverter convertToButtonBlueStyle:self.loginButton];
    [UIStyleConverter convertToBorderlessButtonStyle:self.forgotPasswordButton];
    [UIStyleConverter convertToLoadingStyle:self.loadingSpinner];

    self.username.placeholder = NSLocalizedString(@"UsernameWord", nil);
    self.password.placeholder = NSLocalizedString(@"PasswordWord", nil);
    
    [self.loginButton setTitle:NSLocalizedString(@"LoginWord", nil) forState:UIControlStateNormal];
    [self.loginButton setTitle:NSLocalizedString(@"LoginWord", nil) forState:UIControlStateSelected];
    [self.loginButton setTitle:NSLocalizedString(@"LoginWord", nil) forState:UIControlStateDisabled];
    
    [self.forgotPasswordButton setTitle:NSLocalizedString(@"Forgot Password?", nil) forState:UIControlStateNormal];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [UIStyleConverter convertToBlueNavigationBarStyle:self.navigationController.navigationBar];
    }
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Login"];
    
    self.username.text = [UserInfo CurrentPlayer].username;
    self.password.text = [UserInfo CurrentPlayer].password;
    
    // small color bug fix for iPad
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [self.logoutButton setTitleTextAttributes:
         [NSDictionary dictionaryWithObjectsAndKeys:
          [UIColor whiteColor], NSForegroundColorAttributeName,nil]
                                         forState:UIControlStateNormal];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction) closeButtonPressed:(UIBarButtonItem *)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction) logoutButtonPressed:(UIBarButtonItem *)sender
{
    [[LoginManager sharedInstance] logoutRobloxUser];    
    
    self.username.text = @"";
    self.password.text = @"";
    
    dispatch_async(dispatch_get_main_queue(), ^{
        self.loggingInView.hidden = true;
        [self.username resignFirstResponder];
        [self.password resignFirstResponder];
        [RobloxAlert RobloxOKAlertWithMessageAndDelegate:@"You have been logged out." Delegate:self];
    });

}

- (IBAction) loginButtonPressed:(UIButton *)sender
{
    [self doLogin];
}

- (IBAction) usernameDidEndOnExit:(UITextField *)sender
{
    [self.password becomeFirstResponder];
}

- (IBAction) passwordDidEndOnExit:(UITextField *)sender
{
    [self doLogin];
}

-(void) doLogin
{
    self.loggingInView.hidden = false;
    [[LoginManager sharedInstance] loginWithUsername:self.username.text password:self.password.text completionBlock:^(NSError *loginError) {
        if ([RBXFunctions isEmpty:loginError]) {
            // login successful
            [RBXFunctions dispatchOnMainThread:^{
                [RobloxGoogleAnalytics setPageViewTracking:@"Login/Success"];
                
                self.loggingInView.hidden = true;
                
                if (self.username.isFirstResponder)
                {
                    [self.username resignFirstResponder];
                }
                if (self.password.isFirstResponder)
                {
                    [self.password resignFirstResponder];
                }
                
                [RobloxAlert RobloxOKAlertWithMessageAndDelegate:@"Login Succedeed!" Delegate:self];
            }];
        } else {
            // login failure
            [RBXFunctions dispatchOnMainThread:^{
                [RobloxGoogleAnalytics setPageViewTracking:@"Login/Failure"];
                
                self.loggingInView.hidden = true;
                [RobloxAlert RobloxAlertWithMessage:loginError.domain];
            }];
        }
    }];

    [RobloxGoogleAnalytics setPageViewTracking:@"Login/Try"];
}

@end
