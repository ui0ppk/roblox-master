//
//  UITabBarItem+CustomBadge.h
//  CityGlance
//
//  Created by Enrico Vecchio on 05/18/14.
//  Downloaded from Github and additional functionality added by Kyler Mulherin 10/23/2014.
//  Copyright (c) 2014 Cityglance SRL. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UITabBarItem (CustomBadge)



-(void) setMyAppCustomBadgeValue: (NSString *) value;

-(void) setCustomBadgeValue: (NSString *) value withColor: (UIColor *) labelColor;

-(void) setCustomBadgeValue: (NSString *) value withFont: (UIFont *) font andFontColor: (UIColor *) color andBackgroundColor: (UIColor *) backColor;

@end
