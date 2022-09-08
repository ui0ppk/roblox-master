#include "RobloxInput.h"
#include "PlaceLauncher.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/GamepadService.h"
#include "V8DataModel/UserInputService.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8datamodel/TouchInputService.h"
#include "Network/Players.h"

#define GRAVITY_ACCELERATION 9.80665f

// how quickly (in seconds) a user has to tap the screen to have a mouse down/up gesture sent
static float tapSensitivity = 0.25f;
// how much a tap can move in pixels on screen
static int tapTouchMoveTolerance = 20;

RobloxInput::RobloxInput()
{
	if (RBX::UserInputService* inputService = getInputService()) 
	{
		// code below will listen to UserInputService when it fires a mouse event post event (bool tells whether the mouse event was used by app)
		inputService->processedEventSignal.connect(boost::bind(&RobloxInput::postEventProcessed, this, _1, _2));

		inputService->updateInputSignal.connect(boost::bind(&RobloxInput::processControllerBufferMap, this));
		inputService->getSupportedGamepadKeyCodesSignal.connect(boost::bind(&RobloxInput::getSupportedGamepadKeyCodes, this, _1));
	}
	// init ptrs to the services
	getTouchService();
	getGamepadService();

	tapLocation = RBX::Vector3::zero();

	connectedControllerMap[RBX::InputObject::TYPE_GAMEPAD1] = false;
	connectedControllerMap[RBX::InputObject::TYPE_GAMEPAD2] = false;
	connectedControllerMap[RBX::InputObject::TYPE_GAMEPAD3] = false;
	connectedControllerMap[RBX::InputObject::TYPE_GAMEPAD4] = false;
}

RBX::DataModel* RobloxInput::getDataModel() 
{
    weak_ptr<RBX::Game> game = PlaceLauncher::getPlaceLauncher().getCurrentGame();
    if (shared_ptr<RBX::Game> sharedGame = game.lock())
    {
        if (RBX::DataModel* dm = sharedGame->getDataModel().get())
        {
            return dm;
        }
    }

	return NULL;
}

void RobloxInput::sendWorkspaceEvent(RBX::Vector3 touchPosition)
{
    if (RBX::UserInputService* inputService = getInputService())
    {
        shared_ptr<RBX::InputObject> fakeMouseDownEvent = RBX::Creatable<
        RBX::Instance>::create<RBX::InputObject>(
                                                 RBX::InputObject::TYPE_MOUSEBUTTON1,
                                                 RBX::InputObject::INPUT_STATE_BEGIN, touchPosition,
                                                 G3D::Vector3( 0.0f, 0.0f, 0.0f ),
                                                 getDataModel());
        inputService->processToolEvent(fakeMouseDownEvent);
        
        shared_ptr<RBX::InputObject> fakeMouseUpEvent = RBX::Creatable<
        RBX::Instance>::create<RBX::InputObject>(
                                                 RBX::InputObject::TYPE_MOUSEBUTTON1,
                                                 RBX::InputObject::INPUT_STATE_END, touchPosition,
                                                 G3D::Vector3( 0.0f, 0.0f, 0.0f ),
                                                 getDataModel());
        inputService->processToolEvent(fakeMouseUpEvent);
    }
}

void RobloxInput::sendWorkspaceEvent(shared_ptr<RBX::InputObject> inputObject)
{
    sendWorkspaceEvent(inputObject->getRawPosition());
}

RBX::GamepadService* RobloxInput::getGamepadService() 
{
    if (shared_ptr<RBX::GamepadService> gamepadService = weakGamepadService.lock())
    {
        return gamepadService.get();
    }
    else
    {
        if (RBX::DataModel* dm = getDataModel())
        {
            dm->submitTask([=](...)
            {
                if (RBX::GamepadService* gamepadService = RBX::ServiceProvider::create<RBX::GamepadService>(dm))
                {
                    weakGamepadService = weak_from(gamepadService);
                }
            }, RBX::DataModelJob::Write);

        }
	}

	return NULL;
}

RBX::UserInputService* RobloxInput::getInputService() 
{
	if (RBX::DataModel* dm = getDataModel()) 
	{
		if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<
				RBX::UserInputService>(dm))
		{
			return inputService;
		}
	}

	return NULL;
}

