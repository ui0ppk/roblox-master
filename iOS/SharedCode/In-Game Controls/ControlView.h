//
//  ControlView.h
//  IOSClient
//
//  Created by Ben Tkacheff on 8/21/12.
//  Copyright (c) 2012 tolkring@gmail.com. All rights reserved.
//
#pragma once

#include "v8datamodel/UserInputService.h"
#include "V8DataModel/TextBox.h"
#include "v8datamodel/Game.h"

#import <UIKit/UIKit.h>

#import "RbxInputView.h"

@interface ControlView : ControlComponent <UIGestureRecognizerDelegate>
{
    RbxInputView* rbxInputView;
    
    RBX::Vector2int16 frameSize;
    
    UITouch* tapTouch;
    G3D::Vector2 tapTouchBeginPos;
    
    double tapSensitivity;
    int tapTouchMoveTolerance;
    
    boost::weak_ptr<RBX::Game> game;
    
    rbx::signals::scoped_connection dmUserInputTextBoxFocusCon;
    rbx::signals::scoped_connection dmUserInputTextBoxReleaseFocusCon;
    rbx::signals::scoped_connection dmUserInputProcessMouseEventCon;
    
    // fake mouse events (for backwards compatibility
    shared_ptr<RBX::InputObject> mouseButton1Event;
    shared_ptr<RBX::InputObject> mouseMoveEvent;
}

- (id) init:(CGRect)frame withGame:(boost::shared_ptr<RBX::Game>) newGame;
- (void) dealloc;

- (RbxInputView*) getRbxInputView;

- (void) disconnectEvents;
- (void) setupEvents;

- (void) setGame:(boost::shared_ptr<RBX::Game>) newGame;
- (boost::shared_ptr<RBX::Game>) getGame;

- (void) textBoxFocusGained:(boost::shared_ptr<RBX::TextBox>) textBoxFocused;
- (void) textBoxFocusLost:(boost::shared_ptr<RBX::TextBox>) textBoxUnfocused;


// Gesture Recognizers
// Tap
- (void) oneFingerSingleTap;
- (UITouch*) checkTouchesForTap:(NSSet *)touches withEvent:(UIEvent *)event;
- (void) checkTapTouchMove:(NSSet*) touchesSet;
- (void) invalidateTapGesture:(id) oldTapTouch;

- (void) postMouseEventProcessed:(bool) processedEvent inputObject: (void*) uiTouch event:(const shared_ptr<RBX::InputObject>&) event;

@end
