/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/SteppedInstance.h"
#include "V8DataModel/UserController.h"


namespace RBX {

	class UserInputBase;
	class SkateboardPlatform;


	extern const char* const sSkateboardController;
	class SkateboardController : public DescribedCreatable<SkateboardController, Controller, sSkateboardController>
	{
	private:

		// IStepped
		/*override*/ void onStepped(const Stepped& event);

		float throttle;
		float steer;

		void setThrottle(float value);
		void setSteer(float value);
        
        void onSteppedTouchInput();
        void onSteppedKeyboardInput();

		weak_ptr<SkateboardPlatform> skateboardPlatform;
	public:
		SkateboardController();

		void setSkateboardPlatform(SkateboardPlatform* value);

		// lua api:
		float getThrottle() const { return throttle; }
		float getSteer() const { return steer; }

		rbx::signal<void(std::string)> axisChangedSignal;
	};


} // namepsace