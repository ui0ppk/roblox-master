//
//  RBXUIUtil.m
//  RobloxMobile
//
//  Created by Ashish Jain on 9/4/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBXUIUtil.h"

#import "RBXFunctions.h"
#import "UIButton+Extensions.h"

@implementation RBXUIUtil

+ (void) increaseTappableAreaOfButton:(UIButton *)button {
    if ([RBXFunctions isEmpty:button]) {
        return;
    }
    
    // Used in conjunction with UIButton+Extensions.h to increase the tappable area of the button without increasing the size of the button
    [button setHitTestEdgeInsets:UIEdgeInsetsMake(-10, -10, -10, -10)];
}

+ (void) view:(UIView *)view setOrigin:(CGPoint)newOrigin {
    if ([RBXFunctions isEmpty:view]) {
        return;
    }

    CGRect frame = view.frame;
    frame.origin = newOrigin;
    view.frame = frame;
}

+ (void) view:(UIView *)view setSize:(CGSize)newSize {
    if ([RBXFunctions isEmpty:view]) {
        return;
    }
    
    CGRect frame = view.frame;
    frame.size = newSize;
    view.frame = frame;
}

@end