RBX::TouchInputService* RobloxInput::getTouchService()
{
    if (shared_ptr<RBX::TouchInputService> touchService = weakTouchInputService.lock())
    {
        return touchService.get();
    }
    else
    {
        if (RBX::DataModel* dm = getDataModel())
        {
            dm->submitTask([=](...)
            {
                if (RBX::TouchInputService* touchService = RBX::ServiceProvider::create<RBX::TouchInputService>(dm))
                {
                    weakTouchInputService = weak_from(touchService);
                }
            }, RBX::DataModelJob::Write);
        }
    }
    
    return NULL;
}

void RobloxInput::postEventProcessed(const shared_ptr<RBX::Instance>& event, bool processedEvent) 
{
	if (processedEvent)
	{
		if (RBX::InputObject* inputObject = RBX::Instance::fastDynamicCast<RBX::InputObject>(event.get()))
		{
			if (inputObject->isTouchEvent() && inputObject->getRawPosition() == tapLocation)
			{
				tapEventId = -1;
				tapLocation = RBX::Vector3::zero();
			}
		}
	}
}

void RobloxInput::passTextInput(RBX::TextBox* textBox, std::string newText, bool enterPressed, int cursorPosition)
{
	if (enterPressed)
	{
		if (RBX::UserInputService* userInputService = getInputService())
		{
			if (!userInputService->showStatsBasedOnInputString(newText.c_str())) 
			{
				userInputService->textboxDidFinishEditing(newText.c_str(), true);
			}
		}
	}
	else
	{
		if (textBox)
		{
			textBox->setBufferedText(newText, cursorPosition);
		}
	}
}

void RobloxInput::externalReleaseFocus(RBX::TextBox* currentTextBox) 
{
	if (currentTextBox)
		currentTextBox->externalReleaseFocus(currentTextBox->getText().c_str(), false, shared_ptr<RBX::InputObject>());
}

void RobloxInput::sendGestureEvent(RBX::UserInputService::Gesture gestureToSend,
		shared_ptr<RBX::Reflection::Tuple> args,
		shared_ptr<RBX::Reflection::ValueArray> touchLocations) 
{
	if (RBX::UserInputService* userInputService = getInputService())
		userInputService->addGestureEventToProcess(gestureToSend, touchLocations, args);
}

RBX::KeyCode getKeyCodeFromAndroidAxisInt(int axisKeyCode, int value = -1)
{
	switch(axisKeyCode)
	{
		case 22: 
		case 18:
		{
			return RBX::SDLK_GAMEPAD_BUTTONR2;
		}
		case 23: 
		case 17:
		{
			return RBX::SDLK_GAMEPAD_BUTTONL2;
		}
		case 0:
		case 1:
		{
			return RBX::SDLK_GAMEPAD_THUMBSTICK1;
		}
		case 11:
		case 14:
		{
			return RBX::SDLK_GAMEPAD_THUMBSTICK2;
		}
		case 15:
		{
			if (value < 0)
				return RBX::SDLK_GAMEPAD_DPADLEFT;
			else
				return RBX::SDLK_GAMEPAD_DPADRIGHT;
		}
		case 16:
		{
			if (value < 0)
				return RBX::SDLK_GAMEPAD_DPADDOWN;
			else
				return RBX::SDLK_GAMEPAD_DPADUP;
		}

		default: 
		{
			return RBX::SDLK_UNKNOWN;
		}
	}
}

RBX::KeyCode getKeyCodeFromAndroidButtonInt(int androidKeycode)
{
	switch (androidKeycode)
	{
		case 96: return RBX::SDLK_GAMEPAD_BUTTONA;
		case 97: return RBX::SDLK_GAMEPAD_BUTTONB;
		case 102: return RBX::SDLK_GAMEPAD_BUTTONL1;
		case 104: return RBX::SDLK_GAMEPAD_BUTTONL2;
		case 103: return RBX::SDLK_GAMEPAD_BUTTONR1;
		case 105: return RBX::SDLK_GAMEPAD_BUTTONR2;
		case 109: return RBX::SDLK_GAMEPAD_BUTTONSELECT;
		case 108: return RBX::SDLK_GAMEPAD_BUTTONSTART;
		case 106: return RBX::SDLK_GAMEPAD_BUTTONL3;
		case 107: return RBX::SDLK_GAMEPAD_BUTTONR3;
		case 99: return RBX::SDLK_GAMEPAD_BUTTONX;
		case 100: return RBX::SDLK_GAMEPAD_BUTTONY;
		case 20: return RBX::SDLK_GAMEPAD_DPADDOWN;
		case 21: return RBX::SDLK_GAMEPAD_DPADLEFT;
		case 22: return RBX::SDLK_GAMEPAD_DPADRIGHT;
		case 19: return RBX::SDLK_GAMEPAD_DPADUP;

		default: return RBX::SDLK_UNKNOWN;
	}
}

