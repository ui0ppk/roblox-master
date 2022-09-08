#include "stdafx.h"
#include "SDLGameController.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/GamepadService.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/ContentProvider.h"

#define MAX_AXIS_VALUE 32767.0f

SDLGameController::SDLGameController(shared_ptr<RBX::DataModel> newDM)
{
	dataModel = newDM;
    initSDL();
}

void SDLGameController::initSDL()
{
	if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) != 0)
{	
		std::string error = SDL_GetError();
		fprintf(stderr,
			"\nUnable to initialize SDL:  %s\n",
			error.c_str()
			);
		return;
	}

	RBX::ContentId gameControllerDb = RBX::ContentId::fromAssets("fonts/gamecontrollerdb.txt");
	std::string filePath = RBX::ContentProvider::findAsset(gameControllerDb);

	if (SDL_GameControllerAddMappingsFromFile(filePath.c_str()) == -1)
	{
		std::string error = SDL_GetError();
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Unable to add SDL controller mappings because %s", error.c_str());
	}

	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		sharedDM->submitTask(boost::bind(&SDLGameController::bindToDataModel, this), RBX::DataModelJob::Write);
	}
}

void SDLGameController::bindToDataModel()
{
	if (RBX::UserInputService* inputService = getUserInputService())
	{
		renderSteppedConnection = inputService->updateInputSignal.connect(boost::bind(&SDLGameController::updateControllers, this));
		getSupportedGamepadKeyCodesConnection = inputService->getSupportedGamepadKeyCodesSignal.connect(boost::bind(&SDLGameController::findAvailableGamepadKeyCodesAndSet, this, _1));
	}

	if (RBX::HapticService* hapticService = getHapticService())
	{
		setEnabledVibrationMotorsConnection = hapticService->setEnabledVibrationMotorsSignal.connect(boost::bind(&SDLGameController::setVibrationMotorsEnabled, this, _1));
		setVibrationMotorConnection = hapticService->setVibrationMotorSignal.connect(boost::bind(&SDLGameController::setVibrationMotor, this, _1, _2, _3));
	}
}

SDLGameController::~SDLGameController()
{
	renderSteppedConnection.disconnect();
	getSupportedGamepadKeyCodesConnection.disconnect();

	setEnabledVibrationMotorsConnection.disconnect();
	setVibrationMotorConnection.disconnect();

	for (boost::unordered_map<int, HapticData>::iterator iter = hapticsFromGamepadId.begin(); iter != hapticsFromGamepadId.end(); ++iter )
	{
		SDL_Haptic* haptic = iter->second.hapticDevice;
		int hapticEffectId = iter->second.hapticEffectId;

		SDL_HapticDestroyEffect(haptic, hapticEffectId);
		SDL_HapticClose(haptic);
	}
	hapticsFromGamepadId.clear();

    SDL_Quit();
}

RBX::UserInputService* SDLGameController::getUserInputService()
{
	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		if (RBX::UserInputService* inputService = RBX::ServiceProvider::create<RBX::UserInputService>(sharedDM.get()))
		{
			return inputService;
		}
	}

	return NULL;
}

RBX::HapticService* SDLGameController::getHapticService()
{
	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		if (RBX::HapticService* hapticService = RBX::ServiceProvider::create<RBX::HapticService>(sharedDM.get()))
		{
			return hapticService;
		}
	}

	return NULL;
}

RBX::GamepadService* SDLGameController::getGamepadService()
{
	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		if (RBX::GamepadService* gamepadService = RBX::ServiceProvider::create<RBX::GamepadService>(sharedDM.get()))
		{
			return gamepadService;
		}
	}

	return NULL;
}

SDL_GameController* SDLGameController::removeControllerMapping(int joystickId)
{
	SDL_GameController* gameController = NULL;
	RBX::UserInputService* inputService = getUserInputService();

	if (joystickIdToGamepadId.find(joystickId) != joystickIdToGamepadId.end())
	{
		int gamepadId = joystickIdToGamepadId[joystickId];
		if (gamepadIdToGameController.find(gamepadId) != gamepadIdToGameController.end())
		{
			gameController = gamepadIdToGameController[gamepadId].second;
			gamepadIdToGameController.erase(gamepadId);

			if (inputService)
			{
                inputService->safeFireGamepadDisconnected(RBX::GamepadService::getGamepadEnumForInt(gamepadId));
			}
		}

		if (hapticsFromGamepadId.find(gamepadId) != hapticsFromGamepadId.end())
		{
			SDL_Haptic* haptic = hapticsFromGamepadId[gamepadId].hapticDevice;
			int hapticEffectId = hapticsFromGamepadId[gamepadId].hapticEffectId;

			SDL_HapticDestroyEffect(haptic, hapticEffectId);
			SDL_HapticClose(haptic);

			hapticsFromGamepadId.erase(gamepadId);
		}
	}
	
	return gameController;
}

