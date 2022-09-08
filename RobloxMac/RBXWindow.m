//
//  RBXWindow.m
//  MacClient
//
//  Created by Ben Tkacheff on 9/30/13.
//
//

#import "RBXWindow.h"

@implementation RBXWindow

// necessary override for when game window becomes borderless (otherwise it is never key)
- (BOOL)canBecomeKeyWindow
{
    return YES;
}



@end
