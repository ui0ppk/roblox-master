#include "XboxGameController.h"

#include <vector>

#include "reflection/type.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/GamepadService.h"

using namespace boost;
using namespace RBX;

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

XboxGameController::XboxGameController(shared_ptr<RBX::DataModel> newDM, shared_ptr<ControllerBuffer> newControllerBuffer) :
	controllerBuffer(newControllerBuffer)
{
	currentVibration.LeftMotorLevel = 0;
	currentVibration.RightMotorLevel = 0;
	currentVibration.LeftTriggerLevel = 0;
	currentVibration.RightTriggerLevel = 0;

	controllerBuffer->setXboxGameController(this);
	initializedControllerGuidMap = false;

	setDataModel(newDM);
}

void XboxGameController::handleNewGamepad(IGamepad^ gamepad)
{
	int rbxGamepadInt = getRbxGamepadIntFromGamepad(gamepad);

	if (rbxGamepadInt >= 0)
	{
		gamepadConnectedMap[rbxGamepadInt].isConnected = true;
		gamepadConnectedMap[rbxGamepadInt].guid = gamepad->Id;

		fireGamepadConnectedEvent(rbxGamepadInt);
	}
}

void XboxGameController::handleGamepadRemove(IGamepad^ gamepad)
{
	gamepadConnectedMap.erase(gamepad->Id);

	fireGamepadDisconnectedEvent(gamepad->Id);
}

void XboxGameController::setupGamepadHandling()
{
	// first clear out all old data
	Windows::Xbox::Input::Gamepad::GamepadAdded -= gamepadAddedEvent;
	Windows::Xbox::Input::Gamepad::GamepadRemoved -= gamepadRemovedEvent;

	gamepadConnectedMap.clear();

	// now lets setup all existing gamepads
	IVectorView<IGamepad^>^ gamepads = Windows::Xbox::Input::Gamepad::Gamepads;
	for ( unsigned int i = 0; i < gamepads->Size; ++i )
	{
		handleNewGamepad(gamepads->GetAt(i));
	}

	// finally listen to gamepads being removed and added
	gamepadAddedEvent = Windows::Xbox::Input::Gamepad::GamepadAdded += ref new EventHandler<GamepadAddedEventArgs^ >( [=]( Platform::Object^ , GamepadAddedEventArgs^ args )
	{
		handleNewGamepad(args->Gamepad);
	});
	gamepadRemovedEvent = Windows::Xbox::Input::Gamepad::GamepadRemoved += ref new EventHandler<GamepadRemovedEventArgs^ >( [=]( Platform::Object^ , GamepadRemovedEventArgs^ args )
	{
		handleGamepadRemove(args->Gamepad);
	});
}

void XboxGameController::resetControllerGuidMap()
{
	initializedControllerGuidMap = false;
	xboxLiveIdToRBXGamepadType.clear();
}

void XboxGameController::setDataModel(shared_ptr<RBX::DataModel> newDM)
{
	dataModel = newDM;

	setupGamepadHandling();

	if ( RBX::UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(newDM.get()) )
	{
		getSupportedGamepadKeyCodesConnection.disconnect();
		getSupportedGamepadKeyCodesConnection = inputService->getSupportedGamepadKeyCodesSignal.connect(boost::bind(&XboxGameController::findAvailableGamepadKeyCodesAndSet, this, _1));

		updateInputConnection.disconnect();
		updateInputConnection = inputService->updateInputSignal.connect(boost::bind(&XboxGameController::processControllerBuffer, this));
	}

	if ( RBX::HapticService* hapticService = RBX::ServiceProvider::create<HapticService>(newDM.get()) )
	{
		setEnabledVibrationMotorsConnection.disconnect();
		setVibrationMotorConnection.disconnect();

		setEnabledVibrationMotorsConnection = hapticService->setEnabledVibrationMotorsSignal.connect(boost::bind(&XboxGameController::setVibrationMotorsEnabled, this, _1));
		setVibrationMotorConnection = hapticService->setVibrationMotorSignal.connect(boost::bind(&XboxGameController::setVibrationMotor, this, _1, _2, _3));
	}
}
int XboxGameController::getRbxGamepadIntFromGamepad(Windows::Xbox::Input::IGamepad^ gamepad)
{
	if (!initializedControllerGuidMap)
	{
		IVectorView<IGamepad^>^ gamepads = Windows::Xbox::Input::Gamepad::Gamepads;
		for ( unsigned int i = 0; i < gamepads->Size; ++i )
		{
			if (gamepads->GetAt(i) == gamepad)
			{
				return i;
			}
		}

		return -1;
	}

	if (gamepad == nullptr)
	{
		return -1;
	}
	
	User^ user = nullptr;
	try
	{
		user = gamepad->User;
	}
	catch (std::runtime_error)
	{
		return -1;
	}

	if (user == nullptr)
	{
		return -1;
	}

	Platform::String^ xboxUserId;
	try
	{
		xboxUserId = user->XboxUserId;
	}
	catch (std::runtime_error)
	{
		return -1;
	}

	if (xboxUserId == nullptr)
	{
		return -1;
	}

	if (xboxUserId->IsEmpty())
	{
		return -1;
	}

	try
	{
		if (user->IsGuest)
		{
			return -1;
		}
	}
	catch (std::runtime_error)
	{
		return -1;
	}

	RBX::InputObject::UserInputType gamepadEnum = InputObject::TYPE_NONE;
	int rbxGamepadInt = 0;
	
	if (!getRBXGamepadFromXboxLiveId(xboxUserId, gamepad->Id, gamepadEnum, rbxGamepadInt))
	{
		return -1;
	}

	return rbxGamepadInt;
}

