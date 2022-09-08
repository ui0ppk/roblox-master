/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Balancing.h"
#include "Util/Name.h"

namespace RBX {
	namespace HUMAN {

		extern const char* const  sSwimming;

		class Swimming : public Named<HumanoidState, sSwimming>
		{
		private:
			typedef Named<HumanoidState, sSwimming> Super;
			Vector3 initialLinearVelocity;
			static float velocityDecay();

			/*override*/ StateType getStateType() const {return SWIMMING;}
			/*override*/ void fireEvents();
			/*override*/ bool enableAutoJump()	const	{ return false; }

			Velocity desiredVelocity;

		protected:
			///////////////////////////////////////////////////////////////////////////
			// Humanoid::State
			/*override*/ void onComputeForceImpl();
			/*override*/ void onSimulatorStepImpl(float stepDt);

		public:
			Swimming(Humanoid* humanoid, StateType priorState);



			static const float kTurnSpeed() {return  6.0f;}				// note Humanoid autoTurnSpeed is 8.0f;
			static const float kTurnAccelMax() {return 20000.0f * kTurnSpeed();}
		};

	} // namespace HUMAN
}	// namespace

