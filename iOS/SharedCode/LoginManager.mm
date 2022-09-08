//
//  LoginManager.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 4/19/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "LoginManager.h"
#import <FacebookSDK/FacebookSDK.h>
#import <Foundation/NSURL.h>

#import "ABTestManager.h"
#import "CrashReporter.h"
#import "iOSSettingsService.h"
#import "KeychainItemWrapper.h"
#import "NSDictionary+Parsing.h"
#import "UserInfo.h"
#import "RBCaptchaViewController.h"
#import "RBCaptchaV2ViewController.h"
#import "Reachability.h"
#import "RobloxAlert.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxInfo.h"
#import "RobloxNotifications.h"
#import "RobloxWebUtility.h"
#import "RobloxXSRFRequest.h"
#import "RBXFunctions.h"
#import "SessionReporter.h"
#import "SignupVerifier.h"
#include "util/standardout.h"
#include "util/Http.h"

DYNAMIC_FASTFLAGVARIABLE(EnableSessionLogin, false);
DYNAMIC_FASTFLAGVARIABLE(EnableFacebookLoginMaster, false);
DYNAMIC_FASTFLAGVARIABLE(EnableLoginLogoutSignupWithApi, false);

@implementation NSString (Escaping)
- (NSString*)stringWithPercentEscape
{
    return [self stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet alphanumericCharacterSet]];
}
@end

@interface LoginManager()

@property (nonatomic) BOOL rememberPassword;
@property (nonatomic) BOOL performingAutoLogin;
@property (nonatomic) int  loginRetryCounter;
@property (nonatomic, strong) NSOperationQueue *requestQueue;

@end

@implementation LoginManager

#pragma mark - odd Captcha functions
+(NonRotatableNavigationController*) CaptchaForLoginWithUsername:(NSString*)username
                                                 andV1Completion:(SocialLoginCompletionBlock)completion1
                                                 andV2Completion:(LoginManagerCompletionBlock)completion2
{
    UIViewController* controller = [LoginManager apiProxyEnabled]   ? [RBCaptchaV2ViewController CaptchaV2ForLoginWithUsername:username completionHandler:completion2]
                                                                    : [RBCaptchaViewController CaptchaWithCompletionHandler:completion1];
    NonRotatableNavigationController* navigation = [[NonRotatableNavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    
    return navigation;
}
+(NonRotatableNavigationController*) CaptchaForSignupWithUsername:(NSString*)username
                                                  andV1Completion:(SocialLoginCompletionBlock)completion1
                                                  andV2Completion:(LoginManagerCompletionBlock)completion2
{
    UIViewController* controller = [LoginManager apiProxyEnabled]   ? [RBCaptchaV2ViewController CaptchaV2ForSignupWithUsername:username andCompletion:completion2]
                                                                    : [RBCaptchaViewController CaptchaWithCompletionHandler:completion1];
    NonRotatableNavigationController* navigation = [[NonRotatableNavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    
    return navigation;
}
+(NonRotatableNavigationController*) CaptchaForSocialSignupWithUsername:(NSString*)username
                                                        andV1Completion:(SocialLoginCompletionBlock)completion1
                                                        andV2Completion:(LoginManagerCompletionBlock)completion2
{
    UIViewController* controller = [LoginManager apiProxyEnabled]   ? [RBCaptchaV2ViewController CaptchaV2ForSocialSignupWithUsername:username andCompletion:completion2]
                                                                    : [RBCaptchaViewController CaptchaWithCompletionHandler:completion1];
    NonRotatableNavigationController* navigation = [[NonRotatableNavigationController alloc] initWithRootViewController:controller];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    
    return navigation;
}


#pragma mark LIFE CYCLE FUNCTIONS
+ (id)sharedInstance
{
    static dispatch_once_t rbxLoginManPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxLoginManPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(id) init
{
    if(self = [super init])
    {
        self.rememberPassword = [[[NSUserDefaults standardUserDefaults] objectForKey:@"rememberMyPassword"] boolValue];
        self.loginRetryCounter = 0;
        self.requestQueue = [[NSOperationQueue alloc] init];
    }
    
    [LoginManager initializeCookieManagementPolicy];
    
    [[NSNotificationCenter defaultCenter] addObserver:[self class]
                                             selector:@selector(updateCookiesInAppHttpLayer)
                                                 name:NSHTTPCookieManagerCookiesChangedNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:[self class]
                                             selector:@selector(updateCookiesInAppHttpLayer)
                                                 name:RBX_NOTIFY_LOGIN_SUCCEEDED
                                               object:nil];
    
    return self;
}

#pragma mark - LIFE CYCLE FUNCTIONS
- (void) applicationWillTerminate
{
    [self processBackground];
    
    [self doSocialLogout];
}

#pragma mark LOG IN FUNCTIONS

+ (BOOL) apiProxyEnabled
{
    BOOL val = DFFlag::EnableLoginLogoutSignupWithApi;
    return val;
}

- (void) loginWithUsername:(NSString *) username password:(NSString *)password completionBlock:(LoginManagerCompletionBlock)loginCompletionBlock
{
    if ([[self class] apiProxyEnabled] == YES) {
        [self loginV2WithUsername:username password:password completionBlock:loginCompletionBlock];
        return;
    }
    
    //check for a connection, error messages are automattically dispatched
    if (![self isConnectedToInternet])
    {
        if (nil != loginCompletionBlock) {
            loginCompletionBlock([NSError errorWithDomain:@"Not connected to internet" code:999 userInfo:nil]);
        }
        return;
    }
    
    //construct the url
    //NOTE- APPEND A TRAILING SLASH IN THE URL TO ENSURE THAT JSON IS RETURNED -Kyler (10/12/2015)
    NSString* loginUrlString = [[RobloxInfo getSecureBaseUrl] stringByAppendingString:@"/mobileapi/login/"];
    NSURL *url = [NSURL URLWithString: loginUrlString];
    NSDate* startingTime = [NSDate date];
    
    //configure the request
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60*7];
    NSString *encodedUserName = [username stringWithPercentEscape];
    NSString *encodedPassword = [password stringWithPercentEscape];
    NSString *postData = [NSString stringWithFormat:@"username=%@&password=%@", encodedUserName, encodedPassword];
    NSString *length = [NSString stringWithFormat:@"%lu", (unsigned long)[postData length]];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setValue:length forHTTPHeaderField:@"Content-Length"];
    [theRequest setHTTPMethod:@"POST"];
    [theRequest setHTTPBody:[postData dataUsingEncoding:NSASCIIStringEncoding]];
    
#ifdef RBX_INTERNAL
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginManager: Posting login attempt with postData = %s", [postData cStringUsingEncoding:NSUTF8StringEncoding]);
#else
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginManager: Posting login attempt with postData = <omitted to protect password, create internal build for postdata string>");
#endif
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginViewController: Attempting login connection with url = %s", [loginUrlString cStringUsingEncoding:NSUTF8StringEncoding]);
    
    //__weak LoginManager* weakSelf = self;
    
    //send the request
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *data, NSError *error)
     {
         NSDictionary* userLoginInfo = [[NSDictionary alloc] initWithObjectsAndKeys:encodedUserName,ENCODED_USERNAME_KEY,
                                        encodedPassword,ENCODED_PW_KEY,
                                        password,NONENCODED_PW_KEY, nil];
         
         
         [self processLoginResponse:response
                         forRequest:theRequest
                          startedAt:startingTime
                          loginData:data
                         loginError:error
                      userLoginInfo:userLoginInfo
                  completionHandler:^(NSError *loginResponseError) {
             
             if ([RBXFunctions isEmpty:loginResponseError])
             {
                 //we have a successful login
                 [[ABTestManager sharedInstance] fetchExperimentsForUser:[UserInfo CurrentPlayer].userId];
                 
                 //mark the user as logged in
                 [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
                 
                 //establish a gigya session
                 [self doSocialNotifyGigyaLoginWithContext:RBXAContextLogin withCompletion:^(bool success, NSString *message) //weakself
                  {
                      //NSLog(@"Gigya Notify Login (Success, Message) : (%@, %@)", success ? @"YES" : @"NO", message);
                      [self processBackground]; //weakself
                  }];
                 
                 //post a notification
                 [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:self userInfo:userLoginInfo];
                 
                 self.loginRetryCounter = 0; //weakself
             }
             else
             {
                 NSString *errorCode = loginResponseError.domain;
                 if ([errorCode isEqualToString:NSLocalizedString(@"BadCookieTryAgain", nil)])
                 {
                     //For some reason, an authenticated user is trying to login
                     //Hitting this error SHOULD have corrected the cookie state
                     if (self.loginRetryCounter < 1) //weakself
                     {
                         self.loginRetryCounter++; //weakself
                         [self loginWithUsername:username password:password completionBlock:nil]; //weakself
                     }
                     else
                     {
                         //to prevent us from getting into an infinite loop, bail out after two attempts
                         //for safety, mark the user as not logged in
                         [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
                         
                         //report what went wrong
                         errorCode = NSLocalizedString(@"BadCookieError", nil);
                         NSDictionary* errorDict = [[NSDictionary alloc] initWithObjectsAndKeys:errorCode,@"Error", nil];
                         [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:self userInfo:errorDict];
                         
                         //keep some analytics on this kind of event.
                         [[RBXEventReporter sharedInstance] reportError:RBXAErrorBadCookieTryAgain withContext:RBXAContextLogin withDataString:@"FailedAfterMultipleRetries"];
                         
                         //reset the retry counter
                         self.loginRetryCounter = 0;
                     }
                 }
                 else
                 {
                     //for safety, mark the user as not logged in
                     [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
                     
                     //report what went wrong
                     NSDictionary* errorDict = [[NSDictionary alloc] initWithObjectsAndKeys:errorCode,@"Error", nil];
                     [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:self userInfo:errorDict];
                     
                     //reset the retry counter
                     self.loginRetryCounter = 0;
                 }
             }
             
             if (nil != loginCompletionBlock) {
                 loginCompletionBlock(loginResponseError);
             }
         }];
     }];
}

- (void) processLoginResponse:(NSURLResponse *)response forRequest:(NSURLRequest *)request startedAt:(NSDate *)startTime loginData:(NSData *)responseData loginError:(NSError *)error userLoginInfo:(NSDictionary *)userLoginInfo completionHandler:(void(^)(NSError *loginResponseError))loginResponseCompletionHandler
{
    //keep some analytic information
    RBXAnalyticsResult analyticsResult = RBXAResultUnknown;
    RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
    NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
    
    //initialize the message for that the user will see if something went wrong
    //NOTE- an empty string means a successful login
    NSString *message = nil;
    
    NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
    if(httpResponse == nil)
    {
        //make sure that we actually have a response
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"LoginViewController: Login failed due to improper cast");
        message = [NSString stringWithFormat:NSLocalizedString(@"LoginFailedTitle", nil), @"Status Code : 501"];
        
        analyticsResult = RBXAResultFailure;
        analyticsError = RBXAErrorNoHTTPResponse;
    }
    else if (error)
    {
        // some sort of error happened at the cocoa level, report this and bail
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"LoginViewController: Login failed due to NSError: %s",[[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding]);
        message = [NSString stringWithFormat:NSLocalizedString(@"LoginFailedTitle", nil), @"Status Code : 502"];
        
        analyticsResult = RBXAResultFailure;
        analyticsError = RBXAErrorEndpointReturnedError;
    }
    else if (responseData == nil)
    {
        message = [NSString stringWithFormat:NSLocalizedString(@"LoginFailedTitle", nil), NSLocalizedString(@"SocialGigyaErrorNotifyLoginNoResponse", nil)];
        analyticsResult = RBXAResultFailure;
        analyticsError = RBXAErrorNoResponse;
    }
    else //We've made it through the initial problems, now to check for a succcessful status code
    {
        // status code 200 means web response was successful (not necessarily successful login though!)
        if ([httpResponse statusCode] == 200)
        {
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginViewController: Login succeeded with status code 200");
            
            //parse out the user's data, successful JSON looks something like this....
            // PunishmentInfo = "<null>";
            // Status = OK;
            // UserInfo =     {
            //    IsAnyBuildersClubMember = 1;
            //    RobuxBalance = 4467;
            //    ThumbnailUrl = "http://t5.rbxcdn.com/1e54cb9a0bb2177577d816831ab05249";
            //    TicketsBalance = 1573;
            //    UserID = 68465808;
            //    UserName = iMightBeLying;
            // };
            NSError* jsonParsingError = nil;
            NSDictionary* responseDict = [NSJSONSerialization JSONObjectWithData:responseData options:kNilOptions error:&jsonParsingError];
            
            if (jsonParsingError || responseDict == nil)
            {
                //couldn't even parse out the response, mark this as a local issue and escape
                analyticsError = RBXAErrorJSON;
                analyticsResult = RBXAResultFailure;
                message = NSLocalizedString(@"SocialGigyaErrorNotifyLoginJSONParse", nil);
            }
            else
            {
                //successful json parsed
                NSString* status = [responseDict stringForKey:@"Status" withDefault:nil];
                if (!status)
                {
                    //Analytics information
                    [RobloxGoogleAnalytics setPageViewTracking:[NSString stringWithFormat:@"Login/Failure/NoStatus"]];
                    analyticsResult = RBXAResultFailure;
                    analyticsError = RBXAErrorJSONReadFailure;
                    message = NSLocalizedString(@"SocialGigyaErrorNotifyLoginJSONParse", nil);
                }
                else if( [status isEqualToString:@"OK"] ) // we logged in successfully
                {
                    //save information out to the Current Player
                    UserInfo* currentPlayer = [UserInfo CurrentPlayer];
                    NSDictionary *userInfoDict = [responseDict dictionaryForKey:@"UserInfo"];
                    if (userInfoDict)
                    {
                        currentPlayer.userId =      [userInfoDict numberForKey:@"UserID"];
                        currentPlayer.rbxBal =      [userInfoDict numberForKey:@"RobuxBalance"];
                        currentPlayer.tikBal =      [userInfoDict numberForKey:@"TicketsBalance"];
                        currentPlayer.userThumbNailUrl = [userInfoDict stringForKey:@"ThumbnailUrl"];
                        currentPlayer.bcMember =     [userInfoDict stringForKey:@"IsAnyBuildersClubMember"];
                        currentPlayer.username = [userInfoDict objectForKey:@"UserName"];
                    }
                    currentPlayer.password = [userLoginInfo objectForKey:NONENCODED_PW_KEY];
                    currentPlayer.encodedPassword = [userLoginInfo objectForKey:ENCODED_PW_KEY];
                    currentPlayer.encodedUsername = [userLoginInfo objectForKey:ENCODED_USERNAME_KEY];
                    
                    //analytic tracking
                    [RobloxGoogleAnalytics setPageViewTracking:@"Login/Success"];
                    [[NSUserDefaults standardUserDefaults] setObject:@"NO" forKey:@"isGameCenterUser"];
                    [[CrashReporter sharedInstance] logStringKeyValue:@"UserName" withValue: [currentPlayer username]];
                    analyticsResult = RBXAResultSuccess;
                    
                    //Everything is A-OK, return nothing so that the doLogin function knows we succeeded
                    message = @"";
                }
                else // something went wrong with login, convey message to user
                {
                    //Analytics information
                    [RobloxGoogleAnalytics setPageViewTracking:[NSString stringWithFormat:@"Login/Failure/%@", status]];
                    analyticsResult = RBXAResultFailure;
                    
                    
                    // User credentials error
                    if ([status isEqualToString:@"InvalidUsername"] || [status isEqualToString:@"InvalidPassword"] || [status isEqualToString:@"MissingRequiredField"])
                    {
                        //only clear out the user defaults for these specific errors
                        message = NSLocalizedString(@"InvalidUsernameOrPw", nil);
                        
                        if      ([status isEqualToString:@"InvalidUsername"])       { analyticsError = RBXAErrorInvalidUsername; }
                        else if ([status isEqualToString:@"InvalidPassword"])       { analyticsError = RBXAErrorInvalidPassword; }
                        else if ([status isEqualToString:@"MissingRequiredField"])  { analyticsError = RBXAErrorMissingField;    }
                    }
                    
                    //Floodcheck and captcha
                    else if([status isEqualToString:@"SuccessfulLoginFloodcheck"] || [status isEqualToString:@"FailedLoginFloodcheck"] || [status isEqualToString:@"FailedLoginPerUserFloodcheck "])
                    {
                        message = NSLocalizedString(@"TooManyAttempts", nil);
                        
                        if      ([status isEqualToString:@"SuccessfulLoginFloodcheck"])     { analyticsError = RBXAErrorFloodcheckSuccess; }
                        else if ([status isEqualToString:@"FailedLoginFloodcheck"])         { analyticsError = RBXAErrorFloodcheckFailure; }
                        else if ([status isEqualToString:@"FailedLoginPerUserFloodcheck"])  { analyticsError = RBXAErrorFloodcheckFailurePerUser; }
                    }
                    
                    //Banned accounts
                    else if([status isEqualToString:@"AccountNotApproved"] ) // Account Banned or someother issue
                    {
                        message = NSLocalizedString(@"NeedsExternalLogin", nil);
                        analyticsError = RBXAErrorNotApproved;
                        
                        NSDictionary* punishmentInfo = [responseDict objectForKey:@"PunishmentInfo"];
                        if (punishmentInfo)
                        {
                            NSString *banBeginString = [punishmentInfo objectForKey:@"BeginDateString"];
                            if(banBeginString)
                            {
                                NSString *banEndString = [punishmentInfo objectForKey:@"EndDateString"];
                                NSString *messageToUSer = [punishmentInfo objectForKey:@"MessageToUser"];
                                if([banEndString length] != 0)
                                    message = [NSString stringWithFormat:NSLocalizedString(@"BannedMessage", nil), banBeginString, banEndString, messageToUSer];
                                else
                                    message = [NSString stringWithFormat:NSLocalizedString(@"BannedForeverMessage", nil), banBeginString, messageToUSer];
                            }
                        }
                    }
                    
                    //Bad Cookie? What the crap is all this?
                    else if ([status isEqualToString:@"BadCookieTryAgain"])
                    {
                        message = NSLocalizedString(@"BadCookieTryAgain", nil);
                        analyticsError = RBXAErrorBadCookieTryAgain;
                    }
                    
                    //WHO KNOWS WHAT WENT WRONG
                    else
                    {
                        message = NSLocalizedString(@"UnknownLoginError", nil);
                        analyticsError = RBXAErrorUnknownLoginException;
                    }
                    
                    //print some information out
                    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"LoginViewController: Login failure despite http 200 response, error given %s", [message cStringUsingEncoding:NSUTF8StringEncoding]);
                }
            }
        }
        else // we didn't even get a good web response, something went wrong, lets print out all info we have
        {
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"LoginViewController: Login failed with http response code: %ld", (long)[httpResponse statusCode]);
            
            //DEBUG - print out the header fields to the response
            NSDictionary* httpRespHdrs = [httpResponse allHeaderFields];
            NSArray* keys = [httpRespHdrs allKeys];
            for (NSString* key in keys)
            {
                NSString* value = [httpRespHdrs valueForKey:key];
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginViewController: http header info: %s = %s", [key cStringUsingEncoding:NSUTF8StringEncoding], [value cStringUsingEncoding:NSUTF8StringEncoding]);
            }
            
            
            //figure out what went wrong
            analyticsResult = RBXAResultFailure;
            
            if (httpResponse.statusCode == 404 || httpResponse.statusCode == 500)
            {
                //server error
                message = NSLocalizedString(@"UnknownLoginError", nil); //Unknown server error has occurred. Please try again.
                
                if (httpResponse.statusCode == 404)
                    analyticsError = RBXAErrorHttpError404;
                
                if (httpResponse.statusCode == 500)
                    analyticsError = RBXAErrorHttpError500;
            }
            else
            {
                //default error message - display the message and the status code
                message = [NSString stringWithFormat:NSLocalizedString(@"LoginFailedTitle", nil), @"Status Code : "];
                message = [NSString stringWithFormat:@"%@%li", message, (long)httpResponse.statusCode];
                analyticsError = RBXAErrorBadResponse;
            }
        }
    }
    
    
    //take the information we have gathered, and report it
    [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextLogin
                                                              result:analyticsResult
                                                            rbxError:analyticsError
                                                        startingTime:startTime
                                                   attemptedUsername:[userLoginInfo stringForKey:ENCODED_USERNAME_KEY]
                                                          URLRequest:request
                                                    HTTPResponseCode:httpResponse.statusCode
                                                        responseData:responseData
                                                      additionalData:additionalData];
    
    
    // 2015.0920 ajain - Reset the flag to NO here;
    // We are only using this flag for the purpose of setting "performingAutoLogin" in the analyticsErrorPayload
    if (self.performingAutoLogin == YES) { self.performingAutoLogin = NO; }
    
    NSError *loginResponseError = nil;
    if (![RBXFunctions isEmptyString:message]) {
        NSError *parsingError;
        NSDictionary* infoDict = @{};
        if (responseData)
        {
            infoDict = [NSJSONSerialization JSONObjectWithData:responseData options:kNilOptions error:&parsingError];
            if (parsingError)
                infoDict = @{};
        }
        loginResponseError = [NSError errorWithDomain:message code:httpResponse.statusCode userInfo:infoDict];
    }
    
    if (nil != loginResponseCompletionHandler) {
        loginResponseCompletionHandler(loginResponseError);
    }
}

- (void) loginV2WithUsername:(NSString *) username password:(NSString *)password completionBlock:(LoginManagerCompletionBlock)loginV2CompletionHandler
{
    //check for a connection, error messages are automattically dispatched
    if (![self isConnectedToInternet])
    {
        if (nil != loginV2CompletionHandler) {
            loginV2CompletionHandler([NSError errorWithDomain:@"Not connected to internet" code:998 userInfo:nil]);
        }
        return;
    }
    
    NSString* loginUrlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/login/v1/"];
    NSURL *url = [NSURL URLWithString: loginUrlString];
    NSDate* startTime = [NSDate date];
    
    //configure the request
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60 * 7];
    NSString *encodedUserName = [username stringWithPercentEscape];
    NSString *encodedPassword = [password stringWithPercentEscape];
    NSString *postData = [NSString stringWithFormat:@"username=%@&password=%@", encodedUserName, encodedPassword];
    NSString *length = [NSString stringWithFormat:@"%lu", (unsigned long)[postData length]];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setValue:length forHTTPHeaderField:@"Content-Length"];
    [theRequest setHTTPMethod:@"POST"];
    [theRequest setHTTPBody:[postData dataUsingEncoding:NSASCIIStringEncoding]];
    
