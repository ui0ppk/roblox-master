//
//  RBAccountManagerSocialViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/14/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import "RBAccountManagerSocialViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "RobloxAlert.h"
#import "RBActivityIndicatorView.h"
#import "UIView+Position.h"
#import "iOSSettingsService.h"
#import "RBXEventReporter.h"

DYNAMIC_FASTFLAGVARIABLE(EnableFacebookConnection, true);
DYNAMIC_FASTFLAGVARIABLE(EnableTwitterConnection, false);
DYNAMIC_FASTFLAGVARIABLE(EnableGooglePlusConnection, false);

@interface RBSocialLinkView ()
@property (nonatomic) BOOL isConnected;
@property (nonatomic, retain) NSString* providerID;

@property (nonatomic, retain) UILabel* lblSocialPlatformName;
@property (nonatomic, retain) UILabel* lblConnectedUserName;
@property (nonatomic, retain) UIButton* btnConnect;
@property (nonatomic, retain) UIButton* btnDisconnect;
@property (nonatomic, retain) RBActivityIndicatorView* spinner;

@property (weak, nonatomic) UIViewController* controllerRef;
@end

@implementation RBSocialLinkView

-(void) initWithSocialName:(NSString*)socialName
                  iconName:(NSString*)iconName
              providerName:(NSString*)providerName
        andRefToController:(UIViewController*)controllerRef
{
    _controllerRef = controllerRef;
    _providerID = providerName;
    
    _spinner = [[RBActivityIndicatorView alloc] init];
    [_spinner setHidden:YES];
    
    _lblConnectedUserName = [[UILabel alloc] init];
    [_lblConnectedUserName setText:@""];
    [_lblConnectedUserName setFont:[RobloxTheme fontBodySmall]];
    [_lblConnectedUserName setNumberOfLines:2];
    [_lblConnectedUserName setLineBreakMode:NSLineBreakByWordWrapping];
    [_lblConnectedUserName setTextAlignment:NSTextAlignmentCenter];
    
    _lblSocialPlatformName = [[UILabel alloc] init];
    [_lblSocialPlatformName setText:socialName];
    [_lblSocialPlatformName setFont:[RobloxTheme fontBody]];
    
    _btnConnect = [[UIButton alloc] init];
    [_btnConnect setTitle:NSLocalizedString(@"ConnectWord", nil) forState:UIControlStateNormal];
    //[_btnConnect setImage:[UIImage imageNamed:iconName] forState:UIControlStateNormal];
    [_btnConnect addTarget:self action:@selector(connect:) forControlEvents:UIControlEventTouchUpInside];
    [RobloxTheme applyToModalSubmitButton:_btnConnect];
    
    _btnDisconnect = [[UIButton alloc] init];
    [_btnDisconnect setTitle:NSLocalizedString(@"DisconnectWord", nil) forState:UIControlStateNormal];
    //[_btnDisconnect setImage:[UIImage imageNamed:iconName] forState:UIControlStateNormal];
    [_btnDisconnect addTarget:self action:@selector(disconnect:) forControlEvents:UIControlEventTouchUpInside];
    [RobloxTheme applyToModalCancelButton:_btnDisconnect];
    
    [self addSubview:_spinner];
    [self addSubview:_lblSocialPlatformName];
    [self addSubview:_lblConnectedUserName];
    [self addSubview:_btnConnect];
    [self addSubview:_btnDisconnect];
}

-(void) layoutSubviews
{
    [super layoutSubviews];
    
    CGSize cellThird = CGSizeMake(self.frame.size.width * 0.33, self.frame.size.height);
    
    [_lblSocialPlatformName setFrame:CGRectMake(0, 0, cellThird.width, cellThird.height)];
    [_lblConnectedUserName  setFrame:CGRectMake(cellThird.width * 2, 0, cellThird.width, cellThird.height)];
    
    [_btnConnect    setFrame:CGRectMake(cellThird.width * 1, 0, cellThird.width, cellThird.height)];
    [_btnDisconnect setFrame:CGRectMake(cellThird.width * 1, 0, cellThird.width, cellThird.height)];
    
    
    //float imgHeight = cellThird.height * 0.25;
    //int margin = 5;
    //[_btnConnect setImageEdgeInsets:UIEdgeInsetsMake(imgHeight, margin, imgHeight, cellThird.width - (imgHeight + margin))];
    //[_btnConnect setTitleEdgeInsets:UIEdgeInsetsMake(0, imgHeight + (margin * 2), 0, margin)];
    
    [_spinner setFrame:CGRectMake(self.center.x - (cellThird.height * 0.5), 0, cellThird.height, cellThird.height)];
}

-(void) setIsConnected:(BOOL)connectedToIdentity {
    _isConnected = connectedToIdentity;
    
    [_btnConnect setHidden:_isConnected];
    [_btnDisconnect setHidden:!_isConnected];
    [_lblConnectedUserName setHidden:!_isConnected];
    
    [_spinner setHidden:YES];
    
    if (_isConnected)
    {
        NSString* identity = [[UserInfo CurrentPlayer] getNameConnectedToIdentity:_providerID];
        [_lblConnectedUserName setText:identity ? identity : @""];
    }
    else
        [_lblConnectedUserName setText:@""];
}

