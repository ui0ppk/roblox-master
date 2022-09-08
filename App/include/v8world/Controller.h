/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX {

	class LegacyController 
	{
	public:
		typedef enum InputType {NO_INPUT = 0,
								LEFT_TRACK_INPUT,
								RIGHT_TRACK_INPUT,
								RIGHT_LEFT_INPUT,			// -1.0 == right, 1.0 == left
								BACK_FORWARD_INPUT,			// -1.0 == back, 1.0 == forward
								STRAFE_INPUT,
								UP_DOWN_INPUT,
								BUTTON_1_INPUT, 
								BUTTON_2_INPUT, 
								BUTTON_3_INPUT, 
								BUTTON_4_INPUT, 
								BUTTON_3_4_INPUT,
								CONSTANT_INPUT,
								SIN_INPUT,
								NUM_INPUT_TYPES} InputType;
							// If you add more items here, 
							// please update the associated string matrix in ControllerTypes.cpp
	};

} // namespace
