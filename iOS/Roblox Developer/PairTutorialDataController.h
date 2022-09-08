//
//  PairTutorialDataController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/15/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PairTutorialViewController.h"

@interface PairTutorialDataController : UIViewController <UIPageViewControllerDataSource>

@property (strong, nonatomic) UIPageViewController *pageViewController;
@property (strong, nonatomic) NSArray *pageTitles;
@property (strong, nonatomic) NSArray *pageInstructions;
@property (strong, nonatomic) NSArray *pageImages;

@end
