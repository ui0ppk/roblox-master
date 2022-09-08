//
//  HapticService.h
//  App
//
//  Created by Ben Tkacheff on 2/09/16.
//
//

#pragma once

#include "V8Tree/Service.h"

namespace RBX
{
	extern const char* const sHapticService;

	class HapticService
		: public DescribedCreatable<HapticService, Instance, sHapticService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
	public:
		typedef enum
		{
			MOTOR_LARGE = 0,
			MOTOR_SMALL = 1,
			MOTOR_LEFTTRIGGER = 2,
			MOTOR_RIGHTTRIGGER = 3,
			MOTOR_NONE = 4,
		} VibrationMotor;

	private:
		typedef boost::unordered_map<VibrationMotor, bool> VibrationEnabledMap;
		typedef boost::unordered_map<VibrationMotor, shared_ptr<const RBX::Reflection::Tuple> > VibrationStateMap;

		typedef boost::unordered_map<InputObject::UserInputType, VibrationStateMap > InputVibrationMap;
		typedef boost::unordered_map<InputObject::UserInputType, VibrationEnabledMap > InputVibrationEnabledMap;

		InputVibrationEnabledMap vibrationMotorsEnabledMap;
		InputVibrationMap vibrationMotorsStateMap;

	public:
		HapticService();

		rbx::signal<void(InputObject::UserInputType, VibrationMotor, shared_ptr<const RBX::Reflection::Tuple>)> setVibrationMotorSignal;
		rbx::signal<void(InputObject::UserInputType)> setEnabledVibrationMotorsSignal;

		void setEnabledVibrationMotors(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor, bool isEnabled);

		bool isVibrationSupported(InputObject::UserInputType inputType);
		bool isMotorSupported(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor);

		void setMotor(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor, shared_ptr<const RBX::Reflection::Tuple> args);
		shared_ptr<const RBX::Reflection::Tuple> getMotor(InputObject::UserInputType inputType, HapticService::VibrationMotor vibrationMotor);
	};
}