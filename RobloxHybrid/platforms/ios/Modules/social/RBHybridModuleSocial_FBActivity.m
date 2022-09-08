//
//  RBHybridModuleSocial_FBActivity.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/19/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleSocial_FBActivity.h"
#import <FacebookSDK/FacebookSDK.h>
#import <Social/Social.h>

@implementation RBHybridModuleSocial_FBActivity
{
    FBLinkShareParams* _shareParams;
    BOOL _shareWithFBApp;
}

+ (UIActivityCategory)activityCategory
{
    return UIActivityCategoryShare;
}

- (NSString *)activityType
{
    return @"com.roblox.activity.facebook";
}

- (NSString *)activityTitle
{
    return @"Facebook";
}

- (UIImage *)activityImage
{
    UIImage* image = [UIImage imageNamed:@"Social-FB-Icon"];
    return image;
}

// For some reason on iOS7 without this function the icon is completely gray
- (UIImage *)_activityImage
{
    UIImage* image = [UIImage imageNamed:@"Social-FB-Icon"];
    return image;
}

- (BOOL)canPerformWithActivityItems:(NSArray *)activityItems
{
    return YES;
}

- (void)prepareWithActivityItems:(NSArray *)activityItems
{
    _shareParams = [[FBLinkShareParams alloc] init];
    _shareParams.name = _text;
    _shareParams.link = _link;
    _shareParams.picture = _imageURL;
    
    _shareWithFBApp = ![FBDialogs canPresentOSIntegratedShareDialog] && [FBDialogs canPresentShareDialogWithParams:_shareParams];
}

- (UIViewController *)activityViewController
{
    if(!_shareWithFBApp)
    {
        SLComposeViewController *composeViewController = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeFacebook];
        [composeViewController setInitialText:_text];
        
        if(_link)
        {
            [composeViewController addURL:_link];
        }
        
        if(_image)
        {
            [composeViewController addImage:_image];
        }
        
        [composeViewController setCompletionHandler:^(SLComposeViewControllerResult result)
        {
            [self activityDidFinish:(result == SLComposeViewControllerResultDone)];
        }];
        
        return composeViewController;
    }
    
    return nil;
}

- (void)performActivity
{
    if(_shareWithFBApp)
    {
        // FBDialogs call to open Share dialog within the Facebook app
        [FBDialogs
            presentShareDialogWithParams:_shareParams
            clientState:nil
            handler:^(FBAppCall *call, NSDictionary *results, NSError *error)
            {
                BOOL success = !error && !(results[@"completionGesture"] && [results[@"completionGesture"] isEqualToString:@"cancel"]);
                [self activityDidFinish:success];
            }];
    }
}

@end
