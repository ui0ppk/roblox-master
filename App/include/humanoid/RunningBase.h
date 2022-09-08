/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Balancing.h"


namespace RBX {
	class Body;

namespace HUMAN {

	class RunningBase : public Balancing
	{
	private:
		typedef Balancing Super;
	protected:
		Velocity floorVelocity;

		// The following velocities are w.r.t the ground that the figure is walking on. The ground may be moving
		Velocity desiredVelocity;	// desired velocity in world coordinates relative to the floor's velocity
		float desiredAltitude;		// ignored if 0.0

		void rotateWithGround(Body* body);
		void hoverOnFloor(Body* body);
		void move(Body* body);


		///////////////////////////////////////////////////////////////////////////
		// Humanoid::State
		/*override*/ void onComputeForceImpl();
		/*override*/ void onSimulatorStepImpl(float stepDt);

		/*override*/ float getYAxisRotationalVelocity() const {return desiredVelocity.rotational.y;}

		/*override*/ bool armsShouldCollide()		const	{return false;}	
		/*override*/ bool legsShouldCollide()		const	{return false;}	

	public:
		RunningBase(Humanoid* humanoid, StateType priorState);
		RunningBase(Humanoid* humanoid, StateType priorState, const float kP, const float kD);

		/*override*/ void onCFrameChangedFromReflection();

		static const float kTurnP() {return 7500.0f;}
		static const float kTurnPForRotatePGS() {return 450.0f;}
		static const float kTurnPForFreeFallPGS() {return 375.0f;}
	};

	} // namespace HUMAN
}	// namespace RBX