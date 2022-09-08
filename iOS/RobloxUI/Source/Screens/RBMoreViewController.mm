//
//  RBMoreViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/9/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBMoreViewController.h"
#import "MoreTileButton.h"
#import "RBMobileWebViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"
#import "RobloxNotifications.h"
#import "UITabBarItem+CustomBadge.h"
#import "RBXMessagesPollingService.h"
#import "iOSSettingsService.h"
#import "RobloxWebUtility.h"
#import "RBXFunctions.h"

#import "RBGroupsScreenController.h"
#import "RBInventoryScreenController.h"
#import "RBProfileViewController.h"
#import "RBWebMessagesViewController.h"
#import "RBWebProfileViewController.h"

#define MS_openRobux                @"MORE SCREEN - Open Robux"
#define MS_openBuildersClub         @"MORE SCREEN - Open Builders Club"
#define BLOG_openRobux              @"BLOG SCREEN - Open Robux"
#define BLOG_openBuildersClub       @"BLOG SCREEN - Open Builders Club"
#define CHARACTER_openRobux         @"CHARACTER SCREEN - Open Robux"
#define CHARACTER_openBuildersClub  @"CHARACTER SCREEN - Open Builders Club"
#define FORUM_openRobux             @"FORUM SCREEN - Open Robux"
#define FORUM_openBuildersClub      @"FORUM SCREEN - Open Builders Club"
#define SETTINGS_openRobux          @"SETTINGS SCREEN - Open Robux"
#define SETTINGS_openBuildersClub   @"SETTINGS SCREEN - Open Builders Club"
#define TRADE_openRobux             @"TRADE SCREEN - Open Robux"
#define TRADE_openBuildersClub      @"TRADE SCREEN - Open Builders Club"

DYNAMIC_FASTFLAGVARIABLE(EnablePinchToZoomOnSponsored, true);

@interface RBMoreViewController ()

@end

@implementation RBMoreViewController
{
    RBXAnalyticsCustomData mostRecentTab;
}

-(id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
        [self initNotificationPolling];
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    //localize strings
    [self setTitle:NSLocalizedString(@"MoreWord", nil)];
    [_lblEvents setText:NSLocalizedString(@"EventsWord", nil)];
    [_lblEvents setFont:[RobloxTheme fontH4]];
    
    //apply themes
    [self setViewTheme:RBXThemeGeneric];
    [self.view setBackgroundColor:[RobloxTheme lightBackground]];
    
    _btnEvent1.hidden = YES;
    _btnEvent2.hidden = YES;
    _btnEvent3.hidden = YES;
    _btnEvent4.hidden = YES;
    
    [self fetchEvents];
}
- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextMain];
    
    [self updateBadge];
    
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [_btnCharacter setIsEnabled:[self isCharacterLinkEnabled]];
        [_btnForum setIsEnabled:[self isForumLinkEnabled]];
        [_btnTrade setIsEnabled:[self isTradeLinkEnabled]];
    }
    
    [self fetchEvents];
    
    //set a default
    mostRecentTab = RBXACustomTabMore;
}

-(RBXAnalyticsCustomData) getMostRecentTab
{
    return mostRecentTab;
}

