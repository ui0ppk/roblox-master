//
//  RBXEventReporter.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 6/11/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBXEventReporter.h"
#import "RobloxInfo.h"
#import "RobloxData.h"
#import "Reachability.h"
#import "iOSSettingsService.h"
#import "RobloxWebUtility.h"
#import "RBXFunctions.h"
#import "SessionReporter.h"

#define PARAM_TARGET @"t"
#define PARAM_EVENT @"evt"
#define PARAM_CONTEXT @"ctx"
#define PARAM_LOCAL_TIME @"lt"
#define PARAM_BUTTON @"btn"
#define PARAM_FIELD @"field"
#define PARAM_ERROR @"error"
#define PARAM_ACTION_TYPE @"aType"
#define PARAM_RESULT @"result"
#define PARAM_CUSTOM @"cstm"
#define PARAM_MOBILE_DEVICE_ID @"mdid"
#define PARAM_SORT @"sort"
#define PARAM_PAGE @"pg"
#define PARAM_ITEMS_IN_SORT @"tis"
#define PARAM_PLACE_ID @"pid"
#define PARAM_APP_STORE_SOURCE @"appStoreSource"

#define REPORTS_FILE_NAME @"analytics.txt"
#define APP_STORE_SOURCE @"apple"

@implementation RBXEventReporter
{
    NSMutableArray* _actionTypes;
    NSMutableArray* _buttonNames;
    NSMutableArray* _contextNames;
    NSMutableArray* _customData;
    NSMutableArray* _eventNames;
    NSMutableArray* _errorNames;
    NSMutableArray* _fieldNames;
    NSMutableArray* _resultNames;
    NSMutableArray* _gameLocationNames;
}


