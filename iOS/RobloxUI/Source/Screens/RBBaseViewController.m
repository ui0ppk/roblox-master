//
//  RBBaseViewController.m
//  RobloxMobile
//
//  Created by Christian Hresko on 6/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBBaseViewController.h"
#import "RBBarButtonMenu.h"
#import "RobloxInfo.h"
#import "UserInfo.h"
#import "RBPurchaseViewController.h"
#import "RBAccountManagerViewController.h"
#import "UIAlertView+Blocks.h"
#import "LoginManager.h"
#import "Flurry.h"
#import "RBMobileWebViewController.h"
#import "RBGameViewController.h"
#import "RBProfileViewController.h"
#import "RBModalPopUpViewController.h"
#import "RBWebGamePreviewScreenController.h"
#import "RBWebProfileViewController.h"
#import "SearchResultCollectionViewController.h"
#import "NativeSearchNavItem.h"
#import "RobloxNotifications.h"
#import "UIView+Position.h"


//---METRICS---
@interface RBBaseViewController ()

@end

@implementation RBBaseViewController

- (void) setViewTheme:(RBXTheme)theme
{
    viewTheme = theme;
    [RobloxTheme applyTheme:viewTheme toViewController:self quickly:YES];
}

- (void) setFlurryEventsForExternalLinkEvent:(NSString*)webExternalLinkEvent
                             andWebViewEvent:(NSString*)webLocalLinkEvent
                         andOpenProfileEvent:(NSString*)webProfileLinkEvent
                      andOpenGameDetailEvent:(NSString*)webGameDetailLinkEvent
{
    _flurryOpenExternalLinkEvent = webExternalLinkEvent;
    _flurryOpenWebViewEvent = webLocalLinkEvent;
    _flurryOpenProfileEvent = webProfileLinkEvent;
    _flurryOpenGameDetailEvent = webGameDetailLinkEvent;
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    if (viewTheme)
        [RobloxTheme applyTheme:viewTheme toViewController:self quickly:YES];
    
    if (addSearchListeners)
    {
        //Games
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displaySelectedGame:) name:RBXNotificationGameSelected object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displayGameSearchResults:) name:RBXNotificationSearchGames object:nil];
        
        //Users
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displayUserSearchResults:) name:RBXNotificationSearchUsers object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displaySelectedUser:) name:RBXNotificationUserSelected object:nil];
    }
}
- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    if (viewTheme)
        [RobloxTheme applyTheme:viewTheme toViewController:self quickly:NO];
}
- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    if (addSearchListeners)
    {
        //Games
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBXNotificationGameSelected  object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBXNotificationSearchGames   object:nil];
        
        //Users
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBXNotificationSearchUsers   object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBXNotificationUserSelected  object:nil];
        
    }
}


