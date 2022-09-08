//
//  NativeSearchNavItem.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 11/10/14.
//  Modified from code by Ariel Lichten.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"

@interface NativeSearchNavItem : UIBarButtonItem

typedef NS_ENUM(NSInteger, SearchStatus)
{
    SearchStatusOpen,
    SearchStatusClosed
};

- (id)initWithSearchType:(SearchResultType)searchType
            andContainer:(UIViewController*)target
             compactMode:(BOOL)compactMode;

@end
