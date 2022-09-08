//
//  RbxInputView.mm
//  Roblox iOS Shared Code
//
//  Created by Ben Tkacheff on 10/25/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "RbxInputView.h"
#import "ObjectiveCUtilities.h"
#import "UIScreen+PortraitBounds.h"

#include "v8datamodel/GamepadService.h"

#define MOTION_UPDATE_RATE_PER_SEC 60.0

@implementation RbxInputView

/////////////////////////////////////////////////////////////////////////////////////
//
//  Begin Init Code
///////////////////////////////////////////////////////////////

- (id) init: (CGRect) frame
{
    self = [super init];
    if (self)
    {
        [self setFrame:frame];
        
        storedTouches = [[NSMutableArray alloc] init];
        
        self.multipleTouchEnabled = YES;
        
        // have to flip width and height since they return in portrait mode
        CGRect bounds = [[UIScreen mainScreen] portraitBounds];
        windowSize = G3D::Vector2int16(bounds.size.height,bounds.size.width);
        
        [self initGestureRecognizers];
        
        motionManager = [[CMMotionManager alloc] init];
        motionManager.deviceMotionUpdateInterval = 1.0/MOTION_UPDATE_RATE_PER_SEC;
        motionManager.accelerometerUpdateInterval = 1.0/MOTION_UPDATE_RATE_PER_SEC;
        motionManager.gyroUpdateInterval = 1.0/MOTION_UPDATE_RATE_PER_SEC;
        
        controllersConnectedMap[0] = false;
        controllersConnectedMap[1] = false;
        controllersConnectedMap[2] = false;
        controllersConnectedMap[3] = false;
        
        paused = false;
        
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(gcControllerConnected:)
                                                     name:GCControllerDidConnectNotification object:nil ];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(gcControllerDisconnected:)
                                                     name:GCControllerDidDisconnectNotification object:nil ];
    }
    return self;
}

// any init that requires use of a datamodel or its services, put in this function
- (void) datamodelInit
{
    [self getGameFromControlView]->getDataModel()->submitTask(boost::bind(boostFuncFromSelector(@selector(dataModelInitWithLock), self)), RBX::DataModelJob::Write);
    
    if( RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel] )
    {
        userInputService->motionEventListeningStarted.connect(boostFuncFromSelector_1<std::string>(@selector(motionDidStartListening:), self));
    }
    
    [self setupControllers];
}

-(void) dataModelInitWithLock
{
    weakTouchInputService = shared_from([self getTouchInputServiceForGameDataModel]);
}

-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [motionManager stopDeviceMotionUpdates];
}

/////////////////////////////////////////////////////////////////////////////////////
//
//  End Init Code
///////////////////////////////////////////////////////////////










/////////////////////////////////////////////////////////////////////////////////////
//
// Begin GameController Code
///////////////////////////////////////////////////////////////

-(RBX::Gamepad) getControllerKeyMapForControllerEnum:(RBX::InputObject::UserInputType) gamepadEnum
{
    if (RBX::GamepadService* gamepadService = [self getGamepadServiceForGameDataModel])
    {
        return gamepadService->getGamepadState(RBX::GamepadService::getGamepadIntForEnum(gamepadEnum));
    }
    
    return RBX::Gamepad();
}


-(RBX::Gamepad) getControllerKeyMapForController:(NSUInteger) playerIndex
{
    if (RBX::GamepadService* gamepadService = [self getGamepadServiceForGameDataModel])
    {
        return gamepadService->getGamepadState(playerIndex);
    }

    return RBX::Gamepad();
}

-(void) controllerPauseHandler:(GCController*) controller isPaused:(BOOL)isPaused
{
    float buttonValue = isPaused ? 1.0f : 0.0f;

    {
        boost::mutex::scoped_lock mutex(ControllerBufferMutex);
        controllerBufferMap[RBX::GamepadService::getGamepadEnumForInt(controller.playerIndex)][RBX::SDLK_GAMEPAD_BUTTONSTART].push_back(G3D::Vector3(0,0,buttonValue));
    }
}

