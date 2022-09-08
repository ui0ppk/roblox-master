//
//  UIStyleConverter.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 8/13/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "UIStyleConverter.h"

@implementation UIStyleConverter

+(void) convertToLoadingStyle:(UIImageView*) imageView
{
    [imageView setImage:[UIImage animatedImageNamed:@"loading-" duration:0.6f]];
}

+(void) convertToTitleStyle:(UILabel*) label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:24.0f];
    label.textColor = [UIColor colorWithRed:0.5 green:0.5 blue:0.5 alpha:1];
}

+(void) convertToBlueTitleStyle:(UILabel*) label
{
    label.font = [UIFont fontWithName:@"SourceSansPro" size:18.0f];
    label.textColor = [UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1];
}

+(void) convertUIButtonToLabelStyle:(UIButton*) button
{
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18.0f]];
    
    [button setTitleColor:[UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1] forState:UIControlStateHighlighted];
    [button setTitleColor:[UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1] forState:UIControlStateSelected];
    
    button.layer.backgroundColor = [[UIColor whiteColor] CGColor];
}

+(void) convertToHyperlinkStyle:(UIButton*) button
{
    button.layer.backgroundColor = [[UIColor clearColor] CGColor];
    
    [button setTitleColor:[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] forState:UIControlStateHighlighted];
    [button setTitleColor:[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] forState:UIControlStateSelected];
    
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:14.0f]];
}

+(void) convertToLabelStyle:(UILabel*) label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:18.0f];
    label.textColor = [UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1];
}

+(void) convertToLargeLabelStyle:(UILabel*) label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Light" size:24.0f];
    label.textColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1];
}

+(void) convertToButtonStyle:(UIButton*) button
{
    button.titleLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:18.0f];
    
    button.showsTouchWhenHighlighted = YES;
    button.adjustsImageWhenHighlighted = NO;
    button.adjustsImageWhenDisabled = YES;
    
    [button setTitleColor:[UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1] forState:UIControlStateHighlighted];
    [button setTitleColor:[UIColor colorWithRed:0.25 green:0.25 blue:0.25 alpha:1] forState:UIControlStateSelected];
    
    button.layer.borderWidth = 1.0f;
    button.layer.borderColor = [[UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:0] CGColor];
    button.layer.cornerRadius = 4.0f;
    button.layer.backgroundColor = [[UIColor whiteColor] CGColor];
    button.layer.shadowOpacity = 0.2f;
    button.layer.shadowOffset = CGSizeMake(0, 1);
    button.layer.shadowRadius = 2.0f;
}

+(void) convertToBorderlessButtonStyle:(UIButton*) button
{
    button.titleLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14.0f];
    [button setTitleColor:[UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1] forState:UIControlStateHighlighted];
    [button setTitleColor:[UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1] forState:UIControlStateSelected];
}

+(void) convertToButtonBlueStyle:(UIButton*) button
{
    button.showsTouchWhenHighlighted = YES;
    button.adjustsImageWhenHighlighted = NO;
    button.adjustsImageWhenDisabled = YES;
    
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
    
    [button setTitleColor:[UIColor colorWithRed:169.0/255.0 green:169.0/255.0 blue:169.0/255.0 alpha:1] forState:UIControlStateDisabled];
    
    button.titleLabel.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18.0f];
    button.layer.borderWidth = 1.0f;
    button.layer.borderColor = [[UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:0] CGColor];
    button.layer.cornerRadius = 4.0f;
    button.layer.backgroundColor = [[UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1] CGColor];
    button.layer.shadowOpacity = 0.2f;
    button.layer.shadowOffset = CGSizeMake(0, 1);
    button.layer.shadowRadius = 2.0f;
}

+(void) convertToBlueNavigationBarStyle:(UINavigationBar*) bar
{
    NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[UIFont
                                                                           fontWithName:@"SourceSansPro-Semibold" size:18.0], NSFontAttributeName,
                                [UIColor whiteColor], NSForegroundColorAttributeName,
                                nil];
    
    [bar setTitleTextAttributes:attributes];
    
    NSDictionary *textAttributes = @{ NSForegroundColorAttributeName       : [UIColor whiteColor],
                                      NSFontAttributeName            : [UIFont fontWithName:@"SourceSansPro-Regular" size:16.0]
                                      };
    
    [[UIBarButtonItem appearanceWhenContainedIn: [UINavigationController class],nil]
     setTitleTextAttributes:textAttributes
     forState:UIControlStateNormal];
    
    [bar setTintColor:[UIColor whiteColor]];
    [bar setBarTintColor:[UIColor colorWithRed:37.0/255.0f green:80.0/255.0 blue:148.0/255.0 alpha:1]];
}

