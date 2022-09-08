//
//  WelcomeScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/20/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "WelcomeScreenController.h"
#import "RobloxInfo.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxMemoryManager.h"
#import "LoginManager.h"
#import "RobloxNotifications.h"
#import "RobloxTheme.h"
#import "RobloxAlert.h"
#import "Flurry.h"
#import "UserInfo.h"
#import "RBActivityIndicatorView.h"
#import "TermsAgreementController.h"
#import "ABTestManager.h"
#import "RBXEventReporter.h"
#import "NonRotatableNavigationController.h"
#import "NSDictionary+Parsing.h"
#import "SocialSignUpViewController.h"
#import "SignUpScreenController.h"
#import "RBXFunctions.h"
#import "LoginScreenController.h"
#import "LoginManager.h"

//---METRICS---
#define WSC_playNowPressed  @"WELCOME SCREEN - Play Now Pressed"
#define WSC_loginFinished   @"WELCOME SCREEN - Log In Finished"
#define WSC_signupFinished  @"WELCOME SCREEN - Sign Up Finished"

@implementation WelcomeScreenController
{
    IBOutlet UIButton *_loginButton;
    IBOutlet UIButton *_signUpButton;
    IBOutlet UIButton *_playNowButton;
    IBOutlet UIButton *_gigyaLogin;
    IBOutlet UILabel* _versionLabel;
    IBOutlet RBActivityIndicatorView* _loadingSpinner;
    IBOutlet UIPickerView *_environmentPicker;
    IBOutlet UIWebView *_finePrint;
    NSMutableArray *envs;
    
    BOOL _transitionedToGamesPage;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self initializeUIElements];
    
    //by-pass this screen if we can help it
    if ([[ABTestManager sharedInstance] IsInTestMobileGuestMode])
    {
#ifndef RBX_INTERNAL
        [self goToGamesPage];
        return;
#endif
    }
    else
    {
        //report to the server that the app is loaded and ready to go
        [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextLanding];
    }
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [_loadingSpinner stopAnimating];
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];

    [self hideUIButtons];
    //[_loadingSpinner startAnimating];
    
    _transitionedToGamesPage = NO;
    
    //auto log in will set the CurrentPlayer to be logged in if it worked
    if ([[UserInfo CurrentPlayer] userLoggedIn])
    {
        [self goToGamesPage];
    }
    else
    {
        // Subscribe to notifications
        [self addListeners:nil];
        
        [_loadingSpinner stopAnimating];
        [self revealUIButtons];
    }
}

