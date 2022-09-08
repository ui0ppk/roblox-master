//
//  RBBarButtonMenu.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/4/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RBBarButtonMenu : UIBarButtonItem

- (instancetype) initWithTitle:(NSString*)title style:(UIBarButtonItemStyle)style;
- (instancetype) initWithImage:(UIImage *)image style:(UIBarButtonItemStyle)style;

- (void)addItemWithTitle:(NSString *)title target:(id)target action:(SEL)action;

@end