void SDLGameController::setupControllerId(int joystickId, int gamepadId, SDL_GameController *pad)
{
	gamepadIdToGameController[gamepadId] = std::pair<int,SDL_GameController*>(joystickId, pad);
	joystickIdToGamepadId[joystickId] = gamepadId;

	if (RBX::UserInputService* inputService = getUserInputService())
	{
        inputService->safeFireGamepadConnected(RBX::GamepadService::getGamepadEnumForInt(gamepadId));
	}
}

void SDLGameController::addController(int gamepadId)
{
	if ( SDL_IsGameController(gamepadId) ) 
	{
		SDL_GameController *pad = SDL_GameControllerOpen(gamepadId);

		if (pad)
		{
			SDL_Joystick *joy = SDL_GameControllerGetJoystick(pad);
			int joystickId = SDL_JoystickInstanceID(joy);

			setupControllerId(joystickId, gamepadId, pad);
		}
	}
}

void SDLGameController::removeController(int joystickId)
{
	if (SDL_GameController *pad = removeControllerMapping(joystickId))
	{
		SDL_GameControllerClose(pad);
	}
}

RBX::Gamepad SDLGameController::getRbxGamepadFromJoystickId(int joystickId)
{
	if (joystickIdToGamepadId.find(joystickId) != joystickIdToGamepadId.end())
	{
		if (RBX::GamepadService* gamepadService = getGamepadService())
		{
			int gamepadId = joystickIdToGamepadId[joystickId];
			return gamepadService->getGamepadState(gamepadId);
		}
	}

	return RBX::Gamepad();
}

RBX::KeyCode getKeyCodeFromSDLAxis(SDL_GameControllerAxis sdlAxis, int& axisValueChanged)
{
	switch (sdlAxis)
	{
	case SDL_CONTROLLER_AXIS_LEFTX:
		axisValueChanged = 0;
		return RBX::SDLK_GAMEPAD_THUMBSTICK1;
	case SDL_CONTROLLER_AXIS_LEFTY:
		axisValueChanged = 1;
		return RBX::SDLK_GAMEPAD_THUMBSTICK1;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		axisValueChanged = 0;
		return RBX::SDLK_GAMEPAD_THUMBSTICK2;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		axisValueChanged = 1;
		return RBX::SDLK_GAMEPAD_THUMBSTICK2;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		axisValueChanged = 2;
		return RBX::SDLK_GAMEPAD_BUTTONL2;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		axisValueChanged = 2;
		return RBX::SDLK_GAMEPAD_BUTTONR2;
    case SDL_CONTROLLER_AXIS_INVALID:
    case SDL_CONTROLLER_AXIS_MAX:
        return RBX::SDLK_UNKNOWN;
	}

	return RBX::SDLK_UNKNOWN;
}

RBX::KeyCode getKeyCodeFromSDLButton(SDL_GameControllerButton sdlButton)
{
	switch (sdlButton)
	{
	case SDL_CONTROLLER_BUTTON_A:
		return RBX::SDLK_GAMEPAD_BUTTONA;
	case SDL_CONTROLLER_BUTTON_B:
		return RBX::SDLK_GAMEPAD_BUTTONB;
	case SDL_CONTROLLER_BUTTON_X:
		return RBX::SDLK_GAMEPAD_BUTTONX;
	case SDL_CONTROLLER_BUTTON_Y:
		return RBX::SDLK_GAMEPAD_BUTTONY;

	case SDL_CONTROLLER_BUTTON_START:
		return RBX::SDLK_GAMEPAD_BUTTONSTART;
	case SDL_CONTROLLER_BUTTON_BACK:
		return RBX::SDLK_GAMEPAD_BUTTONSELECT;

	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return RBX::SDLK_GAMEPAD_DPADUP;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return RBX::SDLK_GAMEPAD_DPADDOWN;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return RBX::SDLK_GAMEPAD_DPADLEFT;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return RBX::SDLK_GAMEPAD_DPADRIGHT;

	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		return RBX::SDLK_GAMEPAD_BUTTONL1;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		return RBX::SDLK_GAMEPAD_BUTTONR1;

	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		return RBX::SDLK_GAMEPAD_BUTTONL3;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		return RBX::SDLK_GAMEPAD_BUTTONR3;
            
    case SDL_CONTROLLER_BUTTON_INVALID:
    case SDL_CONTROLLER_BUTTON_GUIDE:
    case SDL_CONTROLLER_BUTTON_MAX:
        return RBX::SDLK_UNKNOWN;
	}

	return RBX::SDLK_UNKNOWN;
}

