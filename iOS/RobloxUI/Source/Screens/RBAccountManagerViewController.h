//
//  RBAccountManagerViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

//-----------------------Navigation View Controller--------------------------------
@interface RBAccountManagerViewController : UITableViewController <UIGestureRecognizerDelegate, UITableViewDelegate>

//individual cell labels - because Static cells don't use prototypes
@property IBOutlet UILabel* lblCellPassword;
@property IBOutlet UILabel* lblCellEmail;
@property IBOutlet UILabel* lblCellUpgrade;
@property IBOutlet UILabel* lblSocial;
@property IBOutlet UILabel* lblLogOut;


//Warnings
@property IBOutlet UIImageView* imgWarningEmail;
@property IBOutlet UIImageView* imgWarningPassword;
@property IBOutlet UILabel* lblWarningEmail;
@property IBOutlet UILabel* lblWarningPassword;

@property UITapGestureRecognizer* gestureRecognizer;
- (void)didTapOutside:(UIGestureRecognizer*)sender;
- (IBAction)dismissView:(id)sender;
@end