//
//  GameKeyboard.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 10/23/12.
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#import "GameKeyboard.h"
#import "UIScreen+PortraitBounds.h"
#include "v8datamodel/InputObject.h"
#import "RobloxInfo.h"

#define TEXTVIEWHEIGHT 28


static void runExternalReleaseFocus(shared_ptr<RBX::TextBox> currentTextbox) {
    if (currentTextbox.get())
        currentTextbox->externalReleaseFocus(currentTextbox->getText().c_str(), false, shared_ptr<RBX::InputObject>());
}

@implementation GameKeyboard

+ (id)sharedInstance
{
    static dispatch_once_t pred = 0;
    static GameKeyboard *shared = nil;
    
    dispatch_once(&pred, ^{ // Need to use GCD for thread-safe allocation
        shared = [[GameKeyboard alloc] init];
    });
    return shared;
}

- (id) init
{
    if (self = [super init])
    {
        currentTextBox = boost::shared_ptr<RBX::TextBox>();
        
        CGRect bounds = [[UIScreen mainScreen] portraitBounds];
        bounds = CGRectMake(0,0,bounds.size.height,bounds.size.width);
        
        self.frame = bounds;
        [self setUserInteractionEnabled:NO];
        
        textView = [[UITextField alloc] initWithFrame:CGRectMake(5,bounds.size.height/2,bounds.size.width - 10,TEXTVIEWHEIGHT)];
        textView.borderStyle = UITextBorderStyleRoundedRect;
        textView.delegate = self;
        textView.autocorrectionType = UITextAutocorrectionTypeNo;
        textView.hidden = YES;
        
        [self addSubview:textView];
        
        [[NSNotificationCenter defaultCenter] addObserver:self  selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillChangeFrame:) name:UIKeyboardWillChangeFrameNotification object:nil];
    }
    return self;
}

-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(void) hideKeyboard
{
    currentTextBox = boost::shared_ptr<RBX::TextBox>();
    textView.text = @"";
    textView.hidden = YES;
    [self setUserInteractionEnabled:NO];
    [textView resignFirstResponder];
}

-(void)keyboardWillHide:(NSNotification *) notification
{
    if (RBX::Game* game = [self getGameFromControlView])
        if (shared_ptr<RBX::DataModel> currDataModel = game->getDataModel())
            if(currentTextBox && currentTextBox.get())
                currDataModel->submitTask(boost::bind(runExternalReleaseFocus, currentTextBox), RBX::DataModelJob::TaskType::Write);
    
    [self hideKeyboard];
}

-(void)keyboardWillChangeFrame:(NSNotification *) notification
{
    if (![RobloxInfo isDeviceOSVersionPreiOS8])
    {
        dispatch_async(dispatch_get_main_queue(),^{
            //use information pulled from the notification to reposition the keyboard
            CGRect endFrame = [[notification.userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
            CGRect textFrame = CGRectMake(0, endFrame.origin.y - TEXTVIEWHEIGHT, endFrame.size.width, TEXTVIEWHEIGHT);
            [textView setFrame:textFrame];
        });
    }
}

-(void) setDefaultString:(NSString*) defaultString
{
    textView.placeholder = defaultString;
}

-(void) setParentView:(UIView*) parentView
{
    if(!parentView)
        [self hideKeyboard];
    
    [parentView addSubview:self];
}

- (bool) showKeyboard:(const char*) stringToShow
{
    if(textView.hidden)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            textView.text = [NSString stringWithUTF8String:stringToShow];
            
            CGRect bounds = [[UIScreen mainScreen] portraitBounds];
            bounds = CGRectMake(0,0,bounds.size.height,bounds.size.width);
            
            int margin = 5;
            if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
                textView.frame = CGRectMake(margin,bounds.size.height/2.5f,bounds.size.width - (margin + margin),TEXTVIEWHEIGHT);
            else
                textView.frame = CGRectMake(margin,bounds.size.height/2,bounds.size.width - (margin + margin),TEXTVIEWHEIGHT);
            
            textView.hidden = NO;
            [self setUserInteractionEnabled:YES];
            [textView becomeFirstResponder];
        });
        return YES;
    }
    
    return NO;
}
- (bool) showKeyboardWithTextBox:(boost::shared_ptr<RBX::TextBox>) newTextBox
{
    if(textView.hidden && newTextBox)
    {
        currentTextBox = newTextBox;
        return [self showKeyboard:currentTextBox->getBufferedText().c_str()];
    }
    return NO;
}

-(NSString*) getText
{
    return textView.text;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
	if (currentTextBox)
	{
		currentTextBox->setBufferedText([textView.text stringByReplacingCharactersInRange:range withString:string].UTF8String, range.location + string.length);
	}
	return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    if(RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(currentTextBox.get()))
        
    if (!userInputService->showStatsBasedOnInputString([textView.text UTF8String]))
    {
        userInputService->textboxDidFinishEditing([textView.text UTF8String], true);
        // make sure our ui calls happen on main thread
        dispatch_async(dispatch_get_main_queue(), ^(void) {
            [self hideKeyboard];
        });
    }
    
    return YES;
}

- (void) textFieldDidEndEditing:(UITextField *)textField
{
    if(![textView isFirstResponder])
        return;
    
    if(RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(currentTextBox.get()))
        userInputService->textboxDidFinishEditing([textView.text UTF8String], false);
    
    // make sure our ui calls happen on main thread
    dispatch_async(dispatch_get_main_queue(), ^(void) {
        [self hideKeyboard];
    });
}

@end

