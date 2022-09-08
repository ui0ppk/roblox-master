//
//  RBRoundBorder.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBRoundedeBorder.h"
#import "UIView+Position.h"

@implementation RBRoundedeBorder
{
    UIImageView* _left;
    UIImageView* _right;
    UIImageView* _top;
    UIImageView* _bottom;
    UIImageView* _topLeft;
    UIImageView* _topRight;
    UIImageView* _bottomLeft;
    UIImageView* _bottomRight;
}

- (instancetype)init
{
    self = [super init];
    if(self)
    {
        _left = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-left-middle"]];
        _right = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-right-middle"]];
        _top = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-top-middle"]];
        _bottom = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-bottom-middle"]];
        _topLeft = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-top-left"]];
        _topRight = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-top-right"]];
        _bottomLeft = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-bottom-left"]];
        _bottomRight = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rounded-border-bottom-right"]];
        
        [self addSubview:_left];
        [self addSubview:_right];
        [self addSubview:_top];
        [self addSubview:_bottom];
        [self addSubview:_topLeft];
        [self addSubview:_topRight];
        [self addSubview:_bottomLeft];
        [self addSubview:_bottomRight];
    }
    return self;
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    if(self.superview != nil)
    {
        CGRect bounds = self.superview.bounds;
        
        CGFloat borderWidth = _left.frame.size.width;
        CGFloat borderOffset = borderWidth * 0.5;
        bounds = CGRectInset(bounds, -borderOffset, -borderOffset);
        
        // Set corners
        _topLeft.x = bounds.origin.x;
        _topLeft.y = bounds.origin.y;
        
        _bottomLeft.x = bounds.origin.x;
        _bottomLeft.bottom = CGRectGetMaxY(bounds);
        
        _topRight.right = CGRectGetMaxX(bounds);
        _topRight.y = bounds.origin.y;
        
        _bottomRight.right = CGRectGetMaxX(bounds);
        _bottomRight.bottom = CGRectGetMaxY(bounds);
        
        // Set borders
        _left.x = _topLeft.x;
        _left.y = _topLeft.bottom;
        _left.height = _bottomLeft.y - _topLeft.bottom;
        
        _top.x = _topLeft.right;
        _top.y = _topLeft.y;
        _top.width = _topRight.x - _topLeft.right;
        
        _right.x = _topRight.x;
        _right.y = _topRight.bottom;
        _right.height = _bottomRight.y - _topRight.bottom;
        
        _bottom.x = _bottomLeft.right;
        _bottom.y = _bottomLeft.y;
        _bottom.width = _bottomRight.x - _bottomLeft.right;
    }
}

@end
