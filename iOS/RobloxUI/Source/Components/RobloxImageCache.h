//
//  RobloxCache.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 8/29/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface RobloxImageCache : NSObject

+ (RobloxImageCache*) sharedInstance;

- (UIImage*) getImage:(NSString*)key size:(CGSize)size;
- (void) setImage:(UIImage*)image key:(NSString*)key size:(CGSize)size;

@end