#ifdef RBX_INTERNAL
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginManager: Posting login attempt with postData = %s", [postData cStringUsingEncoding:NSUTF8StringEncoding]);
#else
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginManager: Posting login attempt with postData = <omitted to protect password, create internal build for postdata string>");
#endif
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginViewController: Attempting login connection with url = %s", [loginUrlString cStringUsingEncoding:NSUTF8StringEncoding]);
    
    [[[NSURLSession sharedSession] dataTaskWithRequest:theRequest completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        
        //keep some analytic information
        RBXAnalyticsResult analyticsResult = RBXAResultUnknown;
        RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
        NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
        NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
        
        if ([RBXFunctions isEmpty:error] && ![RBXFunctions isEmpty:data])
        {
            [UserInfo clearUserInfo];
            [UserInfo CurrentPlayer].encodedUsername = encodedUserName;
            [UserInfo CurrentPlayer].encodedPassword = encodedPassword;
            [UserInfo CurrentPlayer].password = password;
            
            NSDictionary *responseDict = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:&error];
            if (responseDict && !error)
            {
                if ([responseDict objectForKey:@"userId"])
                {
                    // Login successful
                    analyticsResult = RBXAResultSuccess;
                    
                    //we have a successful login
                    [[ABTestManager sharedInstance] fetchExperimentsForUser:[UserInfo CurrentPlayer].userId];
                    
                    //mark the user as logged in
                    [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
                    [UserInfo CurrentPlayer].userId = [responseDict numberForKey:@"userId"];
                    
                    //establish a gigya session
                    [self doSocialNotifyGigyaLoginWithContext:RBXAContextLogin withCompletion:^(bool success, NSString *message) //weakself
                     {
                         //NSLog(@"Gigya Notify Login (Success, Message) : (%@, %@)", success ? @"YES" : @"NO", message);
                         [self processBackground]; //weakself
                     }];
                    
                    self.loginRetryCounter = 0; //weakself
                    
                    // get more info...
                    [self getUserAccountInfoForContext:RBXAContextLogin completionHandler:loginV2CompletionHandler];
                }
                else if ([responseDict objectForKey:@"message"])
                {
                    analyticsResult = RBXAResultFailure;

                    // If the principal sent back a 'message' then you're in big trouble mister.
                    NSString *errorMessage = [responseDict objectForKey:@"message"];
                    if ([errorMessage.lowercaseString isEqualToString:@"captcha".lowercaseString])
                    {
                        analyticsError = RBXAErrorCaptcha;
                        error = [NSError errorWithDomain:NSLocalizedString(@"TooManyAttempts", nil) code:httpResponse.statusCode userInfo:responseDict];
                    }
                    else if ([errorMessage.lowercaseString isEqualToString:@"credentials".lowercaseString])
                    {
                        analyticsError = RBXAErrorInvalidPassword;
                        error = [NSError errorWithDomain:NSLocalizedString(@"InvalidUsernameOrPw", nil) code:httpResponse.statusCode userInfo:responseDict];
                    }
                    else if ([errorMessage.lowercaseString isEqualToString:@"privileged".lowercaseString])
                    {
                        analyticsError = RBXAErrorPrivileged;
                        error = [NSError errorWithDomain:errorMessage code:httpResponse.statusCode userInfo:responseDict];
                    }
                    else if ([errorMessage.lowercaseString isEqualToString:@"twostepverification".lowercaseString])
                    {
                        analyticsError = RBXAErrorTwoStepVerification;
                        error = [NSError errorWithDomain:errorMessage code:httpResponse.statusCode userInfo:responseDict];
                    }
                    
                    if (nil != loginV2CompletionHandler) {
                        loginV2CompletionHandler(error);
                    }
                }
                else
                {
                    analyticsResult = RBXAResultFailure;
                    analyticsError = RBXAErrorCannotParseData;
                    error = [NSError errorWithDomain:@"Response payload cannot be parsed" code:httpResponse.statusCode userInfo:responseDict];
                }
            }
            else
            {
                analyticsResult = RBXAResultFailure;
                analyticsError = RBXAErrorJSONParseFailure;
                error = [NSError errorWithDomain:@"Parsed payload contained errors" code:httpResponse.statusCode userInfo:@{}];
            }
        }
        else
        {
            analyticsResult = RBXAResultFailure;
            analyticsError = RBXAErrorNoHTTPResponse;
            error = [NSError errorWithDomain:@"Erroneous response received from the server" code:httpResponse.statusCode userInfo:@{}];
        }
        
        //take the information we have gathered, and report it
        [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextLogin
                                                                  result:analyticsResult
                                                                rbxError:analyticsError
                                                            startingTime:startTime
                                                       attemptedUsername:username
                                                              URLRequest:theRequest
                                                        HTTPResponseCode:httpResponse.statusCode
                                                            responseData:data
                                                          additionalData:additionalData];

        
    }] resume];
}

