//
//  ControlComponent.m
//  IOSClient
//
//  Created by Ben Tkacheff on 9/6/12.
//  Copyright (c) 2012 tolkring@gmail.com. All rights reserved.
//

#import "ControlComponent.h"
#import "ControlView.h"

#include "v8datamodel/TouchInputService.h"

@implementation ControlComponent

- (id) init
{
    if (self = [super init])
    {
        self.userInteractionEnabled = true;
    }
    return self;
}

- (ControlView*) findControlView
{
    if ([self isKindOfClass:[ControlView class]])
        return (ControlView*)self;
    
    id nextSuperView = self.superview;
    
    while (nextSuperView != nil)
    {
        if ([nextSuperView isKindOfClass:[ControlView class]])
            return nextSuperView;
        
        if ([nextSuperView isKindOfClass:[UIView class]])
        {
            UIView* theUIView = nextSuperView;
            nextSuperView = theUIView.superview;
        }
        else
            nextSuperView = nil;
    }
    
    return nil;
}

-(RBX::Game*) getGameFromControlView
{
    ControlView* controlView = [self findControlView];
    if (controlView == nil)
        return nil;
    
    return [controlView getGame].get();
}

- (RBX::GamepadService*) getGamepadServiceForGameDataModel
{
    if (RBX::Game* game = [self getGameFromControlView])
        if (shared_ptr<RBX::DataModel> currDataModel = game->getDataModel())
            return RBX::ServiceProvider::create<RBX::GamepadService>(currDataModel.get());
    
    return nil;
}

- (RBX::UserInputService*) getUserInputServiceForGameDataModel
{
    if (RBX::Game* game = [self getGameFromControlView])
        if (shared_ptr<RBX::DataModel> currDataModel = game->getDataModel())
            return RBX::ServiceProvider::find<RBX::UserInputService>(currDataModel.get());
    
    return nil;
}

- (RBX::TouchInputService*) getTouchInputServiceForGameDataModel
{
    if (RBX::Game* game = [self getGameFromControlView])
        if (shared_ptr<RBX::DataModel> currDataModel = game->getDataModel())
            return RBX::ServiceProvider::create<RBX::TouchInputService>(currDataModel.get());
    
    return nil;
}
@end
