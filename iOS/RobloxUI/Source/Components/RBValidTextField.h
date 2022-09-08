//
//  RBValidTextField.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 7/9/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RBValidTextField : UIView <UITextFieldDelegate>

@property (nonatomic, getter=getText,   setter=setText:)    NSString* text;
@property (nonatomic, getter=getTitle,  setter=setTitle:)   NSString* titleText;
@property (nonatomic, getter=getHint,   setter=setHint:)    NSString* hintText;
@property (nonatomic, getter=getError,  setter=setError:)   NSString* errorMessage;


//mutators
-(void) setProtectedTextEntry:(bool)isProtected;
-(void) setKeyboardType:(UIKeyboardType)type;
-(void) setValidationBlock:(void (^)())block;
-(void) setExitOnEnterBlock:(void (^)())block;
-(void) setNextResponder:(id)aResponder;
-(void) forceUpdate;

-(void) markAsValid;
-(void) markAsNormal;
-(void) markAsInvalid;

-(void) showError:(NSString*)errorMessage;
-(void) hideError;

//accessors
-(bool) isValidated;
-(bool) isEditing;

//odd functions
-(BOOL) becomeFirstResponder;
-(BOOL) resignFirstResponder;
@end