void XboxGameController::remapXboxLiveIdToRBXGamepadId(Platform::String^ xboxLiveId, unsigned long long controllerId, RBX::InputObject::UserInputType gamepadType)
{
	// todo: actually do remapping instead of blowing away mapping and adding one in (need this now for xbox cert)
	xboxLiveIdToRBXGamepadType.clear();
	std::pair<Platform::String^,unsigned long long> liveIdControllerIdPair(xboxLiveId,controllerId);
	xboxLiveIdToRBXGamepadType[liveIdControllerIdPair] = gamepadType;
	initializedControllerGuidMap = true;
}

bool XboxGameController::getRBXGamepadFromXboxLiveId(Platform::String^ xboxLiveId, unsigned long long controllerId, RBX::InputObject::UserInputType& gamepadEnum, int& rbxGamepadInt)
{
	std::pair<Platform::String^,unsigned long long> liveIdControllerIdPair(xboxLiveId, controllerId);

	std::map<std::pair<Platform::String^, unsigned long long>, RBX::InputObject::UserInputType>::iterator iter = xboxLiveIdToRBXGamepadType.find(liveIdControllerIdPair);
	if (iter == xboxLiveIdToRBXGamepadType.end())
	{
		return false;
	}

	gamepadEnum = iter->second;
	rbxGamepadInt = RBX::GamepadService::getGamepadIntForEnum(gamepadEnum);

	return true;
}

void XboxGameController::updateVibration()
{
	const uint64 gamepadGuid = getGamepadGuid(RBX::InputObject::TYPE_GAMEPAD1);
	IVectorView<IGamepad^>^ gamepads = Windows::Xbox::Input::Gamepad::Gamepads;

	for ( unsigned int i = 0; i < gamepads->Size; ++i )
	{
		IGamepad^ gamepad = gamepads->GetAt(i);

		if (gamepad->Id == gamepadGuid)
		{
			gamepad->SetVibration(currentVibration);
			break;
		}
	}
}

void XboxGameController::processControllerBuffer()
{
	shared_ptr<RBX::DataModel> sharedDM = dataModel.lock();
	if (!sharedDM)
	{
		return;
	}

	GamepadService* gamepadService = RBX::ServiceProvider::find<GamepadService>(sharedDM.get());
	if (!gamepadService)
	{
		return;
	}

	UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(sharedDM.get());
	if (!inputService)
	{
		return;
	}

	ControllerBuffer::GamepadValueBufferMap bufferMap = controllerBuffer->getBufferedInput();
	for (ControllerBuffer::GamepadValueBufferMap::iterator iter = bufferMap.begin(); iter != bufferMap.end(); ++iter)
	{
		RBX::Gamepad gamepad = gamepadService->getGamepadState(GamepadService::getGamepadIntForEnum((*iter).first.first));
		RBX::KeyCode keyCode = (*iter).first.second;
		ControllerBuffer::GamepadValueStateHistoryVector historyVector = (*iter).second;

		for (ControllerBuffer::GamepadValueStateHistoryVector::iterator historyIter = historyVector.begin(); historyIter != historyVector.end(); ++historyIter)
		{
			InputObject::UserInputState newState = (*historyIter).second;
			Vector3 newValue =  (*historyIter).first;

			G3D::Vector3 lastPos = gamepad[keyCode]->getPosition();

			if (lastPos != newValue)
			{
				gamepad[keyCode]->setPosition(newValue);

				RBX::InputObject::UserInputState currentState = RBX::InputObject::INPUT_STATE_CHANGE;
				if (newValue == G3D::Vector3::zero())
				{
					currentState = RBX::InputObject::INPUT_STATE_END;
				}
				else if (newValue.z >= 1.0f)
				{
					currentState = RBX::InputObject::INPUT_STATE_BEGIN;
				}

				gamepad[keyCode]->setDelta(newValue - lastPos);
				gamepad[keyCode]->setInputState(currentState);

				inputService->dangerousFireInputEvent(gamepad[keyCode], NULL);
			}
		}
	}

	updateVibration();
}

