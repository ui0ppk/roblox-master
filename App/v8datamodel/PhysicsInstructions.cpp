/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"


#include "V8DataModel/PhysicsInstructions.h"		// TODO - minimize these includes, and in the .h file
#include "V8DataModel/Workspace.h"
#include "v8datamodel/PhysicsSettings.h"
#include "V8World/World.h"
#include "Network/Player.h"

LOGGROUP(CyclicExecutiveThrottling)

namespace RBX {

PhysicsInstructions::PhysicsInstructions()
: timeSinceLastRadiusChange(0.0)
, throttleTimer(0.0)
, throttleAdjustTime(PhysicsSettings::singleton().getThrottleAdjustTime())
, averageDt(30, 1.0)						// 7 samples should be enough ~= 1/4 second
, averageDutyDt(30, 1.0)
, averageCyclicDt(30, 1.0)
, requestedDutyPercent(0.0)
, bandwidthExceeded(false)
, networkBufferHealth(1.0)
{
}

double PhysicsInstructions::dPhysicsServerDutyPercent()
{
	// 3/11/09 - changed from 0.15 to 0.10
	// 11/28/12 - changed from 0.10 to 0.40 under a FASTFLAG to ease up iPad players' lives
	return 0.60;
}

/*
EThrottle:

	averageFps	instantFps	instantDuty
	Good		Good		Good				up
	Good		Bad			Good				up
	Good		Bad			Bad					down
	Good		Good		Bad					down
	Bad			Good		Good				down
	Bad			Bad			Good				down
	Bad			Bad			Bad					down
	Bad			Good		Bad					down
*/

void PhysicsInstructions::changeSimulationRadius(RBX::Network::Player* dPhysPlayer, float change)
{
	timeSinceLastRadiusChange = 0.0;

	RBXASSERT(dPhysPlayer);
	float currentRadius = dPhysPlayer->getSimulationRadius();
	float newRadius = change * currentRadius;
	dPhysPlayer->updateSimulationRadius(newRadius);
}

void PhysicsInstructions::changeMaxSimulationRadius(RBX::Network::Player* dPhysPlayer, float change)
{
	RBXASSERT(dPhysPlayer);
	float currentMaxRadius = dPhysPlayer->getMaxSimulationRadius();
	float newRadius = change * currentMaxRadius;
	dPhysPlayer->setMaxSimulationRadius(newRadius);
}

#pragma optimize( "", off )

double PhysicsInstructions::dPhysicsClientDutyPercent() {return 0.40;}		// note this is higher than normal
double PhysicsInstructions::dPhysicsClientEThrottleDutyPercent() {return 0.60;}		// note this is higher than normal

// dt	  : timespanSinceLastStep.seconds();
// dutyDt : timespanOfLastStep.seconds();

void PhysicsInstructions::setThrottlesBase(RBX::Network::Player* dPhysPlayer, Workspace* workspace, bool realTimePerfOK, bool dutyPerfOK, double avgDutyPercent, double dt)
{
	if (realTimePerfOK)
	{
		if (!dutyPerfOK)
		{
			throttleTimer += dt;

			if (throttleTimer >= throttleAdjustTime)
			{
				throttleTimer = 0.0;
				workspace->getWorld()->getEThrottle().increaseLoad(false);
			}
		}
		else
		{
			throttleTimer = 0.0;
			workspace->getWorld()->getEThrottle().increaseLoad(true);
		}
	}
	else // react immediately to low frame rate
	{
		throttleTimer = 0.0;
		workspace->getWorld()->getEThrottle().increaseLoad(false);
	}

	FASTLOG1F(FLog::CyclicExecutiveThrottling, "ThrottleTimer= %f", throttleTimer);
	//StandardOut::singleton()->printf(MESSAGE_INFO, "%f", workspace->getWorld()->getEnvironmentSpeed());

	if (!dPhysPlayer)
		return;

	if ((timeSinceLastRadiusChange += dt) < throttleAdjustTime)
		return;

	if (bandwidthExceeded)
		changeSimulationRadius(dPhysPlayer, 0.7);
	else if (!realTimePerfOK || avgDutyPercent > dPhysicsClientDutyPercent())
		changeSimulationRadius(dPhysPlayer, 0.7);
	else if (realTimePerfOK && avgDutyPercent < dPhysicsClientDutyPercent() - 0.2)
		changeSimulationRadius(dPhysPlayer, 1.05);

	if (networkBufferHealth <= 0.5)
		changeMaxSimulationRadius(dPhysPlayer, 0.95);
	else if (networkBufferHealth >= 0.9)
		changeMaxSimulationRadius(dPhysPlayer, 1.05);
}

void PhysicsInstructions::setCyclicThrottles(RBX::Network::Player* dPhysPlayer, Workspace* workspace, double cyclicDt, double dt, double dutyDt)
{

	averageCyclicDt.sample(cyclicDt);
	averageDt.sample(dt);
	averageDutyDt.sample(dutyDt);

	double avgCyclicDt = averageCyclicDt.getAverage();					// average world steps per dt
	double avgDt = averageDt.getAverage();								// average delta time between steps
	double avgDuty = averageDutyDt.getAverage();						// average time per step

	FASTLOG1F(FLog::CyclicExecutiveThrottling, "avgWorldStepFreq= %f", avgCyclicDt);
	bool stepsOK = ((avgCyclicDt/avgDt) > 0.6);

	// percentage of time the previous step took over the average frame time
	double prevDutyPercent = dutyDt / avgDt; 

	// ratio of avg time per step over avg delta time between steps, i.e. on average, what percentage of time per frame was occupied by physics step
	double avgDutyPercent = avgDuty / avgDt; 

	bool dutyOK = ((prevDutyPercent < requestedDutyPercent) && (avgDutyPercent < requestedDutyPercent));

	setThrottlesBase(dPhysPlayer, workspace, stepsOK, dutyOK, avgDutyPercent, dt);
}

void PhysicsInstructions::setThrottles(RBX::Network::Player* dPhysPlayer, Workspace* workspace, double dt, double dutyDt)
{
	averageDt.sample(dt);
	averageDutyDt.sample(dutyDt);

	double avgDt = averageDt.getAverage();			// average delta time between steps
	double avgDuty = averageDutyDt.getAverage();	// average time per step

	double avgFps = 1.0 / avgDt; 

	// percentage of time the previous step took over the average frame time
	double prevDutyPercent = dutyDt / avgDt; 
	
	// ratio of avg time per step over avg delta time between steps, i.e. on average, what percentage of time per frame was occupied by physics step
	double avgDutyPercent = avgDuty / avgDt; 

	bool fpsOK = (avgFps > 40);

	bool dutyOK = ((prevDutyPercent < requestedDutyPercent) && (avgDutyPercent < requestedDutyPercent));

	setThrottlesBase(dPhysPlayer, workspace, fpsOK, dutyOK, avgDutyPercent, dt);
}

#pragma optimize( "", on )


} // namespace


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag6 = 0;
    };
};
