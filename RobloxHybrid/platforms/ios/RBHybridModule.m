//
//  RBHybridModule.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 2/5/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBHybridModule.h"
#import "RBHybridBridge.h"
#import "NSDictionary+Parsing.h"

@implementation RBHybridModule
{
    NSMutableDictionary* _functions;
}

-(instancetype)initWithModuleID:(NSString*)moduleID
{
    self = [super init];
    if(self)
    {
        self.moduleID = moduleID;
        
        _functions = [[NSMutableDictionary alloc] init];
    }
    return self;
}

-(void)registerFunction:(NSString*)functionName withSelector:(SEL)selector
{
    [_functions setObject:[NSValue valueWithPointer:selector] forKey:functionName];
}

-(void)executeCommand:(RBHybridCommand*)command
{
    if([command.functionName isEqualToString:@"supports"])
    {
        NSString* functionName = [command.params stringForKey:@"functionName" withDefault:@"nil"];
        BOOL functionExists = functionName != nil && [_functions objectForKey:functionName] != nil;
        [command executeCallback:functionExists];
    }
    else
    {
        NSValue* selectorValue = [_functions objectForKey:command.functionName];
        if(selectorValue == nil)
        {
            NSLog(@"ERROR - Function %@ not available in module of type %@", command.functionName, command.moduleID);
            return;
        }
        
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [self performSelector:(SEL)[selectorValue pointerValue] withObject:command];
        #pragma clang diagnostic pop
    }
}

@end