//Constructors
+(id) sharedInstance
{
    static dispatch_once_t rbxAnalyticsPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxAnalyticsPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(id) init
{
    self = [super init];
    if (self)
    {
        [self initArrays];
    }
    return self;
}
-(void) initArrays
{
    //NOTE - it is important that all of these arrays match up to the enums
    _actionTypes = [NSMutableArray arrayWithCapacity:RBXAActionTypeCount];
    _actionTypes[RBXAActionClose]   = @"close";
    _actionTypes[RBXAActionSubmit]  = @"submit";
    
    _buttonNames = [NSMutableArray arrayWithCapacity:RBXAButtonNameCount];
    _buttonNames[RBXAButtonYes]         = @"yes";
    _buttonNames[RBXAButtonNo]          = @"no";
    _buttonNames[RBXAButtonLogin]       = @"login";
    _buttonNames[RBXAButtonSignup]      = @"signup";
    _buttonNames[RBXAButtonSocialSignIn]= @"socialSignIn";
    _buttonNames[RBXAButtonAbout]       = @"about";
    _buttonNames[RBXAButtonSubmit]      = @"submit";
    _buttonNames[RBXAButtonClose]       = @"close";
    _buttonNames[RBXAButtonPlayNow]     = @"playNow";
    _buttonNames[RBXAButtonSearchOpen]  = @"searchOpen";
    _buttonNames[RBXAButtonSearchClose] = @"searchClose";
    _buttonNames[RBXAButtonSearch]      = @"search";
    _buttonNames[RBXAButtonRobux]       = @"robux";
    _buttonNames[RBXAButtonBuildersClub]= @"buildersClub";
    _buttonNames[RBXAButtonPopUpOkay]   = @"popUpOkay";
    _buttonNames[RBXAButtonPopUpCancel] = @"popUpCancel";
    _buttonNames[RBXAButtonMoreButton]  = @"moreButton";
    _buttonNames[RBXAButtonSeeAll]      = @"seeAll";
    _buttonNames[RBXAButtonMessage]     = @"sendMessage";
    _buttonNames[RBXAButtonFriendRequest]=@"sendFriendRequest";
    _buttonNames[RBXAButtonFollow]      = @"toggleFollowUser";
    _buttonNames[RBXAButtonJoinInGame]  = @"joinInGame";
    _buttonNames[RBXAButtonBadge]       = @"badge";
    _buttonNames[RBXAButtonReadMessage] = @"readMessage";
    _buttonNames[RBXAButtonRefresh]     = @"refresh";
    _buttonNames[RBXAButtonReply]       = @"reply";
    _buttonNames[RBXAButtonArchive]     = @"archive";
    _buttonNames[RBXAButtonConnect]     = @"connect";
    _buttonNames[RBXAButtonDisconnect]  = @"disconnect";
    
    _contextNames= [NSMutableArray arrayWithCapacity:RBXAContextNameCount];
    _contextNames[RBXAContextAppLaunch]     = @"appLaunch";
    _contextNames[RBXAContextAppEnterForeground]= @"appEnterForeground";
    _contextNames[RBXAContextProtocolLaunch]= @"protocolLaunch";
    _contextNames[RBXAContextLanding]       = @"landing";
    _contextNames[RBXAContextSignup]        = @"signup";
    _contextNames[RBXAContextLogin]         = @"login";
    _contextNames[RBXAContextSocialLogin]   = @"socialLogin";
    _contextNames[RBXAContextSocialSignup]  = @"socialSignup";
    _contextNames[RBXAContextCaptcha]       = @"captcha";
    _contextNames[RBXAContextSettings]      = @"settings";
    _contextNames[RBXAContextSettingsEmail] = @"settingsEmail";
    _contextNames[RBXAContextSettingsPassword]=@"settingsPassword";
    _contextNames[RBXAContextSettingsSocial]= @"settingsSocial";
    _contextNames[RBXAContextSettingsSocialConnect]= @"settingsSocialConnect"; //needed for analytics
    _contextNames[RBXAContextSettingsSocialDisconnect]= @"settingsSocialDisconnect"; //needed for analytics, nothing more
    _contextNames[RBXAContextLogout]        = @"logout";
    _contextNames[RBXAContextMain]          = @"main";
    _contextNames[RBXAContextGames]         = @"games";
    _contextNames[RBXAContextGamesSeeAll]   = @"gamesSeeAll";
    _contextNames[RBXAContextSearch]        = @"search";
    _contextNames[RBXAContextSearchResults] = @"searchResults";
    _contextNames[RBXAContextMessages]      = @"messages";
    _contextNames[RBXAContextMore]          = @"more";
    _contextNames[RBXAContextPurchasingRobux]= @"purchasingRobux";
    _contextNames[RBXAContextPurchasingBC]  = @"purchasingBC";
    _contextNames[RBXAContextProfile]       = @"profile";
    _contextNames[RBXAContextDraftMessage]  = @"draftMessage";
    _contextNames[RBXAContextHybrid]        = @"rbhybrid";

    
    _customData  = [NSMutableArray arrayWithCapacity:RBXACustomDataCount];
    _customData[RBXACustomUsers]        = @"users";
    _customData[RBXACustomGames]        = @"games";
    _customData[RBXACustomCatalog]      = @"catalog";
    _customData[RBXACustomTabHome]      = @"tabHome";
    _customData[RBXACustomTabGames]     = @"tabGames";
    _customData[RBXACustomTabFriends]   = @"tabFriends";
    _customData[RBXACustomTabCatalog]   = @"tabCatalog";
    _customData[RBXACustomTabMessages]  = @"tabMessages";
    _customData[RBXACustomTabMore]      = @"tabMore";
    _customData[RBXACustomTabProfile]   = @"tabProfile";
    _customData[RBXACustomTabGroups]    = @"tabGroups";
    _customData[RBXACustomTabTrade]     = @"tabTrade";
    _customData[RBXACustomTabInventory] = @"tabInventory";
    _customData[RBXACustomTabCharacter] = @"tabCharacter";
    _customData[RBXACustomTabForum]     = @"tabForum";
    _customData[RBXACustomTabBlog]      = @"tabBlog";
    _customData[RBXACustomTabHelp]      = @"tabHelp";
    _customData[RBXACustomTabSettings]  = @"tabSettings";
    _customData[RBXACustomTabEvent]     = @"tabEvent";
    _customData[RBXACustomSectionInbox] = @"inbox";
    _customData[RBXACustomSectionSent]  = @"sent";
    _customData[RBXACustomSectionArchive]=@"archive";
    _customData[RBXACustomSectionNotifications]=@"notifications";
    
    _eventNames  = [NSMutableArray arrayWithCapacity:RBXAEventNameCount];
    _eventNames[RBXAEventAppLaunch]             = @"appLaunch";
    _eventNames[RBXAEventScreenLoaded]          = @"screenLoaded";
    _eventNames[RBXAEventButtonClick]           = @"buttonClick";
    _eventNames[RBXAEventFormFieldValidation]   = @"formFieldValidation";
    _eventNames[RBXAEventFormInteraction]       = @"formInteraction";
    _eventNames[RBXAEventOpenGameDetail]        = @"gameDetailReferral";
    _eventNames[RBXAEventError]                 = @"clientsideError";
    
    _errorNames  = [NSMutableArray arrayWithCapacity:RBXAErrorNameCount];
    _errorNames[RBXAErrorNoError]                   = @"NoError";
    _errorNames[RBXAErrorUnknownLoginException]     = @"UnknownLoginException";
    _errorNames[RBXAErrorNoConnection]              = @"NoInternetConnection";
    _errorNames[RBXAErrorTooShort]                  = @"TooShort";
    _errorNames[RBXAErrorTooLong]                   = @"TooLong";
    _errorNames[RBXAErrorEmailInvalid]              = @"EmailInvalid";
    _errorNames[RBXAErrorMissingRequiredField]      = @"MissingRequiredField";
    _errorNames[RBXAErrorBirthdayInvalid]           = @"BirthdayInvalid";
    _errorNames[RBXAErrorGenderInvalid]             = @"GenderInvalid";
    _errorNames[RBXAErrorPasswordInvalid]           = @"PasswordInvalid";
    _errorNames[RBXAErrorPrivileged]                = @"Privileged";
    _errorNames[RBXAErrorTwoStepVerification]       = @"TwoStepVerification";
    _errorNames[RBXAErrorWindowClosed]              = @"WindowClosed";
    _errorNames[RBXAErrorPasswordMismatch]          = @"PasswordMismatch";
    _errorNames[RBXAErrorNoResponse]                = @"NoResponse";
    _errorNames[RBXAErrorNoResponseSuggestion]      = @"NoResponseSuggestion";
    _errorNames[RBXAErrorEndpointReturnedError]     = @"EndpointReturnedError";
    _errorNames[RBXAErrorUsernameNotAllowed]        = @"UsernameNotAllowed";
    _errorNames[RBXAErrorUsernameTaken]             = @"UsernameTake";
    _errorNames[RBXAErrorUsernameInvalidWeb]        = @"UsernameInvalidWeb";
    _errorNames[RBXAErrorFloodcheck]                = @"Floodcheck";
    _errorNames[RBXAErrorValidationJSONException]   = @"ValidationJSONException";
    _errorNames[RBXAErrorJSONReadFailure]           = @"JSONReadFailure";
    _errorNames[RBXAErrorJSONParseFailure]          = @"JSONParseFailure";
    _errorNames[RBXAErrorIncorrectCaptcha]          = @"IncorrectCaptcha";
    _errorNames[RBXAErrorMiscWebErrors]             = @"misc_web_errors";
    _errorNames[RBXAErrorBadResponse]               = @"BadResponse";
    _errorNames[RBXAErrorCannotParseData]           = @"CannotParseData";
    _errorNames[RBXAErrorNotApproved]               = @"NotApprovedOrBanned";
    _errorNames[RBXAErrorHttpError400]              = @"HttpError400";
    _errorNames[RBXAErrorHttpError403]              = @"HttpError403";
    _errorNames[RBXAErrorHttpError404]              = @"HttpError404";
    _errorNames[RBXAErrorHttpError500]              = @"HttpError500";
    _errorNames[RBXAErrorUnsupportedEncoding]       = @"FailureUnsupportedEncoding";
    _errorNames[RBXAErrorJSON]                      = @"FailureJSON";
    _errorNames[RBXAErrorNoHTTPResponse]            = @"FailureNoResponse";
    _errorNames[RBXAErrorInvalidUsername]           = @"FailureInvalidUsername";
    _errorNames[RBXAErrorInvalidPassword]           = @"FailureInvalidPassword";
    _errorNames[RBXAErrorMissingField]              = @"FailureMissingField";
    _errorNames[RBXAErrorFloodcheckSuccess]         = @"FailureSuccessFloodcheck";
    _errorNames[RBXAErrorFloodcheckFailure]         = @"FailureFailedFloodcheck";
    _errorNames[RBXAErrorFloodcheckFailurePerUser]  = @"FailurePerUserFloodcheck";
    _errorNames[RBXAErrorFloodcheckAccountCreate]   = @"FailureAccountCreateFloodcheck";
    _errorNames[RBXAErrorAccountNotApproved]        = @"AccountNotApproved";
    _errorNames[RBXAErrorIncompleteJSON]            = @"FailureIncompleteJSON";
    _errorNames[RBXAErrorMissingUserInfo]           = @"MissingUserInfo";
    _errorNames[RBXAErrorGigya]                     = @"FailureGigya";
    _errorNames[RBXAErrorGigyaLogin]                = @"FailureGigyaLogin";
    _errorNames[RBXAErrorGigyaKeyMissing]           = @"FailureGigyaKeyMissing";
    _errorNames[RBXAErrorPostLoginUnspecified]      = @"FailurePostLoginUnspecified";
    _errorNames[RBXAErrorCaptcha]                   = @"FailureCaptcha";
    _errorNames[RBXAErrorAlreadyAuthenticated]      = @"AlreadyAuthenticated";
    _errorNames[RBXAErrorUnknownError]              = @"FailureUnknownError";
    _errorNames[RBXAErrorUnexpectedResponseCode]    = @"FailureUnexpectedResponseCode";
    _errorNames[RBXAErrorBadCookieTryAgain]         = @"FailureBadCookie";
    

    
    _fieldNames = [NSMutableArray arrayWithCapacity:RBXAFieldNameCount];
    _fieldNames[RBXAFieldEmail]         = @"email";
    _fieldNames[RBXAFieldUsername]      = @"username";
    _fieldNames[RBXAFieldPassword]      = @"password";
    _fieldNames[RBXAFieldPasswordVerify]= @"passwordVerify";
    _fieldNames[RBXAFieldGender]        = @"gender";
    _fieldNames[RBXAFieldBirthday]      = @"birthday";
    
    _resultNames = [NSMutableArray arrayWithCapacity:RBXAResultCount];
    _resultNames[RBXAResultUnknown] = @"unknown";
    _resultNames[RBXAResultSuccess] = @"success";
    _resultNames[RBXAResultFailure] = @"failure";
    
    _gameLocationNames = [NSMutableArray arrayWithCapacity:RBXALocationCount];
    _gameLocationNames[RBXALocationProfile]     = @"Profile";
    _gameLocationNames[RBXALocationGames]       = @"Games";
    _gameLocationNames[RBXALocationGamesSeeAll] = @"GamesSeeAll";
    _gameLocationNames[RBXALocationGameSearch]  = @"GameSearch";
}
-(void) initMobileDeviceId
{
    _mobileDeviceID = [RobloxData browserTrackerId];
    
    if ([RBXFunctions isEmpty:_mobileDeviceID])
        [RobloxData initializeBrowserTrackerWithCompletion:^(bool success, NSString *browserTracker) {
            if (success)
                _mobileDeviceID = browserTracker;
        }];
}

/////////////////////////////////////////////////////////////////////
// Accessors
-(NSString *) nameForContext:(RBXAnalyticsContextName)contextType
{
    return _contextNames[contextType];
}
-(NSString *) nameForResult:(RBXAnalyticsResult)resultType
{
    return _resultNames[resultType];
}
-(NSString *) nameForError:(RBXAnalyticsErrorName)errorType
{
    return _errorNames[errorType];
}



/////////////////////////////////////////////////////////////////////
// Event Reporting
// - creates reports based on the user interactions within the app
//   then enqueues them for the EventReporter to send them
-(NSString*) getLocalTime
{
    NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZZ"];
    [dateFormatter setLocale:[NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"]];
    
    return [dateFormatter stringFromDate:[NSDate date]];
}
-(void) reportEvent:(nonnull NSDictionary*)params
{
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    bool analyticsEnabled = iOSSettings->GetValueEnableAnalyticsEventReporting();
    if (!analyticsEnabled) { return; }
    
    
    //due to a timing issue, make sure the time is recorded before dispatching to a background thread
    NSString* timeOfRequest = [self getLocalTime];
    
    [RBXFunctions dispatchOnBackgroundThread:^
     {
        //Make sure we have a device id, can't report events without it
        if (!_mobileDeviceID)
        {
            [self initMobileDeviceId];
            if (!_mobileDeviceID)
            {
                //NSLog(@"Could not initialize the mobile device id, cannot report event.");
                return;
            }
        }
        
        //format the parameters, build in the mobileDeviceId, target, and local time
        NSString* paramsString = [NSString stringWithFormat:@"%@=%@&%@=%@&%@=%@&",
                                  PARAM_TARGET, @"mobile",
                                  PARAM_MOBILE_DEVICE_ID, _mobileDeviceID,
                                  PARAM_LOCAL_TIME, timeOfRequest];
        
        //loop through the provided parameters and build those into the string as well
        for (NSString* key in params)
            if (params[key] && ((NSString*)params[key]).length > 0)
                paramsString = [NSString stringWithFormat:@"%@%@=%@&", paramsString, key, params[key]];
            else
                NSLog(@"Empty parameter for key : %@",key);
        
        //remove the last ampersand from the formatted parameter string
        paramsString = [paramsString substringToIndex:paramsString.length - 1];
        
        
        //construct the url
        NSString* host = [[RobloxInfo getWWWBaseUrl] substringFromIndex:[@"http://www." length]];
        NSString* urlString = [NSString stringWithFormat:@"http://ecsv2.%@pe?%@",
                               host,
                               paramsString];
         
        //send the request to the server
        //NSLog(@"Reporting Event : %@", urlString);
        [RobloxData reportEvent:urlString
                 withCompletion:nil];
    }];
}



//App Life Cycle events
-(void) reportAppLaunch:(RBXAnalyticsContextName)context
{
    NSDictionary* eventParams = @{ PARAM_EVENT              : _eventNames[RBXAEventAppLaunch],
                                   PARAM_CONTEXT            : _contextNames[context],
                                   PARAM_APP_STORE_SOURCE   : APP_STORE_SOURCE };
    [self reportEvent:eventParams];
}
-(void) reportAppClose:(RBXAnalyticsContextName)context
{
    //DO SOMETHING
}


//Screen loaded
-(void) reportScreenLoaded:(RBXAnalyticsContextName)context
{
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventScreenLoaded],
                                   PARAM_CONTEXT : _contextNames[context] };
    [self reportEvent:eventParams];
}


//Button clicks
-(void) reportButtonClick:(RBXAnalyticsButtonName)buttonName
              withContext:(RBXAnalyticsContextName)context
{
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventButtonClick],
                                   PARAM_CONTEXT : _contextNames[context],
                                   PARAM_BUTTON  : _buttonNames[buttonName] };
    [self reportEvent:eventParams];
}
-(void) reportButtonClick:(RBXAnalyticsButtonName)buttonName
              withContext:(RBXAnalyticsContextName)context
           withCustomData:(RBXAnalyticsCustomData)customData
{
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventButtonClick],
                                   PARAM_CONTEXT : _contextNames[context],
                                   PARAM_BUTTON  : _buttonNames[buttonName],
                                   PARAM_CUSTOM  : _customData[customData] };
    [self reportEvent:eventParams];
}
-(void) reportButtonClick:(RBXAnalyticsButtonName)buttonName
              withContext:(RBXAnalyticsContextName)context
     withCustomDataString:(nonnull NSString*)customDataString
{
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventButtonClick],
                                   PARAM_CONTEXT : _contextNames[context],
                                   PARAM_BUTTON  : _buttonNames[buttonName],
                                   PARAM_CUSTOM  : customDataString };
    [self reportEvent:eventParams];
}
-(void) reportTabButtonClick:(RBXAnalyticsCustomData)buttonName
{
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventButtonClick],
                                   PARAM_CONTEXT : _contextNames[RBXAContextMain],
                                   PARAM_BUTTON  : _customData[buttonName] };
    [self reportEvent:eventParams];
}