//button actions
-(void) connect:(id)sender      {
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonConnect withContext:RBXAContextSettingsSocial withCustomDataString:_providerID];
    
    [_btnConnect setHidden:YES];
    [_btnDisconnect setHidden:YES];
    [_spinner setHidden:NO];
    [_spinner startAnimating];
    
    
    //refresh the userInfo first
    try
    {
        [[LoginManager sharedInstance] doSocialFetchGigyaInfoWithUID:[[UserInfo CurrentPlayer] GigyaUID]
                                                         isLoggingIn:NO
                                                      withCompletion:^(bool success, NSString *message)
         {
             //an optimization is to check if they are already connected, then bail out
             if ([[UserInfo CurrentPlayer] isConnectedToIdentity:_providerID])
             {
                 dispatch_async(dispatch_get_main_queue(), ^{
                     //They are already connected to something, refresh their settings for them
                     [_spinner stopAnimating];
                     [_spinner setHidden:YES];
                     [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"SocialConnectErrorAlreadyConnected", nil)];
                     [self setIsConnected:YES];
                 });
                 return;
             }
             
             
             //now connect it
            [[LoginManager sharedInstance] doSocialConnect:_controllerRef
                                                toProvider:_providerID
                                            withCompletion:^(bool success, NSString *message)
             {
                dispatch_async(dispatch_get_main_queue(), ^
                 {
                     [_spinner stopAnimating];
                     [_spinner setHidden:YES];
                     
                     if (success)
                     {
                         [self setIsConnected:YES];
                         NSString* successMessage = [NSString stringWithFormat:NSLocalizedString(@"YouAreNowConnectedPhrase", nil),_providerID];
                         [RobloxAlert RobloxAlertWithMessage:successMessage];
                     }
                     else
                     {
                         if (message)
                             [RobloxAlert RobloxAlertWithMessage:message];
                         [self setIsConnected:NO];
                     }
                     
                });
            }];
         }];
    }
    catch (NSException* exception)
    {
        //catch any weird last second exceptions because Gigya is awful
        dispatch_async(dispatch_get_main_queue(), ^
       {
           [_spinner stopAnimating];
           [_spinner setHidden:YES];
           [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"SocialConnectErrorUnexpectedError", nil)];
           [self setIsConnected:NO];
       });
    }
}
-(void) disconnect:(id)sender   {
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonDisconnect withContext:RBXAContextSettingsSocial withCustomDataString:_providerID];
    
    [_btnConnect setHidden:YES];
    [_btnDisconnect setHidden:YES];
    [_spinner setHidden:NO];
    [_spinner startAnimating];
    
    //refresh the userInfo first
    try
    {
        [[LoginManager sharedInstance] doSocialFetchGigyaInfoWithUID:[[UserInfo CurrentPlayer] GigyaUID]
                                                         isLoggingIn:NO
                                                      withCompletion:^(bool success, NSString *message)
        {
            //an optimization is to check if they are already disconnected, then bail out
            if ([[UserInfo CurrentPlayer] isConnectedToIdentity:_providerID] == false)
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [_spinner stopAnimating];
                    [_spinner setHidden:YES];
                    [self setIsConnected:NO];
                });
                return;
            }
            
            //now disconnect from everything
            [[LoginManager sharedInstance] doSocialDisconnect:_providerID
                                               withCompletion:^(bool success, NSString *message)
             {
                 dispatch_async(dispatch_get_main_queue(), ^
                {
                    [_spinner stopAnimating];
                    [_spinner setHidden:YES];
                    
                    if (success)
                        [self setIsConnected:NO];
                    else
                    {
                        if (message)
                            [RobloxAlert RobloxAlertWithMessage:message];
                        [self setIsConnected:YES];
                    }
                });
             }];
        }];
        
    }
    catch (NSException* exception)
    {
        //catch any weird last second exceptions because Gigya is awful
        dispatch_async(dispatch_get_main_queue(), ^
                       {
                           [_spinner stopAnimating];
                           [_spinner setHidden:YES];
                           [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"SocialConnectErrorUnexpectedError", nil)];
                           [self setIsConnected:NO];
                       });
    }
}

@end




@interface RBAccountManagerSocialViewController()
@property (nonatomic, retain) RBActivityIndicatorView* loadingSpinner;

@end

@implementation RBAccountManagerSocialViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextSettingsSocial];
    
    [self initializeUIElements];
    
    //Refresh the user's account info
    [_loadingSpinner startAnimating];
    [_loadingSpinner setHidden:NO];
    
    [self fetchSocialInformationWithCallback:^(bool success, NSString *message)
     {
        dispatch_async(dispatch_get_main_queue(), ^{
            [_loadingSpinner stopAnimating];
            [_loadingSpinner setHidden:YES];
            
            if (success)
            {
                [_rbsFacebook setIsConnected:[[UserInfo CurrentPlayer] isConnectedToFacebook]];
                [_rbsTwitter setIsConnected:[[UserInfo CurrentPlayer] isConnectedToTwitter]];
                [_rbsGPlus setIsConnected:[[UserInfo CurrentPlayer] isConnectedToGooglePlus]];
                
                [self toggleIdentityHidden:NO];
                [_lblWarning setHidden:YES];
            }
            else
            {
                [_lblWarning setHidden:NO];
                [self toggleIdentityHidden:YES];
            }
        });
    }];
    
}