#pragma mark
#pragma Button Actions
-(IBAction)openBlog:(id)sender
{
    mostRecentTab = RBXACustomTabBlog;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //this URL is responsive and should be fine
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:@"http://blog.roblox.com"
                                                              withTitle:NSLocalizedString(@"BlogWord", nil)
                                                               andTheme:RBXThemeSocial
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:BLOG_openRobux
                                                       andBCFlurryEvent:BLOG_openBuildersClub];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openCharacter:(id)sender
{
    mostRecentTab = RBXACustomTabCharacter;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //this will cause problems on iPhone
    NSString* url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"My/Character.aspx"];
    
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:NSLocalizedString(@"CharacterWord", nil)
                                                               andTheme:RBXThemeCreative
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:CHARACTER_openRobux
                                                       andBCFlurryEvent:CHARACTER_openBuildersClub];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openForum:(id)sender
{
    mostRecentTab = RBXACustomTabForum;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //this runs risks on iPhone
    NSString* url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"Forum/default.aspx"];
    
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:NSLocalizedString(@"ForumWord", nil)
                                                               andTheme:RBXThemeSocial
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:FORUM_openRobux
                                                       andBCFlurryEvent:FORUM_openBuildersClub];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openGroups:(id)sender
{
    mostRecentTab = RBXACustomTabGroups;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //RBGroupsScreenController* screen = [[RBGroupsScreenController alloc] initWithNavButtons:YES];
    //[self.navigationController pushViewController:screen animated:YES];
    
    NSString* url = [[RobloxInfo getBaseUrl] stringByAppendingString:[RobloxInfo thisDeviceIsATablet] ? @"My/Groups.aspx" : @"my-groups"];
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:NSLocalizedString(@"GroupsWord", nil)
                                                               andTheme:RBXThemeSocial
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:nil
                                                       andBCFlurryEvent:nil];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openHelp:(id)sender
{
    mostRecentTab = RBXACustomTabHelp;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //NSString* helpURL = [NSString stringWithFormat:@"http://%@.help.roblox.com/hc/%@"] //figure out how to localize this url?!!
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:@"http://en.help.roblox.com/hc/en-us"
                                                              withTitle:NSLocalizedString(@"HelpWord", nil)
                                                               andTheme:RBXThemeGeneric
                                                               addIcons:NO
                                                   withRobuxFlurryEvent:nil
                                                       andBCFlurryEvent:nil];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openInventory:(id)sender
{
    mostRecentTab = RBXACustomTabInventory;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //RBInventoryScreenController* screen = [[RBInventoryScreenController alloc] initWithNavButtons:YES];
    //[self.navigationController pushViewController:screen animated:YES];
    
    NSString* url = [NSString stringWithFormat:@"%@users/%@/inventory#!/hats", [RobloxInfo getWWWBaseUrl], [UserInfo CurrentPlayer].userId];
    
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:NSLocalizedString(@"InventoryWord", nil)
                                                               andTheme:RBXThemeGame
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:nil
                                                       andBCFlurryEvent:nil];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openCatalog:(id)sender
{
    mostRecentTab = RBXACustomTabCatalog;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    NSString* url = [[RobloxInfo getBaseUrl] stringByAppendingString:@"catalog/"];

    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:NSLocalizedString(@"CatalogWord", nil)
                                                               andTheme:RBXThemeGame
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:CHARACTER_openRobux
                                                       andBCFlurryEvent:CHARACTER_openRobux];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openProfile:(id)sender
{
    mostRecentTab = RBXACustomTabProfile;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    [self pushProfileControllerWithUserID:[UserInfo CurrentPlayer].userId];
}
-(IBAction)openSettings:(id)sender
{
    mostRecentTab = RBXACustomTabSettings;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //if ([RobloxInfo thisDeviceIsATablet])
    //    [self didPressEditAccount];
    //else
        [self performSegueWithIdentifier:@"pushSettings" sender:self];
}
-(IBAction)openTrade:(id)sender
{
    mostRecentTab = RBXACustomTabTrade;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    //THIS WILL CAUSE PROBLEMS ON iPhone
    NSString* url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"My/Money.aspx"];
    
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:NSLocalizedString(@"TradeWord", nil)
                                                               andTheme:RBXThemeSocial
                                                           addIcons:YES
                                                     withRobuxFlurryEvent:TRADE_openRobux
                                                         andBCFlurryEvent:TRADE_openBuildersClub];
    [self.navigationController pushViewController:moreWeb animated:YES];
}
-(IBAction)openEvent:(id)sender
{
    mostRecentTab = RBXACustomTabEvent;
    [[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonMoreButton withContext:RBXAContextMain withCustomData:mostRecentTab];
    
    MoreTileButton* btnSender = sender;
    RBXSponsoredEvent* eventInfo = (RBXSponsoredEvent*)btnSender.extraInfo;
    
    //open the page held at the url
    NSString* url = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:eventInfo.eventPageURLExtension];
    
    RBMobileWebViewController* moreWeb = [self makeWebControllerwithURL:url
                                                              withTitle:eventInfo.eventName
                                                               andTheme:RBXThemeGame
                                                               addIcons:YES
                                                   withRobuxFlurryEvent:nil
                                                       andBCFlurryEvent:nil];
    [moreWeb setToShowWholeScreen: DFFlag::EnablePinchToZoomOnSponsored];
    [self.navigationController pushViewController:moreWeb animated:YES];
}

-(RBMobileWebViewController*) makeWebControllerwithURL:(NSString*)stringURL
                       withTitle:(NSString*)pageTitle
                        andTheme:(RBXTheme)viewTheme
                        addIcons:(BOOL)addIcons
            withRobuxFlurryEvent:(NSString*)flurryRobux
                andBCFlurryEvent:(NSString*)flurryBC

{
    RBMobileWebViewController* moreWeb = [[RBMobileWebViewController alloc] initWithNavButtons:YES];
    
    [moreWeb setViewTheme:viewTheme];
    [moreWeb setUrl:stringURL];
    [moreWeb setTitle:pageTitle];
    
    if (addIcons)
        [moreWeb addRobuxIconWithFlurryEvent:flurryRobux andBCIconWithFlurryEvent:flurryBC];
    
    return moreWeb;
}

