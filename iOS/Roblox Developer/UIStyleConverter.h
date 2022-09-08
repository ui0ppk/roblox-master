//
//  UIStyleConverter.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/13/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface UIStyleConverter : NSObject


+(void) convertToButtonStyle:(UIButton*) button;
+(void) convertToButtonBlueStyle:(UIButton*) button;
+(void) convertToBorderlessButtonStyle:(UIButton*) button;

+(void) convertToTitleStyle:(UILabel*) label;
+(void) convertToBlueTitleStyle:(UILabel*) label;

+(void) convertToLabelStyle:(UILabel*) label;
+(void) convertToLargeLabelStyle:(UILabel*) label;
+(void) convertToLoadingStyle:(UIImageView*) imageView;
+(void) convertToTextFieldStyle:(UITextField*) textField;
+(void) convertToBoldTextFieldStyle:(UITextField*) textField;

+(void) convertUIButtonToLabelStyle:(UIButton*) button;
+(void) convertToHyperlinkStyle:(UIButton*) button;

+(void) convertToBlueNavigationBarStyle:(UINavigationBar*) bar;
+(void) convertToNavigationBarStyle;
+(void) convertToPagingStyle;

+(void) convertToTexturedBackgroundStyle:(UIView*) view;
@end
