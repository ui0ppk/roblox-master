//
//  RBHybridModuleSocial_FBActivity.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/19/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RBHybridModuleSocial_FBActivity : UIActivity

@property(weak, nonatomic) UIViewController* parentController;
@property(strong, nonatomic) NSString* text;
@property(strong, nonatomic) NSURL* link;
@property(strong, nonatomic) NSURL* imageURL;
@property(strong, nonatomic) UIImage* image;

@end