-(void) controllerButtonHandler:(GCController*) controller button:(GCControllerButtonInput*) button value:(float) value pressed:(BOOL) pressed
{
    G3D::Vector3 buttonValue(0,0,value);
    RBX::KeyCode firedKeyCode = RBX::SDLK_UNKNOWN;
    
    if (button == controller.extendedGamepad.buttonA ||
        button == controller.gamepad.buttonA)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONA;
    }
    else if (button == controller.extendedGamepad.buttonB ||
             button == controller.gamepad.buttonB)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONB;
    }
    else if (button == controller.extendedGamepad.buttonX ||
             button == controller.gamepad.buttonX)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONX;
    }
    else if (button == controller.extendedGamepad.buttonY ||
             button == controller.gamepad.buttonY)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONY;
    }
    else if (button == controller.extendedGamepad.leftShoulder ||
             button == controller.gamepad.leftShoulder)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONL1;
    }
    else if (button == controller.extendedGamepad.rightShoulder ||
             button == controller.gamepad.rightShoulder)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONR1;
    }
    else if (button == controller.extendedGamepad.rightTrigger)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONR2;
    }
    else if (button == controller.extendedGamepad.leftTrigger)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_BUTTONL2;
    }
    
    if (firedKeyCode != RBX::SDLK_UNKNOWN)
    {
        {
            boost::mutex::scoped_lock mutex(ControllerBufferMutex);
            controllerBufferMap[RBX::GamepadService::getGamepadEnumForInt(controller.playerIndex)][firedKeyCode].push_back(buttonValue);
        }
    }
}

-(void) controllerDirectionPadHandler:(GCController*)controller pad:(GCControllerDirectionPad*)pad xValue:(float)xValue yValue:(float)yValue
{
    if (pad == controller.extendedGamepad.dpad ||
        pad == controller.gamepad.dpad)
    {
        GCControllerDirectionPad* dpad = controller.extendedGamepad.dpad;
        if (pad == controller.gamepad.dpad)
        {
           dpad = controller.gamepad.dpad;
        }
        
        {
            boost::mutex::scoped_lock mutex(ControllerBufferMutex);
            
            RBX::InputObject::UserInputType gamepadEnum = RBX::GamepadService::getGamepadEnumForInt(controller.playerIndex);
            
            {
                G3D::Vector3 leftPressed = G3D::Vector3(0,0,dpad.left.pressed ? 1 : 0);
                G3D::Vector3 rightPressed = G3D::Vector3(0,0,dpad.right.pressed ? 1 : 0);
                
                if (controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADLEFT].empty() ||
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADLEFT].back() != leftPressed)
                {
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADLEFT].push_back(leftPressed);
                }
                
                if (controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADRIGHT].empty() ||
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADRIGHT].back() != rightPressed)
                {
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADRIGHT].push_back(rightPressed);
                }
            }

            {
                G3D::Vector3 upPressed = G3D::Vector3(0,0,dpad.up.pressed ? 1 : 0);
                G3D::Vector3 downPressed = G3D::Vector3(0,0,dpad.down.pressed ? 1 : 0);
                
                if (controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADUP].empty() ||
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADUP].back() != upPressed)
                {
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADUP].push_back(upPressed);
                }
                if (controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADDOWN].empty() ||
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADDOWN].back() != downPressed)
                {
                    controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADDOWN].push_back(downPressed);
                }
            }
        }
        
        return;
    }
    
    RBX::KeyCode firedKeyCode = RBX::SDLK_UNKNOWN;
    if (pad == controller.extendedGamepad.leftThumbstick)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_THUMBSTICK1;
    }
    else if (pad == controller.extendedGamepad.rightThumbstick)
    {
        firedKeyCode = RBX::SDLK_GAMEPAD_THUMBSTICK2;
    }
    
    if (firedKeyCode != RBX::SDLK_UNKNOWN)
    {
        {
            boost::mutex::scoped_lock mutex(ControllerBufferMutex);
            
            const G3D::Vector3 newValue(xValue,yValue,0);
            const RBX::InputObject::UserInputType gamepadEnum = RBX::GamepadService::getGamepadEnumForInt(controller.playerIndex);
            
            if (controllerBufferMap[gamepadEnum].find(firedKeyCode) == controllerBufferMap[gamepadEnum].end())
            {
                KeycodeInputs newKeycodeInputs;
                newKeycodeInputs.push_back(newValue);
                
                controllerBufferMap[gamepadEnum][firedKeyCode] = KeycodeInputs(newKeycodeInputs);
                return;
            }
            
            KeycodeInputs bufferedKeyValues = controllerBufferMap[gamepadEnum][firedKeyCode];
            bufferedKeyValues.push_back(newValue);
                
            controllerBufferMap[gamepadEnum][firedKeyCode] = bufferedKeyValues;
        }
    }
}


