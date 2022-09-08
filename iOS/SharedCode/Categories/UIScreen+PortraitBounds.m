//
//  UIScreen+PortraitSize.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/9/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "UIScreen+PortraitBounds.h"

@implementation UIScreen (PortraitBounds)

- (CGRect) portraitBounds
{
    // In iOS8, Apple fixed "bounds": It now returns the screen size based on the current orientation.
    // portraitBounds will return the bounds always in the fixed coordinate space
    if( [self respondsToSelector:@selector(fixedCoordinateSpace)] )
    {
        return [self.coordinateSpace convertRect:self.bounds toCoordinateSpace:self.fixedCoordinateSpace];
    }
    else
    {
        return self.bounds;
    }
}

@end
