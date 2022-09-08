/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/Controller.h"
#include "reflection/enumconverter.h"

namespace RBX {
namespace Reflection {
	template<>
	EnumDesc<LegacyController::InputType>::EnumDesc()
		:EnumDescriptor("InputType")
	{
		addPair(LegacyController::NO_INPUT, "NoInput");
		addPair(LegacyController::LEFT_TRACK_INPUT, "LeftTread");
		addPair(LegacyController::RIGHT_TRACK_INPUT, "RightTread");
		addPair(LegacyController::RIGHT_LEFT_INPUT, "Steer");
		addPair(LegacyController::BACK_FORWARD_INPUT, "Throtle");
		addPair(LegacyController::UP_DOWN_INPUT, "UpDown");
		addPair(LegacyController::BUTTON_1_INPUT, "Action1");
		addPair(LegacyController::BUTTON_2_INPUT, "Action2");
		addPair(LegacyController::BUTTON_3_INPUT, "Action3");
		addPair(LegacyController::BUTTON_4_INPUT, "Action4");
		addPair(LegacyController::BUTTON_3_4_INPUT, "Action5");
		addPair(LegacyController::CONSTANT_INPUT, "Constant");
		addPair(LegacyController::SIN_INPUT, "Sin");
	}
}//namespace Reflection
}// namespace RBX