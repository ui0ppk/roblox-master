//
//  RBValidTextField.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 7/9/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBValidTextField.h"
#import "RobloxTheme.h"
#import "UIView+Position.h"
#import <QuartzCore/QuartzCore.h>

@implementation RBValidTextField
{
    //UI elements
    UITextField* _txtInput;
    UILabel* _lblError;
    UILabel* _lblTitle;
    UILabel* _lblHelper;
    
    //private variables
    bool _isValidated;
    bool _shouldRestrictCharacters;
    void (^validationBlock)();
    void (^exitOnEnterBlock)();
    id _nextResponder;
    
    
    CGSize _helperSize;
    CGSize _inputSize;
}


//Initializization and view layout stuff
-(void) initialize  {
    validationBlock = nil;
    exitOnEnterBlock = nil;
    _isValidated = false;
    _nextResponder = nil;
    _shouldRestrictCharacters = YES;
    
    
    [self setClipsToBounds:NO];
    
    _txtInput = [[UITextField alloc] init];
    [RobloxTheme applyToModalLoginTextField:_txtInput];
    [_txtInput.layer setBorderWidth:1];
    [_txtInput.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [_txtInput.layer setCornerRadius:5.0];
    [_txtInput setDelegate:self];
    [_txtInput setFont:[RobloxTheme fontBody]];
    [_txtInput setKeyboardType:UIKeyboardTypeDefault];
    [_txtInput setText:@""];
    [_txtInput setTextAlignment:NSTextAlignmentLeft];
    [_txtInput setContentVerticalAlignment:UIControlContentVerticalAlignmentCenter];
    [_txtInput setTextColor:[RobloxTheme colorGray1]];
    [_txtInput setLeftViewMode:UITextFieldViewModeAlways];
    [_txtInput setOpaque:YES];
    [_txtInput setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [_txtInput setAutocorrectionType:UITextAutocorrectionTypeNo];
    [_txtInput setReturnKeyType:UIReturnKeyDone];
    [_txtInput setClearButtonMode:UITextFieldViewModeWhileEditing];
    
    _lblTitle = [[UILabel alloc] init];
    [_lblTitle setText:@""];
    [_lblTitle setOpaque:YES];
    [RobloxTheme applyToModalLoginTitleLabel:_lblTitle];
    
    
    //_lblHint = [[UILabel alloc] init];
    //[_lblHint setText:@""];
    //[_lblHint setOpaque:YES];
    //[_lblHint setLineBreakMode:NSLineBreakByWordWrapping];
    //[_lblHint setNumberOfLines:2];
    //[RobloxTheme applyToModalLoginHintLabel:_lblHint];
    
    _lblHelper = [[UILabel alloc] init];
    [_lblHelper setText:@""];
    [_lblHelper setOpaque:YES];
    [_lblHelper setHidden:YES];
    [RobloxTheme applyToModalLoginHintLabel:_lblHelper];
    [_lblHelper setFont:[RobloxTheme fontBodySmall]];
    //[_lblHelper setLineBreakMode:NSLineBreakByTruncatingTail];
    
    _lblError = [[UILabel alloc] init];
    [_lblError setFont:[RobloxTheme fontBodySmall]];
    [_lblError setTextColor:[RobloxTheme colorRed1]];
    [_lblError setTextAlignment:NSTextAlignmentLeft];
    [_lblError setText:@""];
    [_lblError setOpaque:YES];
    [_lblError setHidden:YES];
    [_lblError setNumberOfLines:1];
    [_lblError setAdjustsFontSizeToFitWidth:YES];
    
    //add the objects to the view
    [self addSubview:_txtInput];
    [self addSubview:_lblTitle];
    //[self addSubview:_lblHint];
    [self addSubview:_lblError];
    [self addSubview:_lblHelper];
    
    [_txtInput  setFrame:CGRectZero];
    [_lblHelper setFrame:CGRectZero];
    [_lblError  setFrame:CGRectZero];
}
-(void) layoutSubviews {
    //layout the views
    int margin = 12; //self.width * 0.05;
    _helperSize= CGSizeMake(self.width, 14);
    _inputSize = CGSizeMake(self.width, self.height);
    
    [_txtInput  setFrame:CGRectMake(0, _helperSize.height, _inputSize.width, _inputSize.height)];
    [_lblHelper setFrame:CGRectMake(0, self.height, _helperSize.width, _helperSize.height)];
    [_lblError  setFrame:CGRectMake(0, self.height, _helperSize.width, _helperSize.height)];
    
    [_txtInput setLeftView:[[UIView alloc] initWithFrame:CGRectMake(0, 0, margin, _txtInput.height)]];
    
    if (_lblError.hidden)
    {
        if (_lblHelper.hidden)
            [_txtInput setY:0];
        else
            [_txtInput setY:_lblHelper.bottom];
            
    }
    
    if (_lblTitle.text.length > 0)
    {
        //int margin = self.width * 0.05;
        CGSize textSize = [_lblTitle.text sizeWithAttributes:@{NSFontAttributeName:_lblTitle.font}];
        [_lblTitle  setFrame:CGRectMake(margin, _txtInput.y, textSize.width, _txtInput.height)];
        //[_lblHint   setFrame:CGRectMake(_lblTitle.right + margin, _txtInput.y, MAX(0, _txtInput.width - (_lblTitle.right + margin + margin)), _txtInput.height)];
    }
    
    //if (_lblHelper.text.length > 0) //(_lblHint.text.length > 0)
    //{
    //    CGSize textSize = [_lblHelper.text sizeWithAttributes:@{NSFontAttributeName:_lblHelper.font}];
    //    if (textSize.width >= _lblHelper.width)
    //        [_lblHelper setText:_lblHint.text];
    //}
}
-(id)   init        {
    self = [super init];
    if (self)
        [self initialize];
    
    return self;
}
-(id)   initWithFrame:(CGRect)frame         {
    self = [super initWithFrame:frame];
    if (self)
        [self initialize];
    
    return self;
}
-(id)   initWithCoder:(NSCoder *)aDecoder   {
    self = [super initWithCoder:aDecoder];
    if (self)
        [self initialize];
    
    return self;
}


//Mutators
-(void) setText:(NSString *)text            { _txtInput.text = text; [self forceUpdate]; }
-(void) setTitle:(NSString*)titleText       {
    [_lblTitle setText:titleText];
    //[_lblHelper setText:titleText];

    int margin = self.width * 0.05;
    CGSize textSize = [_lblTitle.text sizeWithAttributes:@{NSFontAttributeName:_lblTitle.font}];
    [_lblTitle  setFrame:CGRectMake(margin, _txtInput.y, textSize.width, _txtInput.height)];
    //[_lblHint   setFrame:CGRectMake(_lblTitle.right + margin, _txtInput.y, MAX(0, (_txtInput.width - _lblTitle.width) - margin), _txtInput.height)];
    
}
-(void) setHint:(NSString*)hintText         {
    //[_lblHint setText:hintText];
    //[_lblHelper setText:[NSString stringWithFormat:@"%@ (%@)", _lblHelper.text, hintText]];
    [_lblHelper setText:hintText];
}
-(void) setError:(NSString*)errorMessage    { [_txtInput setText:errorMessage]; }
-(void) setNextResponder:(id)aResponder     {
    _nextResponder = aResponder;

    [_txtInput setReturnKeyType: (_nextResponder != nil) ? UIReturnKeyNext : UIReturnKeyDone];
}
-(void) setValidationBlock:(void(^)())block { validationBlock = block; }
-(void) setExitOnEnterBlock:(void (^)())block { exitOnEnterBlock = block; }
-(void) forceUpdate {
    if (!self)
        return;
    
    if (_txtInput.text && _txtInput.text.length > 0)
    {
        //the user has entered text, so keep the helpers hidden
        _lblTitle.hidden = YES;
        //_lblHint.hidden = YES;
    }
    else
    {
        //the user has not entered any text yet, show the helper hints again
        _lblTitle.hidden = NO;
        //_lblHint.hidden  = NO;
    }
    
    
    _lblHelper.hidden = YES;
    [_txtInput setY:0];
    
    if (_lblError.hidden)
    {
        //everything is normal
        [self markAsNormal];
    }
}

-(void) hideError {
    dispatch_async(dispatch_get_main_queue(), ^
    {
        [_txtInput setY:0];
        _lblError.hidden = YES;
    });
}
-(void) showError:(NSString*)errorMessage       {
    if (errorMessage && errorMessage.length > 0)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            //dynamically size the labels according to the size of the frame provided
            [_txtInput  setY:0];
            _lblError.text = errorMessage;
            _lblError.hidden = NO;
            _lblHelper.hidden = YES;
        });
    }
    else
    {
        [self hideError];
    }
}
-(void) setProtectedTextEntry:(bool)isProtected { _txtInput.secureTextEntry = isProtected; }
-(void) setKeyboardType:(UIKeyboardType)type    { [_txtInput setKeyboardType:type]; }

