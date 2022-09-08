//
//  RBActivityIndicatorView.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 8/26/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBActivityIndicatorView.h"
#import <UIKit/UIKit.h>

#define SPINNER_ICON_FRAME CGRectMake(0, 0, 32, 32)

@implementation RBActivityIndicatorView
{
    UIImageView* _imageView;
}

- (id)initWithFrame:(CGRect)frame
{
    if (frame.size.width == 0 && frame.size.height == 0) {
        frame = SPINNER_ICON_FRAME;
    }
    
    self = [super initWithFrame:frame];
    if (self)
    {
        [self initUI];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        [self initUI];
    }
    return self;
}

- (void) initUI
{
    self.backgroundColor = [UIColor clearColor];
    
    UIImage* loadingIcon = [UIImage imageNamed:@"Loading Icon"];
    _imageView = [[UIImageView alloc] initWithImage:loadingIcon];
    _imageView.frame = self.bounds;
    _imageView.contentMode = UIViewContentModeCenter;
    [self addSubview:_imageView];
    
    self.hidden = YES;
}

-(void) startAnimating
{
    self.hidden = NO;
    
    if(_imageView.layer.animationKeys.count == 0)
    {
        CABasicAnimation* rotationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
        rotationAnimation.toValue = [NSNumber numberWithFloat: M_PI * 2.0 /* full rotation*/];
        rotationAnimation.duration = 1.0;
        rotationAnimation.cumulative = YES;
        rotationAnimation.repeatCount = HUGE_VALF;
        rotationAnimation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
        
        [_imageView.layer addAnimation:rotationAnimation forKey:@"rotationAnimation"];
    }
}

-(void) stopAnimating
{
    self.hidden = YES;
    [_imageView.layer removeAllAnimations];
}



@end
