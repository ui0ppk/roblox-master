/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/IMoving.h"
#include "FastLog.h"
#include "v8datamodel/PartInstance.h"
#include "util/MovementHistory.h"
#include "v8world/Assembly.h"
#include "network/NetworkOwner.h"

LOGGROUP(GfxClusters)
LOGGROUP(GfxClustersFull)

FASTFLAGVARIABLE(CheckSleepOptimization, false)

namespace RBX {

#define MAX_STEPS_TO_SLEEP 30

IMoving::IMoving() 
: stepsToSleep(0)			
, lastUpdateTime(Time::nowFast())
, iMovingManager(NULL)
{}

IMoving::~IMoving()
{
    RBXASSERT(!iMovingManager);
}

void IMoving::makeMoving()
{
	RBXASSERT(stepsToSleep > 0);
	onSleepingChanged(false);
	if (iMovingManager) {
		iMovingManager->moved(this);
	}
}

void IMoving::setMovingManager(IMovingManager* _iMovingManager)
{
	if (iMovingManager) {
		iMovingManager->remove(this);
	}
	
	iMovingManager = _iMovingManager;

	if (iMovingManager && (stepsToSleep > 0)) {
		makeMoving();
	}
}

bool IMoving::checkSleep()
{
	RBXASSERT(stepsToSleep > 0);
    
	if (!FFlag::CheckSleepOptimization && isInContinousMotion())
        return false;

	if (stepsToSleep > 1)
	{
		stepsToSleep--;
		return false;
	}
    else if (FFlag::CheckSleepOptimization && isInContinousMotion())
	{
		stepsToSleep = MAX_STEPS_TO_SLEEP;
        return false;
	}
	else
	{
		RBXASSERT(stepsToSleep == 1);
		stepsToSleep = 0;
		onSleepingChanged(true);			// can aggregate now - going to sleep
        clearMovementHistory();
		return true;
	}
}

void IMoving::notifyMoved() 
{
	if (stepsToSleep == 0)
    {
        FASTLOG1(FLog::GfxClustersFull, "IMoving::notifyMoved wakes part %p", this);

		stepsToSleep = MAX_STEPS_TO_SLEEP;		// one second chatter
		makeMoving();
	}
	else
	{
		stepsToSleep = MAX_STEPS_TO_SLEEP;		// one second chatter
	}
}

void IMoving::forceSleep()
{
	stepsToSleep = 0;
    clearMovementHistory();
	if (iMovingManager) {
		iMovingManager->remove(this);
	}
}

/////////////////////////////////////////////////////////
const MovementHistory& IMoving::getMovementHistory() const
{
    if (movementHistory)
    {
        return *movementHistory;
    }
    else
    {
        return MovementHistory::getDefaultHistory();
    }
}

void IMoving::clearMovementHistory()
{
    if (movementHistory)
    {
        movementHistory->clearNodeHistory();
    }
}

void IMoving::addMovementNode(const CoordinateFrame& cFrame, const Velocity& velocity, const Time& timeStamp)
{
    if (!movementHistory)
    {
        // init movement history
        movementHistory.reset(new MovementHistory(cFrame, velocity, timeStamp));
    }
    else
    {
        movementHistory->addNode(cFrame, velocity, timeStamp);
    }
}

void IMoving::setLastCFrame(const CoordinateFrame& cFrame)
{
    if (lastCFrame)
    {
        *lastCFrame = cFrame;
    }
    else
    {
        lastCFrame.reset(new CoordinateFrame(cFrame));
    }
}

const CoordinateFrame& IMoving::getLastCFrame(const CoordinateFrame& defaultCFrame) const
{
    if (lastCFrame)
    {
        return *lastCFrame;
    }
    else
    {
        return defaultCFrame;
    }
}

void IMoving::setLastUpdateTime(const Time& time)
{
    lastUpdateTime = time;
}

const Time& IMoving::getLastUpdateTime() const
{
    return lastUpdateTime;
}

/////////////////////////////////////////////////////////

IMovingManager::IMovingManager()
{
	current = moving.end();
}

IMovingManager::~IMovingManager() 
{
	RBXASSERT(current == moving.end());
	RBXASSERT(moving.empty());
}

void IMovingManager::remove(IMoving* iMoving)
{
	if (current!=moving.end() && *current==iMoving)
		moving.erase(current++);
	else
		moving.erase(iMoving);
}

void IMovingManager::moved(IMoving* iMoving)
{
	moving.insert(iMoving);
}	

void IMovingManager::onMovingHeartbeat()		// put parts to sleep here if not moving for a long time, notify
{
    if(moving.size() > 0)
        FASTLOG(FLog::GfxClusters, "Moving heartbeat!");
    
	RBXASSERT(current == moving.end());
	current = moving.begin();
	while (current != moving.end()) 
	{
		IMoving* iMoving = *current;
		if (iMoving->checkSleep())
			remove(iMoving);
		else
			++current;
	}
}

void IMovingManager::updateHistory()
{
    RBXASSERT(current == moving.end());
    current = moving.begin();
    Time timeStamp = Time::nowFast();
    while (current != moving.end()) 
    {
        IMoving* iMoving = *current;
        if (iMoving->getConstPartPrimitiveVirtual() && Assembly::isAssemblyRootPrimitive(iMoving->getConstPartPrimitiveVirtual()))
        {
            SystemAddress owner = iMoving->getConstPartPrimitiveVirtual()->getNetworkOwner();
            if (owner == Network::NetworkOwner::Server() || owner == Network::NetworkOwner::ServerUnassigned())
            {
                // add the node only if it is stationary or if it has not been added actively
                iMoving->addMovementNode(iMoving->getConstPartPrimitiveVirtual()->getCoordinateFrame(), iMoving->getConstPartPrimitiveVirtual()->getPV().velocity, timeStamp);
            }
        }
        current++;
    }
}

} // namespace
