/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/Ragdoll.h"
#include "Humanoid/Humanoid.h"
#include "v8datamodel/PartInstance.h"

namespace RBX {
	
	namespace HUMAN {


const char* const sRagdoll = "Ragdoll";


Ragdoll::Ragdoll(Humanoid* humanoid, StateType priorState)
	:Named<HumanoidState, sRagdoll>(humanoid, priorState)
{
	setTimer(8.0f);
}

Ragdoll::~Ragdoll()
{
	// Resume monitoring the touchedHard signal
	getHumanoid()->setTouchedHard(false);
}

void Ragdoll::onStepImpl()
{
	// Compute Ragdoll exit condition
	Humanoid* humanoid = getHumanoid();
	PartInstance* part = humanoid->getTorsoSlow();
	if (getTimer() <= 7.0)
	{
		if (part->getLinearVelocity().magnitude() < 1.0f &&
			part->getRotationalVelocity().magnitude() < 1.0f)
			setTimer(0.0f);
	}
}

}
} // namespace