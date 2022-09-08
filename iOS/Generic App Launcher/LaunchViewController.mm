//
//  LoadingGameController.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 11/19/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "LaunchViewController.h"
#import "PlaceLauncher.h"
#import "ObjectiveCUtilities.h"
#include "v8datamodel/DataModel.h"

@interface LaunchViewController ()

@end

@implementation LaunchViewController

@synthesize loadingImageView;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
    }
    return self;
}

-(id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSString* loadingImage = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"RbxLoadingImage"];
    [loadingImageView setImage:[UIImage imageNamed:loadingImage]];
}

-(void) gameLoaded
{
    [self dismissViewControllerAnimated:NO completion:^{
        [[PlaceLauncher sharedInstance] presentGameViewController];
    }];
}

-(void) startGame
{
    NSNumber *placeId = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"RbxPlaceId"];
    
    [[PlaceLauncher sharedInstance] startGame:[placeId intValue] controller:self request:JOIN_GAME_REQUEST_PLACEID presentGameAutomatically:YES];
    
    shared_ptr<RBX::Game> currentGame = [[PlaceLauncher sharedInstance] getCurrentGame];
    
    if(currentGame->getDataModel()->getIsGameLoaded())
        [self gameLoaded];
    else
        currentGame->getDataModel()->gameLoadedSignal.connect(boostFuncFromSelector(@selector(gameLoaded),self));
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self startGame];
    });
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
@end