void RobloxInput::handleGamepadButtonInput(int deviceId, int keyCode, int buttonState)
{
	if (deviceIdToGamepadId.find(deviceId) == deviceIdToGamepadId.end())
	{
		return;
	}
	
	const RBX::InputObject::UserInputType gamepadEnum = deviceIdToGamepadId[deviceId];
	const RBX::KeyCode rbxKeycode = getKeyCodeFromAndroidButtonInt(keyCode);

	{
		boost::mutex::scoped_lock mutex(ControllerBufferMutex);
		RBX::Vector3 newValue(0,0,buttonState);

		if (controllerBufferMap[gamepadEnum][rbxKeycode].empty() ||
				controllerBufferMap[gamepadEnum][rbxKeycode].back() != newValue)
		{
			
			controllerBufferMap[gamepadEnum][rbxKeycode].push_back(newValue);
		}
	}
}

void RobloxInput::handleGamepadAxisInput(int deviceId, int actionType, float newValueX, float newValueY, float newValueZ)
{
	if (deviceIdToGamepadId.find(deviceId) == deviceIdToGamepadId.end())
	{
		return;
	}


	if (RBX::GamepadService* gamepadService = getGamepadService())
	{
		const RBX::KeyCode rbxKeycode = getKeyCodeFromAndroidAxisInt(actionType, newValueZ);
		const RBX::InputObject::UserInputType gamepadEnum = deviceIdToGamepadId[deviceId];
		
		if (rbxKeycode == RBX::SDLK_GAMEPAD_DPADUP ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADDOWN ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADRIGHT ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADLEFT)
		{
			newValueZ =  fabs(newValueZ);
		}

		if (rbxKeycode == RBX::SDLK_GAMEPAD_BUTTONR2 || rbxKeycode == RBX::SDLK_GAMEPAD_BUTTONL2 ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADUP ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADDOWN ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADRIGHT ||
			rbxKeycode == RBX::SDLK_GAMEPAD_DPADLEFT)
		{

			{
				boost::mutex::scoped_lock mutex(ControllerBufferMutex);
				RBX::Vector3 newValue(0,0,newValueZ);

				if (controllerBufferMap[gamepadEnum][rbxKeycode].empty() ||
					controllerBufferMap[gamepadEnum][rbxKeycode].back() != newValue)
				{
					controllerBufferMap[gamepadEnum][rbxKeycode].push_back(newValue);

					// since android combines up/down, left/right dpad as axis, but we
					// use these as button values, make sure both dpad directions get their
					// state returned to netural when we get a zero value
					if (newValue.z == 0.0f)
					{
						if (rbxKeycode == RBX::SDLK_GAMEPAD_DPADUP)
						{
							controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADDOWN].push_back(newValue);
						}
						else if (rbxKeycode == RBX::SDLK_GAMEPAD_DPADRIGHT)
						{
							controllerBufferMap[gamepadEnum][RBX::SDLK_GAMEPAD_DPADLEFT].push_back(newValue);
						}
					}
				}
			}
		}
		else
		{
			{
				boost::mutex::scoped_lock mutex(ControllerBufferMutex);
				RBX::Vector3 newVector(newValueX,newValueY,0);

				if (controllerBufferMap[gamepadEnum][rbxKeycode].empty() ||
					controllerBufferMap[gamepadEnum][rbxKeycode].back() != newVector)
				{
					controllerBufferMap[gamepadEnum][rbxKeycode].push_back(newVector);
				}
			}
		}
	}
}

RBX::InputObject::UserInputType RobloxInput::mapDeviceIdToControllerEnum(int deviceId)
{
	RBX::InputObject::UserInputType controllerNum = RBX::InputObject::TYPE_NONE;

	if (deviceIdToGamepadId.find(deviceId) != deviceIdToGamepadId.end())
	{
		controllerNum = deviceIdToGamepadId[deviceId];
		connectedControllerMap[controllerNum] = true;
		
		return controllerNum;
	}

	for (ConnectedControllerMap::iterator iter = connectedControllerMap.begin(); iter != connectedControllerMap.end(); iter++)
	{
		if ((*iter).second == false)
		{
			controllerNum = (*iter).first;
			break;
		}
	}

	connectedControllerMap[controllerNum] = true;
	deviceIdToGamepadId[deviceId] = controllerNum;

	return controllerNum;
}

