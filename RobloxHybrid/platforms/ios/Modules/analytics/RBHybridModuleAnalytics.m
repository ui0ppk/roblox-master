//
//  RBHybridModuleDialogs.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleAnalytics.h"
#import "RBHybridCommand.h"
#import "NSDictionary+Parsing.h"
#import "Flurry.h"

#define MODULE_ID @"Analytics"

@implementation RBHybridModuleAnalytics

-(instancetype) init
{
    self = [super initWithModuleID:MODULE_ID];
    if(self)
    {
        [self registerFunction:@"trackEvent" withSelector:@selector(trackEvent:)];
    }
    return self;
}

/**
 * Present a single button dialog
 *
 * @param {string} eventName - Event name
 * @param {Object} params - Dictionary with the event parameters. Only the first level will be saved. (i.e. no nested objects).
 *
 */
-(void)trackEvent:(RBHybridCommand*)command
{
    NSString* event = [command.params stringForKey:@"eventName" withDefault:@"Roblox"];
    NSDictionary* params = [command.params dictionaryForKey:@"params" withDefault:@{}];
    
    [Flurry logEvent:event withParameters:params];
}

@end
