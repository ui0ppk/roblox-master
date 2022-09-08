//
//  RBMainNavigationController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/25/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBMainNavigationController.h"
#import "PlaceLauncher.h"
#import "RobloxNotifications.h"

@interface RBMainNavigationController ()

@end

@implementation RBMainNavigationController
{
    UIInterfaceOrientationMask phoneMask;
}


-(id) initWithCoder:(nonnull NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        phoneMask = UIInterfaceOrientationMaskPortrait;
        
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(updateRotatePermission:)
                                                         name:RBX_NOTIFY_GAME_FINISHED_LOADING
                                                       object:nil];
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(updateRotatePermission:)
                                                         name:RBX_NOTIFY_GAME_DID_LEAVE
                                                       object:nil];
        }
    }
    return self;
}
-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(void) updateRotatePermission:(NSNotification*)notification
{
    //in iOS 9, the app does not rotate the keyboard properly when getting into game
    //so when leaving game, we need to reset to the app defaults
    phoneMask = ([notification.name isEqualToString:RBX_NOTIFY_GAME_FINISHED_LOADING]) ? UIInterfaceOrientationMaskLandscape : UIInterfaceOrientationMaskPortrait;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [UIViewController attemptRotationToDeviceOrientation];
    });
    
}

-(BOOL)shouldAutorotate
{
    return YES;
}

#ifdef __IPHONE_9_0
-(UIInterfaceOrientationMask) supportedInterfaceOrientations
#else
-(NSUInteger)supportedInterfaceOrientations
#endif
{
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) ? phoneMask
                                                                    : UIInterfaceOrientationMaskLandscape;
}


@end
