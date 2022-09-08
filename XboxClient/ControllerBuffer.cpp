#include "ControllerBuffer.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/GamepadService.h"
#include "XboxGameController.h"

using namespace Windows::Foundation::Collections;
using namespace RBX;

#define INPUT_POLLING_HZ 90.0f

ControllerBuffer::ControllerBuffer() :
	xboxGameController(NULL)
{}

void ControllerBuffer::updateBuffer()
{
	if (xboxGameController == nullptr)
	{
		return;
	}

	boost::mutex::scoped_lock lock(changeInputBufferMutex);

	IVectorView<IGamepad^>^ gamepads = Windows::Xbox::Input::Gamepad::Gamepads;

	for ( unsigned int i = 0; i < gamepads->Size; ++i )
	{
		int rbxGamepadInt = xboxGameController->getRbxGamepadIntFromGamepad(gamepads->GetAt(i));

		if (rbxGamepadInt >= 0)
		{
			IGamepadReading^ reading = gamepads->GetAt(i)->GetCurrentReading();

			bufferGamepadButtons(rbxGamepadInt, reading);
			bufferGamepadAxis(rbxGamepadInt, reading);
		}
	}
}

void ControllerBuffer::processButton(InputObject::UserInputType userInputType, const RBX::KeyCode buttonCode, const int buttonState)
{
	if (buttonCode == SDLK_UNKNOWN || userInputType == InputObject::TYPE_NONE)
	{
		return;
	}

	InputObject::UserInputState newState = (buttonState == 1) ? InputObject::INPUT_STATE_BEGIN : InputObject::INPUT_STATE_END;
	GamepadValue gamepadButtonCombo(userInputType, buttonCode);
	GamepadValueState newGamepadButtonState(Vector3(0,0,buttonState), newState);

	if (bufferedInput.find(gamepadButtonCombo) == bufferedInput.end() ||
		bufferedInput[gamepadButtonCombo].back().first != newGamepadButtonState.first)
	{
		bufferedInput[gamepadButtonCombo].push_back(newGamepadButtonState);
	}
}

void ControllerBuffer::bufferGamepadButtons(const int controllerIndex, IGamepadReading^& gamepadReading)
{
	InputObject::UserInputType gamepadType = GamepadService::getGamepadEnumForInt(controllerIndex);

	processButton(gamepadType, SDLK_GAMEPAD_BUTTONA, gamepadReading->IsAPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONB, gamepadReading->IsBPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONX, gamepadReading->IsXPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONY, gamepadReading->IsYPressed ? 1 : 0);

	processButton(gamepadType, SDLK_GAMEPAD_DPADLEFT, gamepadReading->IsDPadLeftPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_DPADRIGHT, gamepadReading->IsDPadRightPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_DPADUP, gamepadReading->IsDPadUpPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_DPADDOWN, gamepadReading->IsDPadDownPressed ? 1 : 0);

	processButton(gamepadType, SDLK_GAMEPAD_BUTTONL1, gamepadReading->IsLeftShoulderPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONR1, gamepadReading->IsRightShoulderPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONL3, gamepadReading->IsLeftThumbstickPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONR3, gamepadReading->IsRightThumbstickPressed ? 1 : 0);

	processButton(gamepadType, SDLK_GAMEPAD_BUTTONSTART, gamepadReading->IsMenuPressed ? 1 : 0);
	processButton(gamepadType, SDLK_GAMEPAD_BUTTONSELECT, gamepadReading->IsViewPressed ? 1 : 0);
}


void ControllerBuffer::processAxis(RBX::InputObject::UserInputType userInputType, const RBX::KeyCode axisCode, const Vector3 axisValue)
{
	if (axisCode == SDLK_UNKNOWN || userInputType == InputObject::TYPE_NONE)
	{
		return;
	}

	RBX::InputObject::UserInputState newState = RBX::InputObject::INPUT_STATE_CHANGE;
	if (axisValue == G3D::Vector3::zero())
	{
		newState = RBX::InputObject::INPUT_STATE_END;
	}
	else if (axisValue.z >= 1.0f)
	{
		newState = RBX::InputObject::INPUT_STATE_BEGIN;
	}

	GamepadValue gamepadAxisCombo(userInputType, axisCode);
	GamepadValueState newGamepadAxisState(axisValue, newState);

	if (bufferedInput.find(gamepadAxisCombo) == bufferedInput.end() ||
		bufferedInput[gamepadAxisCombo].back().first != newGamepadAxisState.first)
	{
		bufferedInput[gamepadAxisCombo].push_back(newGamepadAxisState);
	}
}
void ControllerBuffer::bufferGamepadAxis(const int controllerIndex, IGamepadReading^& gamepadReading)
{
	InputObject::UserInputType gamepadType = GamepadService::getGamepadEnumForInt(controllerIndex);

	processAxis(gamepadType, SDLK_GAMEPAD_THUMBSTICK1, Vector3(gamepadReading->LeftThumbstickX, gamepadReading->LeftThumbstickY, 0));
	processAxis(gamepadType, SDLK_GAMEPAD_THUMBSTICK2, Vector3(gamepadReading->RightThumbstickX, gamepadReading->RightThumbstickY, 0));

	processAxis(gamepadType, SDLK_GAMEPAD_BUTTONL2, Vector3(0, 0, gamepadReading->LeftTrigger));
	processAxis(gamepadType, SDLK_GAMEPAD_BUTTONR2, Vector3(0, 0, gamepadReading->RightTrigger));
}


ControllerBuffer::GamepadValueBufferMap ControllerBuffer::getBufferedInput()
{
	GamepadValueBufferMap tmpMap;
	{
		boost::mutex::scoped_lock lock(changeInputBufferMutex);
		bufferedInput.swap(tmpMap);
	}

	return tmpMap;
}

float ControllerBuffer::getPollingMsec()
{
	return (1.0f/INPUT_POLLING_HZ) * 1000.0f;
}
