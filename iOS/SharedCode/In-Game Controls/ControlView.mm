//
//  ControlView.m
//  IOSClient
//
//  Created by Ben Tkacheff on 8/21/12.
//  Copyright (c) 2012 tolkring@gmail.com. All rights reserved.
//
#import "ControlView.h"
#import "GameKeyboard.h"
#import "PlaceLauncher.h"
#include "ObjectiveCUtilities.h"
#import "RobloxNotifications.h"

@implementation ControlView

- (id) init:(CGRect)frame withGame:(boost::shared_ptr<RBX::Game>) newGame
{
    self = [super init];
    if (self)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(gotStartLeaveGameNotification:)
                                                     name:RBX_NOTIFY_GAME_START_LEAVING
                                                    object:nil ];
        game = newGame;
        [self setupEvents];
        
        // initial size has coordinates for height/width flipped, as it always returns portait mode, no matter our current orientation :(
        CGRect correctRect = CGRectMake(0,0,frame.size.height,frame.size.width);
        frameSize = RBX::Vector2int16(frame.size.height, frame.size.width);
        
        // self initialization
        self.multipleTouchEnabled = YES;
        self.frame = correctRect;
        
        [self setupInputControls];
        
        mouseMoveEvent = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_MOUSEMOVEMENT,
                                                                                                              RBX::InputObject::INPUT_STATE_CHANGE,
                                                                                                              RBX::Vector3(-1,-1,0),
                                                                                                              RBX::Vector3(0,0,0),
                                                                                                              [self getGame]->getDataModel().get());
        
        mouseButton1Event = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_MOUSEBUTTON1,
                                                                                                            RBX::InputObject::INPUT_STATE_BEGIN,
                                                                                                            RBX::Vector3(-1,-1,0),
                                                                                                            RBX::Vector3(0,0,0),
                                                                                                            [self getGame]->getDataModel().get());
    }
    
    return self;
}
-(void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [[GameKeyboard sharedInstance] setParentView:nil];
    
    rbxInputView = nil;
    
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
}

- (RbxInputView*) getRbxInputView
{
    return rbxInputView;
}

- (void) setGame:(boost::shared_ptr<RBX::Game>) newGame
{
    game = newGame;
    if(boost::shared_ptr<RBX::Game> sharedGame = game.lock())
    {
        [self dataModelChanged:sharedGame->getDataModel().get()];
    }
}

- (void) gotStartLeaveGameNotification:(NSNotification *)aNotification
{
    [self disconnectEvents];
}


-(void) dataModelChanged:(RBX::DataModel*) dataModel
{
    if(dataModel)
    {
        [self setupEvents];
        [self setupInputControls];
    }
    else // we have a null datamodel, tear down connections
        [self disconnectEvents];
}

- (void) postMouseEventProcessed:(bool) processedEvent inputObject: (void*) uiTouch event: (const shared_ptr<RBX::InputObject>&) event
{
    if (uiTouch && processedEvent)
    {
        void* tapTouchPtr = (__bridge void*) tapTouch;
        if( uiTouch && (uiTouch == tapTouchPtr) )
            [self invalidateTapGesture:nil];
    }
}

- (void) textBoxFocusGained:(boost::shared_ptr<RBX::TextBox>) textBoxFocused
{
    if(textBoxFocused != NULL && textBoxFocused != boost::shared_ptr<RBX::TextBox>())
        [[GameKeyboard sharedInstance] showKeyboardWithTextBox: textBoxFocused];
    else
        [[GameKeyboard sharedInstance] showKeyboard: ""];
}

- (void) textBoxFocusLost:(boost::shared_ptr<RBX::TextBox>) textBoxUnfocused
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [[GameKeyboard sharedInstance] hideKeyboard];
    });
}

- (boost::shared_ptr<RBX::Game>) getGame
{
    return game.lock();
}

- (void) setupEvents
{
    if(boost::shared_ptr<RBX::Game> sharedGame = game.lock())
    {
        [self bindToUserInputService:sharedGame->getDataModel()];
    }
}

- (void) disconnectEvents
{
    dmUserInputTextBoxFocusCon.disconnect();
    dmUserInputTextBoxReleaseFocusCon.disconnect();
    dmUserInputProcessMouseEventCon.disconnect();
}

- (void) bindToUserInputService:(shared_ptr<RBX::DataModel>) dataModel
{
    if( RBX::UserInputService* userInputService = RBX::ServiceProvider::find<RBX::UserInputService>(dataModel.get()) )
    {
        // code below will listen to UserInputService and detect when a textbox is in focus (so we can show the virtual keyboard)
        dmUserInputTextBoxFocusCon = userInputService->textBoxGainFocus.connect( boostFuncFromSelector_1< boost::shared_ptr<RBX::Instance> >
                                                                                 (@selector(textBoxFocusGained:),self) );
        
        dmUserInputTextBoxReleaseFocusCon = userInputService->textBoxReleaseFocus.connect( boostFuncFromSelector_1< boost::shared_ptr<RBX::Instance> >
                                                                                (@selector(textBoxFocusLost:),self) );
        
        // code below will listen to UserInputService when it fires a mouse event post event (bool tells whether the mouse event was used by app)
        dmUserInputProcessMouseEventCon = userInputService->processedMouseEvent.connect( boostFuncFromSelector_3<bool, void*, const shared_ptr<RBX::InputObject>& >
                                                      (@selector(postMouseEventProcessed:inputObject:event:),self) );
    }
}