+(void) convertToNavigationBarStyle
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
    {
        NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[UIFont
                                                                               fontWithName:@"SourceSansPro-Semibold" size:18.0], NSFontAttributeName,
                                    [UIColor whiteColor], NSForegroundColorAttributeName,
                                    nil];
        
        [[UINavigationBar appearance] setTitleTextAttributes:attributes];
        
        
        // your bar button text attributes dictionary
        NSDictionary *textAttributes = @{ NSForegroundColorAttributeName       : [UIColor whiteColor],
                                          NSFontAttributeName            : [UIFont fontWithName:@"SourceSansPro-Regular" size:16.0]
                                          };
        
        [[ UIBarButtonItem appearanceWhenContainedIn: [UINavigationController class],nil]
         setTitleTextAttributes:textAttributes
         forState:UIControlStateNormal];
        
        [[UINavigationBar appearance] setTintColor:[UIColor whiteColor]];
        [[UINavigationBar appearance] setBarTintColor:[UIColor colorWithRed:37.0/255.0f green:80.0/255.0 blue:148.0/255.0 alpha:1]];
    }
    else
    {
        NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[UIFont
                                                                               fontWithName:@"SourceSansPro-Regular" size:18.0], NSFontAttributeName,
                                    [UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1], NSForegroundColorAttributeName,
                                    nil];
        
        [[UINavigationBar appearance] setTitleTextAttributes:attributes];
        
        
        // your bar button text attributes dictionary
        NSDictionary *textAttributes = @{ NSForegroundColorAttributeName       : [UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:1480.0/255.0 alpha:1],
                                          NSFontAttributeName            : [UIFont fontWithName:@"SourceSansPro-Regular" size:16.0]
                                          };
        
        [[ UIBarButtonItem appearanceWhenContainedIn: [UINavigationController class],nil]
         setTitleTextAttributes:textAttributes
         forState:UIControlStateNormal];
        
        [[UINavigationBar appearance] setTintColor:[UIColor colorWithRed:37.0/255.0 green:80.0/255.0 blue:148.0/255.0 alpha:1]];
        [[UINavigationBar appearance] setBarTintColor:[UIColor whiteColor]];
    }
}

+(void) convertToBoldTextFieldStyle:(UITextField*) textField
{
    textField.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18.0f];
    [textField setTextColor:[UIColor colorWithRed:50.0/255.0 green:50.0/255.0 blue:50.0/255.0 alpha:1]];
}

+(void) convertToTextFieldStyle:(UITextField*) textField
{
    // store original text to restore after we set attributes
    NSString* origText = textField.text;
    
    textField.attributedText =
    [[NSAttributedString alloc] initWithString:@""
                                    attributes:@{
                                                 NSForegroundColorAttributeName: [UIColor colorWithRed:169.0/255.0 green:169.0/255.0 blue:169.0/255.0 alpha:1],
                                                 NSFontAttributeName : [UIFont fontWithName:@"SourceSansPro-Regular" size:18.0f]
                                                 }];
    
    [textField setTintColor:[UIColor colorWithRed:37.0/255.0f green:80.0/255.0 blue:148.0/255.0 alpha:1]];
    
    if (origText.length > 0)
    {
        textField.text = origText;
    }
}

+(void) convertToPagingStyle
{
    UIPageControl *pageControl = [UIPageControl appearance];
    pageControl.pageIndicatorTintColor = [UIColor lightGrayColor];
    pageControl.currentPageIndicatorTintColor = [UIColor blackColor];
    pageControl.backgroundColor = [UIColor whiteColor];
}

+(void) convertToTexturedBackgroundStyle:(UIView*) view
{
    [view setBackgroundColor:[UIColor colorWithPatternImage:[UIImage imageNamed:@"bg_pattern"]]];
}

@end
