//
//  RbxInputView.h
//  Roblox iOS Shared Code
//
//  Created by Ben Tkacheff on 10/25/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//
#pragma once

#import <GameController/GameController.h>
#import <CoreMotion/CoreMotion.h>
#import "ControlComponent.h"

#include "v8datamodel/TouchInputService.h"
#include "v8datamodel/UserInputService.h"

@class RbxInputView;

typedef std::vector<G3D::Vector3> KeycodeInputs;
typedef boost::unordered_map<RBX::KeyCode, KeycodeInputs> BufferedGamepadState;
typedef boost::unordered_map<RBX::InputObject::UserInputType, BufferedGamepadState> BufferedGamepadStates;

static boost::mutex ControllerBufferMutex;

@interface RbxInputView : ControlComponent <UIGestureRecognizerDelegate>
{
    G3D::Vector2int16 windowSize;
    
    // Gesture Input
    // each direction needs its own recognizer, nice...
    UISwipeGestureRecognizer *swipeRightRecognizer;
    UISwipeGestureRecognizer *swipeLeftRecognizer;
    UISwipeGestureRecognizer *swipeUpRecognizer;
    UISwipeGestureRecognizer *swipeDownRecognizer;
    
    UITapGestureRecognizer *tapRecognizer;
    UITapGestureRecognizer *twoFingerTapRecognizer;
    UITapGestureRecognizer *threeFingerTapRecognizer;
    
    UIRotationGestureRecognizer* rotationRecognizer;
    UILongPressGestureRecognizer* longPressRecognizer;
    UIPinchGestureRecognizer *pinchRecognizer;
    UIPanGestureRecognizer *panRecognizer;
    
    
    // Touch Input
    boost::unordered_map<void*, shared_ptr<RBX::InputObject> > touchInputMap;
    NSMutableArray* storedTouches;
    
    
    // Motion Input
    CMMotionManager *motionManager;
    NSOperationQueue *motionQueue;
    CMAttitude* refAttitude;
    
    
    // Controller Input
    bool paused;
    std::map<int, bool> controllersConnectedMap;
    BufferedGamepadStates controllerBufferMap;
    
    // DataModel service references
    weak_ptr<RBX::TouchInputService> weakTouchInputService;
}

- (id) init: (CGRect)frame;
// any init that requires use of a datamodel or its services, put in this function
- (void) datamodelInit;

// Basic Input handling
-(void) sendTouchEvent:(UITouch*) touch;
-(void) sendTouchEvent:(UITouch*) touch shouldOverride:(BOOL) shouldOverride overrideState:(UITouchPhase) overrideState;

-(void) cancelAllTouches;

-(void) basicGestureConfig:(UIGestureRecognizer*) gesture;

// Gesture handlers
-(void) twoFingerPinch:(UIPinchGestureRecognizer *)recognizer;
-(void) tapGesture:(UITapGestureRecognizer*) recognizer;
-(void) swipeGesture:(UISwipeGestureRecognizer*)recognizer;
-(void) longPressGesture:(UILongPressGestureRecognizer*) recognizer;
-(void) panGesture:(UIPanGestureRecognizer*) recognizer;
-(RBX::UserInputService::SwipeDirection) getRbxSwipeDirection:(UISwipeGestureRecognizerDirection) uiSwipeDirection;

// Motion Stuff
-(void) startMotionUpdates;
+(BOOL) isGyroscopeAvailable;

// Controller Stuff
-(void) gcControllerConnected:(NSNotification*) notification;
-(void) gcControllerDisconnected:(NSNotification*) notification;
@end
