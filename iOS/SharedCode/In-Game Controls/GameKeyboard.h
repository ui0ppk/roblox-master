//
//  GameKeyboard.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 10/23/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//
#pragma once

#import "ControlComponent.h"
#include "v8datamodel/Textbox.h"

@interface GameKeyboard : ControlComponent <UITextFieldDelegate>
{
    UITextField* textView;
    boost::shared_ptr<RBX::TextBox> currentTextBox;
}

- (id) init;
- (void) dealloc;

+(id) sharedInstance;

-(void) hideKeyboard;
-(bool) showKeyboard:(const char*) stringToShow;
-(bool) showKeyboardWithTextBox:(boost::shared_ptr<RBX::TextBox>) newTextBox;
-(void)keyboardWillChangeFrame:(NSNotification *) notification;
-(void)keyboardWillHide:(NSNotification *) notification;

-(NSString*) getText;

-(void) setDefaultString:(NSString*) defaultString;
-(void) setParentView:(UIView*) parentView;

@end
