//
//  PairTutorialDataController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/15/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "PairTutorialDataController.h"
#import "RobloxGoogleAnalytics.h"

@interface PairTutorialDataController ()

@end

@implementation PairTutorialDataController

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
    
    self.pageTitles = @[NSLocalizedString(@"PairHelpStep1TitleString",nil), NSLocalizedString(@"PairHelpStep2TitleString",nil), NSLocalizedString(@"PairHelpStep3TitleString",nil)];
    self.pageInstructions = @[NSLocalizedString(@"PairHelpStep1String", nil), NSLocalizedString(@"PairHelpStep2String", nil), NSLocalizedString(@"PairHelpStep3String", nil)];
    self.pageImages = @[@"step1.png", @"step2.png", @"step3.png"];
    
    // Create page view controller
    self.pageViewController = [self.storyboard instantiateViewControllerWithIdentifier:@"PageViewController"];
    self.pageViewController.dataSource = self;
    
    PairTutorialViewController *startingViewController = [self viewControllerAtIndex:0];
    NSArray *viewControllers = @[startingViewController];
    [self.pageViewController setViewControllers:viewControllers direction:UIPageViewControllerNavigationDirectionForward animated:NO completion:nil];
    
    // Change the size of page view controller
    self.pageViewController.view.frame = CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height - 30);
    
    [self addChildViewController:_pageViewController];
    [self.view addSubview:_pageViewController.view];
    [self.pageViewController didMoveToParentViewController:self];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (PairTutorialViewController *)viewControllerAtIndex:(NSUInteger)index
{
    if (([self.pageTitles count] == 0) || (index >= [self.pageTitles count])) {
        return nil;
    }
    
    // Create a new view controller and pass suitable data.
    PairTutorialViewController *pairTutorialViewController = [self.storyboard instantiateViewControllerWithIdentifier:@"PairTutorialViewController"];
    pairTutorialViewController.imageFile = self.pageImages[index];
    pairTutorialViewController.instructionText = self.pageInstructions[index];
    pairTutorialViewController.titleText = self.pageTitles[index];
    pairTutorialViewController.pageIndex = index;
    
    [RobloxGoogleAnalytics setCustomVariableWithLabel:@"PairTutorialPageView" withValue:[NSString stringWithFormat: @"%d", (int)index]];
    
    return pairTutorialViewController;
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerBeforeViewController:(UIViewController *)viewController
{
    NSUInteger index = ((PairTutorialViewController*) viewController).pageIndex;
    
    if ((index == 0) || (index == NSNotFound)) {
        return nil;
    }
    
    index--;
    return [self viewControllerAtIndex:index];
}

- (UIViewController *)pageViewController:(UIPageViewController *)pageViewController viewControllerAfterViewController:(UIViewController *)viewController
{
    NSUInteger index = ((PairTutorialViewController*) viewController).pageIndex;
    
    if (index == NSNotFound) {
        return nil;
    }
    
    index++;
    if (index == [self.pageTitles count]) {
        return nil;
    }
    return [self viewControllerAtIndex:index];
}

- (NSInteger)presentationCountForPageViewController:(UIPageViewController *)pageViewController
{
    return [self.pageTitles count];
}

- (NSInteger)presentationIndexForPageViewController:(UIPageViewController *)pageViewController
{
    return 0;
}

@end
