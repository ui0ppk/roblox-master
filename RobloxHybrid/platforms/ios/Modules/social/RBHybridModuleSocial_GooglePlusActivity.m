//
//  RBHybridModuleSocial_GooglePlusActivity.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/19/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleSocial_GooglePlusActivity.h"

#import <GooglePlus/GooglePlus.h>
#import <GoogleOpenSource/GoogleOpenSource.h>

//
// As defined in https://console.developers.google.com/project/roblox-mobile
//
static NSString * const kClientId = @"757902889875-mdpibqck9096ap3gt0minfg85d27kavm.apps.googleusercontent.com";

@interface RBHybridModuleSocial_GooglePlusActivity() <GPPSignInDelegate, GPPShareDelegate>

@end

@implementation RBHybridModuleSocial_GooglePlusActivity

+ (UIActivityCategory)activityCategory
{
    return UIActivityCategoryShare;
}

- (NSString *)activityType
{
    return @"com.roblox.activity.googleplus";
}

- (NSString *)activityTitle
{
    return @"Google+";
}

- (UIImage *)activityImage
{
    UIImage* image = [UIImage imageNamed:@"Social-Google-Icon"];
    return image;
}

// For some reason on iOS7 without this function the icon is completely gray
- (UIImage *)_activityImage
{
    UIImage* image = [UIImage imageNamed:@"Social-Google-Icon"];
    return image;
}

- (BOOL)canPerformWithActivityItems:(NSArray *)activityItems
{
    return YES;
}

- (void)prepareWithActivityItems:(NSArray *)activityItems
{
    [GPPSignIn sharedInstance].clientID = kClientId;
}

- (UIViewController *)activityViewController
{
    return nil;
}

- (void)performActivity
{
    [self signIn];
}

- (void)signIn
{
    GPPSignIn *signIn = [GPPSignIn sharedInstance];
    signIn.shouldFetchGooglePlusUser = YES;
    //signIn.shouldFetchGoogleUserEmail = YES;

    signIn.delegate = self;
    signIn.clientID = kClientId;
    
    signIn.scopes = @[ kGTLAuthScopePlusLogin ];  // "https://www.googleapis.com/auth/plus.login" scope
    //signIn.scopes = @[ @"profile" ];            // "profile" scope
    
    BOOL userLoggedIn = [signIn trySilentAuthentication];
    if(!userLoggedIn)
    {
        [signIn authenticate];
    }
}

// The authorization has finished and is successful if |error| is |nil|.
- (void)finishedWithAuth:(GTMOAuth2Authentication *)auth error:(NSError *)error
{
    if(error == nil)
    {
        [self shareLink];
    }
    else
    {
        [self activityDidFinish:NO];
    }
}

- (void)shareLink
{
    [GPPShare sharedInstance].delegate = self;
    
    id<GPPNativeShareBuilder> shareBuilder = [[GPPShare sharedInstance] nativeShareDialog];
    
    [shareBuilder setPrefillText:_text];
    [shareBuilder setURLToShare:_link];
    [shareBuilder attachImage:_image];
    [shareBuilder setContentDeepLinkID:[_link query]];
    
    [shareBuilder open];
}

- (void)finishedSharing:(BOOL)shared
{
    [self activityDidFinish:shared];
}

@end