void XboxGameController::fireGamepadConnectedEvent(const int controllerIndex)
{
	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(sharedDM.get()))
		{
			inputService->safeFireGamepadConnected(RBX::GamepadService::getGamepadEnumForInt(controllerIndex));
		}
	}
}

void XboxGameController::fireGamepadDisconnectedEvent(const int controllerIndex)
{
	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(sharedDM.get()))
		{
			inputService->safeFireGamepadDisconnected(RBX::GamepadService::getGamepadEnumForInt(controllerIndex));
		}
	}
}

uint64 XboxGameController::getGamepadGuid(const InputObject::UserInputType rbxGamepadId)
{
	return gamepadConnectedMap[RBX::GamepadService::getGamepadIntForEnum(rbxGamepadId)].guid;
}

void XboxGameController::findAvailableGamepadKeyCodesAndSet(RBX::InputObject::UserInputType gamepadType)
{
	if (shared_ptr<RBX::DataModel> sharedDM = dataModel.lock())
	{
		if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(sharedDM.get()))
		{
			shared_ptr<RBX::Reflection::ValueArray> availableGamepadKeyCodes(new RBX::Reflection::ValueArray());

			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONA);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONB);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONX);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONY);

			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONL1);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONR1);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONL2);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONR2);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONL3);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONR3);

			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_THUMBSTICK1);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_THUMBSTICK2);

			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADUP);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADDOWN);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADLEFT);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_DPADRIGHT);

			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONSELECT);
			availableGamepadKeyCodes->push_back(RBX::SDLK_GAMEPAD_BUTTONSTART);

			inputService->setSupportedGamepadKeyCodes(gamepadType, availableGamepadKeyCodes);
		}
	}
}

void XboxGameController::setVibrationMotorsEnabled(RBX::InputObject::UserInputType gamepadType)
{
	if (gamepadType != InputObject::TYPE_GAMEPAD1)
	{
		return;
	}

	if (shared_ptr<DataModel> dm = dataModel.lock())
	{
		if ( RBX::HapticService* hapticService = RBX::ServiceProvider::create<HapticService>(dm.get()) )
		{
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_LARGE, true);
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_SMALL, true);
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_LEFTTRIGGER, true);
			hapticService->setEnabledVibrationMotors(gamepadType, RBX::HapticService::MOTOR_RIGHTTRIGGER, true);
		}
	}
}
void XboxGameController::setVibrationMotor(RBX::InputObject::UserInputType gamepadType, RBX::HapticService::VibrationMotor vibrationMotor, shared_ptr<const RBX::Reflection::Tuple> args)
{
	if (!args)
	{
		return;
	}
	if (args->values.size() <= 0)
	{
		return;
	}

	if (gamepadType == InputObject::TYPE_GAMEPAD1)
	{
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

		switch (vibrationMotor)
		{
		case RBX::HapticService::MOTOR_LARGE:
			currentVibration.LeftMotorLevel = newMotorValue;
			break;
		case RBX::HapticService::MOTOR_SMALL:
			currentVibration.RightMotorLevel = newMotorValue;
			break;
		case RBX::HapticService::MOTOR_LEFTTRIGGER:
			currentVibration.LeftTriggerLevel = newMotorValue;
			break;
		case RBX::HapticService::MOTOR_RIGHTTRIGGER:
			currentVibration.RightTriggerLevel = newMotorValue;
			break;
		default:
			break;
		}
	}
}