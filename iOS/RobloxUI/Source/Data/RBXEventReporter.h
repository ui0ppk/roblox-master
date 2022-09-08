//
//  RBXEventReporter.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 6/11/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
@interface RBXEventReporter : NSObject

@property (atomic, strong, nullable) NSString* mobileDeviceID;

//Dictionary Keys as Enums
typedef NS_ENUM(NSInteger, RBXAnalyticsActionType)
{
    RBXAActionClose,
    RBXAActionSubmit,
    
    RBXAActionTypeCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsButtonName)
{
    RBXAButtonYes,
    RBXAButtonNo,
    RBXAButtonLogin,
    RBXAButtonSignup,
    RBXAButtonSocialSignIn,
    RBXAButtonAbout,
    RBXAButtonSubmit,
    RBXAButtonClose,
    RBXAButtonPlayNow,
    RBXAButtonSearchOpen,
    RBXAButtonSearchClose,
    RBXAButtonSearch,
    RBXAButtonRobux,
    RBXAButtonBuildersClub,
    RBXAButtonPopUpOkay,
    RBXAButtonPopUpCancel,
    RBXAButtonMoreButton,
    RBXAButtonSeeAll,
    RBXAButtonMessage,
    RBXAButtonFriendRequest,
    RBXAButtonFollow,
    RBXAButtonJoinInGame,
    RBXAButtonBadge,
    RBXAButtonReadMessage,
    RBXAButtonRefresh,
    RBXAButtonReply,
    RBXAButtonArchive,
    RBXAButtonConnect,
    RBXAButtonDisconnect,
    
    RBXAButtonNameCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsContextName)
{
    RBXAContextAppLaunch,
    RBXAContextAppEnterForeground,
    RBXAContextProtocolLaunch,
    RBXAContextLanding,
    RBXAContextSignup,
    RBXAContextLogin,
    RBXAContextSocialLogin,
    RBXAContextSocialSignup,
    RBXAContextCaptcha,
    RBXAContextSettings,
    RBXAContextSettingsEmail,
    RBXAContextSettingsPassword,
    RBXAContextSettingsSocial,
    RBXAContextSettingsSocialConnect,
    RBXAContextSettingsSocialDisconnect,
    RBXAContextLogout,
    RBXAContextMain,
    RBXAContextGames,
    RBXAContextGamesSeeAll,
    RBXAContextSearch,
    RBXAContextSearchResults,
    RBXAContextMessages,
    RBXAContextMore,
    RBXAContextPurchasingRobux,
    RBXAContextPurchasingBC,
    RBXAContextProfile,
    RBXAContextDraftMessage,
    RBXAContextHybrid,
    
    RBXAContextNameCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsCustomData)
{
    RBXACustomUsers,
    RBXACustomGames,
    RBXACustomCatalog,
    RBXACustomTabHome,
    RBXACustomTabGames,
    RBXACustomTabFriends,
    RBXACustomTabCatalog,
    RBXACustomTabMessages,
    RBXACustomTabMore,
    RBXACustomTabProfile,
    RBXACustomTabGroups,
    RBXACustomTabTrade,
    RBXACustomTabInventory,
    RBXACustomTabCharacter,
    RBXACustomTabForum,
    RBXACustomTabBlog,
    RBXACustomTabHelp,
    RBXACustomTabSettings,
    RBXACustomTabEvent,
    RBXACustomSectionInbox,
    RBXACustomSectionSent,
    RBXACustomSectionArchive,
    RBXACustomSectionNotifications,
    
    RBXACustomDataCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsEventName)
{
    RBXAEventAppLaunch,
    RBXAEventScreenLoaded,
    RBXAEventButtonClick,
    RBXAEventFormFieldValidation,
    RBXAEventFormInteraction,
    RBXAEventOpenGameDetail,
    RBXAEventError,
    
    RBXAEventNameCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsErrorName)
{
    RBXAErrorNoError,
    RBXAErrorUnknownLoginException,
    RBXAErrorNoConnection,
    RBXAErrorTooShort,
    RBXAErrorTooLong,
    RBXAErrorEmailInvalid,
    RBXAErrorMissingRequiredField,
    RBXAErrorBirthdayInvalid,
    RBXAErrorGenderInvalid,
    RBXAErrorPasswordInvalid,
    RBXAErrorPrivileged,
    RBXAErrorTwoStepVerification,
    RBXAErrorWindowClosed,
    RBXAErrorPasswordMismatch,
    RBXAErrorNoResponse,
    RBXAErrorNoResponseSuggestion,
    RBXAErrorEndpointReturnedError,
    RBXAErrorUsernameNotAllowed,
    RBXAErrorUsernameTaken,
    RBXAErrorUsernameInvalidWeb,
    RBXAErrorFloodcheck,
    RBXAErrorValidationJSONException,
    RBXAErrorJSONReadFailure,
    RBXAErrorJSONParseFailure,
    RBXAErrorIncorrectCaptcha,
    RBXAErrorMiscWebErrors,
    RBXAErrorBadResponse,
    RBXAErrorCannotParseData,
    RBXAErrorNotApproved,
    
    //Reporting Web Errors Analytics
    RBXAErrorHttpError400,
    RBXAErrorHttpError403,
    RBXAErrorHttpError404,
    RBXAErrorHttpError500,
    RBXAErrorUnsupportedEncoding,
    RBXAErrorJSON,
    RBXAErrorNoHTTPResponse,
    RBXAErrorInvalidUsername,
    RBXAErrorInvalidPassword,
    RBXAErrorMissingField,
    RBXAErrorFloodcheckSuccess,
    RBXAErrorFloodcheckFailure,
    RBXAErrorFloodcheckFailurePerUser,
    RBXAErrorFloodcheckAccountCreate,
    RBXAErrorAccountNotApproved,
    RBXAErrorIncompleteJSON,
    RBXAErrorMissingUserInfo,
    RBXAErrorGigya,
    RBXAErrorGigyaLogin,
    RBXAErrorGigyaKeyMissing,
    RBXAErrorPostLoginUnspecified,
    RBXAErrorCaptcha,
    RBXAErrorAlreadyAuthenticated,
    RBXAErrorUnknownError,
    RBXAErrorUnexpectedResponseCode,
    RBXAErrorBadCookieTryAgain,
    
    RBXAErrorNameCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsFieldName)
{
    RBXAFieldEmail,
    RBXAFieldUsername,
    RBXAFieldPassword,
    RBXAFieldPasswordVerify,
    RBXAFieldGender,
    RBXAFieldBirthday,
    RBXAFieldCaptcha,
    
    RBXAFieldNameCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsResult)
{
    RBXAResultUnknown,
    RBXAResultSuccess,
    RBXAResultFailure,
    
    RBXAResultCount
};
typedef NS_ENUM(NSInteger, RBXAnalyticsGameLocations)
{
    RBXALocationProfile,
    RBXALocationGames,
    RBXALocationGamesSeeAll,
    RBXALocationGameSearch,
    
    RBXALocationCount
};

