//
//  UIPopOverController+Helpers.h
//  RobloxMobile
//
//  Created by Ashish Jain on 9/4/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIPopoverController (Helpers)

- (void) dismissPopoverAnimated:(BOOL)animated completionHandler:(void(^)(void))completionHandler;

@end