- (void) getUserAccountInfoForContext:(RBXAnalyticsContextName)context completionHandler:(LoginManagerCompletionBlock)loginV2CompletionHandler
{
    //construct the url
    // https://api.sitetest2.robloxlabs.com/users/account-info
    NSString* loginUrlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/users/account-info"];
    NSURL *url = [NSURL URLWithString: loginUrlString];
    NSDate* startTime = [NSDate date];
    
    //configure the request
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60 * 7];
    
    [[[NSURLSession sharedSession] dataTaskWithRequest:theRequest
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error)
    {
                                         NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                                         RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
        
                                         if ([RBXFunctions isEmpty:error])
                                         {
                                             
                                             if (httpResponse.statusCode == 200)
                                             {
                                                 if (data.length > 0)
                                                 {
                                                     NSDictionary *jsonDict = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:&error];
                                                     if (jsonDict && [RBXFunctions isEmpty:error])
                                                     {
                                                         /*{
                                                             AgeBracket = 0;
                                                             Email =     {
                                                                 IsVerified = 1;
                                                                 Value = "kmulherin@roblox.com";
                                                             };
                                                             HasPasswordSet = 1;
                                                             Username = iMightBeLying;
                                                         }*/
                                                         [UserInfo CurrentPlayer].username = [jsonDict stringForKey:@"Username"];
                                                         [UserInfo CurrentPlayer].userHasSetPassword = [jsonDict boolValueForKey:@"HasPasswordSet"];
                                                         [UserInfo CurrentPlayer].userEmail = [jsonDict stringForKey:@"Email"];
                                                         
                                                         // get the user balances
                                                         [self getUserBalancesForContext:context completionHandler:loginV2CompletionHandler];
                                                     }
                                                     else
                                                     {
                                                         error = [NSError errorWithDomain:@"JSON Parse Error" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                         analyticsError = RBXAErrorJSONParseFailure;
                                                     }
                                                 }
                                                 else
                                                 {
                                                     error = [NSError errorWithDomain:@"Data length is 0" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                     analyticsError = RBXAErrorCannotParseData;
                                                 }
                                             }
                                             else
                                             {
                                                 error = [NSError errorWithDomain:@"httperror" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                 analyticsError = RBXAErrorHttpError400;
                                             }
                                         }
        
                                         //check to see if we have any errors from parsing the response
                                         if(![RBXFunctions isEmpty:error])
                                         {
                                             // In this case we only call the completion handler because we have some kind of an error; otherwise everything is good and the
                                             // completion handler is passed to the getUserBalancesWithCompletionHandler: method.
                                             
                                             //instead of sending up the response data here, send up the time since last login
                                             NSData* dateData = [[NSUserDefaults standardUserDefaults] objectForKey:@"LastSuccessfulLoginDate"];
                                             if (dateData && [dateData length] == sizeof(double))
                                             {
                                                 //dateVal is a time invterval since 1970, unpackage it
                                                 double dateVal;
                                                 memcpy(&dateVal, [dateData bytes], sizeof(double));
                                                 
                                                 NSDate* lastLoginDate = [NSDate dateWithTimeIntervalSince1970:dateVal];
                                                 
                                                 //calculate the time difference from then to now
                                                 NSDate* dateNow = [NSDate date];
                                                 double loginDeltaSeconds = (double)[dateNow timeIntervalSinceDate:lastLoginDate];
                                                 loginDeltaSeconds *= 1000; //convert to milliseconds
                                                 
                                                 //round off the decimal and convert the number to a string
                                                 NSNumberFormatter* nf = [[NSNumberFormatter alloc] init];
                                                 [nf setMaximumFractionDigits:0];
                                                 [nf setRoundingMode:NSNumberFormatterRoundHalfEven];
                                                 NSString* numberString = [nf stringFromNumber:[NSNumber numberWithDouble:loginDeltaSeconds]];
                                                 
                                                 //convert the string to data, and overwrite the data object that is written into the analytics object
                                                 data = [numberString dataUsingEncoding:NSUTF8StringEncoding];
                                                 
                                                 //for additional analytics reporting - send information about the expected cookie expiration
                                                 NSData* expectedExpirationData = [[NSUserDefaults standardUserDefaults] objectForKey:@"LastSuccessfulLoginExpectedExpiration"];
                                                 if (expectedExpirationData && [expectedExpirationData length] == sizeof(double))
                                                 {
                                                     //convert the data stored in the user defaults into a double
                                                     double expectedVal;
                                                     memcpy(&expectedVal, [expectedExpirationData bytes], sizeof(double));
                                                     
                                                     //report the data as we see it
                                                     [[SessionReporter sharedInstance] postAutoLoginFailurePayload:(NSTimeInterval)dateVal
                                                                                                  cookieExpiration:[dateNow timeIntervalSince1970]
                                                                                                expectedExpiration:(NSTimeInterval)expectedVal];
                                                     [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LastSuccessfulLoginExpectedExpiration"];
                                                 }
                                                 
                                                 
                                                 //remove the key so that we do not double report
                                                 [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LastSuccessfulLoginDate"];
                                                 [[NSUserDefaults standardUserDefaults] synchronize];
                                                 
                                                 
                                             }
                                             
                                             //take the information we have gathered, and report it
                                             NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
                                             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:context
                                                                                                       result:RBXAResultFailure
                                                                                                     rbxError:analyticsError
                                                                                                 startingTime:startTime
                                                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                                                   URLRequest:theRequest
                                                                                             HTTPResponseCode:httpResponse.statusCode
                                                                                                 responseData:data
                                                                                               additionalData:additionalData];

                                             if (nil != loginV2CompletionHandler) {
                                                 loginV2CompletionHandler(error);
                                             }
                                         }

                                     }] resume];
}

- (void) getUserBalancesForContext:(RBXAnalyticsContextName)context completionHandler:(LoginManagerCompletionBlock)loginV2CompletionHandler
{
    //construct the url
    // https://api.sitetest2.robloxlabs.com/my/balance
    NSString* loginUrlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/my/balance/"];
    NSURL *url = [NSURL URLWithString: loginUrlString];
    NSDate* startTime = [NSDate date];
    
    //configure the request
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60 * 7];
    
    [[[NSURLSession sharedSession] dataTaskWithRequest:theRequest
                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                         NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                                         RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;

                                         if ([RBXFunctions isEmpty:error])
                                         {
                                             // collect user info and store in user object
                                             if (httpResponse.statusCode == 200)
                                             {
                                                 if (data.length > 0) {
                                                     NSDictionary *jsonDict = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:&error];
                                                     if (jsonDict && [RBXFunctions isEmpty:error]) {
                                                         /*
                                                          {
                                                          robux = 13551;
                                                          tickets = 3025;
                                                          }*/
                                                         [UserInfo CurrentPlayer].rbxBal = [jsonDict numberForKey:@"robux" withDefault:@(0)];
                                                         [UserInfo CurrentPlayer].tikBal = [jsonDict numberForKey:@"tickets" withDefault:@(0)];
                                                         
                                                         //post a notification
                                                         [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:self userInfo:@{@"userLoginInfo":[UserInfo CurrentPlayer]}];
                                                     }
                                                     else
                                                     {
                                                         error = [NSError errorWithDomain:@"JSON Parse Error" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                         analyticsError = RBXAErrorJSONParseFailure;
                                                     }
                                                 }
                                                 else
                                                 {
                                                     error = [NSError errorWithDomain:@"Data length is 0" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                     analyticsError = RBXAErrorCannotParseData;
                                                 }
                                             }
                                             else
                                             {
                                                 error = [NSError errorWithDomain:@"httperror" code:httpResponse.statusCode userInfo:@{@"response":response}];
                                                 analyticsError = RBXAErrorHttpError400;
                                             }
                                         }
                                         
                                         if (![RBXFunctions isEmpty:error])
                                         {
                                             //take the information we have gathered, and report it
                                             NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
                                             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:context
                                                                                                       result:RBXAResultFailure
                                                                                                     rbxError:analyticsError
                                                                                                 startingTime:startTime
                                                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                                                   URLRequest:theRequest
                                                                                             HTTPResponseCode:httpResponse.statusCode
                                                                                                 responseData:data
                                                                                               additionalData:additionalData];
                                         }
                                         
                                         // Trigger the login v2 completion block
                                         if (loginV2CompletionHandler)
                                             loginV2CompletionHandler(error);
                                     }] resume];
}

#pragma mark ACCESSORS AND MUTATORS
-(BOOL) getRememberPassword {
    return _rememberPassword;
}