- (void) viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    if (_gigyaLogin)
        [RobloxTheme applyToFacebookButton:_gigyaLogin];
}
////////////////////////////////////////////////////////////////
// UI Functions
////////////////////////////////////////////////////////////////
- (void) initializeUIElements
{
    //Environment picker for the internal view
#ifndef RBX_INTERNAL
    _environmentPicker.hidden = true;
#else
    [self populateEnvironmentPicker];
    _environmentPicker.dataSource = self;
    _environmentPicker.delegate = self;
    
    //FOR FACEBOOK SIGNUP DEBUGGING
    [_environmentPicker selectRow:7 inComponent:0 animated:NO];
#endif
    
    // Format buttons
    [_loginButton setTitle:NSLocalizedString(@"LoginWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToWelcomeLoginButton:_loginButton];
    
    [_signUpButton setTitle:NSLocalizedString(@"SignupWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToWelcomeLoginButton:_signUpButton];
    
    [_playNowButton setTitle:NSLocalizedString(@"PlayNowButtonLabel", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToWelcomeLoginButton:_playNowButton];
    
    [_gigyaLogin setTitle:NSLocalizedString(@"SignInSocialWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToFacebookButton:_gigyaLogin];
    if (![[LoginManager sharedInstance] isFacebookEnabled])
        [_gigyaLogin setHidden:YES];
    
    //Set the version number
    NSString *version = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    if ([version length])
        _versionLabel.text = version;
    else
        [_versionLabel setHidden:YES];
    
    
    // Fine print
    NSString *htmlFile = [[NSBundle mainBundle] pathForResource:@"SignUpDisclamer" ofType:@"html" inDirectory:nil];
    if(htmlFile)
    {
        NSString* htmlContents = [NSString stringWithContentsOfFile:htmlFile encoding:NSUTF8StringEncoding error:NULL];
        htmlContents = [htmlContents stringByReplacingOccurrencesOfString:@"textPlaceholder" withString:NSLocalizedString(@"HomeFinePrintWords", nil)];
        [_finePrint setDelegate:self];
        [_finePrint loadData:[htmlContents dataUsingEncoding:NSUTF8StringEncoding] MIMEType:@"text/html" textEncodingName:@"UTF-8" baseURL:[NSURL URLWithString:@""]];
        [_finePrint setBackgroundColor:[UIColor clearColor]];
        [_finePrint setOpaque:NO];
    }
}

- (void) hideUIButtons
{
    _loginButton.hidden = YES;
    _signUpButton.hidden = YES;
    _playNowButton.hidden = YES;
    _gigyaLogin.hidden = YES;
}

- (void) revealUIButtons
{
    [RBXFunctions dispatchOnMainThread:^
    {
        float transitionTime = 1.0;
        _loginButton.hidden = NO;   _loginButton.alpha = 0.0;
        _signUpButton.hidden = NO;  _signUpButton.alpha = 0.0;
        _playNowButton.hidden = NO; _playNowButton.alpha = 0.0;
        
        if ([[LoginManager sharedInstance] isFacebookEnabled])
        {
            _gigyaLogin.hidden = NO; _gigyaLogin.alpha = 0.0;
        }
        
        [UIView animateWithDuration:transitionTime
                         animations:^
         {
             _loginButton.alpha = 1.0;
             _signUpButton.alpha = 1.0;
             _playNowButton.alpha = 1.0;
             _gigyaLogin.alpha = 1.0;
         }];
    }];
}

- (IBAction)playNowTouchUpInside:(id)sender
{
    //NSLOG_PRETTY_FUNCTION;
    
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonPlayNow withContext:RBXAContextLanding];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Login/GuestMode"];
    
    [self goToGamesPage];
}

-(IBAction)loginTouchUpInside:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonLogin withContext:RBXAContextLanding];
}

-(IBAction)signupTouchUpInside:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSignup withContext:RBXAContextLanding];
    
    
    
    NSString* controllerName;
    if ([[LoginManager sharedInstance] isFacebookEnabled])
        controllerName = @"SignUpScreenControllerWithSocial";
    else if ([LoginManager apiProxyEnabled])
        controllerName = @"SignUpAPIScreenController";
    else
        controllerName = @"SignUpScreenController";
    
    UIStoryboard* storyboard = [UIStoryboard storyboardWithName:[RobloxInfo getStoryboardName] bundle:nil];
    SignUpScreenController* controller = (SignUpScreenController*)[storyboard instantiateViewControllerWithIdentifier:controllerName];
    controller.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:controller animated:YES completion:nil];
}

-(IBAction)aboutTouchUpInside:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonAbout withContext:RBXAContextLanding];
}

-(IBAction)gigyaTouchUpInside:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonSocialSignIn withContext:RBXAContextLanding];
    [self hideUIButtons];
    [[LoginManager sharedInstance] doSocialLoginFromController:self
                                                   forProvider:[LoginManager ProviderNameFacebook]
                                                withCompletion:^(bool success, NSString *message)
     {
         if (success)
         {
             if ([message isEqualToString:@"newUser"])
             {
                 dispatch_async(dispatch_get_main_queue(), ^{
                     //Show social sign up controller
                     UIStoryboard* storyboard = [UIStoryboard storyboardWithName:[RobloxInfo getStoryboardName] bundle:nil];
                     SocialSignUpViewController* controller = [storyboard instantiateViewControllerWithIdentifier:@"SocialSignUpViewController"];
                     [controller setModalPresentationStyle:UIModalPresentationFormSheet];
                     [self.navigationController presentViewController:controller animated:YES completion:nil];
                 });
             }
         }
         else
         {
             if (message)
                 [RobloxAlert RobloxAlertWithMessage:message];
             [[LoginManager sharedInstance] doSocialLogout];
         }
         dispatch_async(dispatch_get_main_queue(), ^{ [self revealUIButtons]; });
     }];
}


////////////////////////////////////////////////////////////////
// Login / segue functions
////////////////////////////////////////////////////////////////
- (void) addListeners:(NSNotification*) notification
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(signUpFinished:) name:RBX_NOTIFY_SIGNUP_COMPLETED object:nil];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGGED_OUT object:nil];
}

- (void) signUpFinished:(NSNotification*) notification
{
    [RBXFunctions dispatchOnMainThread:^{
        [Flurry logEvent:WSC_signupFinished];
        [self goToGamesPage];
    }];
}

