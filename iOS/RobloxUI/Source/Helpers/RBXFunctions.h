//
//  RBXFunctions.h
//  RobloxMobile
//
//  Created by Ashish Jain on 9/4/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RBXFunctions : NSObject

#define NSLOG_PRETTY_FUNCTION NSLog(@"METHOD NAME - %@", [NSString stringWithCString:__PRETTY_FUNCTION__ encoding:NSASCIIStringEncoding])

+ (BOOL) isEmpty:(id)object;
+ (BOOL) isEmptyString:(NSString *)string;

+ (void) dispatchOnMainThread:(void(^)(void))block;
+ (void) dispatchOnBackgroundThread:(void(^)(void))block;
+ (void) dispatchAfter:(NSUInteger)seconds onMainThread:(void(^)(void))block;
+ (void) dispatchAfter:(NSUInteger)seconds onBackgroundThread:(void(^)(void))block;

+ (NSString*) URLEncodeStringFromString:(NSString *)string;
+ (NSString*) stringWithPercentEscape:(NSString*)string;
#pragma mark - Memory Size functions

+ (NSInteger) sizeOfObject:(id)object;
+ (NSInteger) sizeOfDictionary:(id)object;
+ (NSInteger) sizeOfArray:(NSArray *)myArray;

@end
