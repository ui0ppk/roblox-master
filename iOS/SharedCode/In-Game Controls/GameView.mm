//
//  GameView.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 11/21/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "GameView.h"

@implementation GameView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        self.userInteractionEnabled = YES;
        
        // Initialization code
    }
    return self;
}

-(void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];
}

-(void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];
}

-(void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
}

-(void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
}

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

@end