-(void) setRememberPassword:(BOOL) shouldRemember {
    std::string shouldRememberString = "false";
    if(shouldRemember)
        shouldRememberString = "true";
    
    _rememberPassword = shouldRemember;
    
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginManager: Remember my password set to %s",shouldRememberString.c_str());
    
    [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:shouldRemember] forKey:@"rememberMyPassword"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

-(BOOL) isConnectedToInternet {
    Reachability* reachability = [Reachability reachabilityForInternetConnection];
    NetworkStatus remoteStatus = [reachability currentReachabilityStatus];
    
    if(remoteStatus == NotReachable)
    {
        NSDictionary* errorDict = [[NSDictionary alloc] initWithObjectsAndKeys:NSLocalizedString(@"ConnectionError", nil),@"Error", nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:self userInfo:errorDict];
        return NO;
    }
    else if(remoteStatus == ReachableViaWWAN)
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginViewController: Reachable via Wireless WAN");
    else if(remoteStatus == ReachableViaWiFi)
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LoginViewController: Reachable via Wifi");
    
    return YES;
}


-(BOOL) hasLoginCredentials         {
    //read the keychain and see if we have the appropriate things stored there
    if ([LoginManager sessionLoginEnabled] == NO) {
        KeychainItemWrapper* loginKeychain = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
        NSString* username = [loginKeychain objectForKey:(__bridge id)kSecAttrAccount];
        NSString* pw = [loginKeychain objectForKey:(__bridge id)kSecValueData];
        bool hasUsername = username != nil && username.length > 0;
        bool hasPassword = pw != nil && pw.length > 0;
        
        return hasUsername && hasPassword;
    }
    
    return NO;
}
-(BOOL) hasSocialLoginCredentials   {
    //read the keychain and see if we have the appropriate values stored there
    KeychainItemWrapper* loginKeychain = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
    NSString* gigyaUID = [loginKeychain objectForKey:(__bridge id)kSecAttrDescription];
    GSSession* session = [Gigya session];
    
    bool isValidGigyaUID = gigyaUID && ![gigyaUID isEqualToString:@"null"];
    bool isValidSession = session && session.isValid;
    
    return isValidGigyaUID && isValidSession;
}

+(BOOL) sessionLoginEnabled
{
    BOOL val = DFFlag::EnableSessionLogin;
    return val;
}
#pragma mark ODD FUNCTIONS

-(void) processStartupAutoLogin:(LoginManagerCompletionBlock)autoLoginCompletionBlock
{
    self.performingAutoLogin = YES;
    
    if ([LoginManager sessionLoginEnabled])
    {
         [[LoginManager sharedInstance] getUserAccountInfoForContext:RBXAContextLogin completionHandler:^(NSError *authCookieValidationError)
         {             
             self.performingAutoLogin = NO;
             
             //we have a successful login!
             if ([RBXFunctions isEmpty:authCookieValidationError])
                 [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
             
             if (autoLoginCompletionBlock)
                 autoLoginCompletionBlock(authCookieValidationError);
         }];
    }
    else
    {
        //grab the username and password if it is stored
        //populate the UserInfo object with stored values
        KeychainItemWrapper* loginKeychain = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
        [UserInfo CurrentPlayer].username = [loginKeychain objectForKey:(__bridge id)kSecAttrAccount];
        
        if ([[LoginManager sharedInstance] getRememberPassword])
            [UserInfo CurrentPlayer].password = [loginKeychain objectForKey:(__bridge id)kSecValueData];
        else
        {
            //the app is decidedly not remembering the password, clear it out if it exists
            [loginKeychain setObject:@"" forKey:(__bridge id)kSecValueData];
            [UserInfo CurrentPlayer].password = @"";
        }
    

        //attempt regular automatic login
        if ([[UserInfo CurrentPlayer].username length] != 0 && [[UserInfo CurrentPlayer].password length] != 0)
        {
            self.loginRetryCounter = 0;
            [[LoginManager sharedInstance] loginWithUsername:[UserInfo CurrentPlayer].username password:[UserInfo CurrentPlayer].password completionBlock:^(NSError *loginError)
            {
                self.performingAutoLogin = NO;
                
                if (autoLoginCompletionBlock)
                    autoLoginCompletionBlock(loginError);
            }];
        }
        else if ([self isFacebookEnabled])
        {
            //see if we have a gigya UID in the keychain and check if a gigya session still exists
            NSString* gigyaUID = [loginKeychain objectForKey:(__bridge id)kSecAttrDescription];
            GSSession* session = [Gigya session];
            
            bool isValidGigyaUID = gigyaUID && ![gigyaUID isEqualToString:@"null"];
            bool isValidSession = session && session.isValid;
        
            if (isValidGigyaUID && isValidSession)
            {
                [self doSocialFetchGigyaInfoWithUID:gigyaUID isLoggingIn:YES withCompletion:^(bool success, NSString *message)
                 {
                     if (success)
                     {
                         [self doSocialLoginWithGigyaUser:[[UserInfo CurrentPlayer] userGigyaInfo]
                                                    error:nil
                                            andCompletion:^(bool success, NSString *message)
                         {
                             //set this flag so that subsequent login calls do not report as automatic login attempts
                             self.performingAutoLogin = NO;
                             if (!success || [message isEqualToString:@"newUser"])
                             {
                                 [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:nil];
                             }
                             
                             //we are finished here, escape
                             if (autoLoginCompletionBlock)
                                 autoLoginCompletionBlock(success ? nil : [NSError errorWithDomain:@"SocailLoginFailed" code:998 userInfo:@{@"message":message}]);
                         }];
                     }
                     else
                     {
                         //we have a valid gigyaID and a valid session, but we could not get the information about the user, escape
                         
                         //set this flag so that subsequent login calls do not report as automatic login attempts
                         self.performingAutoLogin = NO;
                         [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:nil];
                         
                         if (autoLoginCompletionBlock)
                             autoLoginCompletionBlock(success ? nil : [NSError errorWithDomain:@"SocailFetchInfoFailed" code:997 userInfo:@{@"message":message}]);
                     }
                 }];
            }
            else
            {
                //facebook auth is enabled, but we do not have a valid session nor valid id, escape
                //set this flag so that subsequent login calls do not report as automatic login attempts
                self.performingAutoLogin = NO;
                [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:nil];
                
                if (autoLoginCompletionBlock)
                    autoLoginCompletionBlock([NSError errorWithDomain:@"SocailNoAccount" code:996 userInfo:nil]);
            }
        }
        else
        {
            //We do not have enough information to attempt a login, post the notification and escape
            [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_FAILED object:nil];
            
            //set this flag so that subsequent login calls are not reported as automatic
            self.performingAutoLogin = NO;
            
            if (autoLoginCompletionBlock)
                autoLoginCompletionBlock([NSError errorWithDomain:@"AutoLoginNoCredentials" code:995 userInfo:nil]);
        }
    }
}
-(void) processBackground
{
    KeychainItemWrapper* loginKeychain = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
    if ([LoginManager sessionLoginEnabled] == NO) {
        //save username and password information
        //NSLog(@"Process Background...");
        NSString* username = [UserInfo CurrentPlayer].username;
        NSString* password = ([[LoginManager sharedInstance] getRememberPassword] ? [UserInfo CurrentPlayer].password : @"");
        //NSLog(@"Login Keychain username - %@", username);
        [loginKeychain setObject:username forKey:(__bridge id)kSecAttrAccount];
        //NSLog(@"Login Keychain password - %@", password);
        [loginKeychain setObject:password forKey:(__bridge id)kSecValueData];
    }

    //save social login information
    NSString* gigyaUID = [[UserInfo CurrentPlayer] GigyaUID] ? [[UserInfo CurrentPlayer] GigyaUID] : @"null";
    //NSLog(@"Social Login Keychain gigyaUID - %@", gigyaUID);
    [loginKeychain setObject:gigyaUID forKey:(__bridge id)kSecAttrDescription];
    
    //save some information about WHEN we have successfully logged in.
    // - this will help when something breaks
    //Things to save :  - Login Date as a UNIX timestamp
    //                  - The date of the expected cookie expiration -NOTE - UPDATED ELSEWHERE
    double loginDate = (double)[[NSDate date] timeIntervalSince1970];
    NSData* loginData = [NSData dataWithBytes:&loginDate length:sizeof(double)];
    
    [[NSUserDefaults standardUserDefaults] setObject:loginData forKey:@"LastSuccessfulLoginDate"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
}

#pragma mark SOCIAL FUNCTIONS
-(bool) isFacebookEnabled {
    bool serverEnabled = DFFlag::EnableFacebookLoginMaster;
    bool isABTestActive = [[ABTestManager sharedInstance] IsPartOfTestMobileGuestMode];
    
    //in order to preserve AB Test data stability, users who were enrolled in the AB Test will not see
    // social sign up
    //~Kyler
    return (serverEnabled && !isABTestActive);
}

+(NSString*) ProviderNameFacebook   { return @"facebook"; }
+(NSString*) ProviderNameTwitter    { return @"twitter"; }
+(NSString*) ProviderNameGooglePlus { return @"google"; }

-(void) doSocialLogout
{
    @try { [Gigya logout]; }
    @catch (NSException* exception) { NSLog(@"Gigya threw an exception : %@", exception); }
}

-(void) updateUserInfoWithGigyaUser:(GSResponse*)gigyaUserData andError:(NSError**)err
{
    /*
     Successful Facebook login looks like this:
     
     method = "socialize.getUserInfo";
     response =     {
     UID = "_guid_0R3hxkDYWMT_MIjmdSCF6S0rs5I6UZ53aLrtDQ8DFxw=";
     UIDSig = "aCnTR84F5dbgLmcr4t9M+dYfZs8=";
     UIDSignature = "ODUbXEo7+Q27S95bfzaQNNtJf1k=";
     callId = 403116ce3e6d4026960e291b9b108683;
     capabilities = "Login, Friends, Actions, Status, Photos, Places, Checkins, FacebookActions";
     email = "kyler.mulherin@gmail.com";
     errorCode = 0;
     firstName = Kyler;
     gender = m;
     identities =         (
         {
         allowsLogin = 1;
         email = "kyler.mulherin@gmail.com";
         firstName = Kyler;
         gender = m;
         isExpiredSession = 0;
         isLoginIdentity = 1;
         lastLoginTime = 1441385231;
         lastName = Mulherin;
         lastUpdated = "2015-09-04T16:47:12.343Z";
         lastUpdatedTimestamp = 1441385232343;
         nickname = "Kyler Mulherin";
         oldestDataUpdated = "2015-09-04T16:47:12.343Z";
         oldestDataUpdatedTimestamp = 1441385232343;
         photoURL = "https://graph.facebook.com/v2.3/10153160554060784/picture?type=large";
         profileURL = "https://www.facebook.com/app_scoped_user_id/10153160554060784/";
         provider = facebook;
         providerUID = 10153160554060784;
         proxiedEmail = "";
         thumbnailURL = "https://graph.facebook.com/v2.3/10153160554060784/picture?type=square";
         }
     );
     isConnected = 1;
     isLoggedIn = 1;
     isSiteUID = 0;
     isSiteUser = 1;
     isTempUser = 0;
     lastName = Mulherin;
     loginProvider = facebook;
     loginProviderUID = 10153160554060784;
     nickname = "Kyler Mulherin";
     oldestDataAge = 0;
     oldestDataUpdatedTimestamp = 1441385232343;
     photoURL = "https://graph.facebook.com/v2.3/10153160554060784/picture?type=large";
     profileURL = "https://www.facebook.com/app_scoped_user_id/10153160554060784/";
     providers = facebook;
     proxiedEmail = "";
     signatureTimestamp = 1441385232;
     statusCode = 200;
     statusReason = OK;
     thumbnailURL = "https://graph.facebook.com/v2.3/10153160554060784/picture?type=square";
     time = "2015-09-04T16:47:12.569Z";
     timestamp = "2015-09-04 16:47:12";
     };
     }*/
    
    NSMutableDictionary* gigyaJSON;
    if (gigyaUserData && gigyaUserData.JSONString)
    {
        gigyaJSON = [NSJSONSerialization JSONObjectWithData:[gigyaUserData.JSONString dataUsingEncoding:NSUTF8StringEncoding]
                                                    options:NSJSONReadingMutableContainers
                                                      error:err];
        if (err)
            gigyaJSON = [NSMutableDictionary dictionary];
    }
    else
    {
        gigyaJSON = [NSMutableDictionary dictionary];
    }
    
    [[UserInfo CurrentPlayer] setUserSocialInfoDict:gigyaJSON];
    [[UserInfo CurrentPlayer] setUserGigyaInfo:gigyaUserData];
}

-(void) doSocialLoginFromController:(UIViewController*)aController
                        forProvider:(NSString *)providerName
                     withCompletion:(SocialLoginCompletionBlock)completionHandler
{
    //if we already have a gigya session, somehow we've gotten into a very weird state
    // close the session, and try again
    if ([Gigya session] && [Gigya session].isValid)
    {
        [Gigya logoutWithCompletionHandler:^(GSResponse *response, NSError *error) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [self doSocialLoginFromController:aController forProvider:providerName withCompletion:completionHandler];
            });
        }];
    }
    else
    {
        //otherwise, do a regular login
        [Gigya loginToProvider:providerName parameters:nil over:aController completionHandler:^(GSUser *user, NSError *error)
         {
             [self doSocialLoginWithGigyaUser:user error:error andCompletion:completionHandler];
         }];
    }
}

