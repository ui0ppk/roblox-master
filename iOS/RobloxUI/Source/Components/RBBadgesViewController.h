//
//  RBBadgesViewController.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/7/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RBBadgesViewController : UIViewController

@property(strong, nonatomic) NSString* gameID;

@property(nonatomic, readonly) NSUInteger numItems;

@property(copy, nonatomic) void (^completionHandler)();

@end
