//
//  MainViewController.m
//  IOSClient
//
//  Created by Ben Tkacheff on 9/20/12.
//  Copyright (c) 2012 tolkring@gmail.com. All rights reserved.
//

#import "MainViewController.h"

@implementation MainViewController

+ (id)sharedInstance
{
    static dispatch_once_t mainViewPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&mainViewPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(void) switchView:(UIView*) newView
{
    self.view = newView;
}

-(void) addSubview:(UIView*) view
{
    if(self.view)
        [self.view addSubview:view];
}

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
	// Do any additional setup after loading the view.
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
}
    
-(UIWindow*) getOgreWindow
{
    return ogreWindow;
}
-(void) setOgreWindow:(UIWindow*) value
{
    ogreWindow = value;
}

-(UIView*) getOgreView
{
    return ogreView;
}
-(void) setOgreView:(UIView*) value
{
    ogreView = value;
}

-(void) setRobloxView:(RobloxView*) newRbxView
{
    rbxView = newRbxView;
}

-(RobloxView*) getRobloxView
{
    return rbxView;
}

-(GameViewController*) getOgreViewController
{
    return ogreViewController;
}
-(void) setOgreViewController:(GameViewController*) value
{
    ogreViewController = value;
}

-(void) setLastNonGameController:(UIViewController*) lastController
{
    lastNonGameController = lastController;
}
-(UIViewController*) getLastNonGameController
{
    return lastNonGameController;
}

-(void) debugPrint
{
    NSLog(@"MainViewController::debugPrint");
    NSLog(@"ogreWindow %@", ogreWindow);
    NSLog(@"ogreView %@", ogreView);
    NSLog(@"ogreViewController %@", ogreViewController);
    NSLog(@"rbxView %p", rbxView);
    NSLog(@"lastNonGameController %@", lastNonGameController);
}

@end
