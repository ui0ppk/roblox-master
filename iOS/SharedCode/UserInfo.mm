//
//  UserInfo.m
//  RobloxMobile
//
//  Created by David York on 10/9/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "UserInfo.h"
#import "RobloxInfo.h"
#import "LoginManager.h"
#import "RobloxGoogleAnalytics.h"
#import "RobloxAlert.h"
#import "KeychainItemWrapper.h"
#import "RobloxData.h"
#import "NSDictionary+Parsing.h"
#import "RobloxNotifications.h"
#import "RBXFunctions.h"

#include "util/StandardOut.h"

UserInfo* _currentPlayer = nil;

NSString* convertToFriendlyString(NSNumber* original);

@implementation UserInfo

-(id)init
{
    self = [super init];
    if (self)
    {
        _userOver13 = YES;
        _userLoggedIn = NO;
        _userHasSetPassword = YES;
        
        self.userSocialInfoDict = [NSMutableDictionary dictionary];
    }
    return self;
}

//ACCESSORS
+(UserInfo*) CurrentPlayer      {
    if (_currentPlayer == nil) {
        _currentPlayer = [[UserInfo alloc] init];
    }
    return _currentPlayer;
}

-(NSString*) Robux { return convertToFriendlyString(self.rbxBal); }
-(NSString*) Tix { return convertToFriendlyString(self.tikBal);; }

-(BOOL) isConnectedToIdentity:(NSString*)identity {
    if (self.userSocialInfoDict)
    {
        NSArray* identities = [self.userSocialInfoDict arrayForKey:@"identities" withDefault:@[]];
        for (NSDictionary* idDict in identities)
        {
            if (idDict)
                if ([[idDict stringForKey:@"provider" withDefault:@" "] isEqualToString:identity])
                    return YES;
        }
    }
    return NO;
}
-(BOOL) isConnectedToFacebook   { return [self isConnectedToIdentity:@"facebook"]; }
-(BOOL) isConnectedToTwitter    { return [self isConnectedToIdentity:@"twitter"];  }
-(BOOL) isConnectedToGooglePlus { return [self isConnectedToIdentity:@"google"]; }
-(NSString*) getNameConnectedToIdentity:(NSString*)identity {
    if (self.userSocialInfoDict)
    {
        NSArray* identities = [self.userSocialInfoDict arrayForKey:@"identities" withDefault:@[]];
        for (NSDictionary* idDict in identities)
        {
            if (idDict)
                if ([[idDict stringForKey:@"provider" withDefault:@" "] isEqualToString:identity])
                    return [idDict stringForKey:@"nickname" withDefault:nil];
        }
    }
    return nil;
}
-(NSString*) GigyaUID                   { return [self.userSocialInfoDict stringForKey:@"UID"                  withDefault:@"null"]; }
-(NSString*) GigyaUIDSignature          { return [self.userSocialInfoDict stringForKey:@"UIDSignature"         withDefault:@"null"]; }
-(NSString*) GigyaSignatureTimestamp    { return [self.userSocialInfoDict stringForKey:@"signatureTimestamp"   withDefault:@"null"]; }
-(NSString*) GigyaName                  { return [self.userSocialInfoDict stringForKey:@"firstName"            withDefault:@"null"]; }
-(NSString*) GigyaPhotoURL              { return [self.userSocialInfoDict stringForKey:@"photoURL"             withDefault:@"null"]; }
-(NSString*) GigyaLoginProvider         { return [self.userSocialInfoDict stringForKey:@"loginProvider"        withDefault:@"null"]; }
-(NSString*) GigyaLoginProviderUID      { return [self.userSocialInfoDict stringForKey:@"loginProviderUID"     withDefault:@"null"]; }
-(NSString*) GigyaProvider              { return [self.userSocialInfoDict stringForKey:@"providers"            withDefault:@"null"]; }
-(NSString*) GigyaGender                {
    NSString* gender = [self.userSocialInfoDict stringForKey:@"gender" withDefault:@"null"];
    if      ([gender isEqualToString:@"m"]) gender = @"MALE";
    else if ([gender isEqualToString:@"f"]) gender = @"FEMALE";
    else                                    gender = @"UNKNOWN";
    return gender;
}
-(NSString*) GigyaBirthDay              { return [self.userSocialInfoDict stringForKey:@"birthDay"             withDefault:@"null"]; }
-(NSString*) GigyaBirthMonth            { return [self.userSocialInfoDict stringForKey:@"birthMonth"           withDefault:@"null"]; }
-(NSString*) GigyaBirthYear             { return [self.userSocialInfoDict stringForKey:@"birthYear"            withDefault:@"null"]; }
-(NSString*) GigyaEmail                 { return [self.userSocialInfoDict stringForKey:@"email"                withDefault:nil]; }

