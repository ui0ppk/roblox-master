//
//  UIView+Position.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/17/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "UIView+Position.h"

@implementation UIView (Position)

- (CGPoint) position
{
    return self.frame.origin;
}

- (CGFloat) height
{
    return self.frame.size.height;
}

- (CGFloat) width
{
    return self.frame.size.width;
}

- (CGFloat) x
{
    return self.frame.origin.x;
}

- (CGFloat) y
{
    return self.frame.origin.y;
}

- (CGFloat) centerY
{
    return self.center.y;
}

- (CGFloat) centerX
{
    return self.center.x;
}

- (CGSize) size
{
    return self.frame.size;
}

- (void)setPosition:(CGPoint)newPosition
{
    CGRect frame = self.frame;
    frame.origin = newPosition;
    self.frame = frame;
}

- (void) setHeight:(CGFloat)newHeight
{
    CGRect frame = self.frame;
    frame.size.height = newHeight;
    self.frame = frame;
}

- (void) setWidth:(CGFloat)newWidth
{
    CGRect frame = self.frame;
    frame.size.width = newWidth;
    self.frame = frame;
}

- (void) setX:(CGFloat)newX
{
    CGRect frame = self.frame;
    frame.origin.x = newX;
    self.frame = frame;
}

- (void) setY:(CGFloat)newY
{
    CGRect frame = self.frame;
    frame.origin.y = newY;
    self.frame = frame;
}

- (void) setSize:(CGSize)size
{
    CGRect frame = self.frame;
    frame.size = size;
    self.frame = frame;
}

- (CGFloat) right
{
    return self.x + self.width;
}

- (void)setRight:(CGFloat)right
{
    self.x = right - self.width;
}

- (CGFloat) bottom
{
    return self.y + self.height;
}

- (void)setBottom:(CGFloat)bottom
{
    self.y = bottom - self.height;
}

- (void) centerInFrame:(CGRect)aFrame
{
    float h = [self height];
    float w = [self width];
    
    //center our frame to the provided frame
    CGRect centeredFrame = CGRectMake(CGRectGetMidX(aFrame) - (w * 0.5),
                                      CGRectGetMidY(aFrame) - (h * 0.5),
                                      w, h);
    self.frame = centeredFrame;
}
@end
