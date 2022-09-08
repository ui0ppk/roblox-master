//
//  StudioPairViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/13/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "TcpViewController.h"

@interface StudioPairViewController : UIViewController <UITextFieldDelegate, UIAlertViewDelegate>
{
    BOOL shouldTryConnect;
    BOOL isVisible;
}

@property (nonatomic) BOOL shouldTryConnect;


@property (retain, nonatomic) IBOutlet UIButton *pairToStudioHelpButton;
@property (retain, nonatomic) IBOutlet UIButton *pairToStudioButton;
@property (retain, nonatomic) IBOutlet UILabel *pairingLabel;
@property (retain, nonatomic) IBOutlet UIButton *enterPairCodeButton;
@property (retain, nonatomic) IBOutlet UITextField *codeField;
@property (retain, nonatomic) IBOutlet UIImageView *loadingSpinner;
@property (retain, nonatomic) IBOutlet UIImageView *pairingVignette;
@property (retain, nonatomic) IBOutlet UIBarButtonItem *clearButton;

@property (retain, nonatomic) IBOutlet NSLayoutConstraint *topMostConstraint;
@property (retain, nonatomic) IBOutlet NSLayoutConstraint *devCodeConstraint;
@property (retain, nonatomic) IBOutlet NSLayoutConstraint *pairButtonConstraint;

- (IBAction) clearPairCode:(UIBarButtonItem *)sender;
- (IBAction) pairWithStudio:(UIButton *)sender;
- (IBAction) codeFieldDidEndOnExit:(UITextField *)sender;
- (IBAction) devCodeAreaPressed:(UIButton *) sender;
- (IBAction) helpButtonPressed:(UIButton *) sender;
- (IBAction) closeButtonPressed:(UIButton *)sender;

@end
