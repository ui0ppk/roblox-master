//
//  PairTutorialViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/15/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface PairTutorialViewController : UIViewController
{
    
}

@property NSUInteger pageIndex;
@property (retain) NSString *titleText;
@property (retain) NSString *instructionText;
@property (retain) NSString *imageFile;

@property (retain, nonatomic) IBOutlet UILabel *instructionTitle;
@property (retain, nonatomic) IBOutlet UILabel *instructionLabel;
@property (retain, nonatomic) IBOutlet UIImageView *instructionImageView;

@end
