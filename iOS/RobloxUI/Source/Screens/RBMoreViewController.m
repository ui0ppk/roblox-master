//
//  RBMoreViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/9/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBMoreViewController.h"
#import "MoreTileButton.h"

@interface RBMoreViewController ()

@property IBOutlet UIButton* btnBC;
@property IBOutlet UIButton* btnRBX;

@end

@implementation RBMoreViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}


#pragma mark
#pragma Button Actions
-(IBAction)openScreenFromSegue:(id)sender
{
    if ([sender isKindOfClass:[MoreTileButton class]])
    {
        MoreTileButton* btn = sender;
        [self performSegueWithIdentifier:btn.segueName sender:self];
    }
}

@end
