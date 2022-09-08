/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Running.h"
#include "Humanoid/Humanoid.h"

DYNAMIC_FASTFLAG(EnableClimbingDirection)


namespace RBX {
namespace HUMAN {

const char* const sRunning =		"Running";
const char* const sRunningSlave =	"RunningSlave";
const char* const sLanded =			"Landed";
const char* const sClimbing =		"Climbing";

Running::Running(Humanoid* humanoid, StateType priorState) : Named<RunningBase, sRunning>(humanoid, priorState)
{
}

void Running::fireEvents()
{
	Super::fireEvents();

	fireMovementSignal(getHumanoid()->runningSignal, getRelativeMovementVelocity().xz().length() );
}

void Running::onComputeForceImpl()
{
	// Compute Ragdoll entrance criteria
	Humanoid *humanoid = getHumanoid();
	if (humanoid->getTouchedHard())
		return;  // Disable balancing torques and moving forces

	float forceCriteria = humanoid->getRagdollCriteria() * 6000.0f;
	float forceCriteriaSquare = forceCriteria * forceCriteria;

	if ((lastForce.squaredLength() > forceCriteriaSquare ||
		 lastTorque.squaredLength() > forceCriteriaSquare)
		 &&
		 computeHitByHighImpactObject()) 
	{
		humanoid->setTouchedHard(true);
		return;
	}
	Super::onComputeForceImpl();
}

RunningSlave::RunningSlave(Humanoid* humanoid, StateType priorState) : Named<Running, sRunningSlave>(humanoid, priorState)
{
}

Landed::Landed(Humanoid* humanoid, StateType priorState)
	: Named<RunningBase, sLanded>(humanoid, priorState, 7500.0f, 50.0f)
{
	setTimer(0.05f);			// time until finished landing after last jump
}

void Climbing::fireEvents()
{
	Super::fireEvents();
	if (DFFlag::EnableClimbingDirection)
	{
		fireMovementSignal(getHumanoid()->climbingSignal, getRelativeMovementVelocity().y );
	}
	else
	{
		fireMovementSignal(getHumanoid()->climbingSignal, std::abs(getRelativeMovementVelocity().y));
	}
}


} // namespace HUMAN
} // namespace RBX
