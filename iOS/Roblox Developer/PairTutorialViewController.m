//
//  PairTutorialViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/15/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "PairTutorialViewController.h"
#import "UIStyleConverter.h"
#import "RobloxGoogleAnalytics.h"

@interface PairTutorialViewController ()

@end

@implementation PairTutorialViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.instructionImageView.image = [UIImage imageNamed:self.imageFile];
    self.instructionLabel.text = self.instructionText;
    self.instructionTitle.text = self.titleText;
    
    [UIStyleConverter convertToLabelStyle:self.instructionLabel];
    [UIStyleConverter convertToTitleStyle:self.instructionTitle];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [UIStyleConverter convertToNavigationBarStyle];
    }
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"PairTutorial"];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