//Navigation buttons
- (void) addRobuxIconWithFlurryEvent:(NSString*)RobuxEvent
            andBCIconWithFlurryEvent:(NSString*)BCEvent
{
    //define some events to be fired
    _flurryEventRobux = RobuxEvent;
    _flurryEventBuildersClub = BCEvent;
    
    UIImage* rbxImage = [UIImage imageNamed:@"Icon Robux Off"];
    UIImage* rbxImageDown = [UIImage imageNamed:@"Icon Robux On"];
    UIImage* bcImage = [UIImage imageNamed:@"Icon BC Off"];
    UIImage* bcImageDown = [UIImage imageNamed:@"Icon BC On"];
    
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        //create the buttons
        UIBarButtonItem* buyRBXButtonItem = [[UIBarButtonItem alloc] initWithImage:rbxImage
                                                                             style:UIBarButtonItemStylePlain
                                                                            target:self
                                                                            action:@selector(didPressBuyRobux)];
        [buyRBXButtonItem setBackgroundImage:rbxImageDown forState:UIControlStateSelected barMetrics:UIBarMetricsDefault];
        
        UIBarButtonItem* buyBCButtonItem = [[UIBarButtonItem alloc] initWithImage:bcImage
                                                                            style:UIBarButtonItemStylePlain
                                                                           target:self
                                                                           action:@selector(didPressBuildersClub)];
        [buyBCButtonItem setBackgroundImage:bcImageDown forState:UIControlStateSelected barMetrics:UIBarMetricsDefault];
    
        //insert the buttons at the head of the list of existing buttons
        NSMutableArray* buttons = [NSMutableArray arrayWithArray:@[buyBCButtonItem, buyRBXButtonItem]];
        [buttons addObjectsFromArray:self.navigationItem.rightBarButtonItems];
        [self.navigationItem setRightBarButtonItems:buttons];
    }
    else
    {
        //If we're on phone, we don't have as much space
        CGRect buttonFrame = CGRectMake(0, 0, 28, 28);
        CGRect layoutFrame = CGRectMake(0, 0, 64, 28);
        
        //so make a custom view to space the icons closer
        UIButton* btnRBX = [UIButton buttonWithType:UIButtonTypeCustom];
        [btnRBX setImage:rbxImage forState:UIControlStateNormal];
        [btnRBX setImage:rbxImageDown forState:UIControlStateSelected];
        [btnRBX addTarget:self action:@selector(didPressBuyRobux) forControlEvents:UIControlEventTouchUpInside];
        [btnRBX setFrame:buttonFrame];
        
        UIButton* btnBC = [UIButton buttonWithType:UIButtonTypeCustom];
        [btnBC setImage:bcImage forState:UIControlStateNormal];
        [btnBC setImage:bcImageDown forState:UIControlStateSelected];
        [btnBC addTarget:self action:@selector(didPressBuildersClub) forControlEvents:UIControlEventTouchUpInside];
        [btnBC setFrame:buttonFrame];
        [btnBC setX:layoutFrame.size.width - btnBC.width];
        
        UIView* customLayout = [[UIView alloc] initWithFrame:layoutFrame];
        [customLayout addSubview:btnBC];
        [customLayout addSubview:btnRBX];
        
        UIBarButtonItem* customBtn = [[UIBarButtonItem alloc] initWithCustomView:customLayout];
        
        NSMutableArray* buttons = [NSMutableArray arrayWithArray:@[customBtn]];
        [buttons addObjectsFromArray:self.navigationItem.rightBarButtonItems];
        [self.navigationItem setRightBarButtonItems:buttons];
    }
    
    
}
- (void) addSearchIconWithSearchType:(SearchResultType)searchType
                  andFlurryEvent:(NSString*)searchEvent
{
    addSearchListeners = YES;
    bool compact = ![RobloxInfo thisDeviceIsATablet];
    NativeSearchNavItem* searchItem = [[NativeSearchNavItem alloc] initWithSearchType:searchType andContainer:self compactMode:compact];
    NSArray* rightNavItems = self.navigationItem.rightBarButtonItems;
    NSMutableArray* items = [NSMutableArray arrayWithArray:rightNavItems];
    [items addObject:searchItem];
    [self.navigationItem setRightBarButtonItems:items];
}
- (void) addLogoutButton
{
    UIBarButtonItem* logoutButton = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"Logout Button"]
                                                                     style:UIBarButtonItemStylePlain
                                                                    target:self
                                                                    action:@selector(didPressLogOut)];
    
    //insert the buttons at the head of the list of existing buttons
    NSMutableArray* buttons = [NSMutableArray arrayWithObject:logoutButton];
    [buttons addObjectsFromArray:self.navigationItem.rightBarButtonItems];
    [self.navigationItem setRightBarButtonItems:buttons];
}

