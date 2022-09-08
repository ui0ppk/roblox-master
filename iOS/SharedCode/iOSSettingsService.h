//
//  iOSSettingsService.h
//  RobloxMobile
//
//  Created by Ganesh Agrawal on 10/30/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#ifndef __RobloxMobile__iOSSettingsService__
#define __RobloxMobile__iOSSettingsService__

#include "v8datamodel/FastLogSettings.h"
#include "util/Statistics.h"

#include <iostream>

/******************************************************************
The value are stored at iOSAppSettings

All exzample is shown with gametest3
To do it on prod replace <gametest3> with <prod>
 
Command to Create the Key with default value 2 for iPadMinimumVersion
SettingsConsole.exe gametest3 create iOSAppSettings.iPadMinimumVersion=2

Command to Set the Key, once created with above command
SettingsConsole.exe gametest3 get iOSAppSettings.iPadMinimumVersion=3
 
Command to Get all the Key
SettingsConsole.exe gametest3 get iOSAppSettings
******************************************************************/


class iOSSettingsService : public RBX::FastLogJSON
{
    
public:
    START_DATA_MAP(iOSSettingsService)
    DECLARE_DATA_INT(iPadMinimumVersion)
    DECLARE_DATA_INT(iPadMaximumVersion)
    DECLARE_DATA_INT(iPhoneMinimumVersion)
    DECLARE_DATA_INT(iPhoneMaximumVersion)
    DECLARE_DATA_INT(iPodMinimumVersion)
    DECLARE_DATA_INT(iPodMaximumVersion)
    DECLARE_DATA_BOOL(DisablePlayButtonForAll)
    DECLARE_DATA_BOOL(DisablePlayButtonForNonBC)
    DECLARE_DATA_INT(iPad1_MaximumIdealParts)
    DECLARE_DATA_INT(iPad2_MaximumIdealParts)
    DECLARE_DATA_INT(iPad3_MaximumIdealParts)
    DECLARE_DATA_INT(iPad4_MaximumIdealParts)
    DECLARE_DATA_INT(iPod4_MaximumIdealParts)
    DECLARE_DATA_INT(iPod5_MaximumIdealParts)
    DECLARE_DATA_INT(iPhone4s_MaximumIdealParts) // References iPhone model #5 (aka iPhone 4s)
    DECLARE_DATA_INT(iPhone5_MaximumIdealParts)  // References iPhone model #6 (aka iPhone 5)
    DECLARE_DATA_STRING(iOSGoogleAnalyticsAccount2)
    DECLARE_DATA_INT(iOSGoogleAnalyticsSampleRate)
    DECLARE_DATA_INT(TimeIntervalBetweenRobuxPurchaseInMinutes)
    DECLARE_DATA_INT(RobloxHDTimeIntervalBetweenRobuxPurchaseInMinutes)
    DECLARE_DATA_INT(TimeIntervalBetweenBCPurchaseInMinutes)
    DECLARE_DATA_INT(TimeIntervalBetweenCatalogPurchaseInMinutes)
    DECLARE_DATA_INT(TimeLimitForBillingServiceRetriesBeforeGivingUp)
    DECLARE_DATA_BOOL(AllowAppleInAppPurchase)
    DECLARE_DATA_BOOL(ReadInAppPurchaseSettingsBeforeEveryPurchase)
    DECLARE_DATA_INT(CrashLoggingLevel)
    DECLARE_DATA_INT(CrashlyticsPercentage)
    DECLARE_DATA_STRING(SearchEndpointIPad)
    DECLARE_DATA_STRING(SearchEndpointIPhone)
    DECLARE_DATA_BOOL(SignUpWithHash)
    DECLARE_DATA_BOOL(CacheUIWebViews)
    DECLARE_DATA_INT(ThumbstickControlStyle)
    DECLARE_DATA_BOOL(FreeMemoryCheckerActive)
    DECLARE_DATA_INT(FreeMemoryCheckerRateMilliSeconds)
    DECLARE_DATA_INT(FreeMemoryCheckerThresholdKiloBytes)
    DECLARE_DATA_BOOL(MemoryBouncerActive)
    DECLARE_DATA_INT(MemoryBouncerEnforceRateMilliSeconds)
    DECLARE_DATA_INT(MemoryBouncerThresholdKiloBytes)
    DECLARE_DATA_INT(MemoryBouncerLimitMegaBytes)
    DECLARE_DATA_INT(MemoryBouncerLimitMegaBytesForLowMemDevices)
    DECLARE_DATA_INT(MemoryBouncerDelayCount)
    DECLARE_DATA_INT(MemoryBouncerBlockSizeKB)
    DECLARE_DATA_INT(MaxMemoryReporterRateMilliSeconds)
    DECLARE_DATA_BOOL(DisplayMemoryWarning)
    DECLARE_DATA_INT(MaxLocalNotifications)
    DECLARE_DATA_INT(MaxLocalNotificationsPerUserID)
    DECLARE_DATA_BOOL(EnableSiteAlertBanner)
    DECLARE_DATA_BOOL(EnableFriendsOnProfile)
    DECLARE_DATA_BOOL(EnableLinkCharacter)
    DECLARE_DATA_BOOL(EnableLinkForgottenPassword)
    DECLARE_DATA_BOOL(EnableLinkForum)
    DECLARE_DATA_BOOL(EnableLinkTrade)
    DECLARE_DATA_BOOL(EnableWebPageGameDetail)
    DECLARE_DATA_BOOL(EnableABTestMobileGuestMode)
    DECLARE_DATA_BOOL(EnableABTestGuestFlavorText)
    DECLARE_DATA_BOOL(EnableAnalyticsEventReporting)
    DECLARE_DATA_INT(CalculateSessionReportEveryMilliSeconds)
    DECLARE_DATA_STRING(RBXGameGenres)
    END_DATA_MAP();
};

#endif /* defined(__RobloxMobile__iOSSettingsService__) */
