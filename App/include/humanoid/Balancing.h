/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/HumanoidState.h"

namespace RBX {
namespace HUMAN {


	class Balancing : public HumanoidState
	{
	private:
		float kP;   // units: 1/sec^2      torque = kP * momentOfInertia * rotation
		float kD;   // units: 1/sec        torque = kD * momentOfInertia * rotVelocity

		Vector3 lastBalanceTorque;
		int tick;

		static int balanceRate(double torqueMag);
		static int balanceRateForPGS();
	protected:
		static const float maxTorqueComponent() {return 4000.0f;}	// units: 1/sec^2      torque <= maxTorqueComponent * momentOfInertia
		
		void setBalanceP(float P) { kP = P; }; 
		void setBalanceD(float D) { kD = D; };

		// Humanoid::State
		/*override*/ void onComputeForceImpl();

	public:
		Balancing(Humanoid* humanoid, StateType priorState);
		Balancing(Humanoid* humanoid, StateType priorState, const float kP, const float kD);
	};


} // namespace HUMAN
} // namespace RBX

