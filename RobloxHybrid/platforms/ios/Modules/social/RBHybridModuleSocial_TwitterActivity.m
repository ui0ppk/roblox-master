//
//  RBHybridModuleSocial_TwitterActivity.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/19/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleSocial_TwitterActivity.h"
#import <Social/Social.h>

@implementation RBHybridModuleSocial_TwitterActivity

+ (UIActivityCategory)activityCategory
{
    return UIActivityCategoryShare;
}

- (NSString *)activityType
{
    return @"com.roblox.activity.twitter";
}

- (NSString *)activityTitle
{
    return @"Twitter";
}

- (UIImage *)activityImage
{
    UIImage* image = [UIImage imageNamed:@"Social-Twitter-Icon"];
    return image;
}

// For some reason on iOS7 without this function the icon is completely gray
- (UIImage *)_activityImage
{
    UIImage* image = [UIImage imageNamed:@"Social-Twitter-Icon"];
    return image;
}

- (BOOL)canPerformWithActivityItems:(NSArray *)activityItems
{
    return YES;
}

- (void)prepareWithActivityItems:(NSArray *)activityItems
{
}

- (UIViewController *)activityViewController
{
    NSString *message = _text;
    NSString *urlString = [_link absoluteString];
    
    // Boldly invoke the target app, because the phone will display a nice message asking to configure the app
    SLComposeViewController *composeViewController = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeTwitter];
    [composeViewController setInitialText:message];
    [composeViewController addURL:[NSURL URLWithString:urlString]];
    
    [composeViewController setCompletionHandler:^(SLComposeViewControllerResult result)
    {
        [self activityDidFinish:(result == SLComposeViewControllerResultDone)];
    }];
    
    return composeViewController;
}

- (void)performActivity
{

}

@end
