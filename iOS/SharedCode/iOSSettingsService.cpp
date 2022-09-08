//
//  iOSSettingsService.cpp
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 10/30/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#include "iOSSettingsService.h"

// Setting Maximum to 0 will allow to run on all versions up
// Setting Minimum to 0 will not allow to run any versions of that device
// The versions are based on Apple's IPSW prefix which is in the format of iPhone3,1
// http://theiphonewiki.com/wiki/index.php?title=Models

DATA_MAP_IMPL_START(iOSSettingsService)

IMPL_DATA(iPadMinimumVersion, 1);// iPad 2
IMPL_DATA(iPadMaximumVersion, 0); // to Allow all up versions
IMPL_DATA(iPhoneMinimumVersion, 1);// 4 for iPhone 4S
IMPL_DATA(iPhoneMaximumVersion, 0); // to allow all up versions
IMPL_DATA(iPodMinimumVersion, 1); // 5 for iPod 5th Gen 4" with Retina
IMPL_DATA(iPodMaximumVersion, 0); // to allow all up versions
IMPL_DATA(DisablePlayButtonForAll, false);
IMPL_DATA(DisablePlayButtonForNonBC, false);
IMPL_DATA(iPad1_MaximumIdealParts, 0);
IMPL_DATA(iPad2_MaximumIdealParts, 0);
IMPL_DATA(iPad3_MaximumIdealParts, 0);
IMPL_DATA(iPad4_MaximumIdealParts, 0);
IMPL_DATA(iPod4_MaximumIdealParts, 0);
IMPL_DATA(iPod5_MaximumIdealParts, 0);
IMPL_DATA(iPhone4s_MaximumIdealParts, 0); // References iPhone model #5 (aka iPhone 4s)
IMPL_DATA(iPhone5_MaximumIdealParts, 0);  // References iPhone model #6 (aka iPhone 5)
IMPL_DATA(TimeIntervalBetweenRobuxPurchaseInMinutes, 10);// 10 Minutes
IMPL_DATA(RobloxHDTimeIntervalBetweenRobuxPurchaseInMinutes, 0);// 0 Minutes
IMPL_DATA(TimeIntervalBetweenBCPurchaseInMinutes, 60*24);// 24 Hours
IMPL_DATA(TimeIntervalBetweenCatalogPurchaseInMinutes, 10);// 10 Minutes
IMPL_DATA(TimeLimitForBillingServiceRetriesBeforeGivingUp, 48); // 48 Hours, Keep retrying with billing service for 48 hours
IMPL_DATA(AllowAppleInAppPurchase, true);
IMPL_DATA(ReadInAppPurchaseSettingsBeforeEveryPurchase, false);

// Crash Reporter
IMPL_DATA(CrashLoggingLevel, 4); // No Messages = 4, Error Only = 3, Error & Warning = 2, Error, Warning & Info = 1, All Messages = 0
IMPL_DATA(CrashlyticsPercentage, 100);

// Set by Default to Empty as we do not want to pollute either our Test account or Prod account
// Test GA Account Roblox Mobile App (Testing) use  : "UA-42322750-2"
// Prod GA Account ROBLOX Mobile App (prod) use     : "UA-42322750-1", This is set on client settings on www.roblox.com
IMPL_DATA(iOSGoogleAnalyticsAccount2, "UA-42322750-1");
IMPL_DATA(iOSGoogleAnalyticsSampleRate, 100);

// Used to determine the endpoint for search. Set to empty string to make
// search bar disappear.
IMPL_DATA(SearchEndpointIPad, "");
IMPL_DATA(SearchEndpointIPhone, "");

IMPL_DATA(SignUpWithHash, true);

// Used to turn UIWebViewCacheManager on/off.  This cache allows for uiwebviews to 
// load instantaneously, but it takes around 60 mb of RAM from our app!
IMPL_DATA(CacheUIWebViews, false);

// Controls how our thumbstick behaves:
// 0 -> Stationary Classic Thumbstick
// 1 -> Thumbstick will follow thumb after a certain distance
IMPL_DATA(ThumbstickControlStyle, 1);

// FreeMemoryChecker - to exit out of place before iOS kills the app
IMPL_DATA(FreeMemoryCheckerActive, false);
IMPL_DATA(FreeMemoryCheckerRateMilliSeconds, 10000); // every 10 seconds
IMPL_DATA(FreeMemoryCheckerThresholdKiloBytes, 20480); // 20 MB free

// Memory Bouncer - forcefully grabs memory to force unloading of background apps
IMPL_DATA(MemoryBouncerActive, false);
IMPL_DATA(MemoryBouncerEnforceRateMilliSeconds, 100); // every .1 seconds
IMPL_DATA(MemoryBouncerThresholdKiloBytes, 5120); // 5 MB free
IMPL_DATA(MemoryBouncerLimitMegaBytes, 250); // 250 MB target
IMPL_DATA(MemoryBouncerLimitMegaBytesForLowMemDevices, 0); // low limit for older devices (iPod4, iPad1, etc) 0 = will disable bouncer to prevent out of memory crashes
IMPL_DATA(MemoryBouncerDelayCount, 0);
IMPL_DATA(MemoryBouncerBlockSizeKB, 0);

IMPL_DATA(MaxMemoryReporterRateMilliSeconds, 0);

IMPL_DATA(DisplayMemoryWarning, false);

IMPL_DATA(MaxLocalNotifications, 20);
IMPL_DATA(MaxLocalNotificationsPerUserID, 10);

IMPL_DATA(EnableSiteAlertBanner, true);

//Profile and Home Page Features
IMPL_DATA(EnableFriendsOnProfile, false);

//Non-Responsive Links
IMPL_DATA(EnableLinkCharacter, false);
IMPL_DATA(EnableLinkForum, false);
IMPL_DATA(EnableLinkForgottenPassword, false);
IMPL_DATA(EnableLinkTrade, false);
IMPL_DATA(EnableWebPageGameDetail, false);

//AB Tests
IMPL_DATA(EnableABTestMobileGuestMode, true);
IMPL_DATA(EnableABTestGuestFlavorText, true);

//Event Reporting
IMPL_DATA(EnableAnalyticsEventReporting, true);

IMPL_DATA(CalculateSessionReportEveryMilliSeconds, 100);

//Genres
IMPL_DATA(RBXGameGenres, "All-1|Adventure-13|Building-19|Comedy-15|Fighting-10|FPS-20|Horror-11|Medieval-8|Military-17|Naval-12|RPG-21|SciFi-9|Sports-14|Town+and+City-7|Western-16");

DATA_MAP_IMPL_END()