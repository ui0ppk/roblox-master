//
//  UIScrollView+Auto.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/17/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "UIScrollView+Auto.h"

@implementation UIScrollView (Auto)

- (void) setContentSizeForDirection:(UIScrollViewDirection)direction
{
    CGRect contentRect = CGRectZero;
    for(UIView* view in self.subviews)
    {
        if(view.hidden == NO)
        {
            contentRect = CGRectUnion(contentRect, view.frame);
        }
    }
    
    if((direction & UIScrollViewDirectionHorizontal) == 0)
        contentRect.size.width = self.frame.size.width;
    
    if((direction & UIScrollViewDirectionVertical) == 0)
        contentRect.size.height = self.frame.size.height;
    
    self.contentSize = contentRect.size;
}

@end
