//
//  MoreTileButton.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/9/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

IB_DESIGNABLE
@interface MoreTileButton : UIButton

@property (nonatomic) IBInspectable NSInteger layoutStyle;
@property (nonatomic) IBInspectable UIImage* imageName;
@property (nonatomic) IBInspectable UIImage* imageDownName;
@property (nonatomic) IBInspectable NSString* iconLabel;
@property (nonatomic) IBInspectable BOOL isEnabled;

//for use with Sponsored Event posts, don't forget to cast it
@property (nonatomic) id extraInfo;

-(void) setBadgeValue:(NSString*)value;
@end
