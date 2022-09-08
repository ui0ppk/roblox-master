/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/FlagStand.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "V8DataModel/Team.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Flag.h"
#include "V8DataModel/Workspace.h"
#include "V8World/Primitive.h"
#include "V8World/Joint.h"
#include "V8World/RigidJoint.h"

namespace RBX {

using namespace RBX::Network;

const char* const sFlagStand = "FlagStand";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<FlagStand, BrickColor> prop_Color("TeamColor", category_Data, &FlagStand::getTeamColor, &FlagStand::setTeamColor);

static Reflection::EventDesc<FlagStand, void(shared_ptr<Instance>)> event_FlagCaptured(&FlagStand::flagCapturedSignal, "FlagCaptured", "player");
REFLECTION_END();

FlagStand::FlagStand() :
	standTouched(FLog::TouchedSignal)
{
	setName(sFlagStand);
}

BrickColor FlagStand::getTeamColor() const { return teamColor; }
void FlagStand::setTeamColor(BrickColor color) {
	this->teamColor = color;
	raisePropertyChanged(prop_Color);
}

void FlagStand::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	// either entering or leaving the world
	if(oldProvider == NULL) {
		if (Network::Players::backendProcessing(this)) {
			FlagStandService *fss = ServiceProvider::create<FlagStandService>(newProvider);
			fss->RegisterFlagStand(this);
			standTouched = this->onDemandWrite()->touchedSignal.connect(boost::bind(&FlagStand::onEvent_standTouched, this, _1));
			FASTLOG2(FLog::TouchedSignal, "Connecting Flagstand to touched signal, instance: %p, part signal: %p", 
				this, &onDemandRead()->touchedSignal);
		}
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if(newProvider == NULL) {
		if (Network::Players::backendProcessing(this)) {
			FlagStandService *fss = ServiceProvider::create<FlagStandService>(oldProvider);
			fss->UnregisterFlagStand(this);
			standTouched.disconnect();
		}
	}
}

// Note - using onStepped means this will only happen/function while running, not while editing
void FlagStand::onStepped()
{
	// If we have a joined flag, then reset the watching flag and clone it;
	if (Flag* joinedFlag = getJoinedFlag()) {
		if (watchingFlag.get() != joinedFlag) {
			watchingFlag = shared_from<Flag>(joinedFlag);
			clonedReplacementFlag = shared_polymorphic_downcast<Flag>(joinedFlag->clone(EngineCreator));	// clone flag + handle etc.
		}
	}

	// If we don't have a flag, check to see if we have lost a watching flag and still have a cloned flag - if so attach it.
	else if (watchingFlag.get()) {
		if (watchingFlag->getJoinedStand()) {		// joined somewhere else - I no longer watch
			watchingFlag.reset();
			clonedReplacementFlag.reset();
		}
		else if (!watchingFlag->getParent())  {		// destroyed or deleted while game running
			watchingFlag.reset();
			FlagStandService *fss = ServiceProvider::create<FlagStandService>(this);
			fss->affixFlagToRandomEmptyStand(clonedReplacementFlag.get());
			clonedReplacementFlag.reset();
		}
	}
}

void FlagStand::onEvent_standTouched(shared_ptr<Instance> other)
{
	if (Network::Players::backendProcessing(this) && other)
	{
		Instance* touchingCharacter = other->getParent();
		Humanoid* humanoid = Humanoid::modelIsCharacter(touchingCharacter);
		if (!humanoid) return;

		Flag *flag = Instance::fastDynamicCast<Flag>(touchingCharacter->findFirstChildOfType<Flag>());
		if (!flag) 
			return;

		Player *p = Players::getPlayerFromCharacter(touchingCharacter);
		if (!p)
			return;

		if (p->getNeutral()) 
			// Neutral player's can't cap the flag
			return;

		// A flag is captured when a player on the same team as the flag stand touches the stand while holding an off-color flag.
		if (	(p->getTeamColor() == this->getTeamColor())
			&&	(flag->getTeamColor() != this->getTeamColor())	)
		{
			flagCapturedSignal(shared_from(p));
			FlagStandService *fss = ServiceProvider::create<FlagStandService>(this);
			fss->affixFlagToRandomEmptyStand(flag);
		}
	}
}


void FlagStand::affixFlag(Flag *flag) 
{	
	RBXASSERT(Network::Players::backendProcessing(this));

	if (!getJoinedFlag())
	{
		flag->setParent(this->getParent());

		if (PartInstance *part = flag->getHandle())
		{
			// orientant flag
			part->setCoordinateFrame(CoordinateFrame());

			Vector3 pt = this->getLocation().translation;
			pt.y += .5f; // little hack - fix

			part->moveToPointAndJoin(pt);
			part->join();					// Note - should be redundant
		}
		else
		{
			RBXASSERT_FISHING(0);
		}
	}
}


Flag *FlagStand::getJoinedFlag()
{
	RBXASSERT(Network::Players::backendProcessing(this));

	// Return true iff the flag stand prim is directly jointed to an object of type Flag
	Primitive *stand_prim = this->getPartPrimitive();
	RigidJoint *r = stand_prim->getFirstRigid();
	while(r != NULL)
	{
		PartInstance *p = PartInstance::fromPrimitive(r->otherPrimitive(stand_prim));
		Flag *f = Instance::fastDynamicCast<Flag>(p->getParent()); // ASSUME: A part's parent will be the flag, if this is a flag.
		if (f) return f;
		r = stand_prim->getNextRigid(r);
	}

	return NULL;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////

const char *const sFlagStandService = "FlagStandService";

FlagStandService::FlagStandService()
{
	setName(sFlagStandService);
	Instance::propArchivable.setValue(this, false); // archivable(false) in cstor def. doesn't work for this.
	FASTLOG1(FLog::ISteppedLifetime, "FlagStandService created - %p", this);
}

FlagStandService::~FlagStandService() 
{
	FASTLOG1(FLog::ISteppedLifetime, "FlagStandService destroyed - %p", this);
}

void FlagStandService::onStepped(const Stepped& event)
{
	std::list<FlagStand *>::iterator iter;
	for(iter = flagStands.begin(); iter != flagStands.end(); iter++)
	{
		(*iter)->onStepped();
	}
}

void FlagStandService::RegisterFlagStand(FlagStand *fs)
{
	flagStands.push_back(fs);
}

void FlagStandService::UnregisterFlagStand(FlagStand *fs)
{
	flagStands.remove(fs);
}

void FlagStandService::affixFlagToRandomEmptyStand(Flag* flag)
{
	if (FlagStand *stand = findRandomEmptyStandForFlag(flag)) {
		stand->affixFlag(flag);
	}
}


FlagStand *FlagStandService::findRandomEmptyStandForFlag(Flag *f)
{
	// Pick a random flag stand of the same color as the flag
	// Check to make sure that stand is not occupied.
	
	if (flagStands.empty()) return NULL;
	
	std::vector<FlagStand *> possibleLocations;
	std::list<FlagStand *>::iterator iter;
	
	for(iter = flagStands.begin(); iter != flagStands.end(); iter++)
	{
		FlagStand* flagStand = *iter;

		if (	(flagStand->getTeamColor() == f->getTeamColor()) 
			&&	!flagStand->getJoinedFlag()	)
		{
			possibleLocations.push_back(*iter);
		}
	}
	
	if (possibleLocations.empty()) return NULL; // no possible locations, return default

	int i = rand() % possibleLocations.size();

	return possibleLocations[i];
}

FlagStand *FlagStandService::FindStandWithFlag(Flag *f)
{
	// Determines if the given flag is in any flag stand
	if (flagStands.empty()) return NULL;

	std::list<FlagStand *>::iterator iter;
	
	for(iter = flagStands.begin(); iter != flagStands.end(); iter++)
	{
		if ((*iter)->getJoinedFlag() == f)
		{
			return *iter;
		}
	}
	
	return NULL;
}


} // namespace
