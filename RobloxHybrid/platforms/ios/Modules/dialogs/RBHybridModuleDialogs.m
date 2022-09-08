//
//  RBHybridModuleDialogs.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModuleDialogs.h"
#import "RBHybridCommand.h"
#import "NSDictionary+Parsing.h"
#import "UIAlertView+Blocks.h"

#define MODULE_ID @"Dialogs"

@implementation RBHybridModuleDialogs

-(instancetype) init
{
    self = [super initWithModuleID:MODULE_ID];
    if(self)
    {
        [self registerFunction:@"alert" withSelector:@selector(alert:)];
        [self registerFunction:@"confirm" withSelector:@selector(confirm:)];
        [self registerFunction:@"prompt" withSelector:@selector(prompt:)];
    }
    return self;
}

/**
 * Present a single button dialog
 *
 * @param {string} text - Content text
 * @param {Function} callback - Callback
 * @param {string} title - Window title
 * @param {string} buttonName - Button name
 * @instance
 */
-(void)alert:(RBHybridCommand*)command
{
    NSString* text = [command.params stringForKey:@"text"];
    NSString* title = [command.params stringForKey:@"title" withDefault:@"Roblox"];
    NSString* buttonName = [command.params stringForKey:@"buttonName" withDefault:NSLocalizedString(@"OkWord", nil)];

    UIAlertView* alert = [[UIAlertView alloc]
        initWithTitle:title
        message:text
        delegate:nil
        cancelButtonTitle:buttonName
        otherButtonTitles:nil];
    
    [alert showWithCompletion:^(UIAlertView *alertView, NSInteger buttonIndex)
    {
        [command executeCallback:YES];
    }];
}

/**
 * Present a dialog with multiple options
 *
 * @param {string} text - Content text
 * @param {Function} callback - Callback
 * @param {string} title - Window title
 * @param {string[]} buttonName - Button labels
 * @instance
 */
-(void)confirm:(RBHybridCommand*)command
{
    NSString* text = [command.params stringForKey:@"text"];
    NSString* title = [command.params stringForKey:@"title" withDefault:@"Roblox"];
    NSArray* buttons = [command.params arrayForKey:@"buttonName"];

    // Validate button
    NSString* firstButton;
    if(buttons.count > 0 && [buttons[0] isKindOfClass:[NSString class]])
    {
        firstButton = buttons[0];
        
        buttons = [buttons subarrayWithRange:NSMakeRange(1, buttons.count - 1)];
    }
    else
    {
        firstButton = NSLocalizedString(@"OkWord", nil);
    }

    UIAlertView* alert = [[UIAlertView alloc]
        initWithTitle:title
        message:text
        delegate:nil
        cancelButtonTitle:firstButton
        otherButtonTitles:nil];
    
    for(NSString* buttonName in buttons)
    {
        [alert addButtonWithTitle:buttonName];
    }
    
    [alert showWithCompletion:^(UIAlertView *alertView, NSInteger buttonIndex)
    {
        [command executeCallback:YES];
    }];
}

-(void)prompt:(RBHybridCommand*)command
{
//    [command executeCallback:@[]];
}

@end