bool SDLGameController::setupHapticsForDevice(int id)
{
	// already set up
	if (hapticsFromGamepadId.find(id) != hapticsFromGamepadId.end())
	{
		return true;
	}

	SDL_Haptic *haptic = NULL;

	// Open the device
	haptic = SDL_HapticOpen(id);
	if (haptic)
	{
		HapticData hapticData;
		hapticData.hapticDevice = haptic;
		hapticData.hapticEffectId = -1;
		hapticData.currentLeftMotorValue = 0.0f;
		hapticData.currentRightMotorValue = 0.0f;

		hapticsFromGamepadId[id] = hapticData;

		return true;
	}

	return false;
}

void SDLGameController::setVibrationMotorsEnabled(RBX::InputObject::UserInputType gamepadType)
{
	int gamepadId = getGamepadIntForEnum(gamepadType);
	if (!setupHapticsForDevice(gamepadId))
	{
		return;
	}

	SDL_Haptic* haptic = hapticsFromGamepadId[gamepadId].hapticDevice;
	if (haptic)
	{
		if (RBX::HapticService* hapticService = getHapticService())
		{
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_LARGE, true);
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_SMALL, true);
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_LEFTTRIGGER, false);
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_RIGHTTRIGGER, false);
		}
	}
}

void SDLGameController::setVibrationMotor(RBX::InputObject::UserInputType gamepadType, RBX::HapticService::VibrationMotor vibrationMotor, shared_ptr<const RBX::Reflection::Tuple> args)
{
	int gamepadId = getGamepadIntForEnum(gamepadType);
	if (!setupHapticsForDevice(gamepadId))
	{
		return;
	}

	float newMotorValue = 0.0f;
	RBX::Reflection::Variant newValue = args->values[0];
	if (newValue.isFloat())
	{
		newMotorValue = newValue.get<float>();
		newMotorValue = G3D::clamp(newMotorValue, 0.0f, 1.0f);
	}
	else // no valid number in first position, lets bail
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "First value to HapticService:SetMotor is not a valid number (must be a number between 0-1)");
		return;
	}

	boost::unordered_map<int, HapticData>::iterator iter = hapticsFromGamepadId.find(gamepadId);

	// make sure we grab old data so we set the motors that haven't changed value
	float leftMotorValue = iter->second.currentLeftMotorValue;
	float rightMotorValue = iter->second.currentRightMotorValue;

	if (vibrationMotor == RBX::HapticService::MOTOR_LARGE)
	{
		leftMotorValue = newMotorValue;
	}
	else if (vibrationMotor == RBX::HapticService::MOTOR_SMALL)
	{
		rightMotorValue = newMotorValue;
	}

	SDL_Haptic* haptic = iter->second.hapticDevice;
	int oldEffectId = iter->second.hapticEffectId;
	if (oldEffectId >= 0)
	{
		SDL_HapticDestroyEffect(haptic, oldEffectId);
	}

	if (leftMotorValue <= 0.0f && rightMotorValue <= 0.0f)
	{
		HapticData hapticData;
		hapticData.hapticDevice = haptic;
		hapticData.hapticEffectId = -1;
		hapticData.currentLeftMotorValue = 0.0f;
		hapticData.currentRightMotorValue = 0.0f;

		hapticsFromGamepadId[gamepadId] = hapticData;
		return;
	}

	// Create the left/right effect
	SDL_HapticEffect effect;
	memset( &effect, 0, sizeof(SDL_HapticEffect) ); // 0 is safe default
	effect.type = SDL_HAPTIC_LEFTRIGHT;
	effect.leftright.large_magnitude = 65535.0f * leftMotorValue;
	effect.leftright.small_magnitude = 65535.0f * rightMotorValue;
	effect.leftright.length = SDL_HAPTIC_INFINITY;

	// Upload the effect
	int hapticEffectId = SDL_HapticNewEffect(haptic, &effect);

	HapticData hapticData;
	hapticData.hapticDevice = haptic;
	hapticData.hapticEffectId = hapticEffectId;
	hapticData.currentLeftMotorValue = leftMotorValue;
	hapticData.currentRightMotorValue = rightMotorValue;

	hapticsFromGamepadId[gamepadId] = hapticData;

	if (haptic && hapticEffectId >= 0)
	{
		SDL_HapticRunEffect(haptic, hapticEffectId, SDL_HAPTIC_INFINITY);
	}
}

