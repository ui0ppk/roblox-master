//
//  TestAccountSigninView.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 7/22/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TestAccountSigninViewController : UIViewController
{
    
}

@property (retain, nonatomic) IBOutlet UIView       *loggingInView;
@property (retain, nonatomic) IBOutlet UITextField  *username;
@property (retain, nonatomic) IBOutlet UITextField  *password;
@property (retain, nonatomic) IBOutlet UIButton     *loginButton;
@property (retain, nonatomic) IBOutlet UIButton     *forgotPasswordButton;
@property (retain, nonatomic) IBOutlet UIImageView  *loadingSpinner;
@property (retain, nonatomic) IBOutlet UIBarButtonItem  *logoutButton;

- (IBAction) closeButtonPressed:(UIBarButtonItem *)sender;
- (IBAction) logoutButtonPressed:(UIBarButtonItem *)sender;
- (IBAction) loginButtonPressed:(UIButton *)sender;

- (IBAction) usernameDidEndOnExit:(UITextField *)sender;
- (IBAction) passwordDidEndOnExit:(UITextField *)sender;

@end
