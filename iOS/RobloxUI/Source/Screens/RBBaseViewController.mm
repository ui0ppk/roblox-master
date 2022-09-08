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
#import "RobloxNotifications.h"
#import "UIView+Position.h"
#import "iOSSettingsService.h"
#import "RobloxWebUtility.h"
#import "NonRotatableNavigationController.h"
#import "LoginManager.h"
#import "RobloxAlert.h"

#import "RBMobileWebViewController.h"
#import "RBGameViewController.h"
#import "RBProfileViewController.h"
#import "RBModalPopUpViewController.h"
#import "RBWebGamePreviewScreenController.h"
#import "RBWebProfileViewController.h"
#import "SearchResultCollectionViewController.h"
#import "NativeSearchNavItem.h"
#import "FeaturedGamesScreenController.h"
#import "GameSortResultsScreenController.h"
#import "NativeSearchNavItem.h"
#import "RBTabBarController.h"

#import "RBXFunctions.h"

DYNAMIC_FASTFLAGVARIABLE(EnableNativeGamesPage, false);
DYNAMIC_FASTFLAGVARIABLE(EnableNativeProfilePage, false);
DYNAMIC_FASTFLAGVARIABLE(EnableNativeSearchGameResults, false);
DYNAMIC_FASTFLAGVARIABLE(EnableNativeSearchPeopleResults, false);

//---METRICS---

@interface RBBaseViewController ()

@property (nonatomic, strong) NSString *flurryEventRobux;
@property (nonatomic, strong) NSString *flurryEventBuildersClub;
@property (nonatomic, strong) NSString *flurryEventEditAccount;
@property (nonatomic, strong) NSString *flurryEventLogOut;
@property (nonatomic, strong) NSString *flurryEventSearch;
@property (nonatomic, strong) NSString *flurryOpenExternalLinkEvent;
@property (nonatomic, strong) NSString *flurryOpenProfileEvent;
@property (nonatomic, strong) NSString *flurryOpenWebViewEvent;
@property (nonatomic, strong) NSString *flurryOpenGameDetailEvent;
@property (nonatomic, strong) NSString *flurryOpenSearchCatalog;
@property (nonatomic, strong) NSString *flurryOpenSearchGames;
@property (nonatomic, strong) NSString *flurryOpenSearchGroups;
@property (nonatomic, strong) NSString *flurryOpenSearch;

@property (nonatomic, strong) UIBarButtonItem *buyRBXButtonItem;
@property (nonatomic, strong) UIBarButtonItem *buyBCButtonItem;

@property (nonatomic) BOOL addSearchListeners;

@property (nonatomic) RBXTheme viewTheme;
@property (nonatomic) RBXAnalyticsCustomData screenEnumName;

@end

@implementation RBBaseViewController

- (void) setViewTheme:(RBXTheme)theme
{
    _viewTheme = theme;
    [RobloxTheme applyTheme:_viewTheme toViewController:self quickly:YES];
}

- (void) setFlurryEventsForExternalLinkEvent:(NSString*)webExternalLinkEvent
                             andWebViewEvent:(NSString*)webLocalLinkEvent
                         andOpenProfileEvent:(NSString*)webProfileLinkEvent
                      andOpenGameDetailEvent:(NSString*)webGameDetailLinkEvent
{
    self.flurryOpenExternalLinkEvent = webExternalLinkEvent;
    self.flurryOpenWebViewEvent = webLocalLinkEvent;
    self.flurryOpenProfileEvent = webProfileLinkEvent;
    self.flurryOpenGameDetailEvent = webGameDetailLinkEvent;
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    if (self.viewTheme)
        [RobloxTheme applyTheme:self.viewTheme toViewController:self quickly:YES];
    
    if (self.addSearchListeners)
    {
        //Games
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displaySelectedGame:) name:RBX_NOTIFY_GAME_SELECTED object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displayGameSearchResults:) name:RBX_NOTIFY_SEARCH_GAMES object:nil];
        
        //Users
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displayUserSearchResults:) name:RBX_NOTIFY_SEARCH_USERS object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(displaySelectedUser:) name:RBX_NOTIFY_USER_SELECTED object:nil];
    }
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    if (self.viewTheme)
        [RobloxTheme applyTheme:self.viewTheme toViewController:self quickly:NO];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    if (self.addSearchListeners)
    {
        //Games
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_GAME_SELECTED  object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_SEARCH_GAMES   object:nil];
        
        //Users
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_SEARCH_USERS   object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_USER_SELECTED  object:nil];
        
    }
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

