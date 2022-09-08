#pragma once

#include <map>
#include "boost\weak_ptr.hpp"
#include "boost\shared_ptr.hpp"

#include "ControllerBuffer.h"

#include "util/KeyCode.h"
#include "Util/G3DCore.h"


#include "V8Tree/Service.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/HapticService.h"

using namespace Windows::Xbox::Input;
using namespace Windows::Xbox::System;

namespace RBX
{
	class DataModel;
	class GamepadService;
	class UserInputService;
}

class XboxGameController
{
	friend class XboxGameController;
	struct ControllerInfo {
		bool isConnected;
		uint64 guid;
	};
private:
	XboxGameController() {} //private because you should always be setting a dm when creating

	boost::weak_ptr<RBX::DataModel> dataModel;
	boost::shared_ptr<ControllerBuffer> controllerBuffer;

	rbx::signals::scoped_connection getSupportedGamepadKeyCodesConnection;
	rbx::signals::scoped_connection updateInputConnection;

	rbx::signals::scoped_connection setEnabledVibrationMotorsConnection;
	rbx::signals::scoped_connection setVibrationMotorConnection;

	Windows::Foundation::EventRegistrationToken gamepadAddedEvent;
	Windows::Foundation::EventRegistrationToken gamepadRemovedEvent;

	std::map<int,ControllerInfo> gamepadConnectedMap;

	std::map<std::pair<Platform::String^, unsigned long long>, RBX::InputObject::UserInputType> xboxLiveIdToRBXGamepadType;

	bool initializedControllerGuidMap;

	GamepadVibration currentVibration;

	void fireGamepadConnectedEvent(const int controllerIndex);
	void fireGamepadDisconnectedEvent(const int controllerIndex); 
	
	void handleNewGamepad(IGamepad^ gamepad);
	void handleGamepadRemove(IGamepad^ gamepad);

	void processControllerBuffer();
	void updateVibration();

	void findAvailableGamepadKeyCodesAndSet(RBX::InputObject::UserInputType gamepadType);

	bool getRBXGamepadFromXboxLiveId(Platform::String^ xboxLiveId, unsigned long long controllerId, RBX::InputObject::UserInputType& gamepadEnum, int& gamepadInt);

	void setVibrationMotorsEnabled(RBX::InputObject::UserInputType gamepadType);

public:	
	XboxGameController(boost::shared_ptr<RBX::DataModel> newDM, shared_ptr<ControllerBuffer> newControllerBuffer);
	
	void setDataModel(boost::shared_ptr<RBX::DataModel> newDM);

	void setupGamepadHandling();
	void resetControllerGuidMap();

	uint64 getGamepadGuid(const RBX::InputObject::UserInputType rbxGamepadId);

	int getRbxGamepadIntFromGamepad(Windows::Xbox::Input::IGamepad^ gamepad);
	void remapXboxLiveIdToRBXGamepadId(Platform::String^ xboxLiveId, unsigned long long controllerId, RBX::InputObject::UserInputType gamepadType);

	void setVibrationMotor(RBX::InputObject::UserInputType gamepadType, RBX::HapticService::VibrationMotor vibrationMotor, shared_ptr<const RBX::Reflection::Tuple> args);
};