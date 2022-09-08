//
//  LoginManager.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 4/19/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <GigyaSDK/Gigya.h>
#import "NonRotatableNavigationController.h"
#import "RBXEventReporter.h"

#define ENCODED_PW_KEY @"encodedPassword"
#define ENCODED_USERNAME_KEY @"encodedUserName"
#define NONENCODED_PW_KEY @"NonEncodedPassword"

typedef void(^LoginManagerCompletionBlock)(NSError *loginError);
typedef void(^SocialLoginCompletionBlock)(bool success, NSString *message);

@interface LoginManager : NSObject

+(id) sharedInstance;
+ (BOOL) apiProxyEnabled;

#pragma mark - odd Captcha functions
+(NonRotatableNavigationController*) CaptchaForLoginWithUsername:(NSString*)username
                                                 andV1Completion:(SocialLoginCompletionBlock)completion1
                                                 andV2Completion:(LoginManagerCompletionBlock)completion2;
+(NonRotatableNavigationController*) CaptchaForSignupWithUsername:(NSString*)username
                                                  andV1Completion:(SocialLoginCompletionBlock)completion1
                                                  andV2Completion:(LoginManagerCompletionBlock)completion2;
+(NonRotatableNavigationController*) CaptchaForSocialSignupWithUsername:(NSString*)username
                                                        andV1Completion:(SocialLoginCompletionBlock)completion1
                                                        andV2Completion:(LoginManagerCompletionBlock)completion2;

#pragma mark - Life cycle functions
-(void) applicationWillTerminate;// call this method as application is going to terminate (saves info for next session)

#pragma mark - Accessors and Mutators
-(BOOL) getRememberPassword;
-(void) setRememberPassword:(BOOL) shouldRemember;
-(BOOL) hasLoginCredentials;
-(BOOL) hasSocialLoginCredentials;
+(BOOL) sessionLoginEnabled;

#pragma mark - Odd functions
-(void) processBackground;

#pragma mark - Social Functions
-(bool) isFacebookEnabled;
+(NSString*) ProviderNameFacebook;
+(NSString*) ProviderNameTwitter;
+(NSString*) ProviderNameGooglePlus;
-(void) doSocialLoginFromController:(UIViewController*)aController
                        forProvider:(NSString*)providerName
                     withCompletion:(SocialLoginCompletionBlock)completionHandler;
-(void) doSocialSignupWithUsername:(NSString*)username
                           gigyaID:(NSString*)gigyaUID
                          birthday:(NSString*)birthdayString
                            gender:(NSString*)playerGender
                             email:(NSString*)playerEmail
                        completion:(SocialLoginCompletionBlock)handler;
-(void) doSocialLogout;
-(void) doSocialFetchGigyaInfoWithUID:(NSString*)gigyaUID isLoggingIn:(bool)isLogin withCompletion:(SocialLoginCompletionBlock)completionHandler;
-(void) doSocialUpdateInfoFromContext:(RBXAnalyticsContextName)context forProvider:providerName withCompletion:(SocialLoginCompletionBlock)handler;
-(void) doSocialConnect:(UIViewController*)aController
             toProvider:(NSString*)providerName
         withCompletion:(void(^)(bool success, NSString* message))completionHandler;
-(void) doSocialDisconnect:(NSString*)providerName withCompletion:(SocialLoginCompletionBlock)handler;
-(void) doSocialNotifyGigyaLoginWithContext:(RBXAnalyticsContextName)context withCompletion:(SocialLoginCompletionBlock)completion;

#pragma mark - Game Center
-(void) doGameCenterLogin;
-(void) updateGameCenterUser;

#pragma mark - Login/Out Functions
-(void) processStartupAutoLogin:(LoginManagerCompletionBlock)autoLoginCompletionBlock;
-(void) loginWithUsername:(NSString*) username password:(NSString*)password completionBlock:(LoginManagerCompletionBlock)completionBlock;
-(void) logoutRobloxUser;

- (void) getUserAccountInfoForContext:(RBXAnalyticsContextName)context completionHandler:(LoginManagerCompletionBlock)loginV2CompletionHandler;

#pragma mark -  Cookie Management
+(NSArray *) cookies;
+(void) initializeCookieManagementPolicy;
+(void) updateCookiesInAppHttpLayer;
+(void) clearAllRobloxCookie;
+(void) printCookies;

@end