//color changing
-(void) changeBorderColor:(UIColor*)color { dispatch_async(dispatch_get_main_queue(), ^ { _txtInput.layer.borderColor=[color CGColor]; }); }
-(void) markAsValid     { [self changeBorderColor:[RobloxTheme colorGreen1]]; _isValidated = YES; [self hideError];}
-(void) markAsNormal    { [self changeBorderColor:[RobloxTheme colorGray4]];  }
-(void) markAsInvalid   { [self changeBorderColor:[RobloxTheme colorRed1]];   _isValidated = NO; }


//Accessors
-(NSString*) getText    { return _txtInput.text; }
-(NSString*) getError   { return _lblError.text; }
-(bool) isValidated     { return _isValidated; }
-(bool) isEditing       { return _txtInput.isEditing; }


//Delegate functions
-(void) textFieldDidBeginEditing:(UITextField *)textField   {
    if (textField == _txtInput)
    {
        _lblTitle.hidden = YES;
        //_lblHint.hidden = YES;

        
        //only reveal the helper text if the error isn't visible
        if (_lblError.hidden)
        {
            _lblHelper.hidden = NO;
            //[_txtInput setY:_lblHelper.bottom];
        }
        else
        {
            _lblHelper.hidden = YES;
        }
    }
}
-(void) textFieldDidEndEditing:(UITextField *)textField     {
    if (textField == _txtInput)
    {
        _isValidated = NO;
        _lblHelper.hidden  = YES;
        _lblError.hidden = YES;
        //[_txtInput setY:0];
        
        [self markAsNormal];
        
        if (textField.text.length > 0)
        {
            //the user has entered text, so keep the helpers hidden
            _lblTitle.hidden = YES;
            //_lblHint.hidden = YES;
            
            if (validationBlock)
                validationBlock();
        }
        else
        {
            //the user has not entered any text yet, show the helper hints again
            _lblTitle.hidden = NO;
            //_lblHint.hidden  = NO;
        }
    }
}
-(BOOL) textFieldShouldReturn:(UITextField *)textField      {
    if (_nextResponder)
        [_nextResponder becomeFirstResponder];
    else
        [self.superview endEditing:YES]; //[self resignFirstResponder]; //<-- THIS LINE IS SUSPECT
    
    if (exitOnEnterBlock)
        exitOnEnterBlock();
    
    return (_nextResponder == nil);
}

//Misc functions
-(BOOL) becomeFirstResponder {
    [super becomeFirstResponder];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self textFieldDidBeginEditing:_txtInput];
        
    });
    return [_txtInput becomeFirstResponder];
}
-(BOOL) resignFirstResponder {
    [super resignFirstResponder];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self textFieldDidEndEditing:_txtInput];
        
        _lblHelper.hidden = YES;
        _lblError.hidden = YES;
    });
    return [_txtInput resignFirstResponder];
}

@end