//Form interactions / validations
-(void) reportFormFieldValidation:(RBXAnalyticsFieldName)fieldName
                      withContext:(RBXAnalyticsContextName)context
                        withError:(RBXAnalyticsErrorName)error
{
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventFormFieldValidation],
                                   PARAM_CONTEXT : _contextNames[context],
                                   PARAM_FIELD   : _fieldNames[fieldName],
                                   PARAM_ERROR   : _errorNames[error] };
    [self reportEvent:eventParams];
}


//Open Game
-(void) reportOpenGameDetailFromSort:(nonnull NSNumber*)placeID
                            fromPage:(RBXAnalyticsGameLocations)page
                              inSort:(nonnull NSNumber*)sortID
                             atIndex:(nonnull NSNumber*)gameIndex
                    totalItemsInSort:(nonnull NSNumber*)count
{
    NSString* context = [NSString stringWithFormat:@"gameSort_SortFilter%@_TimeFilter0_GenreFilter0_Position%@",sortID.stringValue, gameIndex.stringValue];
    NSDictionary* eventParams= @{ PARAM_EVENT           : _eventNames[RBXAEventOpenGameDetail],
                                  PARAM_CONTEXT         : context,
                                  PARAM_PAGE            : _gameLocationNames[page],
                                  PARAM_PLACE_ID        : placeID.stringValue,
                                  PARAM_ITEMS_IN_SORT   : count.stringValue };
    [self reportEvent:eventParams];
}
-(void) reportOpenGameDetailOnProfile:(nonnull NSNumber*)placeID
                             fromPage:(RBXAnalyticsGameLocations)page
                              atIndex:(nonnull NSNumber*)gameIndex
                     totalItemsInSort:(nonnull NSNumber*)count
{
    NSString* context = [NSString stringWithFormat:@"profile_Position%@", gameIndex.stringValue];
    NSDictionary* eventParams= @{ PARAM_EVENT           : _eventNames[RBXAEventOpenGameDetail],
                                  PARAM_CONTEXT         : context,
                                  PARAM_PAGE            : _gameLocationNames[page],
                                  PARAM_PLACE_ID        : placeID.stringValue,
                                  PARAM_ITEMS_IN_SORT   : count.stringValue };
    [self reportEvent:eventParams];
}
-(void) reportOpenGameDetailFromSearch:(nonnull NSNumber*)placeID
                    fromPage:(RBXAnalyticsGameLocations)page
                     atIndex:(nonnull NSNumber*)gameIndex
            totalItemsInSort:(nonnull NSNumber*)count
{
    NSString* context = [NSString stringWithFormat:@"gamesearch_Position%@", gameIndex.stringValue];
    NSDictionary* eventParams= @{ PARAM_EVENT           : _eventNames[RBXAEventOpenGameDetail],
                                  PARAM_CONTEXT         : context,
                                  PARAM_PAGE            : _gameLocationNames[page],
                                  PARAM_PLACE_ID        : placeID.stringValue,
                                  PARAM_ITEMS_IN_SORT   : count.stringValue };
    [self reportEvent:eventParams];
}

//Errors
-(void) reportError:(RBXAnalyticsErrorName)error
        withContext:(RBXAnalyticsContextName)context
     withDataString:(nonnull NSString*)data
{
    //useful for reporting general errors in the system
    NSDictionary* eventParams = @{ PARAM_EVENT   : _eventNames[RBXAEventError],
                                   PARAM_CONTEXT : _contextNames[context],
                                   PARAM_ERROR   : _errorNames[error],
                                   PARAM_CUSTOM  : data };
    [self reportEvent:eventParams];
}



@end
