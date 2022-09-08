//
//  ControlComponent.h
//  IOSClient
//
//  Created by Ben Tkacheff on 9/6/12.
//  Copyright (c) 2012 tolkring@gmail.com. All rights reserved.
//
#pragma once

#import <UIKit/UIKit.h>
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/GamepadService.h"
#include "v8datamodel/TouchInputService.h"

@class ControlView;

@interface ControlComponent : UIImageView
{
    
}

- (RBX::Game*) getGameFromControlView;
- (RBX::GamepadService*) getGamepadServiceForGameDataModel;
- (RBX::UserInputService*) getUserInputServiceForGameDataModel;
- (RBX::TouchInputService*) getTouchInputServiceForGameDataModel;

- (ControlView*) findControlView;

@end
