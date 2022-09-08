//
//  NonRotatableNavigationController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 3/10/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "NonRotatableNavigationController.h"

@interface NonRotatableNavigationController ()

@end

@implementation NonRotatableNavigationController

#ifdef __IPHONE_9_0
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
#else
- (NSUInteger)supportedInterfaceOrientations
#endif
{
    switch ([UIDevice currentDevice].userInterfaceIdiom)
    {
        case UIUserInterfaceIdiomPad:   return UIInterfaceOrientationMaskLandscape;
        case UIUserInterfaceIdiomPhone: return UIInterfaceOrientationMaskPortrait;
        case UIUserInterfaceIdiomUnspecified: return UIInterfaceOrientationMaskPortrait;
        default: return UIInterfaceOrientationMaskPortrait;
    }
}
- (BOOL)shouldAutorotate
{
    return NO;
}

@end