-(void) doSocialLoginWithGigyaUser:(GSResponse*)user
                             error:(NSError*)error
                     andCompletion:(SocialLoginCompletionBlock)completionHandler
{
    //keep some analytic information
    NSDate* startTime = [NSDate date];
    NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
    
    if (error)
    {
        [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialLogin
                                                                  result:RBXAResultFailure
                                                                rbxError:RBXAErrorGigyaLogin
                                                            startingTime:startTime
                                                       attemptedUsername:@"" //Android is using an empty string for an unknown gigya user
                                                              URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"GSAPI.login"]]
                                                        HTTPResponseCode:error.code
                                                            responseData:[[user description] dataUsingEncoding:NSUTF8StringEncoding]
                                                          additionalData:additionalData];
        
        
        //apologize to the user, something has gone wrong attempting to log in with Facebook
        if (completionHandler)
            completionHandler(NO, nil); //NSLocalizedString(@"SocialLoginErrorGigyaAuthentication", nil)
        return;
    }
    
    if (user)
    {
        NSError* err;
        [self updateUserInfoWithGigyaUser:user andError:&err];
        if (err)
        {
            [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialLogin
                                                                      result:RBXAResultFailure
                                                                    rbxError:RBXAErrorGigyaKeyMissing
                                                                startingTime:startTime
                                                           attemptedUsername:@"" //Android is using an empty string for an unknown gigya user
                                                                  URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"GSAPI.login"]]
                                                            HTTPResponseCode:error.code
                                                                responseData:[[user description] dataUsingEncoding:NSUTF8StringEncoding]
                                                              additionalData:additionalData];
            
            if (completionHandler)
                completionHandler(NO, NSLocalizedString(@"SocialLoginErrorBadJSON", nil));
            
            return;
        }
        
        bool missingBirthday =  [RBXFunctions isEmpty:[[UserInfo CurrentPlayer] GigyaBirthDay]]   ||
                                [RBXFunctions isEmpty:[[UserInfo CurrentPlayer] GigyaBirthMonth]] ||
                                [RBXFunctions isEmpty:[[UserInfo CurrentPlayer] GigyaBirthYear]];
        NSString* birthdayString = (missingBirthday) ? @"" : [NSString stringWithFormat:@"&birthMonth=%@&birthDay=%@&birthYear=%@",
                                                              [[UserInfo CurrentPlayer] GigyaBirthMonth],
                                                              [[UserInfo CurrentPlayer] GigyaBirthDay],
                                                              [[UserInfo CurrentPlayer] GigyaBirthYear]] ;
        
        //Format a string for web arguments
        NSString* urlString = [NSString stringWithFormat:@"%@/social/postlogin?loginProvider=%@&loginProviderUID=%@&signatureTimestamp=%@&provider=%@&UID=%@&UIDSignature=%@%@",
                               [[RobloxInfo getApiBaseUrl] stringByReplacingOccurrencesOfString:@"http://" withString:@"https://"],
                               [[UserInfo CurrentPlayer] GigyaLoginProvider],
                               [[UserInfo CurrentPlayer] GigyaLoginProviderUID],
                               [[UserInfo CurrentPlayer] GigyaSignatureTimestamp],
                               [LoginManager ProviderNameFacebook], //[[UserInfo CurrentPlayer] GigyaProvider],
                               [[[UserInfo CurrentPlayer] GigyaUID]          stringWithPercentEscape],
                               [[[UserInfo CurrentPlayer] GigyaUIDSignature] stringWithPercentEscape],
                               birthdayString];
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlString]];
        [request setHTTPMethod:@"POST"];
        [RobloxInfo setDefaultHTTPHeadersForRequest:request];
        
        [NSURLConnection sendAsynchronousRequest:request queue:self.requestQueue completionHandler:^(NSURLResponse *response, NSData *responseData, NSError *responseError)
         {
             RBXAnalyticsResult analyticsResult = RBXAResultUnknown;
             RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
             
             bool success = NO;
             NSString* message;
             NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
             
             if (responseData)
             {
                 NSError* parseErr;
                 NSDictionary* responseObject = [NSJSONSerialization JSONObjectWithData:responseData options:NSJSONReadingMutableContainers error:&parseErr];
                 if (responseObject == nil)
                 {
                     analyticsResult = RBXAResultFailure;
                     analyticsError = RBXAErrorNoHTTPResponse;
                     message = NSLocalizedString(@"SocialLoginErrorBadJSON", nil);
                 }
                 else if (parseErr)
                 {
                     analyticsResult = RBXAResultFailure;
                     analyticsError = RBXAErrorJSONParseFailure;
                     message = NSLocalizedString(@"SocialLoginErrorBadJSON", nil);
                 }
                 else
                 {
                     //success!!
                     NSString* loginStatus = [responseObject stringForKey:@"status"];
                     
                     switch (httpResponse.statusCode )
                     {
                         case 200: {
                             success = YES;
                             if ([loginStatus isEqualToString:@"currentUser"])
                             {
                                 /*
                                  Response here should look like this:
                                  
                                  status = currentUser;
                                  */
                                 //the user already has an account, get some more information about them and then log in
                                 message = @"currentUser";
                                 analyticsResult = RBXAResultSuccess;
                                 
                                 //tell everyone that we have logged in
                                 [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
                                 [[UserInfo CurrentPlayer] UpdatePlayerInfo];
                                 [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
                                 
                                 [self processBackground];
                             }
                             else if ([loginStatus isEqualToString:@"newUser"])
                             {
                                 /*
                                  Response here should look like this:
                                  
                                  birthday = "2002-09-08T00:00:00";
                                  email = "";
                                  gender = Male;
                                  gigyaUid = "_guid_0R3hxkDYWMT_MIjmdSCF6S0rs5I6UZ53aLrtDQ8DFxw=";
                                  status = newUser;
                                  */
                                 message = @"newUser";
                                 analyticsResult = RBXAResultUnknown; //do nothing
                                 
                                 //store these locally so that we can pass these up to the SignupSocialScreen
                                 [[UserInfo CurrentPlayer] setBirthday:[responseObject stringForKey:@"birthday" withDefault:@"null"]];
                                 [[UserInfo CurrentPlayer] setUserEmail:[responseObject stringForKey:@"email"]];
                             }
                             else
                             {
                                 //Why couldn't we determine if the user's Roblox identity?
                                 analyticsResult = RBXAResultFailure;
                                 analyticsError = RBXAErrorUnknownLoginException;
                                 success = NO;
                                 message = NSLocalizedString(@"SocialConnectErrorGigya", nil);
                             }
                         } break;
                         case 400: {
                             analyticsResult = RBXAResultFailure;
                             analyticsError = RBXAErrorHttpError400;
                             message = NSLocalizedString(@"SocialLoginBadRequest", nil);
                         } break;
                         case 403: {
                             analyticsResult = RBXAResultFailure;
                             analyticsError = RBXAErrorHttpError403;
                             
                             if ([loginStatus isEqualToString:@"alreadyAuthenticated"])
                             {
                                 //why aren't you already in the app?
                                 success = YES;
                                 analyticsResult = RBXAResultSuccess;
                                 //message = NSLocalizedString(@"SocialLoginErrorAlreadyAuthenticated", nil);
                                 
                                 [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
                                 [[UserInfo CurrentPlayer] UpdatePlayerInfo];
                                 [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
                             }
                             else if ([loginStatus isEqualToString:@"moderatedUser"])
                             {
                                 message = NSLocalizedString(@"SocialLoginErrorForbidden", nil);
                                 NSDictionary* errorUserInfo = [responseObject dictionaryForKey:@"PunishmentInfo"];
                                 
                                 NSString *banBeginString = [errorUserInfo stringForKey:@"BeginDateString"];
                                 if(banBeginString)
                                 {
                                     NSString *banEndString = [errorUserInfo stringForKey:@"EndDateString"];
                                     NSString *messageToUSer = [errorUserInfo stringForKey:@"MessageToUser"];
                                     if([banEndString length] != 0)
                                         message = [NSString stringWithFormat:NSLocalizedString(@"BannedMessage", nil), banBeginString, banEndString, messageToUSer];
                                     else
                                         message = [NSString stringWithFormat:NSLocalizedString(@"BannedForeverMessage", nil), banBeginString, messageToUSer];
                                 }
                             }
                             else
                                 message = NSLocalizedString(@"SocialLoginErrorForbidden", nil);
                         } break;
                         case 404: { //<--how did this happen, why is Gigya available on mobile but not on server? You should feel bad
                             message = NSLocalizedString(@"SocialErrorUnsupportedAPI", nil);
                             analyticsResult = RBXAResultFailure;
                             analyticsError = RBXAErrorHttpError404;
                         } break;
                         case 500: {
                             message = NSLocalizedString(@"SocialLoginErrorInternalServer", nil);
                             analyticsResult = RBXAResultFailure;
                             analyticsError = RBXAErrorHttpError500;
                         } break;
                         default:
                         {
                             message = NSLocalizedString(@"SocialLoginErrorUnknown", nil);
                             NSLog(@"Unknown Error : %@", error);
                             analyticsResult = RBXAResultFailure;
                             analyticsError = RBXAErrorEndpointReturnedError;
                         } break;
                     }
                 }
             }
             else
             {
                 //no data
                 message = NSLocalizedString(@"SocialLoginErrorUnknown", nil);
                 NSLog(@"Unknown Error : %@", error);
                 analyticsResult = RBXAResultFailure;
                 analyticsError = RBXAErrorEndpointReturnedError;
             }
             
             
             //report some analytics
             if (analyticsResult != RBXAResultUnknown)
             {
                 [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialLogin
                                                                           result:analyticsResult
                                                                         rbxError:analyticsError
                                                                     startingTime:startTime
                                                                attemptedUsername:@"" //Android is using an empty string for an unknown gigya user
                                                                       URLRequest:request
                                                                 HTTPResponseCode:httpResponse.statusCode
                                                                     responseData:responseData
                                                                   additionalData:additionalData];
             }
             
             if (completionHandler)
                 completionHandler(success, message);
         }];
    }
    else
    {
        //for some reason there's no user as well as no error returned, report it
        [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialLogin
                                                                  result:RBXAResultFailure
                                                                rbxError:RBXAErrorGigyaLogin
                                                            startingTime:startTime
                                                       attemptedUsername:@"" //Android is using an empty string for an unknown gigya user
                                                              URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"GSAPI.login"]]
                                                        HTTPResponseCode:error.code
                                                            responseData:[[user description] dataUsingEncoding:NSUTF8StringEncoding]
                                                          additionalData:additionalData];
        
        //apologize to the user, something has gone wrong attempting to log in with Facebook
        if (completionHandler)
            completionHandler(NO, NSLocalizedString(@"SocialLoginErrorGigyaAuthentication", nil));
        return;
    }
}