//Navigation Button Events
- (void)didPressEditAccount
{
    #define ACCOUNT_SETTINGS_POPUP_SIZE CGRectMake(0, 0, 540, 344)
    if (_flurryEventEditAccount)
        [Flurry logEvent:_flurryEventEditAccount];
    
    NSString* storyboardName = [RobloxInfo getStoryboardName];
    UIStoryboard* storyboard = [UIStoryboard storyboardWithName:storyboardName bundle:nil];
    RBAccountManagerViewController* controller = [storyboard instantiateViewControllerWithIdentifier:@"RBAccountManagerController"];
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
    navigation.view.superview.bounds = ACCOUNT_SETTINGS_POPUP_SIZE;
}
- (void)didPressLogOut
{
    if (_flurryEventLogOut)
        [Flurry logEvent:_flurryEventLogOut];
    UIAlertView* view = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Log Out", nil)
                                                   message:NSLocalizedString(@"Are you sure you want to log out?", nil)
                                                  delegate:nil
                                         cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
                                         otherButtonTitles:NSLocalizedString(@"Log Out", nil) , nil];
    
    RBBaseViewController* strongSelf = self;
    [view showWithCompletion:^(UIAlertView *alertView, NSInteger buttonIndex)
     {
         switch (buttonIndex)
         {
             case 0:
                 break;
             case 1:
             {
                 [[LoginManager sharedInstance] doLogout];
                 
                 [strongSelf.tabBarController.navigationController popViewControllerAnimated:YES];
                 
                 break;
             }
             default:
                 break;
         }
     }];
}
- (void) didPressBuyRobux
{
    CGRect PURCHASE_ROBUX_POPUP_SIZE = [RobloxInfo thisDeviceIsATablet] ? CGRectMake(0, 0, 540, 344) : CGRectMake(0, 0, 300, 344);
    if (_flurryEventRobux)
        [Flurry logEvent:_flurryEventRobux];
    
    NSString* baseURL = [RobloxInfo getBaseUrl];
    NSString* url = [baseURL stringByAppendingString:@"mobile-app-upgrades/native-ios/robux"];
    if (![RobloxInfo thisDeviceIsATablet])
        url = [url stringByReplacingOccurrencesOfString:@"m." withString:@"www."];
    
    NSString* titleString = [NSString stringWithFormat:@"%@ : R$%@", NSLocalizedString(@"CurrentRobuxBalanceWord", nil), [UserInfo CurrentPlayer].Robux];
    RBPurchaseViewController* controller = [[RBPurchaseViewController alloc] initWithURL:[NSURL URLWithString:url] andTitle:titleString];
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
    navigation.view.superview.bounds = PURCHASE_ROBUX_POPUP_SIZE;
}
- (void) didPressBuildersClub
{
    if (_flurryEventBuildersClub)
        [Flurry logEvent:_flurryEventBuildersClub];
    
    NSString* baseUrl = [RobloxInfo getBaseUrl];
    NSString* url = [baseUrl stringByAppendingString:@"mobile-app-upgrades/native-ios/bc"];
    if (![RobloxInfo thisDeviceIsATablet])
        url = [url stringByReplacingOccurrencesOfString:@"m." withString:@"www."];
    
    //NSString* BCLevelKey = @"NBCWord";
    //[UserInfo CurrentPlayer].bcMember
    NSString* titleString = NSLocalizedString(@"Builders Club", nil); //[NSString stringWithFormat:@"%@ : %@", NSLocalizedString(@"CurrentBuildersClubWord", nil), NSLocalizedString(BCLevelKey, nil)];
    RBPurchaseViewController* controller = [[RBPurchaseViewController alloc] initWithURL:[NSURL URLWithString:url] andTitle:titleString];
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
}




