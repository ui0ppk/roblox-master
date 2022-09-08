/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/HumanoidState.h"

namespace RBX {

	namespace HUMAN { 

		// pure simulation!

	extern const char* const sDead;
	class Dead : public Named<HumanoidState, sDead>
	{
	private:
		/*override*/ StateType getStateType() const {return DEAD;}
		/*override*/ void onStepImpl();
		/*override*/ void onSimulatorStepImpl(float stepDt);
		/*override*/ void onComputeForceImpl() {}
		/*override*/ bool enableAutoJump()	const	{ return false; }
	public:
		Dead(Humanoid* humanoid, StateType priorState);
	};


	extern const char* const sFallingDown;
	class FallingDown : public Named<HumanoidState, sFallingDown>
	{
	private:
		/*override*/ StateType getStateType() const {return FALLING_DWN;}
		/*override*/ void onComputeForceImpl() {}
		/*override*/ bool enableAutoJump()	const	{ return false; }
	public:
		FallingDown(Humanoid* humanoid, StateType priorState);
	};

	extern const char* const sPhysics;
	class Physics : public Named<HumanoidState, sPhysics>
	{
	private:
		/*override*/ StateType getStateType() const {return PHYSICS;}
		/*override*/ void onComputeForceImpl() {}
		/*override*/ bool enableAutoJump()	const	{ return false; }
	public:
		Physics(Humanoid* humanoid, StateType priorState);
	};



	} // namespace HUMAN
}	// namespace RBX

