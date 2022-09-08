//
//  MoreTileButton.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 12/9/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "MoreTileButton.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"

#define IPHONE_ICON_SIZE CGRectMake(0,0,28,28)
#define IPAD_ICON_SIZE CGRectMake(0,0,40,40)
#define LABEL_HEIGHT 30
#define DEFAULT_IMAGE_NAME @"Icon Home Off"
#define DEFAULT_LABEL_TEXT @"_BUTTON_TEXT_"

@implementation MoreTileButton
{
    UIImageView* imgIcon;
    UILabel* lblTitle;
    UILabel* lblBadge;
}


-(id) initWithFrame:(CGRect)frame
{
    //this gets called by Interface Builder, and in code if you're stupid
    self = [super initWithFrame:frame];
    if (self)
    {
        [self initDefaults];
    }
    return self;
}
-(id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        [self initDefaults];
    }
    return self;
}


-(void) initDefaults
{
    self.enabled = YES;
    
    //initialize the elements
    imgIcon = [[UIImageView alloc] initWithFrame:([RobloxInfo thisDeviceIsATablet] ? IPAD_ICON_SIZE : IPHONE_ICON_SIZE)];
    [imgIcon setTintColor:[UIColor blackColor]];
    [imgIcon setHighlighted:YES];
    [imgIcon setContentMode:UIViewContentModeScaleAspectFit];
    [self addSubview:imgIcon];
    
    lblTitle = [[UILabel alloc] initWithFrame:CGRectMake(0,0,self.frame.size.width, LABEL_HEIGHT)];
    [lblTitle setTextAlignment:NSTextAlignmentCenter];
    [RobloxTheme applyToGamePreviewDetailText:lblTitle];
    [lblTitle setTextColor:[UIColor blackColor]];
    [self addSubview:lblTitle];
    
    [self setBackgroundColor:[UIColor whiteColor]];
    
    self.clipsToBounds = NO;
    self.layer.shadowColor = [[UIColor blackColor] CGColor];
    self.layer.shadowOffset = CGSizeMake(0,5);
    self.layer.shadowOpacity = 0.5;
    
    [self addTarget:self action:@selector(setToDownState) forControlEvents:UIControlEventTouchDown];
    [self addTarget:self action:@selector(setToUpState)   forControlEvents:UIControlEventTouchUpInside];
    [self addTarget:self action:@selector(setToUpState)   forControlEvents:UIControlEventTouchUpOutside];
    
    lblBadge = [[UILabel alloc]initWithFrame:CGRectMake(23,0, 13, 13)];
    lblBadge.textColor = [UIColor whiteColor];
    lblBadge.textAlignment = NSTextAlignmentCenter;
    lblBadge.layer.borderWidth = 1;
    lblBadge.layer.cornerRadius = 8;
    lblBadge.layer.masksToBounds = YES;
    lblBadge.layer.borderColor =[[UIColor clearColor] CGColor];
    lblBadge.layer.shadowColor = [[UIColor clearColor] CGColor];
    lblBadge.layer.shadowOffset = CGSizeMake(0.0, 0.0);
    lblBadge.layer.shadowOpacity = 0.0;
    lblBadge.backgroundColor = [UIColor redColor];
    lblBadge.font = [UIFont fontWithName:@"ArialMT" size:11];
}
-(void) layoutSubviews
{
    [super layoutSubviews];
    
    [imgIcon centerInFrame:CGRectMake(0, 0, self.width, self.height)];
    
    switch (_layoutStyle)
    {
        case 1:
        {
            //square, main buttons
            [imgIcon setY:imgIcon.y - (LABEL_HEIGHT * 0.5)];
            [lblTitle setY:imgIcon.bottom];
            [lblTitle setWidth:self.width];
            [lblBadge setX:(imgIcon.x + imgIcon.width * 0.8)];
            [lblBadge setY:(imgIcon.y - lblBadge.height * 0.5)];
        } break;
            
        case 2:
        {
            //long, horizontal buttons
            [imgIcon setX:20];
            [lblTitle centerInFrame:CGRectMake(0, 0, self.width, self.height)];
            [lblBadge setX:(imgIcon.x + imgIcon.width * 0.8)];
            [lblBadge setY:(imgIcon.y - lblBadge.height * 0.5)];
        } break;
            
        case 3:
        {
            //scale the event icon image to fit the button
            CGSize imgSize = CGSizeMake(117, 30);
            float wideTimes = floorf(self.width / imgSize.width);
            float tallTimes = floorf(self.height / imgSize.height);
            CGSize iconSize = (wideTimes > tallTimes) ? CGSizeMake(imgSize.width * tallTimes, imgSize.height * tallTimes)
                                                      : CGSizeMake(imgSize.width * wideTimes, imgSize.height * wideTimes);
            
            [imgIcon setFrame:CGRectMake(0, 0, iconSize.width, iconSize.height)];
            [imgIcon centerInFrame:self.bounds];
            lblTitle.hidden = YES;
            
            self.layer.shadowOpacity = 0.25;

        } break;
    }
}

-(void) prepareForInterfaceBuilder
{
    [self.layer setBorderColor:[UIColor blackColor].CGColor];
    [self.layer setBorderWidth:1];
}

//Mutators
-(void)setImageName:(UIImage *)imageName
{
    _imageName = imageName;
    [imgIcon setImage:_imageName];
}
-(void)setIconLabel:(NSString *)iconLabel
{
    _iconLabel = iconLabel;
    [lblTitle setText:_iconLabel];
}
-(void)setLayoutStyle:(NSInteger)layoutStyle
{
    _layoutStyle = layoutStyle;
    
    [self layoutSubviews];
}
-(void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    
    [self setLayoutStyle:_layoutStyle];
}
-(void)setIsEnabled:(BOOL)isEnabled
{
    if (isEnabled)
    {
        [self setToUpState];
        self.layer.opacity = 1.0;
    }
    else
    {
        self.backgroundColor = [UIColor lightGrayColor];
        self.layer.opacity = 0.3;
    }
    
    self.enabled = isEnabled;
    _isEnabled = isEnabled;
}

-(void)setBadgeValue:(NSString *)value
{
    if (value)
    {
        lblBadge.text = value;
        [lblBadge setWidth: (10 + [value sizeWithAttributes:@{NSFontAttributeName: lblBadge.font}].width)];
        
        //add the badge
        if (![self.subviews containsObject:lblBadge])
            [self addSubview:lblBadge];
    }
    else
    {
        if ([self.subviews containsObject:lblBadge])
            [lblBadge removeFromSuperview];
    }
}

//Button Stylings
-(void) setToDownState
{
    if (_layoutStyle == 3 || !self.enabled)
        return;
    
    self.backgroundColor = [UIColor lightGrayColor];
    [imgIcon setImage:_imageDownName];
}
-(void) setToUpState
{
    if (_layoutStyle == 3)
        return;
    
    self.backgroundColor = [UIColor whiteColor];
    [imgIcon setImage:_imageName];
}

@end
