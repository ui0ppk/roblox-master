//
//  StudioPairViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 12/13/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "StudioPairViewController.h"
#import "StudioConnector.h"
#import "RobloxAlert.h"
#import "UIStyleConverter.h"
#import "PairTutorialDataController.h"
#import "StudioConnectionViewController.h"
#import "RobloxGoogleAnalytics.h"
#import "UIScreen+PortraitBounds.h"

#define PAIR_BUTTON_OFFSET 150
#define PAIR_BUTTON_ANIMATION_TIME 0.2

@interface StudioPairViewController ()

@end

@implementation StudioPairViewController

@synthesize shouldTryConnect;

- (id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        shouldTryConnect = YES;
        isVisible = NO;
    }
    
    return self;
}

-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.view.translatesAutoresizingMaskIntoConstraints = YES;
    
    [self.codeField setDelegate:self];
    [self localizeStrings];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(pairingDidEnd:)
                                                 name:[[StudioConnector sharedInstance] getPairEndedNotificationString]
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appDidEnterBackground:)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appDidBecomeActive:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    
    [UIStyleConverter convertToButtonBlueStyle:self.pairToStudioButton];
    [UIStyleConverter convertToBorderlessButtonStyle:self.pairToStudioHelpButton];
    [UIStyleConverter convertToLoadingStyle:self.loadingSpinner];
    [UIStyleConverter convertToTextFieldStyle:self.codeField];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [UIStyleConverter convertToNavigationBarStyle];
    }
    
    NSString* storedCode = [[NSUserDefaults standardUserDefaults] stringForKey:@"RbxPairCode"];
    if(storedCode && storedCode.length > 0)
    {
        [self.codeField setText:storedCode];
        [self.pairToStudioButton setAlpha:1];
        [self.pairToStudioButton setEnabled:YES];
    }
    else
    {
        [self.pairToStudioButton setAlpha:0.5];
        [self.pairToStudioButton setEnabled:NO];
    }
    
    // Setup pairing text to be bold for part of it
    UIFont *regularFont = [UIFont fontWithName:@"SourceSansPro-Regular" size:18.0f];
    UIFont *boldFont = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18.0f];
    UIColor *foregroundColor = [UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1];
    
    // Create the attributes
    NSDictionary *attrs = [NSDictionary dictionaryWithObjectsAndKeys:
                           regularFont, NSFontAttributeName,
                           foregroundColor, NSForegroundColorAttributeName, nil];
    NSDictionary *subAttrs = [NSDictionary dictionaryWithObjectsAndKeys:
                              boldFont, NSFontAttributeName, nil];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        self.pairingLabel.text = [self.pairingLabel.text stringByReplacingOccurrencesOfString:@"\n" withString:@""];
    }
    
    NSString* textTime = self.pairingLabel.text;
    const NSRange range = [textTime rangeOfString:@"ROBLOX Dev Code"];
    
    // Create the attributed string (text + attributes)
    NSMutableAttributedString *attributedText =
    [[NSMutableAttributedString alloc] initWithString:self.pairingLabel.text
                                           attributes:attrs];
    [attributedText setAttributes:subAttrs range:range];
    [self.pairingLabel setAttributedText:attributedText];
    
    self.enterPairCodeButton.titleLabel.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18.0f];
    [self.enterPairCodeButton setTitleColor:[UIColor colorWithRed:71.0f/255.0f green:71.0f/255.0f blue:71.0f/255.0f alpha:1.0f] forState:UIControlStateNormal];
    self.enterPairCodeButton.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
    {
        CGRect rect = [[UIScreen mainScreen] portraitBounds];
        if (rect.size.height > 480)
        {
            self.topMostConstraint.constant += 30;
            self.pairButtonConstraint.constant += 15;
            self.devCodeConstraint.constant += 20;
        }
    }
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Pair"];
    
    NSString* storedCode = [[NSUserDefaults standardUserDefaults] stringForKey:@"RbxPairCode"];
    if(storedCode && storedCode.length > 0)
    {
        [self.codeField setText:storedCode];
    }
    
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

-(void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    isVisible = YES;
}

-(void) viewDidDisappear:(BOOL)animated
{
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    isVisible = NO;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super viewDidDisappear:animated];
}

-(void) appDidBecomeActive:(NSNotification *) aNotification
{
    if (isVisible)
    {
        // todo: maybe we need that
    }
}

-(void) appDidEnterBackground:(NSNotification *) aNotification
{
    if (isVisible)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self stopPairing];
        });
    }
}

