//
//  NSDictionary+Parsing.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/28/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "NSDictionary+Parsing.h"

@implementation NSDictionary (Parsing)

- (BOOL) boolValueForKey:(NSString*)key
{
    return [self boolValueForKey:key withDefault:NO];
}

- (BOOL) boolValueForKey:(NSString*)key withDefault:(BOOL)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSNumber class]])
        return [object boolValue];
    else
        return defaultValue;
}

-(NSInteger) intValueForKey:(NSString*)key
{
    return [self intValueForKey:key withDefault:0];
}

-(NSInteger) intValueForKey:(NSString*)key withDefault:(NSInteger)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSNumber class]])
        return [object integerValue];
    else
        return defaultValue;
}

-(NSUInteger) unsignedValueForKey:(NSString*)key
{
    return [self unsignedValueForKey:key withDefault:0];
}

-(NSUInteger) unsignedValueForKey:(NSString*)key withDefault:(NSUInteger)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSNumber class]])
        return [object unsignedIntegerValue];
    else
        return defaultValue;
}

-(double) doubleValueForKey:(NSString*)key
{
    return [self doubleValueForKey:key withDefault:0.0];
}

-(double) doubleValueForKey:(NSString*)key withDefault:(double)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSNumber class]])
        return [object doubleValue];
    else
        return defaultValue;
}

-(NSNumber*) numberForKey:(NSString*)key
{
    return [self numberForKey:key withDefault:[NSNumber numberWithInt:0]];
}

-(NSNumber*) numberForKey:(NSString*)key withDefault:(NSNumber*)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSNumber class]])
        return object;
    else
        return defaultValue;
}

-(NSString*) stringForKey:(NSString*)key
{
    return [self stringForKey:key withDefault:@""];
}

-(NSString*) stringForKey:(NSString*)key withDefault:(NSString*)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil)
    {
        if( [object isKindOfClass:[NSString class]] )
            return object;
        else if( [object isKindOfClass:[NSNumber class]] )
            return [object stringValue];
        else
            return defaultValue;
    }
    else
        return defaultValue;
}

-(NSArray*) arrayForKey:(NSString*)key
{
    return [self arrayForKey:key withDefault:[NSArray array]];
}

-(NSArray*) arrayForKey:(NSString*)key withDefault:(NSArray*)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSArray class]])
        return object;
    else
        return defaultValue;
}

-(NSDictionary*) dictionaryForKey:(NSString*)key
{
    return [self dictionaryForKey:key withDefault:[NSDictionary dictionary]];
}

-(NSDictionary*) dictionaryForKey:(NSString*)key withDefault:(NSDictionary*)defaultValue
{
    id object = [self objectForKey:key];
    if(object != nil && [object isKindOfClass:[NSDictionary class]])
        return object;
    else
        return defaultValue;
}

@end