//pushing view controllers
-(RBXWebRequestReturnStatus) handleWebRequest:(NSURL*)aRequest
{
    //typically called from a webview shouldLoadWithRequest delegate function
    //returns RBXWebRequestReturnWebRequest when the webview can safely load the requested URL
    //returns RBXWebRequestReturnScreenPush when the request has opened another view
    //returns RBXWebRequestReturnUnknown when the domain is unknown and should be handled by pushing out to the browser
    
    NSString* lcaseHost = [[aRequest host] lowercaseString];
    NSString* aRequestString = [[NSString stringWithFormat:@"%@", aRequest] stringByRemovingPercentEncoding];
    aRequestString = [aRequestString lowercaseString];
    
    //check the domain of the URL to see if it is a ROBLOX URL
    bool isRobloxURL = [lcaseHost rangeOfString:@"roblox"].location != NSNotFound;
    
    //also check that the url isn't a Roblox User's URL from a message
    bool isUserURL = [aRequestString rangeOfString:@"user.aspx?id="].location != NSNotFound;
    
    if (isRobloxURL || isUserURL)
    {
        if (isUserURL)
        {
            [self pushProfileControllerWithURL:aRequestString];
            return RBXWebRequestReturnScreenPush;
        }
        else if ([aRequestString rangeOfString:@"/users/"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeCreative];
        }
        else if ([aRequestString rangeOfString:@"-place?id="].location != NSNotFound)
        {
            [self pushGameDetailWithURL:aRequestString];
            return RBXWebRequestReturnScreenPush;
        }
        else if ([aRequestString rangeOfString:@"-item?id="].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([aRequestString rangeOfString:@"/catalog/"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([aRequestString rangeOfString:@"groups.aspx"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeSocial];
        }
        else if ([aRequestString rangeOfString:@"stuff.aspx"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([aRequestString rangeOfString:@"inventory"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([aRequestString rangeOfString:@"/forum/"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeSocial];
        }
        return RBXWebRequestReturnWebRequest;
    }
    else
    {
        //Not exactly sure what we found... just open it in an external web view.
        //[[UIApplication sharedApplication] openURL:aRequest];
        return RBXWebRequestReturnUnknown;
    }
    
    return RBXWebRequestReturnWebRequest;
}

-(void) handleWebRequestWithPopout:(NSURL*)aRequest
{
    NSString* lcaseHost = [[aRequest host] lowercaseString];
    NSString* aRequestString = [[NSString stringWithFormat:@"%@", aRequest] stringByRemovingPercentEncoding];
    aRequestString = [aRequestString lowercaseString];
    
    bool isRobloxURL = [lcaseHost rangeOfString:@"roblox"].location != NSNotFound;
    bool isUserURL = [aRequestString rangeOfString:@"user.aspx?id="].location != NSNotFound;
    
    if (isRobloxURL || isUserURL)
    {
        if (isUserURL)
        {
            [self pushProfileControllerWithURL:aRequestString];
        }
        else if ([aRequestString rangeOfString:@"-place?id="].location != NSNotFound)
        {
            [self pushGameDetailWithURL:aRequestString];
        }
        else if ([aRequestString rangeOfString:@"-item?id="].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeGame];
        }
        else if ([aRequestString rangeOfString:@"groups.aspx"].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeSocial];
        }
        else if ([aRequestString rangeOfString:@"stuff.aspx"].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeGame];
        }
        else
        {
            //Not exactly sure what we found... just open it in an external web view.
            if (_flurryOpenExternalLinkEvent)
                [Flurry logEvent:_flurryOpenExternalLinkEvent];
            [[UIApplication sharedApplication] openURL:aRequest];
        }
    }
    else
    {
        if (_flurryOpenExternalLinkEvent)
            [Flurry logEvent:_flurryOpenExternalLinkEvent];
        [[UIApplication sharedApplication] openURL:aRequest];
    }
}

-(void) pushWebControllerwithURL:(NSString*)stringURL andTheme:(RBXTheme)theme
{
    [self pushWebControllerwithURL:stringURL andTitle:nil andTheme:theme];
}
-(void) pushWebControllerwithURL:(NSString*)stringURL andTitle:(NSString*)pageTitle andTheme:(RBXTheme)theme
{
    if (_flurryOpenWebViewEvent)
        [Flurry logEvent:_flurryOpenWebViewEvent];
    
    RBMobileWebViewController* aWebScreen = [[RBMobileWebViewController alloc] initWithNavButtons:NO];
    [aWebScreen.view setFrame:CGRectMake(self.view.frame.origin.x, self.view.frame.origin.y, self.view.frame.size.width, self.view.frame.size.height)];
    [aWebScreen setUrl:stringURL];
    [aWebScreen setViewTheme:theme];
    
    if (pageTitle)
        [aWebScreen setTitle:pageTitle];
    
    [self.navigationController pushViewController:aWebScreen animated:YES];
}

//Profile
-(void) pushProfileControllerWithUserID:(NSNumber*)userID
{
    if (_flurryOpenProfileEvent)
        [Flurry logEvent:_flurryOpenProfileEvent];
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        NSString* storyboardName = [RobloxInfo getStoryboardName];
        UIStoryboard* sb = [UIStoryboard storyboardWithName:storyboardName bundle:nil];
        RBProfileViewController* builderProfile = (RBProfileViewController*) [sb instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
        builderProfile.userId = userID;
        [self.navigationController pushViewController:builderProfile animated:YES];
    }
    else
    {
        RBWebProfileViewController* builderProfile = [[RBWebProfileViewController alloc] initWithNavButtons:YES];
        builderProfile.userId = userID;
        [builderProfile setViewTheme:RBXThemeCreative];
        [self.navigationController pushViewController:builderProfile animated:YES];
    }
}
-(void) pushProfileControllerWithURL:(NSString*)stringURL
{
    //NOTE - flurry events are handled in other functions
    NSArray* stringParts = [stringURL componentsSeparatedByString:@"user.aspx?id="];
    
    if ([stringParts count] <= 1)
    {
        //failed to parse the url, fallback and use a mobile web view
        [self pushWebControllerwithURL:stringURL andTheme:RBXThemeCreative];
        return;
    }
    NSNumber* userID = [NSNumber numberWithInt:[stringParts[1] integerValue]];
    [self pushProfileControllerWithUserID:userID];
}

//Games
-(void) pushGameDetailWithURL:(NSString*)stringURL
{
    //load up a game
    NSArray* stringParts = [stringURL componentsSeparatedByString:@"?id="];
    if ([stringParts count] <= 1)
    {
        //failed to parse the url, fallback and use a mobile web view
        [self pushWebControllerwithURL:stringURL andTheme:RBXThemeGame];
    }
    else
    {
        //grab the game details
        [RobloxData fetchGameDetails:stringParts[1] completion:^(RBXGameData *game)
         {
             [self pushGameDetailWithGameData:game];
         }];
        
    }
}
-(void) pushGameDetailWithGameData:(RBXGameData*)game
{
    if ([RobloxInfo thisDeviceIsATablet])
    {
        if (_flurryOpenGameDetailEvent)
            [Flurry logEvent:_flurryOpenGameDetailEvent];
        
        
        dispatch_async(dispatch_get_main_queue(), ^
       {
           RBWebGamePreviewScreenController* aGameScreen = [[RBWebGamePreviewScreenController alloc] init];
           aGameScreen.gameData = game;
           [self.navigationController pushViewController:aGameScreen animated:YES];
       });
    }
    else
    {
        [self pushGameWithID:game.placeID.intValue isUser:NO];
    }
}
-(void) pushGameWithID:(int)placeID isUser:(BOOL)isUser
{
    RBGameViewController* controller = [[RBGameViewController alloc] init];
    controller.placeID = placeID;
    controller.isUser = isUser;
    [self presentViewController:controller animated:NO completion:nil];
}

//Search
-(void) pushSearchResultsType:(SearchResultType)resultType
                  withKeyword:(NSString*)searchKeywords
{
    SearchResultCollectionViewController* aCollectionViewController = [[SearchResultCollectionViewController alloc] initWithKeyword:searchKeywords andSearchType:resultType];
    [self.navigationController pushViewController:aCollectionViewController animated:YES];
}


//search result functions
- (void) displaySelectedGame:(NSNotification*)notification
{
    RBXGameData* notificationData = (RBXGameData*)[notification.userInfo objectForKey:@"gameData"];
    [self pushGameDetailWithGameData:notificationData];
}
- (void) displayGameSearchResults:(NSNotification*) notification
{
    //[Flurry logEvent:GS_searchGames];
    NSString* keywords = [notification.userInfo objectForKey:@"keywords"];
    //[self performSegueWithIdentifier:@"searchResults" sender:keywords]; //NOTE- THIS WAY IS DEPRECATED BUT IT OPENS A SPECIFIC SEARCH RESULT SCREEN
    //GameSearchResultsScreenController* controller = segue.destinationViewController;
    //controller.keywords = (NSString*) sender;
    
    //This is how it should be done - later
    [self pushSearchResultsType:SearchResultGames withKeyword:keywords];
}
- (void) displaySelectedUser:(NSNotification*) notification
{
    RBXUserSearchInfo* user = (RBXUserSearchInfo*)[notification.userInfo objectForKey:@"userData"];
    [self pushProfileControllerWithUserID:user.userId];
}
- (void) displayUserSearchResults:(NSNotification*) notification
{
    //NSLog(@"Displaying User Search Results with notification : %@", notification);
    NSString* searchKeywords = (NSString*)[notification.userInfo objectForKey:@"keywords"];
    [self pushSearchResultsType:SearchResultUsers withKeyword:searchKeywords];
}

@end
