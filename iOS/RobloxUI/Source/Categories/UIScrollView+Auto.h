//
//  UIScrollView+Auto.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/17/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, UIScrollViewDirection) {
    UIScrollViewDirectionNone,
    UIScrollViewDirectionHorizontal = 0x1 << 1,
    UIScrollViewDirectionVertical = 0x1 << 2,
    UIScrollViewDirectionBoth = UIScrollViewDirectionHorizontal | UIScrollViewDirectionVertical
};

@interface UIScrollView (Auto)

- (void) setContentSizeForDirection:(UIScrollViewDirection)direction;

@end