// this func is fire by UserInputService's updateInputSignal (thread-safe for DataModel read/write)
-(void) processControllerBufferMap
{
    BufferedGamepadStates tempControllerBufferMap;
    {
        // grab our current input and process
        // need a mutex in case iOS is simultaneously trying to update map
        boost::mutex::scoped_lock mutex(ControllerBufferMutex);
        controllerBufferMap.swap(tempControllerBufferMap);
    }
    
    for (int gamepadNum = RBX::InputObject::TYPE_GAMEPAD1; gamepadNum != (RBX::InputObject::TYPE_GAMEPAD4 + 1); ++gamepadNum)
    {
        RBX::InputObject::UserInputType gamepadEnum = (RBX::InputObject::UserInputType) gamepadNum;
        
        BufferedGamepadState gamepadBufferState = tempControllerBufferMap[gamepadEnum];
        RBX::Gamepad rbxGamepad = [self getControllerKeyMapForControllerEnum:gamepadEnum];
        
        for (BufferedGamepadState::iterator iter = gamepadBufferState.begin(); iter != gamepadBufferState.end(); ++iter)
        {
            boost::shared_ptr<RBX::InputObject> keyInputObject = rbxGamepad[(*iter).first];
            std::vector<RBX::Vector3> positions = (*iter).second;
            
            for (std::vector<RBX::Vector3>::iterator vecIter = positions.begin(); vecIter != positions.end(); ++vecIter)
            {
                bool isButton = false;
                RBX::InputObject::UserInputState state = RBX::InputObject::INPUT_STATE_NONE;
                
                switch (keyInputObject->getKeyCode())
                {
                    case RBX::SDLK_GAMEPAD_BUTTONR2:
                    case RBX::SDLK_GAMEPAD_BUTTONL2:
                    {
                        if ((*vecIter).z >= 1.0f)
                        {
                            state = RBX::InputObject::INPUT_STATE_BEGIN;
                        }
                        else if ((*vecIter).z <= 0.0f)
                        {
                            state = RBX::InputObject::INPUT_STATE_END;
                        }
                        else
                        {
                            state = RBX::InputObject::INPUT_STATE_CHANGE;
                        }
                        break;
                    }
                    case RBX::SDLK_GAMEPAD_BUTTONA:
                    case RBX::SDLK_GAMEPAD_BUTTONB:
                    case RBX::SDLK_GAMEPAD_BUTTONX:
                    case RBX::SDLK_GAMEPAD_BUTTONY:
                    case RBX::SDLK_GAMEPAD_BUTTONR1:
                    case RBX::SDLK_GAMEPAD_BUTTONL1:
                    case RBX::SDLK_GAMEPAD_BUTTONR3:
                    case RBX::SDLK_GAMEPAD_BUTTONL3:
                    case RBX::SDLK_GAMEPAD_BUTTONSTART:
                    case RBX::SDLK_GAMEPAD_DPADDOWN:
                    case RBX::SDLK_GAMEPAD_DPADUP:
                    case RBX::SDLK_GAMEPAD_DPADLEFT:
                    case RBX::SDLK_GAMEPAD_DPADRIGHT:
                    {
                        isButton = true;
                        state = ((*vecIter).z > 0.0f) ? state = RBX::InputObject::INPUT_STATE_BEGIN : state = RBX::InputObject::INPUT_STATE_END;
                        break;
                    }
                        
                    case RBX::SDLK_GAMEPAD_THUMBSTICK1:
                    case RBX::SDLK_GAMEPAD_THUMBSTICK2:
                    {
                        state = (*vecIter) == RBX::Vector3::zero() ? RBX::InputObject::INPUT_STATE_END : RBX::InputObject::INPUT_STATE_CHANGE;
                        break;
                    }
                        
                    default:
                    {
                        break;
                    }
                }
                
                RBX::Vector3 newPosition = (*vecIter);
                if (isButton) // don't use pressure sensitive values to keep consistent behavior on all platforms
                {
                    (newPosition.z > 0.0f) ? newPosition.z = 1.0f : newPosition.z = 0.0f;
                }
            
                
                bool shouldFireEvent = true;
                
                if (keyInputObject->getPosition() != newPosition)
                {
                    keyInputObject->setDelta((newPosition - keyInputObject->getPosition()));
                    keyInputObject->setPosition(newPosition);
                }
                else
                {
                    shouldFireEvent = false;
                }
                
                if (state != RBX::InputObject::INPUT_STATE_NONE)
                {
                    keyInputObject->setInputState(state);
                }
                
                if (shouldFireEvent)
                {
                    if (RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
                    {
                        userInputService->dangerousFireInputEvent(keyInputObject, NULL);
                    }
                }
            }
        }
    }
}

-(void) setupControllerInput:(GCController*) controller
{
    int controllerNum = 0;
    for (std::map<int, bool>::iterator iter = controllersConnectedMap.begin(); iter != controllersConnectedMap.end(); ++iter)
    {
        if ( !(*iter).second )
        {
            controllerNum = (*iter).first;
#ifdef __IPHONE_9_0
            [controller setPlayerIndex:(GCControllerPlayerIndex)controllerNum];
#else
            [controller setPlayerIndex:controllerNum];
#endif
            (*iter).second = true;
            break;
        }
    }
    
    if (controller.extendedGamepad)
    {
        [controller.extendedGamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.extendedGamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.extendedGamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.extendedGamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.extendedGamepad.leftShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.extendedGamepad.rightShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        
        [controller.extendedGamepad.leftThumbstick setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
            [self controllerDirectionPadHandler:controller pad:dpad xValue:xValue yValue:yValue];
        }];
        [controller.extendedGamepad.rightThumbstick setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
            [self controllerDirectionPadHandler:controller pad:dpad xValue:xValue yValue:yValue];
        }];
        [controller.extendedGamepad.dpad setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
            [self controllerDirectionPadHandler:controller pad:dpad xValue:xValue yValue:yValue];
        }];
        
        [controller.extendedGamepad.rightTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.extendedGamepad.leftTrigger setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
    }
    else if(controller.gamepad)
    {
        [controller.gamepad.buttonA setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.gamepad.buttonB setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.gamepad.buttonX setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.gamepad.buttonY setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.gamepad.leftShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        [controller.gamepad.rightShoulder setValueChangedHandler:^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [self controllerButtonHandler:controller button:button value:value pressed:pressed];
        }];
        
        [controller.gamepad.dpad setValueChangedHandler:^(GCControllerDirectionPad *dpad, float xValue, float yValue) {
            [self controllerDirectionPadHandler:controller pad:dpad xValue:xValue yValue:yValue];
        }];
    }
    
    [controller setControllerPausedHandler:^(GCController *controller) {
        paused = !paused;
        [self controllerPauseHandler:controller isPaused:paused];
    }];
    
    if (RBX::UserInputService* inputService = [self getUserInputServiceForGameDataModel])
    {
        inputService->safeFireGamepadConnected(RBX::GamepadService::getGamepadEnumForInt(controllerNum));
    }
}