void SDLGameController::refreshHapticEffects()
{
	for (boost::unordered_map<int, HapticData>::iterator iter = hapticsFromGamepadId.begin(); iter != hapticsFromGamepadId.end(); ++iter )
	{
		SDL_Haptic* haptic = iter->second.hapticDevice;
		int hapticEffectId = iter->second.hapticEffectId;

		if (haptic && hapticEffectId >= 0)
		{
			SDL_HapticRunEffect(haptic, hapticEffectId, SDL_HAPTIC_INFINITY);
		}
	}
}

void SDLGameController::onControllerButton( const SDL_ControllerButtonEvent sdlEvent )
{
	const RBX::KeyCode buttonCode = getKeyCodeFromSDLButton((SDL_GameControllerButton) sdlEvent.button);

	if (buttonCode == RBX::SDLK_UNKNOWN)
	{
		return;
	}

	RBX::Gamepad gamepad = getRbxGamepadFromJoystickId(sdlEvent.which);
	const int buttonState = (sdlEvent.type == SDL_CONTROLLERBUTTONDOWN) ? 1 : 0;
	RBX::InputObject::UserInputState newState = (buttonState == 1) ? RBX::InputObject::INPUT_STATE_BEGIN : RBX::InputObject::INPUT_STATE_END;

	if (newState == gamepad[buttonCode]->getUserInputState())
	{
		return;
	}

	const G3D::Vector3 lastPos = gamepad[buttonCode]->getPosition();

	gamepad[buttonCode]->setPosition(G3D::Vector3(0,0,buttonState));
	gamepad[buttonCode]->setDelta(gamepad[buttonCode]->getPosition() - lastPos);
	gamepad[buttonCode]->setInputState(newState);

	if (RBX::UserInputService* inputService = getUserInputService())
	{
        inputService->dangerousFireInputEvent(gamepad[buttonCode], NULL);
	}
}

void SDLGameController::onControllerAxis( const SDL_ControllerAxisEvent sdlEvent )
{
	int axisValueChanged = -1;
	const RBX::KeyCode axisCode = getKeyCodeFromSDLAxis((SDL_GameControllerAxis) sdlEvent.axis, axisValueChanged);

	if (axisCode == RBX::SDLK_UNKNOWN)
	{
		return;
	}

	float axisValue = sdlEvent.value;
	axisValue /= MAX_AXIS_VALUE;
	axisValue = G3D::clamp(axisValue, -1.0f, 1.0f);

	RBX::Gamepad gamepad = getRbxGamepadFromJoystickId(sdlEvent.which);
	G3D::Vector3 currentPosition = gamepad[axisCode]->getPosition();

	switch (axisValueChanged)
	{
	case 0:
		currentPosition.x = axisValue;
		break;
	case 1:
		currentPosition.y = -axisValue;
		break;
	case 2:
		currentPosition.z = axisValue;
		break;
	default:
		break;
	}

	G3D::Vector3 lastPos = gamepad[axisCode]->getPosition();
	if (lastPos != currentPosition)
	{
		gamepad[axisCode]->setPosition(currentPosition);

		RBX::InputObject::UserInputState currentState = RBX::InputObject::INPUT_STATE_CHANGE;
		if (currentPosition == G3D::Vector3::zero())
		{
			currentState = RBX::InputObject::INPUT_STATE_END;
		}
		else if (currentPosition.z >= 1.0f)
		{
			currentState = RBX::InputObject::INPUT_STATE_BEGIN;
		}

		gamepad[axisCode]->setDelta(currentPosition - lastPos);
		gamepad[axisCode]->setInputState(currentState);

		if (RBX::UserInputService* inputService = getUserInputService())
		{
            inputService->dangerousFireInputEvent(gamepad[axisCode], NULL);
		}
	}
}

void SDLGameController::updateControllers()
{
	SDL_Event sdlEvent;

	while( SDL_PollEvent( &sdlEvent ) ) 
	{
		switch( sdlEvent.type ) 
		{
		case SDL_CONTROLLERDEVICEADDED:
			addController( sdlEvent.cdevice.which );
			break;

		case SDL_CONTROLLERDEVICEREMOVED:
			removeController( sdlEvent.cdevice.which );
			break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			onControllerButton( sdlEvent.cbutton );
			break;

		case SDL_CONTROLLERAXISMOTION:
			onControllerAxis( sdlEvent.caxis );
			break;

		default:
			break;
		}
	}

	refreshHapticEffects();
}

