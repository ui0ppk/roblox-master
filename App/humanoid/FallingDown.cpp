/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Humanoid/FallingDown.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/ModelInstance.h"
#include "Network/Players.h"

namespace RBX {
	namespace HUMAN {

const char* const sPhysics = "Physics";
const char* const sFallingDown = "FallingDown";
const char* const sDead = "Dead";


Dead::Dead(Humanoid* humanoid, StateType priorState)
	: Named<HumanoidState, sDead>(humanoid, priorState)
{
	if (humanoid != NULL) {
		PartInstance *pRoot = humanoid->getTorsoSlow();
		if (pRoot != NULL)
		{
			pRoot->setCanCollide(true);
			pRoot->getPartPrimitive()->setMassInertia(10.0f); 
		}
	}
}

void Dead::onStepImpl()
{
	if(Network::Players::backendProcessing(getHumanoid(), false)){
		getHumanoid()->setHealth(0.0f);		// health goes to zero - do this here rather than constructor - only simulating side does this
	}
}


void Dead::onSimulatorStepImpl(float stepDt)
{
	if (getHumanoid()->breakJointsOnDeath()) {
		// Break all joints
		if (ModelInstance* character = Humanoid::getCharacterFromHumanoid(getHumanoid())) {
			character->breakJoints();
		}
	}
}




FallingDown::FallingDown(Humanoid* humanoid, StateType priorState)
	: Named<HumanoidState, sFallingDown>(humanoid, priorState)
{
	setTimer(3.0f);
}

Physics::Physics(Humanoid* humanoid, StateType priorState)
	: Named<HumanoidState, sPhysics>(humanoid, priorState)
{
}




	} // namespace HUMAN
}	// namespace RBX