-(void) setupControllers
{
    int controllerCount = [GCController controllers].count;
    if (controllerCount > 0)
    {
        for (GCController* controller in [GCController controllers])
        {
            [self setupControllerInput:controller];
        }
    }
    
    if (RBX::UserInputService* inputService = [self getUserInputServiceForGameDataModel])
    {
        inputService->getSupportedGamepadKeyCodesSignal.connect(boostFuncFromSelector_1<RBX::InputObject::UserInputType>(@selector(setSupportedGamepadKeycodes:), self));
        inputService->updateInputSignal.connect(boostFuncFromSelector(@selector(processControllerBufferMap), self));
    }
}

-(shared_ptr<const RBX::Reflection::ValueArray>) getAvailableGamepadKeyCodes:(RBX::InputObject::UserInputType) gamepadType
{
    shared_ptr<RBX::Reflection::ValueArray> availableGamepadKeyCodes(new RBX::Reflection::ValueArray());
    
    int controllerIndex = RBX::GamepadService::getGamepadIntForEnum(gamepadType);
    if (controllerIndex >= 0)
    {
        if ([GCController controllers].count > 0)
        {
            for (GCController* controller in [GCController controllers])
            {
                if ([controller playerIndex] == controllerIndex)
                {
                    // every iOS gamepad supports these
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONA);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONB);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONX);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONY);
                    
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADDOWN);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADUP);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADLEFT);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADRIGHT);
                    
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONL1);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONR1);
                    availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONSTART);
                    
                    // extended has some additional functionality, show this
                    if (controller.extendedGamepad)
                    {
                        availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONR2);
                        availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONL2);
                        
                        availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_THUMBSTICK1);
                        availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_THUMBSTICK2);
                    }
                    
                    break;
                }
            }
        }
    }
    
    return availableGamepadKeyCodes;
}