//Constructor
+(nonnull id) sharedInstance;

//Acessors
-(nonnull NSString *) nameForContext:(RBXAnalyticsContextName)contextType;
-(nonnull NSString *) nameForResult:(RBXAnalyticsResult)resultType;
-(nonnull NSString *) nameForError:(RBXAnalyticsErrorName)errorType;


//GENERIC EVENTS
-(void) reportAppLaunch:(RBXAnalyticsContextName)context;
-(void) reportAppClose:(RBXAnalyticsContextName)context;


-(void) reportScreenLoaded:(RBXAnalyticsContextName)context;

-(void) reportButtonClick:(RBXAnalyticsButtonName)buttonName
              withContext:(RBXAnalyticsContextName)context;
-(void) reportButtonClick:(RBXAnalyticsButtonName)buttonName
              withContext:(RBXAnalyticsContextName)context
           withCustomData:(RBXAnalyticsCustomData)customData;
-(void) reportButtonClick:(RBXAnalyticsButtonName)buttonName
              withContext:(RBXAnalyticsContextName)context
     withCustomDataString:(nonnull NSString*)customDataString;
-(void) reportTabButtonClick:(RBXAnalyticsCustomData)buttonName;


-(void) reportFormFieldValidation:(RBXAnalyticsFieldName)fieldName
                      withContext:(RBXAnalyticsContextName)context
                        withError:(RBXAnalyticsErrorName)error;

-(void) reportOpenGameDetailFromSort:(nonnull NSNumber*)placeID
                            fromPage:(RBXAnalyticsGameLocations)page
                              inSort:(nonnull NSNumber*)sortID
                             atIndex:(nonnull NSNumber*)gameIndex
                    totalItemsInSort:(nonnull NSNumber*)count;
-(void) reportOpenGameDetailOnProfile:(nonnull NSNumber*)placeID
                             fromPage:(RBXAnalyticsGameLocations)page
                              atIndex:(nonnull NSNumber*)gameIndex
                     totalItemsInSort:(nonnull NSNumber*)count;
-(void) reportOpenGameDetailFromSearch:(nonnull NSNumber*)placeID
                              fromPage:(RBXAnalyticsGameLocations)page
                               atIndex:(nonnull NSNumber*)gameIndex
                      totalItemsInSort:(nonnull NSNumber*)count;

-(void) reportError:(RBXAnalyticsErrorName)error
        withContext:(RBXAnalyticsContextName)context
     withDataString:(nonnull NSString*)data;


@end
