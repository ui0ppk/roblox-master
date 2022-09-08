//
//  StartupViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/13/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "StartupViewController.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "AppDelegate.h"
#import "UIStyleConverter.h"
#import "StudioPairViewController.h"
#import "StudioConnectionViewController.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxInfo.h"

@interface StartupViewController ()

@end

@implementation StartupViewController

- (id) initWithCoder:(NSCoder*) decoder
{
    self = [super initWithCoder:decoder];
    if (self)
    {
        [UIStyleConverter convertToNavigationBarStyle];
        [UIStyleConverter convertToPagingStyle];
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [UIStyleConverter convertToTexturedBackgroundStyle:self.view];
    [UIStyleConverter convertToButtonStyle:self.connectToStudioButton];
    [UIStyleConverter convertToButtonStyle:self.settingsButton];
    [UIStyleConverter convertToHyperlinkStyle:self.robloxMobileButton];
    [UIStyleConverter convertToLargeLabelStyle:self.developerLabel];
    
    // if we have a saved account, sign in with it on startup
    if(![[UserInfo CurrentPlayer].username isEqual: @""] && ![[UserInfo CurrentPlayer].password isEqual: @""])
    {
        [[LoginManager sharedInstance] loginWithUsername:[UserInfo CurrentPlayer].username password:[UserInfo CurrentPlayer].password completionBlock:nil];
    }
    
    [RobloxGoogleAnalytics setEventTracking:@"DeviceType" withAction:[RobloxInfo friendlyDeviceName] withLabel:[RobloxInfo deviceOSVersion] withValue:0];
    
    initialLogoConstant = self.verticalLogoConstraint.constant;
    initialButtonConstant = self.verticalButtonConstraint.constant;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [self.navigationController setNavigationBarHidden:YES animated:YES];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    [[UIApplication sharedApplication] setStatusBarHidden:NO];
    
    [self setConstraints:UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication]statusBarOrientation])];
}

-(void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

-(void) viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self.navigationController setNavigationBarHidden:NO animated:animated];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    
    [super viewWillDisappear:animated];
}

- (IBAction) connectButtonPressed:(UIButton *)sender
{
    NSString* storedCode = [[NSUserDefaults standardUserDefaults] stringForKey:@"RbxPairCode"];
    if(!storedCode || storedCode.length <= 0)
    {
        StudioPairViewController *pairViewcontroller = (StudioPairViewController*) [self.storyboard instantiateViewControllerWithIdentifier:@"StudioPairViewController"];
        
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        {
            [self.navigationController pushViewController:pairViewcontroller animated:YES];
        }
        else
        {
            UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:pairViewcontroller];
            [navigation setModalPresentationStyle:UIModalPresentationFormSheet];
            [navigation setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
            
            [self presentViewController:navigation animated:YES completion:nil];
        }
    }
    else
    {
        StudioConnectionViewController *connectionViewController = (StudioConnectionViewController*) [self.storyboard instantiateViewControllerWithIdentifier:@"StudioConnectionViewController"];
        
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        {
            [self.navigationController pushViewController:connectionViewController animated:YES];
        }
        else
        {
            UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:connectionViewController];
            [navigation setModalPresentationStyle:UIModalPresentationFormSheet];
            [navigation setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
            
            [self presentViewController:navigation animated:YES completion:^{}];
        }
    }
}

- (IBAction) robloxMobileButtonPressed:(UIButton *)sender
{
    [RobloxGoogleAnalytics setPageViewTracking:@"StartupView/RobloxMobileButton"];
    NSString *iTunesLink = @"https://itunes.apple.com/us/app/roblox-mobile/id431946152?mt=8";
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:iTunesLink]];
}

- (void) setConstraints:(BOOL) isLandscape
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
    {
        if ( isLandscape )
        {
            self.verticalLogoConstraint.constant = 20;
            self.verticalButtonConstraint.constant = 20;
        }
        else
        {
            self.verticalLogoConstraint.constant = initialLogoConstant;
            self.verticalButtonConstraint.constant = initialButtonConstant;
        }
    }
}


// for iOS 7.0 and below
-(void)willRotateToInterfaceOrientation: (UIInterfaceOrientation)orientation duration:(NSTimeInterval)duration
{
    [super willRotateToInterfaceOrientation:orientation duration:duration];
    
    [self setConstraints:UIInterfaceOrientationIsLandscape(orientation)];

}

// for iOS 8.0 and above
- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    [self setConstraints:(size.width > size.height)];
}

-(NSUInteger)supportedInterfaceOrientations
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        return UIInterfaceOrientationMaskAll;
    else
        return UIInterfaceOrientationMaskLandscape;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        return (interfaceOrientation==UIInterfaceOrientationPortrait) || (interfaceOrientation==UIInterfaceOrientationPortraitUpsideDown) ||
            (interfaceOrientation==UIInterfaceOrientationLandscapeLeft) || (interfaceOrientation==UIInterfaceOrientationLandscapeRight);
    else
        return (interfaceOrientation==UIInterfaceOrientationLandscapeLeft) || (interfaceOrientation==UIInterfaceOrientationLandscapeRight);
}

@end