-(void) setSupportedGamepadKeycodes:(RBX::InputObject::UserInputType) gamepadType
{
    shared_ptr<const RBX::Reflection::ValueArray> newSupportedGamepadKeyCodes = [self getAvailableGamepadKeyCodes:gamepadType];
    
    if (RBX::UserInputService* inputService = [self getUserInputServiceForGameDataModel])
    {
        inputService->setSupportedGamepadKeyCodes(gamepadType, newSupportedGamepadKeyCodes);
    }
}



-(void) gcControllerConnected:(NSNotification*) notification
{
    if (GCController* controller = (GCController*)notification.object)
    {
        if ([controller playerIndex] == GCControllerPlayerIndexUnset)
        {
            [self setupControllerInput:controller];
        }
        else if (RBX::UserInputService* inputService = [self getUserInputServiceForGameDataModel])
        {
            inputService->safeFireGamepadConnected(RBX::GamepadService::getGamepadEnumForInt([controller playerIndex]));
        }
    }
}

-(void) gcControllerDisconnected:(NSNotification*) notification
{
    if (GCController* controller = (GCController*)notification.object)
    {
        int playerIndex = [controller playerIndex];
        
        if (playerIndex != GCControllerPlayerIndexUnset)
        {
            if (controllersConnectedMap[playerIndex])
            {
                controllersConnectedMap[playerIndex] = false;
                
                if (RBX::UserInputService* inputService = [self getUserInputServiceForGameDataModel])
                {
                    inputService->safeFireGamepadDisconnected(RBX::GamepadService::getGamepadEnumForInt(playerIndex));
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
// End GameController Code
///////////////////////////////////////////////////////////////











/////////////////////////////////////////////////////////////////////////////////////
//
// Begin Motion Code
///////////////////////////////////////////////////////////////

-(void) motionDidStartListening:(std::string) motionType
{
    [self startMotionUpdates];
}

-(void) startMotionUpdates
{
    motionQueue = [[NSOperationQueue alloc] init];
    [motionManager startDeviceMotionUpdatesToQueue:motionQueue
                                       withHandler:^(CMDeviceMotion *motion, NSError *error)
     {
         if (error == nil)
         {
             [self updateDeviceMotion:motion];
         }
         else
         {
             NSLog(@"error while updating motion, error is %@",error);
         }
     }];
}

-(void)updateDeviceMotion:(CMDeviceMotion*) deviceMotion
{
    RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel];
    if (!userInputService)
    {
        return;
    }
    
    // make values consistent with Android, and translate easier into roblox coordinate system
    userInputService->fireAccelerationEvent( RBX::Vector3(deviceMotion.userAcceleration.x, deviceMotion.userAcceleration.z, deviceMotion.userAcceleration.y) );
    userInputService->fireGravityEvent( RBX::Vector3(deviceMotion.gravity.x, deviceMotion.gravity.z, -deviceMotion.gravity.y) );
    
    CMAttitude *attitude = deviceMotion.attitude;
    
    if (refAttitude == nil)
    {
        refAttitude = [attitude copy];
    }
    else
    {
        [attitude multiplyByInverseOfAttitude:refAttitude];
        RBX::Vector3 newAttitude = RBX::Vector3(-attitude.roll,attitude.pitch,attitude.yaw);
        
        CMQuaternion quaternion = motionManager.deviceMotion.attitude.quaternion;
        // this is funky, getting quaternion to match up with our camera quaternion (so rotation of camera is easy)
        userInputService->fireRotationEvent(newAttitude, RBX::Vector4(quaternion.w,quaternion.x,quaternion.z,quaternion.y));
    }
}

+(BOOL) isGyroscopeAvailable
{
    CMMotionManager *motionManager = [[CMMotionManager alloc] init];
    BOOL gyroAvailable = motionManager.gyroAvailable;
    return gyroAvailable;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// End Motion Code
///////////////////////////////////////////////////////////////










/////////////////////////////////////////////////////////////////////////////////////
//
// Begin Touch Code
///////////////////////////////////////////////////////////////

-(void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for(UITouch* touch in touches)
    {
        [storedTouches addObject:touch];
        [self sendTouchEvent:touch];
    }
    
    [super touchesBegan:touches withEvent:event];
}

-(void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    for(UITouch* touch in touches)
    {
        [storedTouches removeObject:touch];
        [self sendTouchEvent:touch];
    }
    
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    for(UITouch* touch in touches)
    {
        [storedTouches removeObject:touch];
        [self sendTouchEvent:touch];
    }
    
    [super touchesCancelled:touches withEvent:event];
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    for(UITouch* touch in touches)
    {
        [self sendTouchEvent:touch];
    }
    
    [super touchesMoved:touches withEvent:event];
}

-(void) cancelAllTouches
{
    for(UITouch* touch in storedTouches)
    {
        [self sendTouchEvent:touch shouldOverride:true overrideState:UITouchPhaseCancelled];
    }
    
    [storedTouches removeAllObjects];
}

-(void) sendTouchEvent:(UITouch*) touch
{
    [self sendTouchEvent:touch shouldOverride:false overrideState:UITouchPhaseBegan];
}

// Basic Touch Handlers
-(void) sendTouchEvent:(UITouch*) touch shouldOverride:(BOOL) shouldOverride overrideState:(UITouchPhase) overrideState
{
    if (!touch)
        return;
    
    void* rawUnretainedTouch = (__bridge void*) touch;
    
    RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel];
    if (!userInputService)
    {
        return;
    }
    
    CGPoint nativeLocation = [touch locationInView:self];
    RBX::Vector3 rbxLocation = RBX::Vector3(nativeLocation.x,nativeLocation.y,0);
    
    RBX::InputObject::UserInputState newState;
    UITouchPhase touchState = shouldOverride ? overrideState : touch.phase;
    
    switch (touchState)
    {
        case UITouchPhaseBegan:
            newState = RBX::InputObject::INPUT_STATE_BEGIN;
            break;
        case UITouchPhaseMoved:
            newState = RBX::InputObject::INPUT_STATE_CHANGE;
            break;
        case UITouchPhaseCancelled:
        case UITouchPhaseEnded:
            newState = RBX::InputObject::INPUT_STATE_END;
            break;
        case UITouchPhaseStationary:
            newState = RBX::InputObject::INPUT_STATE_NONE;
    }
    
    if (shared_ptr<RBX::TouchInputService> touchInputService =  weakTouchInputService.lock())
    {
        touchInputService->addTouchToBuffer(rawUnretainedTouch, rbxLocation, newState);
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
// End Touch Code
///////////////////////////////////////////////////////////////









/////////////////////////////////////////////////////////////////////////////////////
//
// Begin Gesture Code
///////////////////////////////////////////////////////////////

-(void) initGestureRecognizers
{
    swipeRightRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeGesture:)];
    [self basicGestureConfig:swipeRightRecognizer];
    
    swipeLeftRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeGesture:)];
    [self basicGestureConfig:swipeLeftRecognizer];
    swipeLeftRecognizer.direction = UISwipeGestureRecognizerDirectionLeft;
    
    swipeUpRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeGesture:)];
    [self basicGestureConfig:swipeUpRecognizer];
    swipeUpRecognizer.direction = UISwipeGestureRecognizerDirectionUp;
    
    swipeDownRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipeGesture:)];
    [self basicGestureConfig:swipeDownRecognizer];
    swipeDownRecognizer.direction = UISwipeGestureRecognizerDirectionDown;
    
    tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapGesture:)];
    [self basicGestureConfig:tapRecognizer];
    
    twoFingerTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapGesture:)];
    twoFingerTapRecognizer.numberOfTouchesRequired = 2;
    [self basicGestureConfig:twoFingerTapRecognizer];
    
    threeFingerTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapGesture:)];
    threeFingerTapRecognizer.numberOfTouchesRequired = 3;
    [self basicGestureConfig:threeFingerTapRecognizer];
    
    rotationRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(rotationGesture:)];
    [self basicGestureConfig:rotationRecognizer];
    
    longPressRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longPressGesture:)];
    [self basicGestureConfig:longPressRecognizer];
    
    pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(twoFingerPinch:)];
    [self basicGestureConfig:pinchRecognizer];
    
    panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panGesture:)];
    [self basicGestureConfig:panRecognizer];
}

