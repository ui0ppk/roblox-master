/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Balancing.h"
#include "Util/Name.h"

namespace RBX {
	namespace HUMAN {

	// Flying occurs when there's no ground below you. You have the ability
	// to turn around the y-axis, but not much else.
	extern const char* const  sFlying;

	class Flying : public Named<Balancing, sFlying>
	{
	private:
		/*override*/ StateType getStateType() const {return FLYING;}

	protected:
		// Humanoid::State
		/*override*/ void onSimulatorStepImpl(float stepDt);
		/*override*/ void onComputeForceImpl();
		/*override*/ bool enableAutoJump()	const	{ return false; }

	public:
		Flying(Humanoid* humanoid, StateType priorState);
	};

	} // namespace HUMAN
}	// namespace