//Navigation buttons
- (void) removeRobuxAndBCIcons
{
    NSMutableArray* barItems = [self.navigationItem.rightBarButtonItems mutableCopy];
    if (barItems.count >= [RobloxInfo thisDeviceIsATablet] ? 2 : 1)
    {
        [barItems removeObjectAtIndex:0];
        if ([RobloxInfo thisDeviceIsATablet])
            [barItems removeObjectAtIndex:0];
        
        //assign the updated list to the navigation item
        [self.navigationItem setRightBarButtonItems:barItems];
    }
}

- (void) addRobuxIconWithFlurryEvent:(NSString*)RobuxEvent
            andBCIconWithFlurryEvent:(NSString*)BCEvent
{
    //define some events to be fired
    self.flurryEventRobux = RobuxEvent;
    self.flurryEventBuildersClub = BCEvent;
    
    UIImage* rbxImage = [UIImage imageNamed:@"Icon Robux Off"];
    UIImage* rbxImageDown = [UIImage imageNamed:@"Icon Robux On"];
    UIImage* bcImage = [UIImage imageNamed:@"Icon BC Off"];
    UIImage* bcImageDown = [UIImage imageNamed:@"Icon BC On"];
    
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        //create the buttons
        if ([RBXFunctions isEmpty:self.buyRBXButtonItem]) {
            self.buyRBXButtonItem = [[UIBarButtonItem alloc] initWithImage:rbxImage
                                                                     style:UIBarButtonItemStylePlain
                                                                    target:self
                                                                    action:@selector(didPressBuyRobux)];
        }
        [self.buyRBXButtonItem setBackgroundImage:rbxImageDown forState:UIControlStateSelected barMetrics:UIBarMetricsDefault];
        
        if ([RBXFunctions isEmpty:self.buyBCButtonItem]) {
            self.buyBCButtonItem = [[UIBarButtonItem alloc] initWithImage:bcImage
                                                                    style:UIBarButtonItemStylePlain
                                                                   target:self
                                                                   action:@selector(didPressBuildersClub)];
        }
        [self.buyBCButtonItem setBackgroundImage:bcImageDown forState:UIControlStateSelected barMetrics:UIBarMetricsDefault];
    
        //insert the buttons at the head of the list of existing buttons
        NSMutableArray* buttons = [NSMutableArray arrayWithArray:@[self.buyRBXButtonItem, self.buyBCButtonItem]];
        [buttons addObjectsFromArray:self.navigationItem.rightBarButtonItems];
        [self.navigationItem setRightBarButtonItems:buttons];
    }
    else
    {
        //If we're on phone, we don't have as much space
        CGRect buttonFrame = CGRectMake(0, 0, 28, 44);
        CGRect layoutFrame = CGRectMake(0, 0, 64, 44);
        
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
    self.addSearchListeners = YES;
    bool compact = ![RobloxInfo thisDeviceIsATablet];
    NativeSearchNavItem* searchItem = [[NativeSearchNavItem alloc] initWithSearchType:searchType andContainer:self compactMode:compact];
    NSArray* rightNavItems = self.navigationItem.rightBarButtonItems;
    NSMutableArray* items = [NSMutableArray arrayWithArray:rightNavItems];
    [items addObject:searchItem];
    [self.navigationItem setRightBarButtonItems:items];
}

//Navigation Button Events
- (void)didPressEditAccount
{
    #define ACCOUNT_SETTINGS_POPUP_SIZE CGRectMake(0, 0, 540, 344)
    if (self.flurryEventEditAccount)
        [Flurry logEvent:self.flurryEventEditAccount];
    
    NSString* storyboardName = [RobloxInfo getStoryboardName];
    UIStoryboard* storyboard = [UIStoryboard storyboardWithName:storyboardName bundle:nil];
    RBAccountManagerViewController* controller = [storyboard instantiateViewControllerWithIdentifier:@"RBAccountManagerController"];
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
    navigation.view.superview.bounds = ACCOUNT_SETTINGS_POPUP_SIZE;
}

