//
//  NSDictionary+Parsing.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/28/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSDictionary (Parsing)

- (BOOL) boolValueForKey:(NSString*)key;
- (BOOL) boolValueForKey:(NSString*)key withDefault:(BOOL)defaultValue;

-(NSInteger) intValueForKey:(NSString*)key;
-(NSInteger) intValueForKey:(NSString*)key withDefault:(NSInteger)defaultValue;

-(NSUInteger) unsignedValueForKey:(NSString*)key;
-(NSUInteger) unsignedValueForKey:(NSString*)key withDefault:(NSUInteger)defaultValue;

-(double) doubleValueForKey:(NSString*)key;
-(double) doubleValueForKey:(NSString*)key withDefault:(double)defaultValue;

-(NSNumber*) numberForKey:(NSString*)key;
-(NSNumber*) numberForKey:(NSString*)key withDefault:(NSNumber*)defaultValue;

-(NSString*) stringForKey:(NSString*)key;
-(NSString*) stringForKey:(NSString*)key withDefault:(NSString*)defaultValue;

-(NSArray*) arrayForKey:(NSString*)key;
-(NSArray*) arrayForKey:(NSString*)key withDefault:(NSArray*)defaultValue;

-(NSDictionary*) dictionaryForKey:(NSString*)key;
-(NSDictionary*) dictionaryForKey:(NSString*)key withDefault:(NSDictionary*)defaultValue;

@end