//MUTATORS
-(void) setUserLoggedIn:(BOOL)userLoggedIn {
    _userLoggedIn = userLoggedIn;
    if (userLoggedIn == NO)
    {
        //clear out the password but not the password
        self.password = @"";
        self.encodedPassword = @"";
        self.userSocialInfoDict = nil;
        
        //clear out the user defaults
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"password"];
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LastUserLoggedIn"];
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LastUserIDLoggedIn"];
        
        //clear out the keychain item password
        KeychainItemWrapper *keychainItem = [[KeychainItemWrapper alloc] initWithIdentifier:[[[NSBundle mainBundle] bundleIdentifier] stringByAppendingString:@"RobloxLogin"]  accessGroup:nil];
        [keychainItem setObject:@"" forKey:(__bridge id)kSecValueData];
        
        //notify others that we've logged out
        [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGGED_OUT object:self];
    }
    else
    {
        [[NSUserDefaults standardUserDefaults] setObject:self.username forKey:@"LastUserLoggedIn"];
        [[NSUserDefaults standardUserDefaults] setInteger:[self.userId integerValue]forKey:@"LastUserIDLoggedIn"];
        
        [self UpdateAccountInfo];
    }

    [[NSUserDefaults standardUserDefaults] synchronize];
}

-(void)UpdatePlayerInfo
{
    if (!_userLoggedIn)
        return;
    
    NSString* urlString;
    urlString = [[RobloxInfo getBaseUrl] stringByAppendingString:@"mobileapi/userinfo"];
    urlString = [urlString stringByReplacingOccurrencesOfString:@"http:" withString:@"https:"];
    
    
    NSURL *url = [NSURL URLWithString: urlString];
    
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60*7];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"GET"];
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *receiptResponseData, NSError *error)
     {
         NSHTTPURLResponse* urlResponse = ( NSHTTPURLResponse*) response;
         
         if(urlResponse == nil)
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"UserInfo: Login failed due to improper cast");
         
         int responseStatusCode = [urlResponse statusCode];
         
         bool bFail = false;
         
         if (responseStatusCode == 200)
         {
             NSDictionary* dict = [[NSDictionary alloc] init];
             NSError* error = nil;
             dict = [NSJSONSerialization JSONObjectWithData:receiptResponseData options:kNilOptions error:&error];
             if (error)
             {
                 NSLog(@"UpdatePlayerInfo ERROR!! %@", error);
                 bFail = true;
             }
             
             /// GET THE INFO
             self.userId = [dict objectForKey:@"UserID"];
             self.username = [dict objectForKey:@"UserName"];
             self.rbxBal = [dict objectForKey:@"RobuxBalance"];
             self.tikBal = [dict objectForKey:@"TicketsBalance"];
             self.userThumbNailUrl = [dict objectForKey:@"ThumbnailUrl"];
             self.bcMember = [dict objectForKey:@"IsAnyBuildersClubMember"];
         }
         else
         {
             RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"UserInfo: Cannot read from %s",[urlString cStringUsingEncoding:NSUTF8StringEncoding]);
             RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"UserInfo: Update failed with http response code: %d",responseStatusCode);
             NSDictionary* httpRespHdrs = [urlResponse allHeaderFields];
             NSArray* keys = [httpRespHdrs allKeys];
             for (NSString* key in keys)
             {
                 NSString* value = [httpRespHdrs valueForKey:key];
                 RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"UserInfo: http header info: %s = %s", [key cStringUsingEncoding:NSUTF8StringEncoding], [value cStringUsingEncoding:NSUTF8StringEncoding]);
             }
             
             bFail = true;
         }
         
         if (bFail && _userLoggedIn)
         {
             // clean up
             [[LoginManager sharedInstance] logoutRobloxUser];
             
             [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_LOGGED_OUT object:self userInfo:nil];
             
             [RobloxGoogleAnalytics setPageViewTracking:@"Logout/Success"];
             [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"Unknown Login Failure", nil)];
         }
         
     }];
   
}
-(void)UpdateAccountInfo
{
#ifndef ROBLOX_DEVELOPER
    [RobloxData fetchMyAccountWithCompletion:^(RBXUserAccount* account)
     {
         _userOver13 = account ? account.userAbove13 : NO;
         self.username = account ? account.userName : nil;
         self.userEmail = account ? account.email : nil;
         //NSNumber* ageBracket = account ? account.ageBracket : nil;
         self.userId = account ? account.userId : nil;
     }];
    [RobloxData fetchAccountNotificationsWithCompletion:^(RBXUserAccountNotifications *notifications) {
        self.accountNotifications = notifications;
    }];
    [RobloxData fetchUserHasSetPasswordWithCompletion:^(bool isSet) {
        self.userHasSetPassword = isSet;
    }];
#endif
}

+(void) clearUserInfo
{
    if (_currentPlayer != nil) {
        _currentPlayer = nil;
    }
}

@end

NSString* convertToFriendlyString(NSNumber* original)
{
    if (original == nil)
        return @"unknown";
    int val = [original intValue];
    if (val >= 1000000) {
        return [NSString stringWithFormat:@"%d mil", val/1000000];
    } else if (val >= 1000) {
        int thousands = val/1000;
        return [NSString stringWithFormat:@"%d,%03d", thousands, val-(thousands*1000)];
    } else {
        return [NSString stringWithFormat:@"%d", val];
    }
}
