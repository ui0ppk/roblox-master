//
//  SettingsViewController.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SettingsViewController : UIViewController <UITableViewDelegate, UITableViewDataSource>
{
    NSArray *debugDisplayTableData;
}

@property (retain, nonatomic) IBOutlet UITableView *debugDisplayTableView;
@property (retain, nonatomic) IBOutlet UILabel *debugDisplayLabel;


@property (retain, nonatomic) IBOutlet UIButton *pairWithStudioButton;
@property (retain, nonatomic) IBOutlet UILabel *pairWithStudioLabel;

@property (retain, nonatomic) IBOutlet UIView *contentView;

- (IBAction) pairWithStudioPressed:(UIButton *)sender;
- (IBAction) loginButtonPressed:(UIBarButtonItem *)sender;

@end
