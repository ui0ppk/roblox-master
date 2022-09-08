/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/MovingNoPhysicsBase.h"

namespace RBX {
	class Clump;

namespace HUMAN {

	extern const char* const sStrafingNoPhysics;

	class StrafingNoPhysics
		: public Named<MovingNoPhysicsBase, sStrafingNoPhysics>
	{
	private:
		/*override*/ StateType getStateType() const {return STRAFING_NO_PHYS;}
	public:
		StrafingNoPhysics(Humanoid* humanoid, StateType priorState);
	};

	} // namespace HUMAN
}	// namespace