- (void) setupInputControls
{
    // how quickly (in seconds) a user has to tap the screen to have a mouse down/up gesture sent
    tapSensitivity = 0.19f;
    // how much a tap can move in pixels on screen
    tapTouchMoveTolerance = 20;
     
    // Subview initialization
    CGRect correctRect = self.frame;
    
    if(rbxInputView != nil)
    {
        [rbxInputView removeFromSuperview];
        rbxInputView = nil;
    }
    
    rbxInputView = [[RbxInputView alloc] init:correctRect];
    [self addSubview:rbxInputView];
    [rbxInputView datamodelInit];

    [[GameKeyboard sharedInstance] setParentView:self];
}

- (void) invalidateTapGesture:(id) oldTapTouch
{
    if(oldTapTouch)
    {
        if( oldTapTouch == tapTouch )
            tapTouch = nil;
    }
    else
        tapTouch = nil;
}

-(UITouch*) checkTouchesForTap:(NSSet *)touches withEvent:(UIEvent *)event
{
    if(tapTouch)
    {
        for(UITouch* touch in touches)
        {
            if(touch == tapTouch)
            {
                UITouch* retainTapTouch = tapTouch;
                [self oneFingerSingleTap];
                return retainTapTouch;
            }
        }
    }
    
    return nil;
}


-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (!tapTouch && [touches count] == 1)
    {
        tapTouch = [touches anyObject];
        CGPoint startPoint = [tapTouch locationInView:self];
        tapTouchBeginPos = G3D::Vector2(startPoint.x,startPoint.y);
        [self performSelector:@selector(invalidateTapGesture:) withObject:[touches anyObject] afterDelay:tapSensitivity];
    }
    
    for (UITouch* touch in touches)
    {
        CGPoint currPoint = [touch locationInView:self];
        
        mouseButton1Event->setInputState(RBX::InputObject::INPUT_STATE_BEGIN);
        mouseButton1Event->setPosition(RBX::Vector3(currPoint.x,currPoint.y,0));
    }
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch* theTapTouch = [self checkTouchesForTap:touches withEvent:event];
    
    for(UITouch* touch in touches)
    {
        CGPoint currPoint = [touch locationInView:self];
        
        if(touch == theTapTouch)
            [self invalidateTapGesture:nil];
        else
        {
            mouseButton1Event->setInputState(RBX::InputObject::INPUT_STATE_END);
            mouseButton1Event->setPosition(RBX::Vector3(currPoint.x,currPoint.y,0));
        }
    }
}


-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self checkTapTouchMove:touches];
    
    for(UITouch* touch in touches)
    {
        CGPoint currPoint = [touch locationInView:self];
        
        mouseMoveEvent->setPosition(G3D::Vector3(currPoint.x, currPoint.y, 0));
    }
}

-(void) checkTapTouchMove:(NSSet*) touchesSet
{
    for(UITouch* touch in touchesSet)
    {
        if(tapTouch == touch)
        {
            CGPoint tapTouchLocationCGPoint = [tapTouch locationInView:self];
            RBX::Vector2 tapTouchLocation = RBX::Vector2(tapTouchLocationCGPoint.x,tapTouchLocationCGPoint.y);
            if((tapTouchLocation - tapTouchBeginPos).length() > tapTouchMoveTolerance)
                [self invalidateTapGesture:nil];
            
            break;
        }
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    for(UITouch* touch in touches)
    {
        if(touch == tapTouch)
        {
            tapTouch = nil;
            return;
        }
    }
}

- (void) oneFingerSingleTap
{
    if(RBX::UserInputService* userInputService = [self getUserInputServiceForGameDataModel])
    {
        CGPoint tapPoint = [tapTouch locationInView:self];
        tapTouch = nil;
        
        shared_ptr<RBX::InputObject> eventDown = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_MOUSEBUTTON1,
                                                                                                         RBX::InputObject::INPUT_STATE_BEGIN,
                                                                                                         RBX::Vector3(tapPoint.x,tapPoint.y,0),
                                                                                                         RBX::Vector3(0,0,0),
                                                                                                         [self getGame]->getDataModel().get());
        userInputService->processToolEvent(eventDown);
        
        shared_ptr<RBX::InputObject> eventUp = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_MOUSEBUTTON1,
                                                                                                       RBX::InputObject::INPUT_STATE_END,
                                                                                                       RBX::Vector3(tapPoint.x,tapPoint.y,0),
                                                                                                       RBX::Vector3(0,0,0),
                                                                                                       [self getGame]->getDataModel().get());
        userInputService->processToolEvent(eventUp);
    }
}


@end