- (void) goToGamesPage
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addListeners:) name:RBX_NOTIFY_LOGGED_OUT object:nil];
    
    if(!_transitionedToGamesPage)
    {
        _transitionedToGamesPage = YES;
        [RBXFunctions dispatchOnMainThread:^{ [self performSegueWithIdentifier:@"goToGamesPage" sender:nil]; }];
    }
}

////////////////////////////////////////////////////////////////
// Internal version's environment picker functions
////////////////////////////////////////////////////////////////
- (void)populateEnvironmentPicker
{
    envs = [[NSMutableArray alloc] init];
    BOOL iPad = [RobloxInfo thisDeviceIsATablet];
 //   NSString* m = (iPad) ? @"" : @"m.";
    NSString* www = (iPad) ? @"www." : @"m.";
    
    // PROD
    [envs addObject:[NSString stringWithFormat:@"http://%@roblox.com/", www]];
    
    // Client Test Env
    [envs addObject:[NSString stringWithFormat:@"http://%@gametest1.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@gametest2.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@gametest3.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@gametest4.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@gametest5.robloxlabs.com/", www]];
    
    // Web Test Env
    [envs addObject:[NSString stringWithFormat:@"http://%@sitetest1.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@sitetest2.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@sitetest3.robloxlabs.com/", www]];
    [envs addObject:[NSString stringWithFormat:@"http://%@sitetest4.robloxlabs.com/", www]];
    
    // Web Personal Test Env
    [envs addObject:[NSString stringWithFormat:@"http://akshay.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://alex.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://andrew.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://anthony.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://antoni.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://baker.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://ernie.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://guru.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://isaiah.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://linjun.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://linjunmobile.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://je.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://jeremy.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://manika.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://rosemary.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://shailendra.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://vlad.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://wooldridge.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://yunpeng.sitetest3.robloxlabs.com/"]];
    [envs addObject:[NSString stringWithFormat:@"http://ying.sitetest3.robloxlabs.com/"]];
}

- (NSInteger)pickerView:(UIPickerView *)thePickerView numberOfRowsInComponent:(NSInteger)component
{
    return envs.count;
}

// Number of columns
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)thePickerView
{
    return 1;
}

// This is where we link the data to the picker
- (NSString *)pickerView:(UIPickerView *)thePickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
    return [envs objectAtIndex:row];
}

// Updates the client settings with the new environment
- (void)pickerView:(UIPickerView *)thePickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
    [RobloxInfo setBaseUrl:[envs objectAtIndex:row]];
    
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW,0), ^{
        [[RobloxWebUtility sharedInstance] updateAllClientSettingsWithCompletion:^{
            // initalize the AB Test - this may take a bit
            [[ABTestManager sharedInstance] fetchExperimentsForBrowserTracker];
            [RBXFunctions dispatchOnMainThread:^{
                [self revealUIButtons];
            }];
        }];
    });
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [[RobloxMemoryManager sharedInstance] startMemoryBouncer];
    });
}


////////////////////////////////////////////////////////////////
// Fine print
////////////////////////////////////////////////////////////////
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    NSString* urlRequestString = [[request URL] absoluteString];
    NSRange finePrintInit = [urlRequestString rangeOfString:@"file"];
    if(finePrintInit.location != NSNotFound)
        return YES;
    
    [self performSegueWithIdentifier:@"FinePrintSegue" sender:urlRequestString];
    //[[UIApplication sharedApplication] openURL:request.URL];
    
    return NO;
}
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    //NSLog(@"Segue Identifier: %@", segue.identifier);
    
    if([segue.identifier isEqualToString:@"FinePrintSegue"])
    {
        TermsAgreementController *controller = (TermsAgreementController *)segue.destinationViewController;
        //NSLog(@"Controller: %d", controller != nil ? 1 : 0);
        controller.url = sender;
    }
    else if ([segue.identifier isEqualToString:@"LoginScreenSegue"])
    {
        //NSLog(@"modally displaying login screen controller");
        LoginScreenController *vc = (LoginScreenController *)segue.destinationViewController;
        vc.dismissalCompletionHandler = ^(LoginScreenDismissalType dismissType, NSError *loginError) {
            
            if (dismissType == LoginScreenDismissalLoginSuccess) {
                [RBXFunctions dispatchOnMainThread:^{
                    [Flurry logEvent:WSC_loginFinished];
                    [self goToGamesPage];
                }];
            } else {
                [RBXFunctions dispatchOnMainThread:^{
                    [self revealUIButtons];
                    [_loadingSpinner stopAnimating];
                }];
            }
        };
    }
}

@end
