//
//  UIButton+Extensions.h
//  RobloxMobile
//
//  Created by Ashish Jain on 9/4/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIButton (Extensions)

@property (nonatomic, assign) UIEdgeInsets hitTestEdgeInsets;

- (void)setColor:(UIColor *)color forState:(UIControlState)state;

@end
