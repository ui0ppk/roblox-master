//
//  UIImage+Utils.m
//  RobloxMobile
//
//  Created by Christian Hresko on 5/30/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "UIImage+Utils.h"

@implementation UIImage (Utils)

- (UIImage *)imageWithRoundedCornersSize:(float)cornerRadius {
    __block UIImageView *imageView;
    dispatch_sync(dispatch_get_main_queue(), ^{
        imageView = [[UIImageView alloc] initWithImage:self];
        // Begin a new image that will be the new image with the rounded corners
        // (here with the size of an UIImageView)
        UIGraphicsBeginImageContextWithOptions(imageView.bounds.size, NO, 1.0);
        
        // Add a clip before drawing anything, in the shape of an rounded rect
        [[UIBezierPath bezierPathWithRoundedRect:imageView.bounds
                                    cornerRadius:cornerRadius] addClip];
        // Draw your image
        [self drawInRect:imageView.bounds];
        
        // Get the image, here setting the UIImageView image
        imageView.image = UIGraphicsGetImageFromCurrentImageContext();
        
        // Lets forget about that we were drawing
        UIGraphicsEndImageContext();
    });
    
    return imageView.image;
}

@end