//
//  RBModalPopUpViewController.m
//  A CLASS THAT SHOULD DISMISS ITSELF WHEN A TAP IS REGISTERED OUTSIDE ITS OWN BOUNDS
//  RobloxMobile
//
//  Created by Kyler Mulherin on 10/15/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBModalPopUpViewController.h"

@interface RBModalPopUpViewController ()
@property UITapGestureRecognizer* rbOutsideTapRecognizer;
@end

@implementation RBModalPopUpViewController
{
    bool _tapsEnabled;
    bool _addCloseButton;
    bool _applyTheme;
}

- (id) init
{
    self = [super init];
    if (self)
    {
        _tapsEnabled = YES;
        _addCloseButton = YES;
        _applyTheme = YES;
    }
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _rbOutsideTapRecognizer = [self getTapRecognizer];
    
    if (_addCloseButton)
    {
        UIButton* close = [RobloxTheme applyCloseButtonToUINavigationItem:self.navigationItem];
        [close addTarget:self action:@selector(dismissView:) forControlEvents:UIControlEventTouchUpInside];
    }
    
    if (_applyTheme)
        [RobloxTheme applyToModalPopupNavBar:self.navigationController.navigationBar];
}
- (UITapGestureRecognizer*) getTapRecognizer
{
    if (_rbOutsideTapRecognizer == nil)
    {
        _rbOutsideTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(didTapOutside:)];
        [_rbOutsideTapRecognizer setCancelsTouchesInView:NO];
        [_rbOutsideTapRecognizer setDelegate:self];
    }
    
    return _rbOutsideTapRecognizer;
}
- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self.view.window addGestureRecognizer:[self getTapRecognizer]];
}
- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [self.view.window removeGestureRecognizer:_rbOutsideTapRecognizer];
}

-(void)disableTapRecognizer
{
    _tapsEnabled = NO;
}
-(void)shouldAddCloseButton:(BOOL)shouldAdd
{
    _addCloseButton = shouldAdd;
}
-(void)shouldApplyModalTheme:(BOOL)shouldApply
{
    _applyTheme = shouldApply;
}

//Gesture Recognition
//Tap Recognition
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer { return YES; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch { return YES; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer { return YES; }
- (void) didTapOutside:(UIGestureRecognizer*)sender
{
    if (!_tapsEnabled)
        return;
        
    //check the position of the tap and dismiss the view if it lies outside the bounds of the view
    if (sender.state == UIGestureRecognizerStateEnded)
    {
        UIView* root = self.view.window.rootViewController.view;
        CGPoint location = [sender locationInView:root];
        location = [self.view convertPoint:location fromView:root];
        if (![self.view pointInside:location withEvent:nil])
        {
            [self dismissViewControllerAnimated:YES completion:nil];
        }
    }
}

//close button
- (void)dismissView:(id)sender { [self dismissViewControllerAnimated:YES completion:nil]; }
@end
