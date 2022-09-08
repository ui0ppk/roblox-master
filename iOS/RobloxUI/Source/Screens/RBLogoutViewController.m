//
//  RBLogoutViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBLogoutViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "LoginManager.h"
#import "ABTestManager.h"
#import "RBXEventReporter.h"
#import "UserInfo.h"

@interface RBLogoutViewController ()
@property IBOutlet UILabel *lblLogOutBody;
@property IBOutlet UIButton *btnCancel;
@property IBOutlet UIButton *btnLogOut;
@property IBOutlet UIImageView* imgFrown;
@property IBOutlet UIView* whiteView;
@property IBOutlet UILabel* lblTitle;

@end

@implementation RBLogoutViewController

-(void)viewDidLoad
{
    //configure the modal popup superclass
    [self shouldAddCloseButton:NO];
    [self shouldApplyModalTheme:NO];
    [super viewDidLoad];
    
    //NSString* titleText = NSLocalizedString(@"LogOutTitle", nil);
    [_lblTitle setText:NSLocalizedString(@"LogoutWord", nil)];
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [_whiteView.layer setShadowColor:[UIColor blackColor].CGColor];
        [_whiteView.layer setShadowOpacity:0.4];
        [_whiteView.layer setShadowRadius:2.0];
        [_whiteView.layer setShadowOffset:CGSizeMake(0.0, 0.5)];
        
        _lblLogOutBody.font = [RobloxTheme fontH4];
    }
    else
    {
        _lblLogOutBody.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:28];
    }
    
    
    
    _lblLogOutBody.text = NSLocalizedString(@"LogOutBody", nil);
    

    [_btnCancel setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalCancelButton:_btnCancel];
    
    [_btnLogOut setTitle:NSLocalizedString(@"LogOutTitle", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalSubmitButton:_btnLogOut];
    
    [_imgFrown.layer setShadowColor:[UIColor blackColor].CGColor];
    [_imgFrown.layer setShadowOpacity:0.2f];
    [_imgFrown.layer setShadowOffset:CGSizeMake(-1.0, 0.5)];
}
-(void)viewDidAppear:(BOOL)animated
{
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextLogout];
}

- (IBAction)onTouchUpInsideCancel:(id)sender {
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose
                                             withContext:RBXAContextLogout];
    [self.navigationController popViewControllerAnimated:YES];
}

- (IBAction)onTouchUpInsideLogout:(id)sender {
    [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
    [[LoginManager sharedInstance] logoutRobloxUser];
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        UINavigationController* pc = (UINavigationController*)self.presentingViewController;
        [self.navigationController dismissViewControllerAnimated:YES completion:^
        {
            if (![[ABTestManager sharedInstance] IsInTestMobileGuestMode])
                [pc popToViewController:pc.viewControllers[1] animated:YES];
            
            //the home screen controller will handle the log out notification
        }];
    }
    else
    {
        if (![[ABTestManager sharedInstance] IsInTestMobileGuestMode])
        {
            UINavigationController* baseNavigation = self.tabBarController.navigationController;
            [baseNavigation popToViewController:baseNavigation.viewControllers[1] animated:YES];
        }
        else
            [self.navigationController popViewControllerAnimated:YES];
    }
}


@end