- (void) didPressBuyRobux
{
    CGRect PURCHASE_ROBUX_POPUP_SIZE = [RobloxInfo thisDeviceIsATablet] ? CGRectMake(0, 0, 540, 344) : CGRectMake(0, 0, 300, 344);
    
    if (self.screenEnumName)
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonRobux
                                                 withContext:RBXAContextMain
                                              withCustomData:self.screenEnumName];
    if (self.flurryEventRobux)
        [Flurry logEvent:self.flurryEventRobux];
    
    RBPurchaseViewController* controller = [[RBPurchaseViewController alloc] initWithRobuxPurchasing];
    NonRotatableNavigationController* navigation = [[NonRotatableNavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
    navigation.view.superview.bounds = PURCHASE_ROBUX_POPUP_SIZE;
    
    if ([self.navigationController.tabBarController respondsToSelector:@selector(getCurrentTabContext)])
    {
        RBXAnalyticsCustomData cd = [((RBTabBarController*)self.navigationController.tabBarController) getCurrentTabContext];
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonRobux withContext:RBXAContextMain withCustomData:cd];
    }
}
- (void) didPressBuildersClub
{
    if (self.screenEnumName)
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonRobux
                                                 withContext:RBXAContextMain
                                              withCustomData:self.screenEnumName ];
    if (self.flurryEventBuildersClub)
        [Flurry logEvent:self.flurryEventBuildersClub];
    
    RBPurchaseViewController* controller = [[RBPurchaseViewController alloc] initWithBCPurchasing];
    NonRotatableNavigationController* navigation = [[NonRotatableNavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
    
    if ([self.navigationController.tabBarController respondsToSelector:@selector(getCurrentTabContext)])
    {
        RBXAnalyticsCustomData cd = [((RBTabBarController*)self.navigationController.tabBarController) getCurrentTabContext];
        [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonBuildersClub withContext:RBXAContextMain withCustomData:cd];
    }
}




//pushing view controllers
-(RBXWebRequestReturnStatus) handleWebRequest:(NSURL*)aRequest
{
    //typically called from a webview shouldLoadWithRequest delegate function
    //returns RBXWebRequestReturnWebRequest when the webview can safely load the requested URL
    //returns RBXWebRequestReturnScreenPush when the request has opened another view
    //returns RBXWebRequestReturnUnknown when the domain is unknown and should be handled by pushing out to the browser
    
    NSString* aRequestString = [[NSString stringWithFormat:@"%@", aRequest] stringByRemovingPercentEncoding];
    aRequestString = [aRequestString lowercaseString];
    
    //do a quick check that the url isn't a Roblox User's URL from a message (FRIEND REQUEST: ACCEPTED)
    // sometimes they don't fit the regular formatting style
    if ([aRequestString rangeOfString:@"/user.aspx?id="].location != NSNotFound)
    {
        [self pushProfileControllerWithURL:aRequestString];
        return RBXWebRequestReturnScreenPush;
    }
    
    //check the domain of the URL to see if it is a ROBLOX URL
    // if it is, then we can handle it in-app
    NSString* lcaseHost = [[aRequest host] lowercaseString];
    if ([lcaseHost rangeOfString:@"roblox"].location != NSNotFound)
    {
        NSUInteger extenstionStart = [aRequestString rangeOfString:@".com"].location + 4;
        if (extenstionStart == NSNotFound)
            return RBXWebRequestReturnWebRequest; // <-- this should never get hit but let's be safe
        NSString* extension = [aRequestString substringFromIndex:extenstionStart];
        
        //NOTE - THIS SHOULD BE CLEANED UP TO USE A DICTIONARY WITH INSTANT LOOKUP
        //THE MORE CASES THERE ARE HERE, THE LONGER EACH LOAD REQUEST TAKES.
        //-Kyler 2015
        
        if ([extension rangeOfString:@"/newlogin"].location != NSNotFound)
        {
            //NOTE- sometime there is this really weird case where the user gets logged out from the website, but not the app
            //So might as well just log the user out and apologize
            [[LoginManager sharedInstance] logoutRobloxUser];
            [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"ErrorUnknownLogout", nil)];
            [self.tabBarController.navigationController popToRootViewControllerAnimated:YES];
            
            
            //prevent the page from loading
            return RBXWebRequestReturnFilter;
        }
        else if ([extension rangeOfString:@"home"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeSocial];
        }
        else if ([extension rangeOfString:@"/users/"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeCreative];
        }
        else if ([extension rangeOfString:@"-place?id="].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"/games"].location != NSNotFound)
        {
            //we might have a native page to handle
            
            [self setViewTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"-item?id="].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"/catalog"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"groups.aspx"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeSocial];
        }
        else if ([extension rangeOfString:@"stuff.aspx"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"inventory"].location != NSNotFound)
        {
            [self setViewTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"/forum"].location != NSNotFound)
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
    
    //do a quick check that the url isn't a Roblox User's URL from a message (FRIEND REQUEST: ACCEPTED)
    // sometimes they don't fit the regular formatting style
    if ([aRequestString rangeOfString:@"/user.aspx?id="].location != NSNotFound)
    {
        //NOTE- in this case, the url will look like this: "applewebdata://461243cd-ad85-4622-b586-c390076a4102/user.aspx?id=59257875"
        [self pushProfileControllerWithURL:aRequestString];
        return;
    }

    //check the domain of the URL to see if it is a ROBLOX URL
    // if it is, then we can handle it in-app
    if ([lcaseHost rangeOfString:@"roblox"].location != NSNotFound)
    {
        NSUInteger extenstionStart = [aRequestString rangeOfString:@".com"].location + 4;
        if (extenstionStart == NSNotFound)
        {
            [[UIApplication sharedApplication] openURL:aRequest];// <-- this should never get hit but let's be safe
            return;
        }
        NSString* extension = [aRequestString substringFromIndex:extenstionStart];
        
        
        if ([extension rangeOfString:@"/newlogin"].location != NSNotFound)
        {
            //prevent the page from loading
            return;
        }
        else if ([extension rangeOfString:@"/home"].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeSocial];
        }
        else if ([extension rangeOfString:@"/users/"].location != NSNotFound)
        {
            [self pushProfileControllerWithURL:aRequestString];
        }
        else if ([extension rangeOfString:@"-place?id="].location != NSNotFound)
        {
            [self pushGameDetailWithURL:aRequestString];
        }
        else if ([extension rangeOfString:@"/games"].location != NSNotFound)
        {
            if (DFFlag::EnableNativeGamesPage)
                [self pushUpdatedGameDetailWithURL:aRequestString];
            else
                [self pushGameDetailWithURL:aRequestString];
        }
        else if ([extension rangeOfString:@"-item?id="].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeGame];
        }
        else if ([extension rangeOfString:@"groups.aspx"].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeSocial];
        }
        else if ([extension rangeOfString:@"stuff.aspx"].location != NSNotFound)
        {
            [self pushWebControllerwithURL:aRequestString andTheme:RBXThemeGame];
        }
        else
        {
            //Not exactly sure what we found... just open it in an external web view.
            if (self.flurryOpenExternalLinkEvent)
                [Flurry logEvent:self.flurryOpenExternalLinkEvent];
            [[UIApplication sharedApplication] openURL:aRequest];
        }
    }
    else
    {
        if (self.flurryOpenExternalLinkEvent)
            [Flurry logEvent:self.flurryOpenExternalLinkEvent];
        [[UIApplication sharedApplication] openURL:aRequest];
    }
}

