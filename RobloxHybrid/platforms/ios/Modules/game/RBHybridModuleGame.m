//
//  RBHybridModuleGame.mm
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleGame.h"
#import "RBGameViewController.h"
#import "NSDictionary+Parsing.h"

#define MODULE_ID @"Game"

@implementation RBHybridModuleGame
{
    RBHybridCommand* _command;
}

-(instancetype) init
{
    self = [super initWithModuleID:MODULE_ID];
    if(self)
    {
        [self registerFunction:@"startWithPlaceID" withSelector:@selector(startGameWithPlaceID:)];
    }
    return self;
}

/**
 * Start a game session
 *
 * @param {string} placeID - ID of the place to start
 * @param {Function} callback
 * @instance
 */
-(void)startGameWithPlaceID:(RBHybridCommand*)command
{
    _command = command;
    
    NSString* placeID = [command.params stringForKey:@"placeID" withDefault:nil];
    if(placeID != nil)
    {
        RBGameViewController* gameController = [[RBGameViewController alloc] initWithLaunchParams:[RBXGameLaunchParams InitParamsForJoinPlace:placeID.integerValue]];
        gameController.completionHandler = ^
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                if(_command)
                {
                    [_command executeCallback:YES];
                }
            });
        };
        
        [command.originWebView.controller presentViewController:gameController animated:NO completion:nil];
    }
    else
    {
        [command executeCallback:NO];
    }
}

@end
