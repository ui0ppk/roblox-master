//
//  UserInfo.h
//  RobloxMobile
//
//  Created by David York on 10/9/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RobloxData.h"
#import <GigyaSDK/Gigya.h>

@interface UserInfo : NSObject

@property (retain, nonatomic) RBXUserAccountNotifications* accountNotifications;
@property (retain, nonatomic) NSMutableDictionary* userSocialInfoDict;
@property (retain, nonatomic) GSResponse* userGigyaInfo;
@property (retain, nonatomic) NSNumber* userId;
@property (retain, nonatomic) NSString* username;
@property (retain, nonatomic) NSString* userEmail;
@property (retain, nonatomic) NSString* password;
@property (retain, nonatomic) NSNumber* rbxBal;
@property (retain, nonatomic) NSNumber* tikBal;
@property (retain, nonatomic) NSString* userThumbNailUrl;
@property (retain, nonatomic) NSString* bcMember;
@property (retain, nonatomic) NSString* encodedPassword;
@property (retain, nonatomic) NSString* encodedUsername;
@property (retain, nonatomic) NSString* birthday;

@property (nonatomic) BOOL userLoggedIn;
@property (nonatomic) BOOL userOver13;
@property (nonatomic) BOOL userHasSetPassword;


//Accessors
+(UserInfo*) CurrentPlayer;
-(NSString*) Robux;
-(NSString*) Tix;

-(BOOL) isConnectedToIdentity:(NSString*)identity;
-(BOOL) isConnectedToFacebook;
-(BOOL) isConnectedToTwitter;
-(BOOL) isConnectedToGooglePlus;
-(NSString*) getNameConnectedToIdentity:(NSString*)identity;
-(NSString*) GigyaUID;
-(NSString*) GigyaUIDSignature;
-(NSString*) GigyaSignatureTimestamp;
-(NSString*) GigyaName;
-(NSString*) GigyaPhotoURL;
-(NSString*) GigyaLoginProvider;
-(NSString*) GigyaLoginProviderUID;
-(NSString*) GigyaProvider;
-(NSString*) GigyaGender;
-(NSString*) GigyaBirthDay;
-(NSString*) GigyaBirthMonth;
-(NSString*) GigyaBirthYear;
-(NSString*) GigyaEmail;

//Mutators
-(void)UpdatePlayerInfo;
-(void)UpdateAccountInfo;

+(void) clearUserInfo;

@end