-(void) doSocialSignupWithUsername:(NSString*)username
                           gigyaID:(NSString*)gigyaUID
                          birthday:(NSString*)birthdayString
                            gender:(NSString*)playerGender
                             email:(NSString*)playerEmail
                        completion:(SocialLoginCompletionBlock)handler
{
    NSDate* startTime = [NSDate date];
    
    //Format a string for web arguments
    NSString* urlString = [NSString stringWithFormat:@"%@/social/signup?username=%@&gigyaUID=%@&birthday=%@&gender=%@%@",
                           [[RobloxInfo getApiBaseUrl] stringByReplacingOccurrencesOfString:@"http://" withString:@"https://"],
                           username ? username : @"null",
                           gigyaUID ? gigyaUID : @"null",
                           birthdayString ? birthdayString : @"null",
                           playerGender ? playerGender : @"null",
                           playerEmail ? [@"&email=" stringByAppendingString:playerEmail] : @""];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    [request setHTTPMethod:@"POST"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    [NSURLConnection sendAsynchronousRequest:request queue:self.requestQueue completionHandler:^(NSURLResponse *response, NSData *responseData, NSError *responseError)
     {
         RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
         bool success = NO;
         NSString* handlerMessage = nil;
         NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
         
         
         if (!httpResponse || !responseData)
         {
             analyticsError = RBXAErrorNoHTTPResponse;
             handlerMessage = NSLocalizedString(@"SocialGigyaErrorNotifyLoginNoResponse", nil);
         }
         else
         {
             NSError* parseError;
             NSDictionary* responseObject = [NSJSONSerialization JSONObjectWithData:responseData options:NSJSONReadingMutableContainers error:&parseError];
             
             if (parseError || !responseObject)
             {
                 analyticsError = RBXAErrorJSONParseFailure;
                 handlerMessage = NSLocalizedString(@"SocialGigyaErrorNotifyLoginNoResponse", nil);
             }
             else
             {
                 analyticsError = RBXAErrorUnknownError;
                 
                 switch (httpResponse.statusCode )
                 {
                     case 200: {
                         NSNumber* userID = [responseObject numberForKey:@"userID"];
                         if (userID)
                         {
                             success = YES;
                             analyticsError = RBXAErrorNoError;
                             
                             [[UserInfo CurrentPlayer] setUserId:userID];
                             [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
                             [[UserInfo CurrentPlayer] UpdatePlayerInfo];
                             [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
                             
                             [self processBackground];
                         }
                         else
                         {
                             analyticsError = RBXAErrorIncompleteJSON;
                             handlerMessage = NSLocalizedString(@"SocialSignupUnknownError", nil);
                         }
                     } break;
                     case 400: {
                         //analyticsError = RBXAErrorHttpError400;
                         handlerMessage = NSLocalizedString(@"SocialSignupErrorInvalidParameters", nil);
                     } break;
                     case 403: {
                         NSString* forbiddenMessage = [responseObject stringForKey:@"status" withDefault:@""];
                         if ([forbiddenMessage isEqualToString:@"alreadyAuthenticated"])
                         {
                             success = YES;
                             //[[UserInfo CurrentPlayer] setUserId:userID];
                             [[UserInfo CurrentPlayer] UpdatePlayerInfo];
                             [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
                             
                             [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
                             //handlerMessage = NSLocalizedString(@"SocialSignupError", nil);
                             
                             analyticsError = RBXAErrorAlreadyAuthenticated; //this won't get called
                         }
                         else if ([forbiddenMessage isEqualToString:@"captcha"])
                         {
                             handlerMessage = NSLocalizedString(@"SocialSignupErrorCaptchaNeeded", nil);
                             analyticsError = RBXAErrorCaptcha;
                         }
                         else
                         {
                             handlerMessage = NSLocalizedString(@"SocialSignupUnknownError", nil);
                             analyticsError = RBXAErrorAlreadyAuthenticated;
                         }
                     } break;
                     case 404: {
                         handlerMessage = NSLocalizedString(@"SocialErrorUnsupportedAPI", nil);
                         analyticsError = RBXAErrorHttpError404;
                     } break; //<--how did this happen, why is Gigya available on mobile but not on server? You should feel bad
                     case 500: {
                         //analyticsError = RBXAErrorHttpError500;
                         handlerMessage = NSLocalizedString(@"SocialSignupUnknownError", nil); } break;
                     default:
                     {
                         analyticsError = RBXAErrorUnknownError;
                         handlerMessage = NSLocalizedString(@"SocialSignupUnknownError", nil);
                     } break;
                 }
             }
         }
         
         if (handler)
             handler(success, handlerMessage);
         
         //send up some data
         [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialSignup
                                                                   result:(success ? RBXAResultSuccess : RBXAResultFailure)
                                                                 rbxError:analyticsError
                                                             startingTime:startTime
                                                        attemptedUsername:username
                                                               URLRequest:request
                                                         HTTPResponseCode:httpResponse.statusCode
                                                             responseData:responseData
                                                           additionalData:nil];
     }];
}

-(void) doSocialNotifyGigyaLoginWithContext:(RBXAnalyticsContextName)context withCompletion:(SocialLoginCompletionBlock)completion
{
    if (![self isFacebookEnabled])
    {
        completion(NO, NSLocalizedString(@"SocialErrorUnsupportedAPI", nil));
        return;
    }
        
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// 1ST) HIT SOCIAL GET UIDSIGNATURE AND TIMESTAMP
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    NSDate* startTime1 = [NSDate date];
    NSString* urlString = [[[RobloxInfo getApiBaseUrl] stringByReplacingOccurrencesOfString:@"http://" withString:@"https://"] stringByAppendingString:@"/social/get-uidsignature-timestamp"];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    [request setHTTPMethod:@"GET"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    [NSURLConnection sendAsynchronousRequest:request queue:self.requestQueue completionHandler:
     ^(NSURLResponse *urlResponse, NSData *responseData, NSError *responseError)
     {
         RBXAnalyticsResult rbxResult = RBXAResultUnknown;
         RBXAnalyticsErrorName rbxError = RBXAErrorNoError;
         
         NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)urlResponse;
         if (httpResponse && responseData && !responseError)
         {
             switch (httpResponse.statusCode)
             {
                 case 200: {
                     NSError* parseErr;
                     NSDictionary* responseObject = [NSJSONSerialization JSONObjectWithData:responseData options:NSJSONReadingMutableContainers error:&parseErr];
                     if (parseErr || !responseObject)
                     {
                         rbxResult = RBXAResultFailure;
                         rbxError = RBXAErrorJSONParseFailure;
                         completion(NO, NSLocalizedString(@"SocialLoginErrorBadJSON", nil));
                     }
                     else if ([[responseObject stringForKey:@"status" withDefault:@" "] isEqualToString:@"success"])
                     {
                         //parse out some variables and hit notify login
                         NSString* environment = [RobloxInfo getEnvironmentName:YES];
                         NSString* uid = [NSString stringWithFormat:@"%@_%@",environment,[UserInfo CurrentPlayer].userId ? [UserInfo CurrentPlayer].userId : @"null"];
                         NSString* uidSignature = [responseObject stringForKey:@"uidSignature" withDefault:@"null"];
                         NSString* timestamp = [responseObject stringForKey:@"timestamp" withDefault:@"null"];
                         
                         [self doSocialNotifyLoginWithUIDSig:uidSignature
                                                UIDTimestamp:timestamp
                                                         UID:uid
                                               andCompletion:^(bool success, NSString *message)
                          {
                             completion(success, message);
                          }];
                     }
                     else
                     {
                         rbxResult = RBXAResultFailure;
                         rbxError = RBXAErrorPostLoginUnspecified;
                         completion(NO, NSLocalizedString(@"SocialLoginErrorBadJSON", nil));
                     }
                     
                 } break;
                 case 404: {
                     rbxResult = RBXAResultFailure;
                     rbxError = RBXAErrorPostLoginUnspecified;
                     completion(NO, NSLocalizedString(@"SocialErrorUnsupportedAPI", nil));
                 } break;
                 default:  {
                     rbxResult = RBXAResultFailure;
                     rbxError = RBXAErrorPostLoginUnspecified;
                     completion(NO, NSLocalizedString(@"SocialServerErrorCouldNotFetchSignature", nil));
                 } break;
             }
         }
         else
         {
             rbxResult = RBXAResultFailure;
             rbxError = RBXAErrorNoHTTPResponse;
             completion(NO, NSLocalizedString(@"SocialSignupUnknownError", nil));
         }
         
         //NOTE- Because this is a train of requests, only report a success once we have made it to the end of the train
         if (rbxResult == RBXAResultFailure)
         {
             NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:context
                                                                       result:rbxResult
                                                                     rbxError:rbxError
                                                                 startingTime:startTime1
                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                   URLRequest:request
                                                             HTTPResponseCode:httpResponse.statusCode
                                                                 responseData:responseData
                                                               additionalData:additionalData];
         }
     }];
}

-(void) doSocialNotifyLoginWithUIDSig:(NSString*)uidSignature
                         UIDTimestamp:(NSString*)timestamp
                                  UID:(NSString*)uid
                        andCompletion:(SocialLoginCompletionBlock)completion
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 2ND) USE THE VALUES TO DO A GIGYA NOTIFY LOGIN, THIS ESTABLISHES A SESSION WITH GIGYA
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    NSDate* startTime2 = [NSDate date];
    NSDictionary* params = @{@"UIDSig":uidSignature,
                             @"UIDTimestamp":timestamp,
                             @"siteUID":uid };
    //NSLog(@"2) Params : %@", params);
    GSRequest* request = [GSRequest requestForMethod:@"socialize.notifyLogin" parameters:params];
    [request sendWithResponseHandler:^(GSResponse *response, NSError *error)
    {
        RBXAnalyticsResult rbxResult = RBXAResultUnknown;
        RBXAnalyticsErrorName rbxError = RBXAErrorNoError;
        NSInteger responseCode = 0;
        
        if (response && response.JSONString && !error)
        {
            //NSLog(@"2) Response : %@", response);
            //NSLog(@"2) Error : %@", error);
            
            NSError* err1;
            NSDictionary* gigyaJSON1 = [NSJSONSerialization JSONObjectWithData:[response.JSONString dataUsingEncoding:NSUTF8StringEncoding]
                                                                       options:NSJSONReadingMutableContainers
                                                                         error:&err1];
            if (!err1 && gigyaJSON1)
            {
                NSString* gigyaUID = [gigyaJSON1 stringForKey:@"UID"];
                NSString* gigyaSessionToken = [gigyaJSON1 stringForKey:@"sessionToken" withDefault:nil];
                NSString* gigyaSessionSecret = [gigyaJSON1 stringForKey:@"sessionSecret" withDefault:nil];
                
                
                
                //NSLog(@"2) Gigya Session = %@", [Gigya session]);
                if (gigyaUID)
                {
                    rbxResult = RBXAResultSuccess;
                    [Gigya setSession:[[GSSession alloc] initWithSessionToken:gigyaSessionToken secret:gigyaSessionSecret]];
                    //NSLog(@"2) Gigya UID : %@", gigyaUID);
                    [[UserInfo CurrentPlayer].userSocialInfoDict setObject:gigyaUID forKey:@"UID"];
                    [self doSocialFetchGigyaInfoWithUID:gigyaUID isLoggingIn:YES withCompletion:^(bool success, NSString *message)
                     {
                         //NSLog(@"END) DO SOCIAL FETCH GIGYA SESSION INFORMATION SUCCESS : %@", success ? @"YES" : @"NO");
                         //NSLog(@"END) - MESSAGE : %@", message);
                         if (completion)
                             completion(success, message);
                     }];
                }
                else
                {
                    responseCode = response.errorCode;
                    rbxResult = RBXAResultFailure;
                    rbxError = RBXAErrorGigyaKeyMissing;
                    if (completion)
                        completion(NO, NSLocalizedString(@"SocialGigyaErrorNotifyLoginJSONParse", nil));
                }
            }
            else
            {
                responseCode = response.errorCode;
                rbxResult = RBXAResultFailure;
                rbxError = RBXAErrorJSONParseFailure;
                if (completion)
                    completion(NO, NSLocalizedString(@"SocialGigyaErrorNotifyLoginJSONParse", nil));
            }
        }
        else
        {
            responseCode = error ? error.code : 0;
            rbxResult = RBXAResultFailure;
            rbxError = RBXAErrorNoHTTPResponse;
            if (completion)
                completion(NO, NSLocalizedString(@"SocialGigyaErrorNotifyLoginNoResponse", nil));
        }
        
        //NOTE- Because this is a train of requests, only report a success once we have made it to the end of the train
        if (rbxResult == RBXAResultFailure)
        {
            NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
            [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialLogin
                                                                      result:rbxResult
                                                                    rbxError:rbxError
                                                                startingTime:startTime2
                                                           attemptedUsername:[UserInfo CurrentPlayer].username
                                                                  URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"GSAPI.socialize.notifyLogin"]]
                                                            HTTPResponseCode:responseCode
                                                                responseData:[[response description] dataUsingEncoding:NSUTF8StringEncoding]
                                                              additionalData:additionalData];
        }
    }];
}
-(void) doSocialFetchGigyaInfoWithUID:(NSString*)gigyaUID isLoggingIn:(bool)isLogin withCompletion:(SocialLoginCompletionBlock)completionHandler
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 3RD) HOPE THIS WORKED OUT, CALL GET USER INFO TO GET THE SOCIAL INFORMATION OF A USER
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    if (!gigyaUID)
    {
        completionHandler(NO, NSLocalizedString(@"SocialFetchGigyaSessionNoID",nil));
        return;
    }
    
    NSDate* startTime3 = [NSDate date];
    
    bool isFacebookEnabled =    [[LoginManager sharedInstance] isFacebookEnabled];
    bool isTwitterEnabled =     NO; //[[LoginManager sharedInstance] isTwitterEnabled];
    bool isGooglePlusEnabled =  NO; //[[LoginManager sharedInstance] isGooglePlusEnabled];
    
    NSMutableString* enabledProviders = [NSMutableString stringWithFormat:@"%@%@%@",
                                         isFacebookEnabled    ? [[LoginManager ProviderNameFacebook] stringByAppendingString:@","]   : @"",
                                         isTwitterEnabled     ? [[LoginManager ProviderNameTwitter] stringByAppendingString:@","]    : @"",
                                         isGooglePlusEnabled  ? [[LoginManager ProviderNameGooglePlus] stringByAppendingString:@","] : @""];
    if (enabledProviders.length > 0)
        [enabledProviders replaceCharactersInRange:NSMakeRange(enabledProviders.length-1, 1) withString:@""];
    
    NSDictionary* params = @{ @"UID":gigyaUID,
                              @"enabledProviders":enabledProviders,
                              @"format":@"json"};
    //NSLog(@"3) Params : %@", params);
    GSRequest* request = [GSRequest requestForMethod:@"socialize.getUserInfo" parameters:params];
    [request sendWithResponseHandler:^(GSResponse *response, NSError *error)
     {
         RBXAnalyticsResult rbxResult = RBXAResultUnknown;
         RBXAnalyticsErrorName rbxError = RBXAErrorNoError;
         NSInteger responseCode = 0;
         
         
         //NSLog(@"3) Response : %@",response);
         //NSLog(@"3) Error : %@", error);
         if (response && response.JSONString)
         {
             NSError* parseErr;
             NSDictionary* json = [NSJSONSerialization JSONObjectWithData:[response.JSONString dataUsingEncoding:NSUTF8StringEncoding]
                                                                  options:NSJSONReadingMutableLeaves
                                                                    error:&parseErr];
             if (json && !parseErr)
             {
                 int statusCode = [json intValueForKey:@"statusCode" withDefault:403];
                 responseCode = statusCode;
                 switch (statusCode)
                 {
                     case 200:
                     {
                         rbxResult = RBXAResultSuccess;
                         [self updateUserInfoWithGigyaUser:response andError:nil];
                         if (completionHandler)
                             completionHandler(YES, nil);
                     } break;
                     default:
                     {
                         rbxResult = RBXAResultFailure;
                         rbxError = RBXAErrorGigyaKeyMissing;
                         if (completionHandler)
                             completionHandler(NO, NSLocalizedString(@"SocialGigyaErrorSessionInfoBadResponse", nil));
                     } break;
                 }
             }
             else
             {
                 rbxResult = RBXAResultFailure;
                 rbxError = RBXAErrorJSONParseFailure;
                 if (completionHandler)
                     completionHandler(NO, NSLocalizedString(@"SocialGigyaErrorSessionInfoBadResponse", nil));
                 
             }
         }
         else if (error)
         {
             rbxResult = RBXAResultFailure;
             rbxError = RBXAErrorGigyaKeyMissing;
             responseCode = error.code;
             if (completionHandler)
                 completionHandler(NO, NSLocalizedString(@"SocialGigyaErrorFetchSessionInformation", nil));
         }
         else
         {
             rbxResult = RBXAResultFailure;
             rbxError = RBXAErrorGigyaKeyMissing;
             if (completionHandler)
                 completionHandler(NO, NSLocalizedString(@"SocialGigyaErrorFetchSessionInformation", nil));
         }
         
         
         if (isLogin)
         {
             NSDictionary* additionalData = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:self.performingAutoLogin] forKey:@"isAutoLogin"];
             
             //Regardless of success or failures, at this point, report our data
             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSocialLogin
                                                                       result:rbxResult
                                                                     rbxError:rbxError
                                                                 startingTime:startTime3
                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                   URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"GSAPI.socialize.getUserInfo"]]
                                                             HTTPResponseCode:responseCode
                                                                 responseData:[[response description] dataUsingEncoding:NSUTF8StringEncoding]
                                                               additionalData:additionalData];
         }
     }];
}

