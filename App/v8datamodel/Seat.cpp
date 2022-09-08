/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Seat.h"

DYNAMIC_FASTINTVARIABLE(ActionStationDebounceTime, 2)
DYNAMIC_FASTFLAGVARIABLE(FixAnchoredSeatingPosition, false)
DYNAMIC_FASTFLAGVARIABLE(FixSeatingWhileSitting, false)

namespace RBX {

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<Seat, bool> propDisabled("Disabled", "Control", &Seat::getDisabled, &Seat::setDisabled);
static const Reflection::RefPropDescriptor<Seat, Humanoid> propOccupant("Occupant", "Control", &Seat::getOccupant, NULL, Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::RemoteEventDesc<Seat, void(shared_ptr<Instance>)> event_createSeatWeld(&Seat::createSeatWeldSignal, "RemoteCreateSeatWeld", "humanoid", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY,	Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Seat, void()> event_destroySeatWeld(&Seat::destroySeatWeldSignal, "RemoteDestroySeatWeld", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

const char* const sSeat = "Seat";

Seat::Seat()
{
	setName(sSeat);
	setDisabled(false);

	createSeatWeldSignal.connect(boost::bind(static_cast<void (Seat::*)(shared_ptr<Instance>)>(&Seat::createSeatWeldInternal), this, _1));
	destroySeatWeldSignal.connect(boost::bind(&Seat::findAndDestroySeatWeldInternal, this));
}

Seat::~Seat(){
}

void Seat::createSeatWeld(Humanoid *h)
{
	Network::GameMode gameMode = Network::Players::getGameMode(h);
	if (gameMode == Network::DPHYS_CLIENT || gameMode == Network::CLIENT)
	{
		if (this->debounceTimeUp())
		{
			event_createSeatWeld.replicateEvent(this, shared_from(h));
			this->debounceTime = Time::now<Time::Fast>();
		}
	}
	else
		Super::createSeatWeld(h);
}

void Seat::findAndDestroySeatWeld()
{
	event_destroySeatWeld.fireAndReplicateEvent(this);
}

void Seat::onSeatedChanged(bool seated, Humanoid* humanoid)
{
	if (!seated && humanoid && (humanoid == Humanoid::getLocalHumanoidFromContext(this)))
	{
		humanoid->setSit(false);
	}
}

void Seat::setOccupant(Humanoid* value)
{
	if (occupant.get() != value)
	{
		occupant = shared_from(value);
		raisePropertyChanged(propOccupant);
	}
}

} // namespace