#pragma mark
#pragma Helper Functions
-(void) initMoreButton:(MoreTileButton*)aButton withEvent:(RBXSponsoredEvent*)eventData
{
    if (!aButton) return;
    
    dispatch_async(dispatch_get_main_queue(), ^
    {
        if (eventData)
        {
            UIImage* eventImage = [UIImage imageWithData:[NSData dataWithContentsOfURL:[NSURL URLWithString:eventData.eventLogoURLString]]];
            [aButton setImageName:eventImage];
            [aButton setExtraInfo:eventData];
            aButton.hidden = NO;
        }
        else
        {
            aButton.hidden = YES;
        }
    });
}
-(void) fetchEvents
{
    [RobloxData fetchSponsoredEvents:^(NSArray *events)
     {
         [RBXFunctions dispatchOnMainThread:^
          {
              _lblEvents.hidden = (events.count == 0);
              
              RBXSponsoredEvent* evt1 = (events.count >= 1) ? events[0] : nil;
              RBXSponsoredEvent* evt2 = (events.count >= 2) ? events[1] : nil;
              RBXSponsoredEvent* evt3 = (events.count >= 3) ? events[2] : nil;
              RBXSponsoredEvent* evt4 = (events.count >= 4) ? events[3] : nil;
              
              //format phone event buttons
              if (![RobloxInfo thisDeviceIsATablet])
              {
                  float contentWidth =  _btnCatalog.width;
                  
                  switch (events.count)
                  {
                      case 1: {
                          //one big wide button
                          [_btnEvent1 setWidth:contentWidth];
                      }break;
                      case 2: {
                          //two stacked wide buttons
                          [_btnEvent1 setWidth:contentWidth];
                          [_btnEvent3 setWidth:contentWidth];
                          
                          evt2 = nil;
                          evt3 = events[1];
                      }break;
                      case 3: {
                          //one big wide button
                          //two small buttons below
                          [_btnEvent1 setWidth:contentWidth];
                          [_btnEvent3 setWidth:_btnEvent4.width];
                          
                          evt2 = nil;
                          evt3 = events[1];
                          evt4 = events[2];
                      }break;
                      default:
                      {
                          //four small buttons
                          [_btnEvent1 setWidth:_btnEvent4.width];
                          [_btnEvent2 setWidth:_btnEvent4.width];
                          [_btnEvent3 setWidth:_btnEvent4.width];
                      }
                  }
              }
              
              //update the button's target - passing nil hides the button
              [self initMoreButton:_btnEvent1 withEvent:evt1];
              [self initMoreButton:_btnEvent2 withEvent:evt2];
              [self initMoreButton:_btnEvent3 withEvent:evt3];
              [self initMoreButton:_btnEvent4 withEvent:evt4];
          }];
         
     }];
}


#pragma mark
#pragma iOS Settings Flags
- (BOOL)isCharacterLinkEnabled
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    return iOSSettings->GetValueEnableLinkCharacter();
}
- (BOOL)isForumLinkEnabled
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    return iOSSettings->GetValueEnableLinkForum();
}
- (BOOL)isTradeLinkEnabled
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    return iOSSettings->GetValueEnableLinkTrade();
}

#pragma mark
#pragma Notification Polling functions
-(void) initNotificationPolling
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateBadge)  name:RBX_NOTIFY_LOGGED_OUT object:nil];
    [self updateBadge];
}
-(void) updateBadge
{
    dispatch_async(dispatch_get_main_queue(), ^
   {
       if (![UserInfo CurrentPlayer].userLoggedIn)
       {
           [_btnSettings setBadgeValue:nil];
           
           if ([self navigationController] && [[self navigationController] tabBarItem])
               [self.navigationController.tabBarItem setBadgeValue:nil];
           return;
       }
       
       //dispatch the update command to the main thread for instant update
       int totalSettings = 0;
       if ([UserInfo CurrentPlayer].accountNotifications)
       {
           totalSettings += ([UserInfo CurrentPlayer].accountNotifications.passwordNotificationEnabled && [UserInfo CurrentPlayer].password == nil) ? 1 : 0;
           totalSettings += ([UserInfo CurrentPlayer].accountNotifications.emailNotificationEnabled    && [UserInfo CurrentPlayer].userEmail == nil)? 1 : 0;
       }

       
       //add the badge to the settings button
       if (_btnSettings)
           [_btnSettings setBadgeValue:(totalSettings <= 0) ? nil : [NSString stringWithFormat:@"%i",totalSettings]];

       //add the badge to the navigational tab bar
       if ([self navigationController] && [[self navigationController] tabBarItem])
       {
           [self.navigationController.tabBarItem setBadgeValue:((totalSettings <= 0) ? nil : [NSString stringWithFormat:@"%i",totalSettings])];
       }
   });
}

@end