-(void) doSocialConnect:(UIViewController*)aController
             toProvider:(NSString*)providerName
         withCompletion:(SocialLoginCompletionBlock)completionHandler
{
    NSDate* startTime = [NSDate date];
    NSString* urlRequest = @"GSAPI.socialize.addConnection";
    NSDictionary* additionalData = [NSDictionary dictionaryWithObject:providerName forKey:@"provider"];
    
    //@try {
    [Gigya addConnectionToProvider:providerName
                        parameters:nil
                              over:aController
                 completionHandler:^(GSUser *user, NSError *error)
     {
         //NSLog(@"Gigya User : %@", user);
         //NSLog(@"Error : %@", error);
         if (error)
         {
             [[RBXEventReporter sharedInstance] reportError:RBXAErrorEndpointReturnedError
                                                withContext:RBXAContextSettingsSocial
                                             withDataString:[@"connect" stringByAppendingString:providerName]];
             completionHandler(NO, NSLocalizedString(@"SocialConnectErrorGigya", nil));
             
             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSettingsSocialConnect
                                                                       result:RBXAResultFailure
                                                                     rbxError:RBXAErrorGigya
                                                                 startingTime:startTime
                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                   URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlRequest]]
                                                             HTTPResponseCode:error.code
                                                                 responseData:[error.description dataUsingEncoding:NSUTF8StringEncoding]
                                                               additionalData:additionalData];
             return;
         }
         
         //update our local information with the new data
         if (user)
         {
             [self updateUserInfoWithGigyaUser:user andError:nil];
             
             //tell the server to also update its information about the user
             [self doSocialUpdateInfoFromContext:RBXAContextSettingsSocialConnect forProvider:providerName withCompletion:^(bool updateSuccess, NSString* updateMessage)
             {
                 //NOTE - ANALYTICS FOR THIS ARE HANDLED WITHIN
                 if (completionHandler)
                 {
                     if (updateSuccess)
                         completionHandler(YES, updateMessage);
                     else
                     {
                         //Gigya has succeeded, but the website has failed to connect the social account.
                         //Therefore, remove the Gigya connection.
                         [Gigya removeConnectionToProvider:providerName
                                         completionHandler:^(GSUser *user, NSError *error)
                         {
                             [self updateUserInfoWithGigyaUser:user andError:nil];
                             completionHandler(NO, updateMessage); //try again later
                         }];
                     }
                 }
             }];
         }
         else
         {
             [[RBXEventReporter sharedInstance] reportError:RBXAErrorNoResponse
                                                withContext:RBXAContextSettingsSocial
                                             withDataString:[@"connect" stringByAppendingString:providerName]];
             completionHandler(NO, NSLocalizedString(@"SocialConnectErrorNoUser", nil));
             
             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSettingsSocialConnect
                                                                       result:RBXAResultFailure
                                                                     rbxError:RBXAErrorGigyaKeyMissing
                                                                 startingTime:startTime
                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                   URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlRequest]]
                                                             HTTPResponseCode:error.code
                                                                 responseData:[error.description dataUsingEncoding:NSUTF8StringEncoding]
                                                               additionalData:additionalData];
         }
     }];
    
}

-(void) doSocialDisconnect:(NSString*)providerName withCompletion:(SocialLoginCompletionBlock)handler
{
    //initialize some analytics data
    NSDate* startTime = [NSDate date];
    NSString* urlRequest = @"GSAPI.socialize.removeConnection";
    NSDictionary* additionalData = [NSDictionary dictionaryWithObject:providerName forKey:@"provider"];
    
    if (![UserInfo CurrentPlayer].userHasSetPassword)
    {
        [[RBXEventReporter sharedInstance] reportError:RBXAErrorMissingRequiredField
                                           withContext:RBXAContextSettingsSocial
                                        withDataString:[@"disconnect" stringByAppendingString:providerName]];
        
        //THIS IS A HACK TO GET AROUND THIS STUPID DISCONNECTION ISSUE
        handler(NO, NSLocalizedString(@"SocialDisconnectErrorForbiddenWithoutPW", nil));
        return;
    }
    
    [Gigya removeConnectionToProvider:providerName
                    completionHandler:^(GSUser *user, NSError *error)
    {
        if (error)
        {
            [[RBXEventReporter sharedInstance] reportError:RBXAErrorEndpointReturnedError
                                               withContext:RBXAContextSettingsSocial
                                            withDataString:[@"disconnect" stringByAppendingString:providerName]];
            
            handler(NO, NSLocalizedString(@"SocialDisconnectErrorGigya", nil));
            
            
            [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSettingsSocialDisconnect
                                                                      result:RBXAResultFailure
                                                                    rbxError:RBXAErrorGigya
                                                                startingTime:startTime
                                                           attemptedUsername:[UserInfo CurrentPlayer].username
                                                                  URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlRequest]]
                                                            HTTPResponseCode:error.code
                                                                responseData:[error.description dataUsingEncoding:NSUTF8StringEncoding]
                                                              additionalData:additionalData];
            return;
        }
    
        NSString* urlString = [NSString stringWithFormat:@"%@social/disconnect?provider=%@",
                               [RobloxInfo getWWWBaseUrl],
                               providerName];
        
        
        RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
        [request XSRFPOST:urlString
               parameters:nil
                  success:^(AFHTTPRequestOperation *operation, id responseObject)
         {
             [self doSocialFetchGigyaInfoWithUID:[[UserInfo CurrentPlayer] GigyaUID]
                                      isLoggingIn:NO
                                  withCompletion:^(bool success, NSString *message)
              {
                  if (user)
                      [self updateUserInfoWithGigyaUser:user andError:nil];
                
                 handler(YES, @"");
                  
                  [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSettingsSocialDisconnect
                                                                            result:RBXAResultSuccess
                                                                          rbxError:RBXAErrorNoError
                                                                      startingTime:startTime
                                                                 attemptedUsername:[UserInfo CurrentPlayer].username
                                                                        URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]]
                                                                  HTTPResponseCode:200
                                                                      responseData:[responseObject dataUsingEncoding:NSUTF8StringEncoding]
                                                                    additionalData:additionalData];
             }];
         }
                  failure:^(AFHTTPRequestOperation *operation, NSError *error)
         {
             RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
             bool wasSuccessful = NO;
             NSString* disconnectMessage = @"";
             switch ([operation response].statusCode)
             {
                 case 200: { wasSuccessful = YES; } break; // <-- How did this happen?!
                 case 400: { disconnectMessage = NSLocalizedString(@"SocialDisconnectErrorUnsupportedProvider", nil); analyticsError = RBXAErrorUnexpectedResponseCode;} break;
                 case 403: { disconnectMessage = NSLocalizedString(@"SocialDisconnectErrorForbiddenWithoutPW", nil); analyticsError = RBXAErrorUnknownError;} break;
                 case 404: { disconnectMessage = NSLocalizedString(@"SocialErrorUnsupportedAPI", nil); analyticsError = RBXAErrorUnexpectedResponseCode;} break;
                 case 503: { disconnectMessage = NSLocalizedString(@"SocialDisconnectErrorServiceUnavailable", nil); analyticsError = RBXAErrorUnexpectedResponseCode;} break;
                 default:
                 {
                     analyticsError = RBXAErrorUnexpectedResponseCode;
                     disconnectMessage = NSLocalizedString(@"SocialDisconnectErrorUnexpectedError", nil);
                 } break;
                     
             }

             
             if (handler)
                 handler(wasSuccessful, disconnectMessage);
             
             [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:RBXAContextSettingsSocialDisconnect
                                                                       result:RBXAResultFailure
                                                                     rbxError:RBXAErrorGigya
                                                                 startingTime:startTime
                                                            attemptedUsername:[UserInfo CurrentPlayer].username
                                                                   URLRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlRequest]]
                                                             HTTPResponseCode:error.code
                                                                 responseData:[error.description dataUsingEncoding:NSUTF8StringEncoding]
                                                               additionalData:additionalData];
         }];
    }];
}
-(void) doSocialUpdateInfoFromContext:(RBXAnalyticsContextName)context forProvider:providerName withCompletion:(SocialLoginCompletionBlock)handler
{
    NSDate* startingTime = [NSDate date];
    
    NSString* urlString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"social/update-info"];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    [request setHTTPMethod:@"POST"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];

    [NSURLConnection sendAsynchronousRequest:request queue:self.requestQueue completionHandler:^(NSURLResponse *response, NSData *responseData, NSError *responseError)
     {
         RBXAnalyticsErrorName analyticsError = RBXAErrorNoError;
         NSString* serverMessage = nil;
         bool serverSuccess = NO;
         NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
         if (response && responseData && !responseError)
         {
             switch (httpResponse.statusCode)
             {
                //success
                 case 200:
                 { //no JSON returned
                     serverSuccess = YES;
                 } break;
                     
                //forbidden
                 case 403:
                 {
                     analyticsError = RBXAErrorUnknownError;
                     NSError* parseErr;
                     NSDictionary* json = [NSJSONSerialization JSONObjectWithData:responseData options:0 error:&parseErr];
                     if (parseErr || !json)
                         serverMessage = NSLocalizedString(@"SocialConnectErrorCouldNotParseError", nil);
                     else
                     {
                         //NSString* updateStatus = [json stringForKey:@"status"]; //status == "failed"
                         NSString* updateMessage = [json stringForKey:@"message" withDefault:NSLocalizedString(@"SocialConnectErrorCouldNotParseError", nil)];
                         
                         serverMessage = updateMessage;
                     }
                     
                 } break;
                     
                 //service unavailable
                 case  503:
                 {
                     analyticsError = RBXAErrorUnexpectedResponseCode;
                     serverMessage = NSLocalizedString(@"SocialConnectErrorCouldNotRefresh",nil);
                 } break;
                     
                 default:
                 {
                     analyticsError = RBXAErrorUnexpectedResponseCode;
                     serverMessage = NSLocalizedString(@"SocialConnectErrorCouldNotRefresh",nil);
                 }break;
             }
             
             
             if (handler)
                 handler(serverSuccess, serverMessage);
         }
         else
         {
            if (handler)
             handler(NO, @"failed");
             
             analyticsError = RBXAErrorNoHTTPResponse;
         }
         
         
         NSDictionary* additionalData = [NSDictionary dictionaryWithObject:providerName forKey:@"provider"];
         [[SessionReporter sharedInstance] postAnalyticPayloadFromContext:context
                                                                   result:serverSuccess ? RBXAResultSuccess : RBXAResultFailure
                                                                 rbxError:analyticsError
                                                             startingTime:startingTime
                                                        attemptedUsername:[UserInfo CurrentPlayer].username
                                                               URLRequest:request
                                                         HTTPResponseCode:httpResponse ? httpResponse.statusCode : 0
                                                             responseData:responseData
                                                           additionalData:additionalData];
     }];
}