-(void) pushWebControllerwithURL:(NSString*)stringURL andTheme:(RBXTheme)theme
{
    //load the url if we are already on a webview
    if ([self.class isSubclassOfClass:RBMobileWebViewController.class])
    {
        [((RBMobileWebViewController*)self) loadURL:stringURL screenURL:NO];
        [self setViewTheme:theme];
    }
    else
        [self pushWebControllerwithURL:stringURL andTitle:nil andTheme:theme];
}
-(void) pushWebControllerwithURL:(NSString*)stringURL andTitle:(NSString*)pageTitle andTheme:(RBXTheme)theme
{
    if (self.flurryOpenWebViewEvent)
        [Flurry logEvent:self.flurryOpenWebViewEvent];
    
    dispatch_async(dispatch_get_main_queue(), ^
    {
        RBMobileWebViewController* aWebScreen = [[RBMobileWebViewController alloc] initWithNavButtons:NO];
        [aWebScreen.view setFrame:CGRectMake(self.view.frame.origin.x, self.view.frame.origin.y, self.view.frame.size.width, self.view.frame.size.height)];
        [aWebScreen setUrl:stringURL];
        [aWebScreen setViewTheme:theme];
        
        if (pageTitle)
            [aWebScreen setTitle:pageTitle];
        
        [self.navigationController pushViewController:aWebScreen animated:YES];
    });
    
}

