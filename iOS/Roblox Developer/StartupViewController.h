//
//  StartupViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/13/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface StartupViewController : UIViewController
{
    int initialLogoConstant;
    int initialButtonConstant;
}

@property (retain, nonatomic) IBOutlet UIButton *settingsButton;
@property (retain, nonatomic) IBOutlet UIButton *connectToStudioButton;
@property (retain, nonatomic) IBOutlet UIButton *robloxMobileButton;
@property (retain, nonatomic) IBOutlet UIImageView *robloxLogo;
@property (retain, nonatomic) IBOutlet UILabel *developerLabel;
@property (retain, nonatomic) IBOutlet NSLayoutConstraint *verticalLogoConstraint;
@property (retain, nonatomic) IBOutlet NSLayoutConstraint *verticalButtonConstraint;

- (IBAction) connectButtonPressed:(UIButton *)sender;
- (IBAction) robloxMobileButtonPressed:(UIButton *)sender;

@end
