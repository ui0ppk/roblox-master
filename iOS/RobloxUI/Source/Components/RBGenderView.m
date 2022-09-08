//
//  RBGenderView.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 7/13/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBGenderView.h"
#import "RobloxTheme.h"
#import "UIView+Position.h"

@implementation RBGenderView
{
    UIButton* _btnMale;
    UIButton* _btnFemale;
    UILabel*  _lblGender;
    void (^touchBlock)();
}

//Constructors and Initialization stuff
-(void) initialize
{
    _playerGender = GENDER_DEFAULT;
    
    [self.layer setBorderWidth:1];
    [self.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [self.layer setCornerRadius:5.0];
    [self setClipsToBounds:YES];
    
    _btnMale = [[UIButton alloc] init];
    [_btnMale addTarget:self action:@selector(didPressButtonMale:) forControlEvents:UIControlEventTouchUpInside];
    [_btnMale setImage:[UIImage imageNamed:@"Gender Male Selected"] forState:UIControlStateSelected];
    [_btnMale setImage:[UIImage imageNamed:@"Gender Male Unselected"] forState:UIControlStateNormal];
    [_btnMale.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
    [_btnMale setTitleColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f] forState:UIControlStateNormal];
    [_btnMale setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    [_btnMale setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
    [_btnMale setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected | UIControlStateHighlighted];
    [_btnMale.layer setBorderWidth:1];
    [_btnMale.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    
    _btnFemale = [[UIButton alloc] init];
    [_btnFemale addTarget:self action:@selector(didPressButtonFemale:) forControlEvents:UIControlEventTouchUpInside];
    [_btnFemale setImage:[UIImage imageNamed:@"Gender Female Selected"] forState:UIControlStateSelected];
    [_btnFemale setImage:[UIImage imageNamed:@"Gender Female Unselected"] forState:UIControlStateNormal];
    [_btnFemale setTitleColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f] forState:UIControlStateNormal];
    [_btnFemale setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    [_btnFemale setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
    [_btnFemale setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected | UIControlStateHighlighted];
    //[_btnFemale.layer setBorderWidth:1];
    //[_btnFemale.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    
    _lblGender = [[UILabel alloc] init];
    [_lblGender setText:NSLocalizedString(@"GenderWord", nil)];
    //[RobloxTheme applyToModalLoginTitleLabel:_lblGender];
    [_lblGender setFont:[RobloxTheme fontBodyBold]];
    [_lblGender setTextColor:[RobloxTheme colorGray1]];
    
    //[_lblGender setTextColor:[RobloxTheme colorGray1]];
    
    //add the objects to the view
    [self addSubview:_lblGender];
    [self addSubview:_btnMale];
    [self addSubview:_btnFemale];
}
-(void) layoutSubviews
{
    //layout the view
    CGFloat cellWidth = self.width / 3;
    int margin = 12; //self.width * 0.05;
    
    [_lblGender setFrame:CGRectMake(margin, 0, (cellWidth - margin), self.height)];
    [_btnMale setFrame:CGRectMake(_lblGender.right, -1, cellWidth, self.height+2)];
    [_btnFemale setFrame:CGRectMake(_btnMale.right-1, -1, cellWidth+1, self.height+2)];
}
-(id) init
{
    self = [super init];
    if (self)
        [self initialize];
    
    return self;
}
-(id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
        [self initialize];
    
    return self;
}
-(id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
        [self initialize];
    
    return self;
}



//Mutator
-(void)setGender:(Gender)gender { _playerGender = gender; }
-(void)setTouchBlock:(void (^)())block { touchBlock = block; }

//Actions
-(void)highlightButton:(UIButton*)aButton
{
    aButton.selected = YES;
}
-(void)resetButton:(UIButton*)aButton
{
    aButton.selected = NO;
}
-(void)didPressButtonMale:(id)sender
{
    if (touchBlock)
        touchBlock();
    
    if (_playerGender == GENDER_GIRL)
        [self resetButton:_btnFemale];
    else if (_playerGender == GENDER_BOY)
    {
        [self resetButton:_btnMale];
        _playerGender = GENDER_DEFAULT;
        [self markAsNormal];
        return;
    }
    
    _playerGender = GENDER_BOY;
    [self highlightButton:_btnMale];
    [self markAsValid];
}
-(void)didPressButtonFemale:(id)sender
{
    if (touchBlock)
        touchBlock();
    
    if (_playerGender == GENDER_BOY)
        [self resetButton:_btnMale];
    else if (_playerGender == GENDER_GIRL)
    {
        [self resetButton:_btnFemale];
        _playerGender = GENDER_DEFAULT;
        [self markAsNormal];
        return;
    }
    
    _playerGender = GENDER_GIRL;
    [self highlightButton:_btnFemale];
    [self markAsValid];
}


//Colors and crap
-(void) changeBorderColor:(UIColor*)color {
    dispatch_async(dispatch_get_main_queue(), ^
    {
        self.layer.borderColor=[color CGColor];
        _btnMale.layer.borderColor = [color CGColor];
        //_btnFemale.layer.borderColor = [color CGColor];
    });
}
-(void) markAsValid     { [self changeBorderColor:[RobloxTheme colorGreen1]]; /*[_lblGender setTextColor:[RobloxTheme colorGray1]];*/ }
-(void) markAsNormal    { [self changeBorderColor:[RobloxTheme colorGray4]];  /*[_lblGender setTextColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f]];*/ }

@end