void RobloxInput::handleGamepadDisconnect(int deviceId)
{
	if (deviceIdToGamepadId.find(deviceId) != deviceIdToGamepadId.end())
	{
		if (RBX::UserInputService* userInputService = getInputService())
		{
			RBX::InputObject::UserInputType controllerNum = deviceIdToGamepadId[deviceId];
			userInputService->safeFireGamepadDisconnected(controllerNum);

			connectedControllerMap[controllerNum] = false;
			deviceIdToGamepadId.erase(deviceId);
		}

	}
}

void RobloxInput::handleGamepadConnect(int deviceId)
{
	RBX::InputObject::UserInputType controllerNum = mapDeviceIdToControllerEnum(deviceId);

	if (RBX::UserInputService* userInputService = getInputService())
	{
		userInputService->safeFireGamepadConnected(controllerNum);
	}
}

void RobloxInput::handleGamepadKeyCodeSupportChanged(int deviceId, int keyCode, bool supported)
{		           
	RBX::InputObject::UserInputType gamepadEnum = mapDeviceIdToControllerEnum(deviceId);         	
	RBX::KeyCode rbxKeycode = getKeyCodeFromAndroidButtonInt(keyCode);

	if (rbxKeycode == RBX::SDLK_UNKNOWN)
	{
		rbxKeycode = getKeyCodeFromAndroidAxisInt(keyCode);
	}
    
	if (gamepadEnum != RBX::InputObject::TYPE_NONE)
	{
		boost::mutex::scoped_lock mutex(SupportedControllerKeyCodeMutex);

		if (gamepadSupportedKeyCodes[gamepadEnum].get() == NULL)
		{
			gamepadSupportedKeyCodes[gamepadEnum].reset(new RBX::Reflection::ValueArray());
		}

		shared_ptr<RBX::Reflection::ValueArray> supportedKeyCodes = gamepadSupportedKeyCodes[gamepadEnum];

		if (!supported)
		{
			for (RBX::Reflection::ValueArray::iterator iter = supportedKeyCodes->begin(); iter != supportedKeyCodes->end(); ++iter)
			{
				const RBX::Reflection::Variant iterVariant = (*iter);

				if (iterVariant.isType<RBX::KeyCode>())
				{
					RBX::KeyCode iterKeyCode = iterVariant.cast<RBX::KeyCode>();

					if (iterKeyCode == rbxKeycode)
					{
						supportedKeyCodes->erase(iter);
						break;
					}
				}
			}
		}
		else
		{
			supportedKeyCodes->push_back(rbxKeycode);
		}

		// this line is probably not necessary
		gamepadSupportedKeyCodes[gamepadEnum] = supportedKeyCodes;
	}
}

void RobloxInput::getSupportedGamepadKeyCodes(RBX::InputObject::UserInputType gamepadEnum)
{
    if (RBX::UserInputService* inputService = getInputService())
    {
    	boost::mutex::scoped_lock mutex(SupportedControllerKeyCodeMutex);
        inputService->setSupportedGamepadKeyCodes(gamepadEnum, gamepadSupportedKeyCodes[gamepadEnum]);
    }
}

// this is a thread safe way to post input to DataModel (we should be in RenderStep)
void RobloxInput::processControllerBufferMap()
{
    BufferedGamepadStates tempControllerBufferMap;
    {
        // grab our current input and process
        // need a mutex in case Android is simultaneously trying to update map
        boost::mutex::scoped_lock mutex(ControllerBufferMutex);
        controllerBufferMap.swap(tempControllerBufferMap);
    }

    for (int gamepadNum = RBX::InputObject::TYPE_GAMEPAD1; gamepadNum != (RBX::InputObject::TYPE_GAMEPAD4 + 1); ++gamepadNum)
    {
        RBX::InputObject::UserInputType gamepadEnum = (RBX::InputObject::UserInputType) gamepadNum;
        
        BufferedGamepadState gamepadBufferState = tempControllerBufferMap[gamepadEnum];

        if (RBX::GamepadService* gamepadService = getGamepadService())
        {
        	RBX::Gamepad rbxGamepad = gamepadService->getGamepadState(RBX::GamepadService::getGamepadIntForEnum(gamepadEnum));
        
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


	                if (state != RBX::InputObject::INPUT_STATE_NONE)
	                {
		                RBX::Vector3 newPosition = (*vecIter);
		                if (isButton) // don't use pressure sensitive values to keep consistent behavior on all platforms
		                {
		                    (newPosition.z > 0.0f) ? newPosition.z = 1.0f : newPosition.z = 0.0f;
		                }
		                
		                bool shouldFireEvent = true;
		                
		                if (keyInputObject->getRawPosition() != newPosition)
		                {
		                    keyInputObject->setDelta((newPosition - keyInputObject->getRawPosition()));
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
		                    if (RBX::UserInputService* inputService = getInputService())
		                    {
		                        inputService->dangerousFireInputEvent(keyInputObject, NULL);
		                    }
		                }
	            	}
	            }
	        }
    	}
    }
}