//Profile
-(void) pushProfileControllerWithUserID:(NSNumber*)userID
{
    if (self.flurryOpenProfileEvent)
        [Flurry logEvent:self.flurryOpenProfileEvent];
    
    if (DFFlag::EnableNativeProfilePage && [RobloxInfo thisDeviceIsATablet])
    {
        NSString* storyboardName = [RobloxInfo getStoryboardName];
        UIStoryboard* sb = [UIStoryboard storyboardWithName:storyboardName bundle:nil];
        RBProfileViewController* builderProfile = (RBProfileViewController*) [sb instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
        builderProfile.userId = userID;
        [self.navigationController pushViewController:builderProfile animated:YES];
    }
    else
    {
        //check if we are a web page, just load the url for the player, don't push a view
        if ([self isKindOfClass:RBMobileWebViewController.class])
        {
            NSString* url = [NSString stringWithFormat:@"%@users/%@/profile", [RobloxInfo getWWWBaseUrl], userID.stringValue];
            [((RBMobileWebViewController*)self) loadURL:url screenURL:NO ];
            [self setViewTheme:RBXThemeSocial];
        }
        else
        {
            RBWebProfileViewController* builderProfile = [[RBWebProfileViewController alloc] initWithNavButtons:YES];
            builderProfile.userId = userID;
            [builderProfile setViewTheme:RBXThemeCreative];
            [self.navigationController pushViewController:builderProfile animated:YES];
        }
    }
}
-(void) pushProfileControllerWithURL:(NSString*)stringURL
{
    //check what style of url we have
    NSString* unparsedID;
    if ([stringURL rangeOfString:@"/user.aspx?id="].location != NSNotFound)
    {
        //Do we have a legacy url? "applewebdata://461243cd-ad85-4622-b586-c390076a4102/user.aspx?id=59257875"
        NSArray* stringParts = [stringURL componentsSeparatedByString:@"/user.aspx?id="];
        if ([stringParts count] > 1)
            unparsedID = stringParts[1];
    }
    else
    {
        NSRange newIDRange = [stringURL rangeOfString:@"/users/"];
        if (newIDRange.location != NSNotFound)
        {
            //or do we have a new url? http://www.roblox.com/users/68465808/profile
            NSString* endOfString = [stringURL substringFromIndex:(newIDRange.location + newIDRange.length)];
            
            NSRange indexOfSlash = [endOfString rangeOfString:@"/profile"];
            
            //parse out the userID if we can find it
            if (indexOfSlash.location != NSNotFound)
                unparsedID = [endOfString substringToIndex:indexOfSlash.location];
        }
    }
    
    //NOTE - flurry events are handled in other functions
    if (unparsedID)
    {
         NSNumber* userID = [NSNumber numberWithLongLong:[unparsedID longLongValue]];
        [self pushProfileControllerWithUserID:userID];
    }
    else
    {
        //failed to parse the url, fallback and use a mobile web view
        [self pushWebControllerwithURL:stringURL andTheme:RBXThemeCreative];
    }
}

//Games
-(void) pushGameDetailWithURL:(NSString*)stringURL
{
    //load up a game
    //NSArray* stringParts = [stringURL componentsSeparatedByString:@"?id="];
    //if ([stringParts count] <= 1)
    //{
        //failed to parse the url, fallback and use a mobile web view
        [self pushWebControllerwithURL:stringURL andTheme:RBXThemeGame];
    //}
    //else
    //{
        //grab the game details
    //    RBBaseViewController __weak *weakSelf = self;

    //    [RobloxData fetchGameDetails:stringParts[1] completion:^(RBXGameData *game)
    //     {
    //         [weakSelf pushGameDetailWithGameData:game];
    //     }];
        
    //}
}
-(void) pushUpdatedGameDetailWithURL:(NSString*)stringURL
{
    //load up a game
    NSString* regex = [NSString stringWithFormat:@"/games/.*/"];
    NSRange range = [stringURL rangeOfString:regex options:NSRegularExpressionSearch];
    if (range.location == NSNotFound)
    {
        NSRange sortFilter = [stringURL rangeOfString:@"sortfilter="];
        if (sortFilter.location != NSNotFound)
        {
            //we might have a specific sort to look at...
            NSString* sortName = [stringURL substringFromIndex:(sortFilter.location + sortFilter.length)];
            sortName = [sortName substringToIndex:[sortName rangeOfString:@"&"].location];
            
            if ([sortName isEqualToString:@"default"])
            {
                [self pushGamesPage];
            }
            else
            {
                [self pushGameSortWithSortId:[NSNumber numberWithInteger:sortName.integerValue]];
            }
        }
        else
        {
            //looks like it is just a regular games page
            [self pushGamesPage];
        }
    }
    else
    {
        //we have game details so grab the game ID, first remove the /games/ and the / at the end
        NSString* gameID = [stringURL substringFromIndex:(range.location + 7)];
        gameID = [gameID substringToIndex:(range.length - 8)];
        
        RBBaseViewController __weak *weakSelf = self;

        [RobloxData fetchGameDetails:gameID completion:^(RBXGameData *game)
         {
             if (game)
                 [weakSelf pushGameDetailWithGameData:game];
             else
                 [weakSelf pushWebControllerwithURL:stringURL andTheme:RBXThemeGame];
         }];
        
    }
}
-(void) pushGameDetailWithGameData:(RBXGameData*)game
{
    if ([RobloxInfo thisDeviceIsATablet] || [self isGameDetailEnabledOnPhone])
    {
        if (self.flurryOpenGameDetailEvent)
            [Flurry logEvent:self.flurryOpenGameDetailEvent];
        
        
        dispatch_async(dispatch_get_main_queue(), ^
       {
           if ([self.class isSubclassOfClass:RBMobileWebViewController.class])
           {
               //if we are a webview, just load the game url
               NSString* gameURL = [NSString stringWithFormat:@"%@PlaceItem.aspx?id=%@", [RobloxInfo getWWWBaseUrl], game.placeID];
               ((RBMobileWebViewController*)self).url = gameURL;
               [self setViewTheme:RBXThemeGame];
           }
           else
           {
               RBWebGamePreviewScreenController* aGameScreen = [[RBWebGamePreviewScreenController alloc] init];
               aGameScreen.gameData = game;
               [self.navigationController pushViewController:aGameScreen animated:YES];
           }
       });
    }
    else
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            RBGameViewController* controller = [[RBGameViewController alloc] initWithLaunchParams:[RBXGameLaunchParams InitParamsForJoinPlace:game.placeID.integerValue]];
            [self presentViewController:controller animated:NO completion:nil];
        });
    }
}
-(void) pushGamesPage
{
    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController* controller;
        if ([RobloxInfo thisDeviceIsATablet] && DFFlag::EnableNativeGamesPage)
        {
            controller = [[FeaturedGamesScreenController alloc] init];
        }
        else
        {
            controller = [[RBMobileWebViewController alloc] initWithNavButtons:NO];
            ((RBMobileWebViewController*)controller).url = [[RobloxInfo getBaseUrl] stringByAppendingString:@"/games"];
        }
        
        [self.navigationController pushViewController:controller animated:YES];
    });
}
-(void) pushGameSortWithSortId:(NSNumber*)sortId
{
    dispatch_async(dispatch_get_main_queue(), ^{
        GameSortResultsScreenController* controller = [[GameSortResultsScreenController alloc] init];
        controller.selectedSort = sortId;
        
        [self.navigationController pushViewController:controller animated:YES];
    });
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
    if (!keywords || keywords.length == 0)
        return;
    
    //This is how it should be done - later
    if (DFFlag::EnableNativeSearchGameResults)
    {
        [self pushSearchResultsType:SearchResultGames withKeyword:keywords];
    }
    else
    {
        NSString* url = [NSString stringWithFormat:@"%@games/?Keyword=%@",
                         [RobloxInfo getWWWBaseUrl],
                         [keywords stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
        [self pushWebControllerwithURL:url andTheme:RBXThemeGame];
    }
    
}
- (void) displaySelectedUser:(NSNotification*) notification
{
    RBXUserSearchInfo* user = (RBXUserSearchInfo*)[notification.userInfo objectForKey:@"userData"];
    [self pushProfileControllerWithUserID:user.userId];
}
- (void) displayUserSearchResults:(NSNotification*) notification
{
    //NSLog(@"Displaying User Search Results with notification : %@", notification);
    NSString* keywords = (NSString*)[notification.userInfo objectForKey:@"keywords"];
    if (!keywords || keywords.length == 0)
        return;
    
    if (DFFlag::EnableNativeSearchPeopleResults)
    {
        [self pushSearchResultsType:SearchResultUsers withKeyword:keywords];
    }
    else
    {
        NSString* url = [NSString stringWithFormat:@"%@%@%@",
                         [RobloxInfo getBaseUrl],
                         [RobloxInfo thisDeviceIsATablet] ? @"search/users?keyword=" : @"people?search=",
                         [keywords stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
        [self pushWebControllerwithURL:url andTheme:RBXThemeSocial];
    }
}


//IOS Flags
- (BOOL)isGameDetailEnabledOnPhone
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    return iOSSettings->GetValueEnableWebPageGameDetail();
}

@end
