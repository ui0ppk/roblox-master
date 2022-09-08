//
//  HapticService.cpp
//  App
//
//  Created by Ben Tkacheff on 2/09/16.
//
//


#include "stdafx.h"

#include "V8DataModel/HapticService.h"

namespace RBX
{
	namespace Reflection {
		template<>
		EnumDesc<HapticService::VibrationMotor>::EnumDesc()
			:EnumDescriptor("VibrationMotor")
		{
			addPair(HapticService::MOTOR_LARGE, "Large");
			addPair(HapticService::MOTOR_SMALL, "Small");
			addPair(HapticService::MOTOR_LEFTTRIGGER, "LeftTrigger");
			addPair(HapticService::MOTOR_RIGHTTRIGGER, "RightTrigger");
		}

		template<>
		HapticService::VibrationMotor& Variant::convert<HapticService::VibrationMotor>(void)
		{
			return genericConvert<HapticService::VibrationMotor>();
		}
	} // namespace Reflection

	template<>
	bool StringConverter<HapticService::VibrationMotor>::convertToValue(const std::string& text, HapticService::VibrationMotor& value)
	{
		return Reflection::EnumDesc<HapticService::VibrationMotor>::singleton().convertToValue(text.c_str(),value);
	}

	const char* const sHapticService = "HapticService";

	REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<HapticService, bool(InputObject::UserInputType)> func_isVibrationSupported(&HapticService::isVibrationSupported, "IsVibrationSupported", "inputType", Security::None);
	static Reflection::BoundFuncDesc<HapticService, bool(InputObject::UserInputType, HapticService::VibrationMotor)> func_isMotorSupported(&HapticService::isMotorSupported, "IsMotorSupported", "inputType", "vibrationMotor", Security::None);

	static Reflection::BoundFuncDesc<HapticService, void(InputObject::UserInputType, HapticService::VibrationMotor, shared_ptr<const RBX::Reflection::Tuple>)> func_setMotor(&HapticService::setMotor, "SetMotor", "inputType", "vibrationMotor", "vibrationValues", Security::None);
	static Reflection::BoundFuncDesc<HapticService, shared_ptr<const RBX::Reflection::Tuple>(InputObject::UserInputType, HapticService::VibrationMotor)> func_getMotor(&HapticService::getMotor, "GetMotor", "inputType", "vibrationMotor", Security::None);
	REFLECTION_END();

	HapticService::HapticService()
	{
		setName(sHapticService);
	}

	void HapticService::setEnabledVibrationMotors(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor, bool isEnabled)
	{
		if (inputType == InputObject::TYPE_NONE || vibrationMotor == MOTOR_NONE)
		{
			return;
		}

		vibrationMotorsEnabledMap[inputType][vibrationMotor] = isEnabled;
	}

	bool HapticService::isVibrationSupported(InputObject::UserInputType inputType)
	{
		for (int i = 0; i < MOTOR_NONE; ++i)
		{
			if (isMotorSupported(inputType, (VibrationMotor) i))
			{
				return true;
			}
		}

		return false;
	}

	bool HapticService::isMotorSupported(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor)
	{
		setEnabledVibrationMotorsSignal(inputType);

		InputVibrationEnabledMap::iterator iter = vibrationMotorsEnabledMap.find(inputType);
		if (iter != vibrationMotorsEnabledMap.end())
		{
			VibrationEnabledMap::iterator vibrationIter = iter->second.find(vibrationMotor);
			if (vibrationIter != iter->second.end())
			{
				return vibrationIter->second;
			}
		}

		return false;
	}

	shared_ptr<const RBX::Reflection::Tuple> HapticService::getMotor(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor)
	{
		InputVibrationMap::iterator iter = vibrationMotorsStateMap.find(inputType);
		if (iter != vibrationMotorsStateMap.end())
		{
			VibrationStateMap::iterator vibrationIter = iter->second.find(vibrationMotor);
			if (vibrationIter != iter->second.end())
			{
				return vibrationIter->second;
			}
		}

		shared_ptr<const RBX::Reflection::Tuple> tuple(rbx::make_shared<Reflection::Tuple>());
		return tuple;
	}

	void HapticService::setMotor(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor, shared_ptr<const RBX::Reflection::Tuple> args)
	{
		if (args && !args->values.empty())
		{
			if (inputType == InputObject::TYPE_NONE)
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error in HapticService:SetMotor inputType is not a valid type");
				return;
			}
			if (vibrationMotor == MOTOR_NONE)
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error in HapticService:SetMotor vibrationMotor is not a valid type");
				return;
			}

			vibrationMotorsStateMap[inputType][vibrationMotor] = args;

			setVibrationMotorSignal(inputType, vibrationMotor, args);
		}
		else
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error in HapticService:SetMotor no values found for vibration.");
		}
	}

} // namespace RBX