#pragma mark - GAME CENTER FUNCTIONS
- (void) doGameCenterLogin
{
    if (![self isConnectedToInternet])
        return;
    
    [self updateGameCenterUser];
    
    
    NSDictionary* userLoginInfo = [[NSDictionary alloc] initWithObjectsAndKeys:@"",ENCODED_USERNAME_KEY,@"GameCenter",ENCODED_PW_KEY,@"GameCenter",NONENCODED_PW_KEY, nil];
    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGIN_SUCCEEDED object:self userInfo:userLoginInfo];
    
    
}

-(void) updateGameCenterUser
{
    [[NSUserDefaults standardUserDefaults] setObject:@"YES" forKey:@"isGameCenterUser"];
    
    [UserInfo CurrentPlayer].username = @"";
    [UserInfo CurrentPlayer].password = @"";
    
    [self processBackground];
    
    [[UserInfo CurrentPlayer] setUserLoggedIn:YES];
}

#pragma mark LOG OUT FUNCTIONS

// 2015.1211 ajain - This method has been renamed to logout for the following reasons...
// 1) easier to search for all calls to logout and
// 2) XCode's autocomplete was getting confused and directing the definition to Gigya's "logout" method of the exact same name.

- (void) logoutRobloxUser
{
    [self doSocialLogout];
    
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LastSuccessfulLoginExpectedExpiration"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LastSuccessfulLoginDate"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    if ([[self class] apiProxyEnabled] == YES) {
        [self logoutRobloxUserV2];
        return;
    }
    
    NSString* logoutUrlString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"mobileapi/logout"];

    NSURL *url = [NSURL URLWithString: logoutUrlString];
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60*7];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"POST"];
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *data, NSError *error)
     {
         NSString* errString = [self processLogOutResponse:response logoutData:data logoutError:error];
         if (errString.length > 0)
         {
             //do something with the error string
             RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "LogoutViewController : failed to logout");
             NSLog(@"- Error String = %@", errString);
         }
         
         [UserInfo clearUserInfo];
     }];
    
}

- (void) logoutRobloxUserV2
{
    NSString *logoutUrlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/sign-out/v1/"];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:logoutUrlString
           parameters:nil
              success:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
    }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        [self clearAuthCookie];
        
        [[RBXEventReporter sharedInstance] reportError:RBXAErrorBadResponse withContext:RBXAContextLogout withDataString:error.description];
        [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
    }];
}

-(NSString*) processLogOutResponse:(NSURLResponse*) response logoutData:(NSData*) data logoutError:(NSError*) error
{
    NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
    if(httpResponse == nil)
    {
        [[RBXEventReporter sharedInstance] reportError:RBXAErrorNoResponse withContext:RBXAContextLogout withDataString:@"clientside_cookies_cleared"];
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"LogoutViewController: Logout failed due to improper cast");
        [[LoginManager sharedInstance] clearAuthCookie];
        return @"Nil response"; // this should never be hit, but I'd rather be cautious
    }
    
    if(error) // some sort of error happened at the cocoa level, report this and bail
    {
        [[RBXEventReporter sharedInstance] reportError:RBXAErrorCannotParseData withContext:RBXAContextLogout withDataString:@"clientside_cookies_cleared"];
        [[LoginManager sharedInstance] clearAuthCookie];
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"LogoutViewController: Logout failed due to NSError: %s",[[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding]);
        return @"JSON Parse Error"; // this should never be hit, but I'd rather be cautious
    }
    
    // status code 200 means web response was successful (not necessarily successful login though!)
    if ([httpResponse statusCode] == 200)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"LogoutViewController: Logout succeeded with status code 200");
        return [self processSuccessfulLogoutResponse: data  httpResponse:httpResponse];
    }
    else // we didn't even get a good web response, something went wrong, lets print out all info we have
    {
        [[RBXEventReporter sharedInstance] reportError:RBXAErrorBadResponse withContext:RBXAContextLogout withDataString:@"clientside_cookies_cleared"];
        [[LoginManager sharedInstance] clearAuthCookie];
        return [self processFailureLogoutResponse:httpResponse];
    }
}

-(NSString*) processSuccessfulLogoutResponse: (NSData*) data httpResponse:(NSHTTPURLResponse*) httpResponse
{
    NSString* message = @"";
    
    
    [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
    return message;
}

-(NSString*) processFailureLogoutResponse:(NSHTTPURLResponse*) httpResponse
{
    [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
    return [NSString stringWithFormat:@"Logout Failed With Code : %ld",(long)[httpResponse statusCode]];
}


#pragma mark - COOKIE MANAGEMENT

+(NSArray *) cookies
{
    // Set all the cookies from the cookie storage
    NSArray* cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:[NSURL URLWithString:[RobloxInfo getBaseUrl]]];
    
    return cookies;
}

+(void) initializeCookieManagementPolicy
{
    //initalize the cookie policy and add a listener for changes to the cookies
    [[NSHTTPCookieStorage sharedHTTPCookieStorage] setCookieAcceptPolicy:NSHTTPCookieAcceptPolicyAlways];
}

+(void) updateCookiesInAppHttpLayer
{
    NSURL* baseUrl = [NSURL URLWithString:[RobloxInfo getBaseUrl]];
    NSArray* cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:baseUrl];
    std::stringstream cookieStream;
    bool firstCookie = true;
    bool updatedSecurityCookie = false;
    for (NSHTTPCookie* cookie in cookies)
    {
        if (!firstCookie)
        {
            cookieStream << "; ";
        }
        cookieStream << [cookie.name UTF8String] << "=" << [cookie.value UTF8String];
        firstCookie = false;
        
        //for analytics - update the expected expiration date whenever we see the auth cookie
        if ([cookie.name isEqualToString:@".ROBLOSECURITY"])
        {
            updatedSecurityCookie = true;
            double expirationDate = (double)[cookie.expiresDate timeIntervalSince1970];
            NSData* expirationData = [NSData dataWithBytes:&expirationDate length:sizeof(double)];
            [[NSUserDefaults standardUserDefaults] setObject:expirationData forKey:@"LastSuccessfulLoginExpectedExpiration"];
        }
    }
    
    if (updatedSecurityCookie)
        [[NSUserDefaults standardUserDefaults] synchronize];
    
    RBX::Http::setCookiesForDomain([[baseUrl host] UTF8String], cookieStream.str().c_str());
#if 0
    //NSLog(@"\n\n\n****Cookies Recd Start****\n");
    //for (NSHTTPCookie *each in cookies)
    //{
    //
    //    NSLog(@"Name: %@ : Value: %@ Date: %@ Path: %@ Domain: %@ HTTP Only: %@\n\n", each.name, each.value, each.expiresDate, each.path, each.domain, (each.isHTTPOnly ? @"True" : @"False"));
    //}
    //NSLog(@"\n %ld ****Cookies Recd End****\n\n\n", (long)cookies.count);
#endif
    
    //NSLog(@"breakpoint");
    
    //NSLOG_PRETTY_FUNCTION;
    //[self printCookies];
}

-(void) clearAuthCookie
{
    //the web failed to do its job, finish the job on the client side
    NSHTTPCookieStorage* storage = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    for (NSHTTPCookie* c in [storage cookies])
    {
        if ([c.name isEqualToString:@".ROBLOSECURITY"])
        {
            [[NSHTTPCookieStorage sharedHTTPCookieStorage] deleteCookie:c];
            break;
        }
    }
}
+(void) clearAllRobloxCookie;
{
    //CAUTION: CALLING THIS FUNCTION REMOVES THE BROWSER TRACKER THAT THE AB-TEST MANAGER USES TO TRACK TEST ENROLLMENT
    
    NSHTTPCookieStorage *cookieStorage = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    for (NSHTTPCookie *each in [[cookieStorage cookiesForURL:[NSURL URLWithString:[RobloxInfo getBaseUrl]]] copy])
        [cookieStorage deleteCookie:each];
    
    for (NSHTTPCookie *each in [[cookieStorage cookiesForURL:[NSURL URLWithString:[RobloxInfo getDomainString]]] copy])
        [cookieStorage deleteCookie:each];
    
    [[UserInfo CurrentPlayer] setUserLoggedIn:NO];
    
    NSLOG_PRETTY_FUNCTION;
    [self printCookies];
}

+(void) printCookies
{
    NSHTTPCookieStorage *cookieStorage = [NSHTTPCookieStorage sharedHTTPCookieStorage];
    
    NSLog(@"\n\n\n\n***Cookie in Storage Start***\n");
    for (NSHTTPCookie *each in [[cookieStorage cookiesForURL:[NSURL URLWithString:[RobloxInfo getBaseUrl]]] copy])
        NSLog(@"Name: %@ : Value: %@ Date: %@ Path: %@ Domain: %@ HTTP Only: %@\n\n", each.name, each.value, each.expiresDate, each.path, each.domain, (each.isHTTPOnly ? @"True" : @"False"));
    
    for (NSHTTPCookie *each in [[cookieStorage cookiesForURL:[NSURL URLWithString:[RobloxInfo getDomainString]]] copy])
        NSLog(@"Name: %@ : Value: %@ Date: %@ Path: %@ Domain: %@ HTTP Only: %@\n\n", each.name, each.value, each.expiresDate, each.path, each.domain, (each.isHTTPOnly ? @"True" : @"False"));
    
    
    NSLog(@"\n***Cookie in Storage End***\n\n\n\n");
}

@end
