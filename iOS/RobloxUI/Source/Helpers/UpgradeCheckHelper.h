//
//  UpgradeCheckHelper.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 4/9/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//
// This class is used by AppDelegate to determine whether our client version is up to date
// with what we offer in the app store.  If not, this class will alert this user.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface UpgradeCheckHelper : NSObject <UIAlertViewDelegate, NSURLConnectionDataDelegate>
{
    UIAlertView* upgradeAlertView;
    NSURLConnection* upgradeConnection;
    
    NSMutableData* upgradeResponseData;
}

+ (UpgradeCheckHelper*) getUpgradeCheckHelper;

+ (NSString *) getUpgradeUrl;
+ (void) checkForUpdate:(NSString*) appName;

@end
