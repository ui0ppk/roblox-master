//
//  UIView+Helpers.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/11/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "UIViewController+Helpers.h"
#import "RobloxTheme.h"

@implementation UIViewController (Helpers)

+ (UIViewController*) topViewController
{
    return [UIViewController topViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
}

+ (UIViewController *)topViewController:(UIViewController *)rootViewController
{
    if (rootViewController.presentedViewController == nil)
    {
        return rootViewController;
    }
    else if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];
        return [self topViewController:lastViewController];
    }
    else if([rootViewController.presentedViewController isKindOfClass:[UIAlertController class]])
    {
        return rootViewController;
    }
    else
    {
        UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;
        return [self topViewController:presentedViewController];
    }
}

@end
