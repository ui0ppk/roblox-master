//
//  RobloxCache.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 8/29/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RobloxImageCache.h"
#import "RobloxNotifications.h"

@implementation RobloxImageCache
{
    NSCache* _memCache;
}

+ (RobloxImageCache*) sharedInstance
{
    static dispatch_once_t once;
    static id instance;
    dispatch_once(&once, ^
    {
        instance = [self new];
    });
    return instance;
}

- (id)init
{
    self = [super init];
    if(self)
    {
        _memCache = [[NSCache alloc] init];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(clearMemory)
                                                     name:UIApplicationDidReceiveMemoryWarningNotification
                                                   object:nil];        
    }
    return self;
}

- (UIImage*) getImage:(NSString*)key size:(CGSize)size
{
    return [_memCache objectForKey:key];
}

- (void) setImage:(UIImage*)image key:(NSString*)key size:(CGSize)size
{
    if(image == nil)
    {
        return;
    }
    
    [_memCache setObject:image forKey:key cost:(size.width * size.height)];
}

- (void) clearMemory
{
    [_memCache removeAllObjects];
}

@end
