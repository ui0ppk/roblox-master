#pragma once

#include "rbx/rbxTime.h"
#include <boost/unordered_map.hpp>
#include "v8datamodel/InputObject.h"

using namespace Windows::Xbox::Input;

class XboxGameController;

class ControllerBuffer
{
public:
	typedef std::pair<RBX::InputObject::UserInputType, RBX::KeyCode> GamepadValue;
	typedef std::pair<RBX::Vector3, RBX::InputObject::UserInputState> GamepadValueState;
	typedef std::vector<GamepadValueState> GamepadValueStateHistoryVector;
	typedef boost::unordered_map<GamepadValue, GamepadValueStateHistoryVector> GamepadValueBufferMap;
private:
	GamepadValueBufferMap bufferedInput;
	boost::mutex changeInputBufferMutex;

	XboxGameController* xboxGameController;

	void bufferGamepadButtons(const int controllerIndex, IGamepadReading^& gamepadReading);
	void bufferGamepadAxis(const int controllerIndex, IGamepadReading^& gamepadReading);

	void processButton(RBX::InputObject::UserInputType userInputType, const RBX::KeyCode buttonCode, const int buttonState);
	void processAxis(RBX::InputObject::UserInputType userInputType, const RBX::KeyCode axisCode, const RBX::Vector3 axisValue);

public:
	ControllerBuffer();

	void setXboxGameController(XboxGameController* newController) {xboxGameController = newController;}

	void updateBuffer();
	float getPollingMsec();

	GamepadValueBufferMap getBufferedInput();
};

