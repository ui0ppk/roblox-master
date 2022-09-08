//
//  RBAccountManagerViewController.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/17/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import "RBAccountManagerViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxHUD.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "UIView+Position.h"

#ifndef kCFCoreFoundationVersionNumber_iOS_8_0
    #define kCFCoreFoundationVersionNumber_iOS_8_0 1129.15
#endif
#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 540, 344)

@interface RBAccountManagerViewController ()

@end

@implementation RBAccountManagerViewController

//View Functions
- (void)viewDidLoad{
    [super viewDidLoad];
    
    [self.tableView setDelegate:self];
    self.view.backgroundColor = [UIColor whiteColor];
    
    //assign some localized strings
    self.navigationItem.title = NSLocalizedString(@"AccountSettingsWord", nil);
    
    //Cells
    [_lblSocial setText:NSLocalizedString(@"SocialWord", nil)];
    [_lblCellEmail setText:NSLocalizedString(@"EmailWord", nil)];
    [_lblWarningEmail setText:NSLocalizedString(@"MissingEmailWord", nil)];
    [_lblLogOut setText:NSLocalizedString(@"LogoutWord", nil)];
    
    //Warnings
    [_lblCellPassword setText:NSLocalizedString(@"PasswordWord", nil)];
    [_lblWarningPassword setText:NSLocalizedString(@"MissingPasswordWord", nil)];
    
    
    // Stylize elements
    if ([RobloxInfo thisDeviceIsATablet])
    {
        UIButton* close = [RobloxTheme applyCloseButtonToUINavigationItem:self.navigationItem];
        [close addTarget:self action:@selector(dismissView:) forControlEvents:UIControlEventTouchUpInside];
        
        [RobloxTheme applyToModalPopupNavBar:self.navigationController.navigationBar];

        _gestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(didTapOutside:)];
        [_gestureRecognizer setNumberOfTapsRequired:1];
        [_gestureRecognizer setCancelsTouchesInView:NO];
        [_gestureRecognizer setDelegate:self];
    }
    
    
    
    // This will remove extra separators from tableview
    self.tableView.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
}
- (void)viewWillAppear:(BOOL)animated{
    [super viewWillAppear:animated];
    
    bool showEmailAlert = NO;
    bool showPasswordAlert = NO;
    
    if ([UserInfo CurrentPlayer].accountNotifications)
    {
        showEmailAlert =    [UserInfo CurrentPlayer].accountNotifications.emailNotificationEnabled && [UserInfo CurrentPlayer].userEmail == nil;
        showPasswordAlert = [UserInfo CurrentPlayer].accountNotifications.passwordNotificationEnabled && [UserInfo CurrentPlayer].password == nil;
    }
    
    _imgWarningEmail.hidden = !showEmailAlert;
    _lblWarningEmail.hidden = !showEmailAlert;
    _imgWarningPassword.hidden = !showPasswordAlert;
    _lblWarningPassword.hidden = !showPasswordAlert;
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    if ([RobloxInfo thisDeviceIsATablet])
        [self.view.window addGestureRecognizer:_gestureRecognizer];
}
- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    if ([RobloxInfo thisDeviceIsATablet])
        [self.view.window removeGestureRecognizer:_gestureRecognizer];
}
- (void)viewWillLayoutSubviews{
    [super viewWillLayoutSubviews];
    
    BOOL isPreiOS8 = NSFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_8_0;
    if (isPreiOS8 && [RobloxInfo thisDeviceIsATablet])
    {
        self.navigationController.view.superview.bounds =  DEFAULT_VIEW_SIZE;
    }
    
    
    [self adjustLabelInCell:_lblSocial];
    [self adjustLabelInCell:_lblCellEmail];
    [self adjustLabelInCell:_lblCellPassword];
    [self adjustLabelInCell:_lblLogOut];
    
    [self adjustWarningInCell:_imgWarningEmail      withLabel:_lblWarningEmail];
    [self adjustWarningInCell:_imgWarningPassword   withLabel:_lblWarningPassword];
}


//Helper Functions
- (void) adjustLabelInCell:(UILabel*)aLabel {
    if (!aLabel)
        return;
    
    //move the cell down to the baseline of its parent view
    [aLabel setX:aLabel.superview.width * 0.1];
    [aLabel setWidth:aLabel.superview.width * 0.4];
    [aLabel setY:(aLabel.superview.superview.height * 0.5) - (aLabel.height * 0.5)];
    
    //style the label
    [aLabel setTextColor:[RobloxTheme colorGray2]];
}
- (void) adjustWarningInCell:(UIImageView*)anImage withLabel:(UILabel*)aLabel {
    if (aLabel)
    {
        [aLabel setSize:CGSizeMake(90, aLabel.superview.height)];
        [aLabel setRight:aLabel.superview.width];
        [aLabel setY:0];
        [aLabel setTextColor:[RobloxTheme colorRed3]];
    }
    if (anImage)
    {
        //move the cell down to the baseline of its parent view
        [anImage setSize:CGSizeMake(22, 22)];
        [anImage setY:(anImage.superview.superview.height * 0.5) - (anImage.height * 0.5)];
        [anImage setRight:aLabel.x - 10];
    }
    
    
    
}

- (void)dismissView:(id)sender { [self dismissViewControllerAnimated:YES completion:nil]; }

//Tap Recognition
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer { return YES; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch { return YES; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer { return YES; }
- (void) didTapOutside:(UIGestureRecognizer*)sender
{
    //check the position of the tap and dismiss the view if it lies outside the bounds of the view
    if (sender.state == UIGestureRecognizerStateEnded)
    {
        UIView* root = self.view.window.rootViewController.view;
        CGPoint location = [sender locationInView:root];
        location = [self.view convertPoint:location fromView:root];
        if (![self.view pointInside:location withEvent:nil])
        {
            [self dismissViewControllerAnimated:YES completion:nil];
        }
    }
}


//Delegate Functions
- (CGFloat)tableView:(nonnull UITableView *)tableView heightForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    if (![[LoginManager sharedInstance] isFacebookEnabled])
        if (indexPath.row == 0 && indexPath.section == 0)
            return 0.0;
    
    return 60.0;
}
-(void) tableView:(nonnull UITableView *)tableView willDisplayCell:(nonnull UITableViewCell *)cell forRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    if (![[LoginManager sharedInstance] isFacebookEnabled])
        if (indexPath.row == 0 && indexPath.section == 0)
        {
            [cell setHidden:YES];
        }
}

@end