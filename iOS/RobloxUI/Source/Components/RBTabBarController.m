//
//  RBTabBarController.m
//  RobloxMobile
//
//  Created by Ashish Jain on 12/8/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import "RBTabBarController.h"

#import "ABTestManager.h"
#import "Flurry.h"
#import "LoginManager.h"
#import "RBMobileWebViewController.h"
#import "RBMoreViewController.h"
#import "RBXFunctions.h"
#import "RobloxAlert.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "SignUpScreenController.h"
#import "UserInfo.h"

//---METRICS---
#define HSC_signUpControllerFromPlayNow @"HOME SCREEN - Sign Up Pressed While Guest"

@interface RBTabBarController () <UITabBarControllerDelegate>

@property (nonatomic) NSInteger selectedIndexLogin;

@end

@implementation RBTabBarController

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.delegate = self;
    
    NSDictionary *dictionary = [NSDictionary dictionaryWithObjectsAndKeys: [RobloxInfo getUserAgentString], @"UserAgent", nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:dictionary];
    
    
    //check if the user is logged in, if not make the Games Page the landing page
    if ([UserInfo CurrentPlayer].userLoggedIn == NO)
        [self setSelectedIndex:1];
    
    //set the images
    if ([RobloxInfo thisDeviceIsATablet])
    {
        [((UIViewController*)self.viewControllers[0]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Home Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[0]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Home On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[1]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Game Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[1]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Game On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[2]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Catalog Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[2]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Catalog On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[3]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Friends Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[3]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Friends On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[4]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Messages Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[4]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Messages On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[5]).tabBarItem setImage:[[UIImage imageNamed:@"Icon More Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[5]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon More On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
    }
    else
    {
        [((UIViewController*)self.viewControllers[0]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Home Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[0]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Home On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[1]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Game Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[1]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Game On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[2]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Friends Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[2]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Friends On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[3]).tabBarItem setImage:[[UIImage imageNamed:@"Icon Messages Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[3]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon Messages On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[4]).tabBarItem setImage:[[UIImage imageNamed:@"Icon More Off"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
        [((UIViewController*)self.viewControllers[4]).tabBarItem setSelectedImage:[[UIImage imageNamed:@"Icon More On"] imageWithRenderingMode:[RobloxTheme iconColoringMode]]];
    }
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(gotLoginSuccessfulNotification:)
                                                 name:RBX_NOTIFY_LOGIN_SUCCEEDED
                                               object:nil ];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(gotLogoutNotification:)
                                                 name:RBX_NOTIFY_LOGGED_OUT
                                               object:nil ];
}

-(void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(RBXAnalyticsCustomData) getCurrentTabContext
{
    RBXAnalyticsCustomData currentTab = RBXACustomTabGames;
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        switch (self.selectedIndex)
        {
            case 0 : { currentTab = RBXACustomTabHome;      } break;
            case 1 : { currentTab = RBXACustomTabGames;     } break;
            case 2 : { currentTab = RBXACustomTabCatalog;   } break;
            case 3 : { currentTab = RBXACustomTabFriends;   } break;
            case 4 : { currentTab = RBXACustomTabMessages;  } break;
            case 5 : {
                currentTab = RBXACustomTabMore;
                
                if ([self.viewControllers[5] respondsToSelector:@selector(getMostRecentTab)])
                    currentTab = [((RBMoreViewController*)self.viewControllers[5]) getMostRecentTab];
            } break;
        }
    }
    else
    {
        switch (self.selectedIndex)
        {
            case 0 : { currentTab = RBXACustomTabHome;      } break;
            case 1 : { currentTab = RBXACustomTabGames;     } break;
            case 2 : { currentTab = RBXACustomTabFriends;   } break;
            case 3 : { currentTab = RBXACustomTabMessages;  } break;
            case 4 : {
                currentTab = RBXACustomTabMore;
                
                if ([self.viewControllers[4] respondsToSelector:@selector(getMostRecentTab)])
                    currentTab = [((RBMoreViewController*)self.viewControllers[4]) getMostRecentTab];
            } break;
        }
    }
    
    return currentTab;
}


- (BOOL)tabBarController:(UITabBarController *)tabBarController shouldSelectViewController:(UIViewController *)viewController
{
    NSUInteger indexOfTab = [self.viewControllers indexOfObject:viewController];
    if (self.selectedIndex == indexOfTab && self.viewControllers.count > 1)
    {
        //drill back to the original screen
        UINavigationController* tappedVC = (UINavigationController*)viewController;
        UIViewController* rootVC = tappedVC.viewControllers[0];
        [tappedVC popToViewController:rootVC animated:YES];
        
        //if it is a webview, reload the original url
        if ([rootVC isKindOfClass:[RBMobileWebViewController class]])
        {
            [(RBMobileWebViewController*)rootVC reloadWebPage];
        }
        else if ([viewController isEqual:[[self viewControllers] objectAtIndex:1]] && [RobloxInfo thisDeviceIsATablet])
        {
            //if we are on the games page, reload the games
            if ([rootVC respondsToSelector:@selector(loadGames)])
                [rootVC performSelector:@selector(loadGames)];
            
        }
        //return NO;
    }
    
    UserInfo* userInfo = [UserInfo CurrentPlayer];
    if(!userInfo.userLoggedIn && ![viewController isEqual:[[self viewControllers] objectAtIndex:1]])
    {
        NSString* controllerName;
        if ([[LoginManager sharedInstance] isFacebookEnabled])
            controllerName = @"SignUpScreenControllerWithSocial";
        else if ([LoginManager apiProxyEnabled])
            controllerName = @"SignUpAPIScreenController";
        else
            controllerName = @"SignUpScreenController";
        
        //ask the user to sign up or log in to see these pages
        [Flurry logEvent:HSC_signUpControllerFromPlayNow];
        NSString* storyboardName = [RobloxInfo getStoryboardName];
        UIStoryboard* storyboard = [UIStoryboard storyboardWithName:storyboardName bundle:nil];
        SignUpScreenController* controller = (SignUpScreenController*)[storyboard instantiateViewControllerWithIdentifier:controllerName];
        controller.modalPresentationStyle = UIModalPresentationFormSheet;
        [self.navigationController presentViewController:controller animated:YES completion:nil];
        self.selectedIndexLogin = [self.viewControllers indexOfObject:viewController];
        
        return NO;
    }
    
    return YES;
}
-(void)tabBarController:(UITabBarController *)tabBarController didSelectViewController:(UIViewController *)viewController
{
    [[RBXEventReporter sharedInstance] reportTabButtonClick:[self getCurrentTabContext]];
}


-(void) gotLoginSuccessfulNotification:(NSNotification*) notification
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self setSelectedIndex:self.selectedIndexLogin];
    });
}
-(void) gotLogoutNotification:(NSNotification*) notification
{
    if ([[ABTestManager sharedInstance] IsInTestMobileGuestMode])
    {
        dispatch_async(dispatch_get_main_queue(), ^
                       {
                           [self setSelectedIndex:1];
                       });
    }
    else
    {
        // Let's look inside the notification and see if there's a treat
        if ([notification.object isKindOfClass:[NSDictionary class]]) {
            id obj = [(NSDictionary *)notification.object objectForKey:@"object"];
            if ([obj isKindOfClass:[NSError class]]) {
                // Oh that's too bad, its not a treat.
                NSError *error = (NSError *)obj;
                
                if ([error.domain.lowercaseString isEqualToString:@"httperror".lowercaseString] && error.code >= 400) {
                    [RBXFunctions dispatchOnMainThread:^{
                        if (self.selectedIndex != 1) {
                            [self setSelectedIndex:1];
                        }
                    }];
                }
            }

        }
    }
}

@end