-(void) basicGestureConfig:(UIGestureRecognizer*) gesture
{
    gesture.delegate = self;
    gesture.cancelsTouchesInView = NO;
    gesture.delaysTouchesBegan = NO;
    gesture.delaysTouchesEnded = NO;
    [self addGestureRecognizer:gesture];
}

-(shared_ptr<RBX::Reflection::ValueArray>) getGestureTouchLocations:(UIGestureRecognizer*) recognizer
{
    shared_ptr<RBX::Reflection::ValueArray> values(rbx::make_shared<RBX::Reflection::ValueArray>());
    for(int i = 0; i < recognizer.numberOfTouches; i++)
    {
        CGPoint touchLocation = [recognizer locationOfTouch:i inView:self];
        values->push_back(RBX::Vector2(touchLocation.x,touchLocation.y));
    }
    return values;
}

// Gesture Handlers
-(RBX::UserInputService::SwipeDirection) getRbxSwipeDirection:(UISwipeGestureRecognizerDirection) uiSwipeDirection
{
    switch (uiSwipeDirection)
    {
        case UISwipeGestureRecognizerDirectionUp:
            return RBX::UserInputService::SwipeDirection::DIRECTION_UP;
        case UISwipeGestureRecognizerDirectionDown:
            return RBX::UserInputService::SwipeDirection::DIRECTION_DOWN;
        case UISwipeGestureRecognizerDirectionLeft:
            return RBX::UserInputService::SwipeDirection::DIRECTION_LEFT;
        case UISwipeGestureRecognizerDirectionRight:
            return RBX::UserInputService::SwipeDirection::DIRECTION_RIGHT;
        default:
            return RBX::UserInputService::SwipeDirection::DIRECTION_NONE;
    }
}