-(void) localizeStrings
{
    [self.codeField setPlaceholder:NSLocalizedString(@"EnterHerePhrase", nil)];
    [self.pairToStudioButton setTitle:NSLocalizedString(@"PairWord", nil) forState:UIControlStateNormal];
    [self.pairingLabel setText:NSLocalizedString(@"RbxDevPairingInstructions", nil)];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void) pairingDidEnd:(NSNotification *)aNotification
{
    if (self.navigationController.topViewController != self)
        return;
    
    NSDictionary* pairingEndedDict = aNotification.userInfo;
    if (pairingEndedDict)
    {
        NSString* didPair = [pairingEndedDict objectForKey:@"didPair"];
        if ([didPair boolValue]) // if we paired, show an alert indicating success
        {
            NSData* ipAddress = [pairingEndedDict objectForKey:@"ipData"];
            if (ipAddress && ipAddress.length > 0)
            {
                NSString* pairedAlertString = NSLocalizedString(@"RobloxDevSuccessfulPair", nil);
                
                NSString *host = nil;
                uint16_t port = 0;
                [GCDAsyncUdpSocket getHost:&host port:&port fromAddress:ipAddress];
                NSString* hostName = [[StudioConnector sharedInstance] hostnameForAddress:host];
                
                if (hostName && hostName.length > 0)
                {
                    pairedAlertString = [NSString stringWithFormat:pairedAlertString, hostName];
                }
                else if (NSString* ipString = [pairingEndedDict objectForKey:@"ipString"])
                {
                    pairedAlertString = [NSString stringWithFormat:pairedAlertString, ipString];
                }
                
                [RobloxAlert RobloxOKAlertWithMessageAndDelegate:pairedAlertString Delegate:self];
                [RobloxGoogleAnalytics setPageViewTracking:@"Pair/Success"];
            }
            else
            {
                [RobloxGoogleAnalytics setPageViewTracking:@"Pair/Fail/NoIP"];
            }
        }
        else // did not pair, let user know why
        {
            NSString* errorReason = [pairingEndedDict objectForKey:@"errorReason"];
            if (errorReason && errorReason.length > 0)
            {
                if ([errorReason isEqualToString:@"timeout"])
                {
                    [RobloxAlert RobloxOKAlertWithMessageAndDelegate:NSLocalizedString(@"RobloxDevPairFailureTimeout", nil) Delegate:self];
                    return;
                }
            }
            [RobloxGoogleAnalytics setPageViewTracking:@"Pair/Fail"];
        }
    }
}

-(void) stopPairingUI
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.loadingSpinner setHidden:YES];
        [self.pairingVignette setHidden:YES];
        [self.pairToStudioButton setTitle:NSLocalizedString(@"PairWord", nil) forState:UIControlStateNormal];
        [self.codeField setEnabled:YES];
    });
}

-(void) startPairingUI
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.loadingSpinner setHidden:NO];
        [self.pairingVignette setHidden:NO];
        [self.pairToStudioButton setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
        [self.codeField setEnabled:NO];
    });
}

-(void) startPairing
{
    [self startPairingUI];
    [[StudioConnector sharedInstance] tryToPairWithStudio:self.codeField.text];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Pair/Start"];
}

-(void) stopPairing
{
    [self stopPairingUI];
    [[StudioConnector sharedInstance] stopPairing];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Pair/Fail/UserPressedCancel"];
}

- (IBAction) pairWithStudio:(UIButton *)sender
{
    if ([self.pairToStudioButton.titleLabel.text isEqualToString:NSLocalizedString(@"PairWord", nil)])
    {
        [self startPairing];
    }
    else
    {
        [self stopPairing];
    }
}

- (IBAction) codeFieldDidEndOnExit:(UITextField *)sender
{
    shouldTryConnect = YES;
    [self pairWithStudio:nil];
}

- (IBAction) devCodeAreaPressed:(UIButton *) sender
{
    [self.codeField becomeFirstResponder];
}

// UITextFieldDelegate Methods
- (BOOL) textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    // only allow numbers to be input
    
    if (string.length == 0)
    {
        [self.pairToStudioButton setEnabled:NO];
        [self.pairToStudioButton setAlpha:0.5];
        
        return YES;
    }
    
    if ([string isEqualToString:@"\n"])
    {
        return YES;
    }
    
    if ( [textField.text length] >= 4 || NSEqualRanges([string rangeOfCharacterFromSet:[NSCharacterSet decimalDigitCharacterSet]],  NSMakeRange(NSNotFound, 0)) )
        return NO;
    
    if ( [textField.text length] >= 3)
    {
        [self.pairToStudioButton setEnabled:YES];
        [self.pairToStudioButton setAlpha:1];
    }
    else
    {
        
        [self.pairToStudioButton setEnabled:NO];
        [self.pairToStudioButton setAlpha:0.5];
    }
    
    return YES;
}

- (IBAction) helpButtonPressed:(UIButton *) sender
{
    PairTutorialDataController *pairTutorialDataController = [self.storyboard instantiateViewControllerWithIdentifier:@"PairTutorialDataController"];
    [self.navigationController pushViewController:pairTutorialDataController animated:YES];
}

- (IBAction) clearPairCode:(UIBarButtonItem *)sender
{
    [[StudioConnector sharedInstance] stopPairing];
    [[StudioConnector sharedInstance] clearPairCode];
    
    self.codeField.text = @"";
    
    shouldTryConnect = NO;
    [RobloxAlert RobloxOKAlertWithMessageAndDelegate:@"Dev Code removed from device" Delegate:self];
}

- (IBAction) closeButtonPressed:(UIButton *)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

// UIAlertViewDelegate
- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if ( buttonIndex == 0 )
    {
        // need to check alertView message against formatted success message to see if we failed pairing
        NSString* successfulPairMessage = NSLocalizedString(@"RobloxDevSuccessfulPair", nil);
        successfulPairMessage = [successfulPairMessage stringByReplacingOccurrencesOfString:@"%@" withString:@""];
        successfulPairMessage = [successfulPairMessage stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
        
        [self stopPairingUI];
        [[StudioConnector sharedInstance] stopPairing];
        
        if (shouldTryConnect) // pairing succeeded, go to connection view and dump this controller along the way (if we are in a mode that supports this)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                StudioConnectionViewController *studioConnectionViewController = [self.storyboard instantiateViewControllerWithIdentifier:@"StudioConnectionViewController"];
                UINavigationController *navigationController = self.navigationController;
                [navigationController popToRootViewControllerAnimated:NO];
                [navigationController pushViewController:studioConnectionViewController animated:YES];
            });
        }
    }
}

@end
