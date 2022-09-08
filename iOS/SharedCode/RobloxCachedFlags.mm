//
//  RobloxCachedFlags.mm
//  RobloxMobile
//
//  Created by Martin Robaszewski on 6/20/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "RobloxCachedFlags.h"

@implementation RobloxCachedFlags

+ (id)sharedInstance
{
    static dispatch_once_t rbxCachedFlagsPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&rbxCachedFlagsPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

-(id) init
{
    if(self = [super init])
        [self sync];
    
    return self;
}

-(void) sync
{
    [[NSUserDefaults standardUserDefaults] synchronize]; 
}


-(BOOL) getBool:(NSString *)key withValue:(BOOL*)val
{
    NSObject *testObject = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    
    if (testObject)
    {
        *val = [[NSUserDefaults standardUserDefaults] boolForKey:key];
        return YES;
    }
    return NO;
}

-(BOOL) getInt:(NSString *)key withValue:(NSInteger*)val
{
    NSObject *testObject = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    
    if (testObject)
    {
        *val = [[NSUserDefaults standardUserDefaults] integerForKey:key];
        return YES;
    }
    return NO;
}


-(BOOL) getString:(NSString *)key withValue:(NSString *)val
{
    
    NSObject *testObject = [[NSUserDefaults standardUserDefaults] objectForKey:key];
    
    if (testObject)
    {
        val = [[NSUserDefaults standardUserDefaults] stringForKey:key];
        return YES;
    }
    
    return NO;
    
}

- (void) setBool:(NSString*) key withValue:(BOOL)val
{
    [[NSUserDefaults standardUserDefaults] setBool:val forKey:key];
    [self sync];
}

- (void) setInt:(NSString*) key withValue:(NSInteger)val
{
    [[NSUserDefaults standardUserDefaults] setInteger:val forKey:key];
    [self sync];
}

- (void) setString:(NSString *)key withValue:(NSString *)val
{
    [[NSUserDefaults standardUserDefaults] setObject:val forKey:key];
    [self sync];
}

@end