-(void) swipeGesture:(UISwipeGestureRecognizer *)recognizer
{
    //todo: renable when input is no longer a performance hit
    if(recognizer.state != UIGestureRecognizerStateEnded)
        return;
    
    shared_ptr<RBX::Reflection::ValueArray> touchLocations = [self getGestureTouchLocations:recognizer];
    
    shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(2);
    args->values[0] = [self getRbxSwipeDirection:recognizer.direction];
    args->values[1] = (int) recognizer.numberOfTouches;
    
    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
        userInputService->addGestureEventToProcess(RBX::UserInputService::GESTURE_SWIPE, touchLocations, args);
    
}

-(void) tapGesture:(UITapGestureRecognizer*) recognizer
{
    shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(0);
    
    shared_ptr<RBX::Reflection::ValueArray> touchLocations = [self getGestureTouchLocations:recognizer];
    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
        userInputService->addGestureEventToProcess(RBX::UserInputService::GESTURE_TAP, touchLocations, args);
}

-(void) rotationGesture:(UIRotationGestureRecognizer*) recognizer
{
    shared_ptr<RBX::Reflection::ValueArray> touchLocations = [self getGestureTouchLocations:recognizer];
    
    shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(3);
    args->values[0] = (float)recognizer.rotation;
    args->values[1] = (float)recognizer.velocity;
    args->values[2] = UIGestureRecognizerStateToUIState(recognizer.state);
    
    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
        userInputService->addGestureEventToProcess(RBX::UserInputService::GESTURE_ROTATE, touchLocations, args);
}

