/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/MovingNoPhysicsBase.h"

namespace RBX {
	class Clump;

namespace HUMAN {

	extern const char* const sRunningNoPhysics;

	class RunningNoPhysics
		: public Named<MovingNoPhysicsBase, sRunningNoPhysics>
	{
	private:
		/*override*/ StateType getStateType() const {return RUNNING_NO_PHYS;}
	public:
		RunningNoPhysics(Humanoid* humanoid, StateType priorState);
	};

	} // namespace HUMAN
}	// namespace