#pragma mark Odd Functions
-(void) fetchSocialInformationWithCallback:(void(^)(bool success, NSString* message))handler
{
    NSString* gigyaUID = [[UserInfo CurrentPlayer] GigyaUID];
    if (gigyaUID)
    {
        //refresh all the user info and see if anything has changed from the server side
        [[LoginManager sharedInstance] doSocialFetchGigyaInfoWithUID:[[UserInfo CurrentPlayer] GigyaUID]
                                                         isLoggingIn:NO
                                                      withCompletion:handler];
    }
    else
    {
        //we clearly don't have ANY information at all, time to get it
        [[LoginManager sharedInstance] doSocialNotifyGigyaLoginWithContext:RBXAContextSettingsSocial withCompletion:handler];
    }
}


#pragma mark UI Functions
-(void) initializeUIElements
{
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [RobloxTheme applyShadowToView:_whiteView];
        
        _loadingSpinner = [[RBActivityIndicatorView alloc] initWithFrame:CGRectMake(_whiteView.center.x-16, _whiteView.center.y-16, 32, 32)];
    }
    else
    {
        _loadingSpinner = [[RBActivityIndicatorView alloc] initWithFrame:CGRectMake(self.view.center.x-16, self.view.center.y-16, 32, 32)];
    }
    
    [self.view addSubview:_loadingSpinner];
    [_loadingSpinner startAnimating];
    
    [_lblTitle setText:NSLocalizedString(@"SocialWord", nil)];
    [_lblWarning setText:NSLocalizedString(@"SocialErrorLoadingConnectionsPhrase", nil)];
    [_lblWarning setHidden:YES];
    
    //Buttons
    [_btnCancel setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalCancelButton:_btnCancel];
    [_btnUpdate setTitle:NSLocalizedString(@"RefreshWord", nil) forState:UIControlStateNormal];
    [RobloxTheme applyToModalSubmitButton:_btnUpdate];
    
    //Social Identities
    //Facebook
    [_rbsFacebook initWithSocialName:NSLocalizedString(@"FacebookWord", nil)
                            iconName:@"Social-FB-Icon"
                        providerName:[LoginManager ProviderNameFacebook]
                  andRefToController:self];
    [_rbsTwitter initWithSocialName:NSLocalizedString(@"TwitterWord", nil)
                           iconName:@"Social-Twitter-Icon"
                       providerName:[LoginManager ProviderNameTwitter]
                 andRefToController:self];
    [_rbsGPlus initWithSocialName:NSLocalizedString(@"GooglePlusWord", nil)
                         iconName:@"Social-Google-Icon"
                     providerName:[LoginManager ProviderNameGooglePlus]
               andRefToController:self];
    
    
    //Hide all buttons
    [self toggleIdentityHidden:YES];
}
-(void) toggleIdentityHidden:(BOOL)hidden
{
    if (!hidden)
    {
        //Only reveal buttons that the server says can be revealed
        [_rbsFacebook setHidden:DFFlag::EnableFacebookConnection    ? NO : YES];
        [_rbsTwitter  setHidden:DFFlag::EnableTwitterConnection     ? NO : YES];
        [_rbsGPlus    setHidden:DFFlag::EnableGooglePlusConnection  ? NO : YES];
    }
    else
    {
        [_rbsFacebook setHidden:YES];
        [_rbsTwitter setHidden:YES];
        [_rbsGPlus setHidden:YES];
    }
}



#pragma mark UI Actions
- (IBAction)updateInfo:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonRefresh withContext:RBXAContextSettingsSocial];
    
    [_loadingSpinner setHidden:NO];
    [_loadingSpinner startAnimating];
    [_loadingSpinner setY:(_btnUpdate.center.y - (_loadingSpinner.height * 0.5))];
    [_lblWarning setHidden:YES];
    
    //define a callback really quick
    void (^handler)(bool,NSString*) = ^(bool success, NSString* message)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [_loadingSpinner setHidden:YES];
            [_loadingSpinner stopAnimating];
            
            if (success)
            {
                [_rbsFacebook setIsConnected:[[UserInfo CurrentPlayer] isConnectedToFacebook]];
                [_rbsTwitter setIsConnected:[[UserInfo CurrentPlayer] isConnectedToTwitter]];
                [_rbsGPlus setIsConnected:[[UserInfo CurrentPlayer] isConnectedToGooglePlus]];
                
                [self toggleIdentityHidden:NO];
                [_lblWarning setHidden:YES];
            }
            else
            {
                [self toggleIdentityHidden:YES];
                [_lblWarning setHidden:NO];
            }
        });
    };
    
    [self fetchSocialInformationWithCallback:handler];
}
- (IBAction)closeController:(id)sender
{
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextSettingsSocial];
    [self.navigationController popViewControllerAnimated:YES];
}


@end