static RBX::InputObject::UserInputState UIGestureRecognizerStateToUIState(UIGestureRecognizerState gestureState)
{
    switch (gestureState)
    {
        case UIGestureRecognizerStateBegan:
            return RBX::InputObject::INPUT_STATE_BEGIN;
        case UIGestureRecognizerStateChanged:
            return RBX::InputObject::INPUT_STATE_CHANGE;
        case UIGestureRecognizerStateEnded:
        case UIGestureRecognizerStateCancelled:
            return RBX::InputObject::INPUT_STATE_END;
        default:
            return RBX::InputObject::INPUT_STATE_NONE;
    }
}

-(void) longPressGesture:(UILongPressGestureRecognizer*) recognizer
{
    shared_ptr<RBX::Reflection::ValueArray> touchLocations = [self getGestureTouchLocations:recognizer];
    
    shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(1);
    args->values[0] = UIGestureRecognizerStateToUIState(recognizer.state);
    
    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
        userInputService->addGestureEventToProcess(RBX::UserInputService::GESTURE_LONGPRESS, touchLocations, args);
}

- (void) twoFingerPinch:(UIPinchGestureRecognizer *)recognizer
{
    shared_ptr<RBX::Reflection::ValueArray> touchLocations = [self getGestureTouchLocations:recognizer];
    
    shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(3);
    args->values[0] = (float)recognizer.scale;
    args->values[1] = (float)recognizer.velocity;
    args->values[2] = UIGestureRecognizerStateToUIState(recognizer.state);
    
    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
        userInputService->addGestureEventToProcess(RBX::UserInputService::GESTURE_PINCH, touchLocations, args);
}

-(void) panGesture:(UIPanGestureRecognizer*) recognizer
{
    shared_ptr<RBX::Reflection::ValueArray> touchLocations = [self getGestureTouchLocations:recognizer];
    
    CGPoint totalTranslation = [recognizer translationInView:self];
    RBX::Vector2 rbxTotalTranslation(totalTranslation.x,totalTranslation.y);
    CGPoint velocity = [recognizer velocityInView:self];
    RBX::Vector2 rbxVelocity(velocity.x,velocity.y);
    
    shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(3);
    args->values[0] = rbxTotalTranslation;
    args->values[1] = rbxVelocity;
    args->values[2] = UIGestureRecognizerStateToUIState(recognizer.state);

    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
        userInputService->addGestureEventToProcess(RBX::UserInputService::GESTURE_PAN, touchLocations, args);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer
{
    return YES;
}

- (BOOL) gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    return YES;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// End Gesture Code
///////////////////////////////////////////////////////////////


@end
