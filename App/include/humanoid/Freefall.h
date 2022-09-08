/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Balancing.h"

namespace RBX {

	namespace HUMAN {

	extern const char* const  sFreefall;

	class Freefall : public Named<Balancing, sFreefall>
	{
	private:
		typedef Named<Balancing, sFreefall> Super;
		bool initialized;				// hack- some data is bad in the constructor - do first time through;
		Vector3 initialLinearVelocity;
		Velocity desiredVelocity;		// Y is world-up
		float torsoFriction;			// I set both of these to zero!
		float headFriction;	

		/*override*/ StateType getStateType() const {return FREE_FALL;}

		/*override*/ void onSimulatorStepImpl(float stepDt);

		/*override*/ void onComputeForceImpl();

		/*override*/  int ladderCheckRate() { return 0; }

		/*override*/ bool armsShouldCollide() const	{return false;}	
		/*override*/ bool legsShouldCollide() const	{return false;}	
		/*override*/ bool enableAutoJump()	const	{ return false; }

		static float characterVelocityInfluence();
		static float floorVelocityInfluence();
		static float velocityDecay();

	public:
		Freefall(Humanoid* humanoid, StateType priorState);

		~Freefall();

		static float kTurnSpeed();
		static float kTurnSpeedForPGS();
		static const float kTurnAccelMax() {return 20000.0f * kTurnSpeed();}  // units: 1/sec^2
	};

	} // namespace HUMAN
}	// namespace

