//
//  RBBaseViewController.h
//  RobloxMobile
//
//  Created by Christian Hresko on 6/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RBXEventReporter.h"

typedef NS_ENUM(NSInteger, RBXWebRequestReturnStatus)
{
    RBXWebRequestReturnScreenPush = 0,
    RBXWebRequestReturnWebRequest = 1,
    RBXWebRequestReturnUnknown = 2,
    RBXWebRequestReturnFilter = 3
};

@interface RBBaseViewController : UIViewController

- (void) setViewTheme:(RBXTheme)theme;

// Flurry Events for Subclasses
- (void) setFlurryEventsForExternalLinkEvent:(NSString*)webExternalLinkEvent
                             andWebViewEvent:(NSString*)webLocalLinkEvent
                         andOpenProfileEvent:(NSString*)webProfileLinkEvent
                      andOpenGameDetailEvent:(NSString*)webGameDetailLinkEvent;

// Navigation Bar Style Functions
- (void) removeRobuxAndBCIcons;
- (void) addRobuxIconWithFlurryEvent:(NSString*)RobuxEvent
            andBCIconWithFlurryEvent:(NSString*)BCEvent;
- (void) addSearchIconWithSearchType:(SearchResultType)searchType
                  andFlurryEvent:(NSString*)searchEvent;
- (void) didPressEditAccount;

// Push ROMA views by parsing URLs in web URLs
-(RBXWebRequestReturnStatus) handleWebRequest:(NSURL*)aRequest;
-(void) handleWebRequestWithPopout:(NSURL*)aRequest;
-(void) pushWebControllerwithURL:(NSString*)stringURL andTheme:(RBXTheme)theme;
-(void) pushWebControllerwithURL:(NSString*)stringURL andTitle:(NSString*)pageTitle andTheme:(RBXTheme)theme;
-(void) pushProfileControllerWithUserID:(NSNumber*)userID;
-(void) pushProfileControllerWithURL:(NSString*)stringURL;
-(void) pushGameDetailWithURL:(NSString*)stringURL;
-(void) pushGameDetailWithGameData:(RBXGameData*)game;
-(void) pushSearchResultsType:(SearchResultType)resultType
                  withKeyword:(NSString*)searchKeywords;
@end
