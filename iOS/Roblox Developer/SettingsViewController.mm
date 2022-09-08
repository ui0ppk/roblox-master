//
//  SettingsViewController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/18/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//
#include <v8datamodel/GuiBuilder.h>

#import "SettingsViewController.h"
#import "StudioPairViewController.h"
#import "UIStyleConverter.h"
#import "TestAccountSigninViewController.h"
#import "RobloxGoogleAnalytics.h"

@interface SettingsViewController ()

@end

@implementation SettingsViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    debugDisplayTableData = [NSArray arrayWithObjects:@"None", @"FPS", @"Summary", @"Physics", @"PhysicsAndOwner", @"Render", nil];
    
    [UIStyleConverter convertToBlueTitleStyle:self.pairWithStudioLabel];
    [UIStyleConverter convertToBlueTitleStyle:self.debugDisplayLabel];
    [UIStyleConverter convertUIButtonToLabelStyle:self.pairWithStudioButton];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [UIStyleConverter convertToBlueNavigationBarStyle: self.navigationController.navigationBar];
        self.debugDisplayTableView.scrollEnabled = NO;
    }
    else
    {
        NSLayoutConstraint *leftConstraint = [NSLayoutConstraint constraintWithItem:self.contentView
                                                                          attribute:NSLayoutAttributeLeft
                                                                          relatedBy:NSLayoutRelationEqual
                                                                             toItem:self.view
                                                                          attribute:NSLayoutAttributeLeft
                                                                         multiplier:1.0
                                                                           constant:0];
        [self.view addConstraint:leftConstraint];
        
        NSLayoutConstraint *rightConstraint = [NSLayoutConstraint constraintWithItem:self.contentView
                                                                           attribute:NSLayoutAttributeWidth
                                                                           relatedBy:NSLayoutRelationEqual
                                                                              toItem:self.view
                                                                           attribute:NSLayoutAttributeWidth
                                                                          multiplier:1.0
                                                                            constant:0];
        [self.view addConstraint:rightConstraint];
    }
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Settings"];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction) pairWithStudioPressed:(UIButton *)sender
{
    StudioPairViewController *pairViewcontroller = (StudioPairViewController*) [self.storyboard instantiateViewControllerWithIdentifier:@"StudioPairViewController"];
    pairViewcontroller.shouldTryConnect = NO;
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
    {
        [self.navigationController pushViewController:pairViewcontroller animated:YES];
    }
    else
    {
        UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:pairViewcontroller];
        [navigation setModalPresentationStyle:UIModalPresentationFormSheet];
        [navigation setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
        
        [self presentViewController:navigation animated:YES completion:nil];
    }
}

- (IBAction) loginButtonPressed:(UIBarButtonItem *)sender
{
    TestAccountSigninViewController *testAccountSigninViewController = (TestAccountSigninViewController*) [self.storyboard instantiateViewControllerWithIdentifier:@"TestAccountSigninViewController"];
    
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:testAccountSigninViewController];
    [navigation setModalPresentationStyle:UIModalPresentationFormSheet];
    [navigation setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
    
    [self presentViewController:navigation animated:YES completion:nil];
    
    [RobloxGoogleAnalytics setPageViewTracking:@"Settings/LoginButton"];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [debugDisplayTableData count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *simpleTableIdentifier = @"SimpleTableItem";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:simpleTableIdentifier];
    
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:simpleTableIdentifier];
    }
    
    cell.accessoryView = [[ UIImageView alloc ] initWithImage:[UIImage imageNamed:@"checkmark.png"]];
    [cell.accessoryView setFrame:CGRectMake(0, 0, 24, 24)];
    
    const int currentIndex = static_cast<int>(RBX::GuiBuilder::getDebugDisplay());
    
    [cell.accessoryView setHidden:(currentIndex != indexPath.row)];
    [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
    [UIStyleConverter convertToLabelStyle:cell.textLabel];
    
    cell.textLabel.text = [debugDisplayTableData objectAtIndex:indexPath.row];
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    for (int section = 0; section < [tableView numberOfSections]; section++)
    {
        for (int row = 0; row < [tableView numberOfRowsInSection:section]; row++)
        {
            NSIndexPath* cellPath = [NSIndexPath indexPathForRow:row inSection:section];
            UITableViewCell* cell = [tableView cellForRowAtIndexPath:cellPath];
            [cell.accessoryView setHidden:YES];
        }
    }
    
    UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
    
    if (cell)
    {
        [cell.accessoryView setHidden:NO];
        RBX::GuiBuilder::setDebugDisplay(static_cast<RBX::GuiBuilder::Display>(indexPath.row));
        
        [RobloxGoogleAnalytics setCustomVariableWithLabel:@"StatsDisplay" withValue:[NSString stringWithFormat: @"%d", (int)indexPath.row]];
    }
    else
    {
        [RobloxGoogleAnalytics setPageViewTracking:@"Settings/StatsDisplay/InvalidCell"];
    }
}

@end