RBX::KeyCode getKeyCodeFromSDLName(std::string sdlName)
{
	if (sdlName.compare("a") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONA;
	if (sdlName.compare("b") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONB;
	if (sdlName.compare("x") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONX;
	if (sdlName.compare("y") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONY;

	if (sdlName.compare("back") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONSELECT;
	if (sdlName.compare("start") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONSTART;

	if (sdlName.compare("dpdown") == 0)
		return RBX::SDLK_GAMEPAD_DPADDOWN;
	if (sdlName.compare("dpleft") == 0)
		return RBX::SDLK_GAMEPAD_DPADLEFT;
	if (sdlName.compare("dpright") == 0)
		return RBX::SDLK_GAMEPAD_DPADRIGHT;
	if (sdlName.compare("dpup") == 0)
		return RBX::SDLK_GAMEPAD_DPADUP;

	if (sdlName.compare("leftshoulder") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONL1;
	if (sdlName.compare("lefttrigger") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONL2;
	if (sdlName.compare("leftstick") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONL3;

	if (sdlName.compare("rightshoulder") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONR1;
	if (sdlName.compare("righttrigger") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONR2;
	if (sdlName.compare("rightstick") == 0)
		return RBX::SDLK_GAMEPAD_BUTTONR3;

	if (sdlName.compare("leftx") == 0 ||
		sdlName.compare("lefty") == 0)
		return RBX::SDLK_GAMEPAD_THUMBSTICK1;

	if (sdlName.compare("rightx") == 0 ||
		sdlName.compare("righty") == 0)
		return RBX::SDLK_GAMEPAD_THUMBSTICK2;

	return RBX::SDLK_UNKNOWN;
}

int SDLGameController::getGamepadIntForEnum(RBX::InputObject::UserInputType gamepadType)
{
	switch (gamepadType)
	{
	case RBX::InputObject::TYPE_GAMEPAD1:
		return 0;
	case RBX::InputObject::TYPE_GAMEPAD2:
		return 1;
	case RBX::InputObject::TYPE_GAMEPAD3:
		return 2;
	case RBX::InputObject::TYPE_GAMEPAD4:
		return 3;
	default:
		break;
	}

	return -1;
}

void SDLGameController::findAvailableGamepadKeyCodesAndSet(RBX::InputObject::UserInputType gamepadType)
{
	shared_ptr<const RBX::Reflection::ValueArray> availableGamepadKeyCodes = getAvailableGamepadKeyCodes(gamepadType);
	if (RBX::UserInputService* inputService = getUserInputService())
	{
		inputService->setSupportedGamepadKeyCodes(gamepadType, availableGamepadKeyCodes);
	}
}

shared_ptr<const RBX::Reflection::ValueArray> SDLGameController::getAvailableGamepadKeyCodes(RBX::InputObject::UserInputType gamepadType)
{
	int gamepadId = getGamepadIntForEnum(gamepadType);

	if ( gamepadId < 0 || (gamepadIdToGameController.find(gamepadId) == gamepadIdToGameController.end()) )
	{
		return shared_ptr<const RBX::Reflection::ValueArray>();
	}

	if (SDL_GameController* gameController = gamepadIdToGameController[gamepadId].second)
	{
		std::string gameControllerMapping(SDL_GameControllerMapping(gameController));

		std::istringstream controllerMappingStream(gameControllerMapping);
		std::string mappingItem;

		shared_ptr<RBX::Reflection::ValueArray> supportedGamepadFunctions(new RBX::Reflection::ValueArray());

		int count = 0;
		while(std::getline(controllerMappingStream, mappingItem, ',')) 
		{
			// first two settings in mapping are hardware id and device name, don't need those
			if (count > 1)
			{
				std::istringstream mappingStream(mappingItem);
				std::string sdlName;
				std::getline(mappingStream, sdlName, ':');

				// platform is always last thing defined in mappings, don't need it so we are done
				if (sdlName.compare("platform") == 0)
				{
					break;
				}

				RBX::KeyCode gamepadCode = getKeyCodeFromSDLName(sdlName);
				if (gamepadCode != RBX::SDLK_UNKNOWN)
				{
					supportedGamepadFunctions->push_back(gamepadCode);
				}
			}
			count++;
		}

		return supportedGamepadFunctions;
	}

	return shared_ptr<const RBX::Reflection::ValueArray>();
}
