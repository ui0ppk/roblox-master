/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "Network/Players.h"
#include "rbx/Debug.h"

namespace RBX {

const char *const sPhysicsService = "PhysicsService";

PhysicsService::~PhysicsService()
{
	RBXASSERT(parts.empty());
	RBXASSERT(!assemblyPhysicsOnConnection.connected());
	RBXASSERT(!assemblyPhysicsOffConnection.connected());
}


void PhysicsService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	assemblyPhysicsOnConnection.disconnect();
	assemblyPhysicsOffConnection.disconnect();
	
	touchesConnection.disconnect();
	playersChangedConnection.disconnect();

	Super::onServiceProvider(oldProvider, newProvider);
	
	onServiceProviderHeartbeatInstance(oldProvider, newProvider);		// hooks up heartbeat

	if (Workspace* w = ServiceProvider::find<Workspace>(newProvider)) 
	{

		touchesConnection = w->stepTouch.connect(boost::bind(&PhysicsService::onTouchStep, this, _1));
		Network::Players* players = ServiceProvider::find<Network::Players>(this);
		playersChangedConnection = players->combinedSignal.connect(boost::bind(&PhysicsService::onPlayersChanged, this, _1, _2));

		SendPhysics* sendPhysics = w->getWorld()->getSendPhysics();
		if (sendPhysics)
		{
			assemblyPhysicsOnConnection = sendPhysics->assemblyPhysicsOnSignal.connect(boost::bind(&PhysicsService::onAssemblyPhysicsOn, this, _1));
			assemblyPhysicsOffConnection = sendPhysics->assemblyPhysicsOffSignal.connect(boost::bind(&PhysicsService::onAssemblyPhysicsOff, this, _1));
		}
	}
}


void PhysicsService::onAssemblyPhysicsOn(Primitive* primitive)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT(primitive->getWorld());		// confirm the Part is in Workspace / primitive is in World
	PartInstance* part = PartInstance::fromPrimitive(primitive);

	RBXASSERT(Assembly::isAssemblyRootPrimitive(part->getPartPrimitive()));

	// force update assembly radius so farther usage is not dirty
	primitive->getAssembly()->computeMaxRadius();

	shared_ptr<PartInstance> sharedPart = shared_from<PartInstance>(part);
	assemblyAddingSignal(sharedPart);

	RBXASSERT(!part->PhysicsServiceHook::is_linked());
	parts.insert(*part);

	if (!iAmServer)
	{
		iAmServer = Workspace::serverIsPresent(this);
	}
	if (iAmServer)
	{
		part->addMovementNode(part->getCoordinateFrame(), part->getVelocity(), Time::nowFast());
	}
}



void PhysicsService::onAssemblyPhysicsOff(Primitive* primitive)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT(primitive->getWorld());		// confirm the Part is in Workspace / primitive is in World
	PartInstance* part = PartInstance::fromPrimitive(primitive);

	RBXASSERT(Assembly::isAssemblyRootPrimitive(part->getPartPrimitive()));

	RBXASSERT(part->PhysicsServiceHook::is_linked());
	parts.remove_element(*part);

	assemblyRemovedSignal(shared_from<PartInstance>(part));
}

void PhysicsService::onHeartbeat(const Heartbeat& event)
{	
	if ((touchesSendList.empty() && !touchesReceiveList.empty()) || touchSentCounter >= touchResetCount)
	{
		touchesSendList.clear();
		touchesSendList.swap(touchesReceiveList);
		touchSentCounter = 0;
		touchSendListId++;
	}
}

void PhysicsService::onTouchStep(const TouchPair& tp)
{
	touchesReceiveList.insert(tp);
}

void PhysicsService::getTouches(std::list<TouchPair>& out)
{
	out.insert(out.end(), touchesSendList.begin(), touchesSendList.end());
}

void PhysicsService::onTouchesSent()
{
	touchSentCounter++;
}

void PhysicsService::onPlayersChanged(Instance::CombinedSignalType type, const ICombinedSignalData* data)
{
	if (Network::Players::backendProcessing(this))
	{
		if (type == Instance::CHILD_ADDED || type == Instance::CHILD_REMOVED)
			touchResetCount = Network::Players::getPlayerCount(this);
	}
}

} // namespace RBX