void RobloxInput::processEvent(int eventId, const int xPos, const int yPos,
		const int eventType, const float winSizeX, const float winSizeY) 
{
    if(RBX::TouchInputService* touchInputService = getTouchService())
    {
        RBX::Vector3 rbxLocation = RBX::Vector3(xPos, yPos, 0);

        RBX::InputObject::UserInputState newState = RBX::InputObject::INPUT_STATE_NONE;
        switch (eventType)
        {
            case 0:
                newState = RBX::InputObject::INPUT_STATE_BEGIN;
                if (tapEventId < 0)
                {
                    tapEventId = eventId;
                    tapTouchBeginPos = RBX::Vector2(xPos, yPos);
                    tapLocation = RBX::Vector3(tapTouchBeginPos, 0);
                    tapTimer.reset();
                }
                break;
            case 1:
                newState = RBX::InputObject::INPUT_STATE_CHANGE;

                if (tapEventId == eventId)
                {
                    if ((RBX::Vector2(xPos, yPos) - tapTouchBeginPos).length() > tapTouchMoveTolerance)
                    {
                        tapEventId = -1;
                        tapLocation = RBX::Vector3::zero();
                    }
                }
                break;
            case 2:
                newState = RBX::InputObject::INPUT_STATE_END;
                if (tapEventId == eventId)
                {
                    if (tapTimer.delta().seconds() <= tapSensitivity)
                    {
                        sendWorkspaceEvent(rbxLocation);
                    }
                    tapEventId = -1;
                    tapLocation = RBX::Vector3::zero();
                }
                break;
        }

        touchInputService->addTouchToBuffer((void*) eventId, rbxLocation, newState);
    }
}

void RobloxInput::sendAccelerometerEvent(float x,float y, float z)
{
	if (RBX::UserInputService* inputService = getInputService())
	{
		 // Measure in g's instead of m/s^2 to be consistent with iOS
		inputService->fireAccelerationEvent( (RBX::Vector3(x, y, z)/GRAVITY_ACCELERATION) );
	}
}

void RobloxInput::sendGravityEvent(float x,float y, float z)
{
	if (RBX::UserInputService* inputService = getInputService())
	{
		 // Measure in g's instead of m/s^2 to be consistent with iOS
		inputService->fireGravityEvent( (RBX::Vector3(x, y, z)/GRAVITY_ACCELERATION));
	}
}

void RobloxInput::sendGyroscopeEvent(float eulerX,float eulerY,float eulerZ,
		float quaternionX, float quaternionY, float quaternionZ, float quaternionW)
{
	if (RBX::UserInputService* inputService = getInputService())
	{
		inputService->fireRotationEvent( RBX::Vector3(eulerX,eulerY,eulerZ), RBX::Vector4(quaternionX,quaternionY,quaternionZ,quaternionW) );
	}
}

void RobloxInput::setAccelerometerEnabled(bool enabled)
{
    if (RBX::DataModel* dm = getDataModel())
    {
        dm->submitTask([=](...){
                if (RBX::UserInputService* inputService = getInputService())
                {
                    inputService->setAccelerometerEnabled(enabled);
                }
        }, RBX::DataModelJob::Write);
    }
}

void RobloxInput::setGyroscopeEnabled(bool enabled)
{
    if (RBX::DataModel* dm = getDataModel())
    {
        dm->submitTask([=](...){
            	if (RBX::UserInputService* inputService = getInputService())
            	{
            		inputService->setGyroscopeEnabled(enabled);
            	}
        }, RBX::DataModelJob::Write);
    }
}
