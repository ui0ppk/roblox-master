//
//  RBXFunctions.m
//  RobloxMobile
//
//  Created by Ashish Jain on 9/4/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBXFunctions.h"
#import <malloc/malloc.h>

@implementation RBXFunctions

+ (BOOL) isEmpty:(id)object {
    if (nil == object) {
        return YES;
    }
    
    if ([object isKindOfClass:[NSNull class]]) {
        return YES;
    }
    
    if ([object isKindOfClass:[NSArray class]]) {
        NSArray *array = (NSArray *)object;
        if (array.count == 0) {
            return YES;
        }
    }
    
    if ([object isKindOfClass:[NSDictionary class]]) {
        NSDictionary *dict = (NSDictionary *)object;
        if (dict.allKeys.count == 0) {
            return YES;
        }
    }
    
    if ([object isKindOfClass:[NSData class]]) {
        NSData *data = (NSData *)object;
        if (data.length == 0) {
            return YES;
        }
    }
    
    return NO;
}

+ (BOOL) isEmptyString:(NSString *)string {
    if (nil == string) {
        return YES;
    }
    
    if (string.length == 0) {
        return YES;
    }
    
    return NO;
}

+ (void) dispatchOnMainThread:(void(^)(void))block {
    if (nil != block) {
        dispatch_async(dispatch_get_main_queue(), block);
    }
}

+ (void) dispatchOnBackgroundThread:(void(^)(void))block {
    if (nil != block) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), block);
    }
}

+ (void) dispatchAfter:(NSUInteger)seconds onMainThread:(void(^)(void))block {
    if (nil != block) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(seconds * NSEC_PER_SEC)), dispatch_get_main_queue(), block);
    }
}

+ (void) dispatchAfter:(NSUInteger)seconds onBackgroundThread:(void(^)(void))block {
    if (nil != block) {
        dispatch_after((int64_t)(seconds * NSEC_PER_SEC), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), block);
    }
}

+ (NSString*) URLEncodeStringFromString:(NSString *)string
{
    return [string stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet characterSetWithCharactersInString:@"!@#$%&*()+'\";:=,/?[] "]];
}

+ (NSString*)stringWithPercentEscape:(NSString*)string
{
    return [string stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet characterSetWithCharactersInString:@"=,!$&'()*+;@?\n\"<>#\t :/"]];
}



#pragma mark - Memory Size functions

+ (NSInteger) sizeOfObject:(id)object
{
    if ([object isKindOfClass:[NSArray class]]) {
        return [self sizeOfArray:object];
    } else if ([object isKindOfClass:[NSDictionary class]]) {
        return [self sizeOfDictionary:object];
    }
    
    id obj = nil;
    int totalSize = 0;
    totalSize += malloc_size((__bridge const void *)(obj));
    
    return totalSize;
}

+ (NSInteger) sizeOfDictionary:(id)object
{
    if ([object isKindOfClass:[NSDictionary class]]) {
        int totalSize = 0;
        
        NSDictionary *myDict = (NSDictionary *)object;
        for(NSString *key in myDict.allKeys)
        {
            if ([myDict objectForKey:key]) {
                id obj = nil;
                obj = [myDict objectForKey:key];
                totalSize += malloc_size((__bridge const void *)(obj));
            }
        }
        
        return totalSize;
    }

    return [self sizeOfObject:object];
}

+ (NSInteger) sizeOfArray:(NSArray *)myArray
{
    id obj = nil;
    int totalSize = 0;
    for(obj in myArray)
    {
        totalSize += malloc_size((__bridge const void *)(obj));
    }
    
    return totalSize;
}

@end
