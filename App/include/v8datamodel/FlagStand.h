/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/BasicPartInstance.h"
#include "G3D/Vector3.h"
#include "v8tree/service.h"
#include "V8DataModel/JointInstance.h"
#include "Util/SteppedInstance.h"
#include "Util/BrickColor.h"

#include <vector>

namespace RBX {

extern const char* const sFlagStand;

class Flag;
class Stepped;

class FlagStand 
	: public DescribedCreatable<FlagStand, BasicPartInstance, sFlagStand>
{
private:
	typedef DescribedCreatable<FlagStand, BasicPartInstance, sFlagStand> Super;
	rbx::signals::scoped_connection_logged standTouched;
	void onEvent_standTouched(shared_ptr<Instance> other);

	shared_ptr<Flag> watchingFlag;
	shared_ptr<Flag> clonedReplacementFlag;

	void affixFlagToRandomEmptyStand(Flag* flag);

public:
	FlagStand();								
	rbx::signal<void(shared_ptr<Instance>)> flagCapturedSignal;
	void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	void onStepped();

	void affixFlag(Flag *flag);
	Flag *getJoinedFlag();

	BrickColor teamColor;
	BrickColor getTeamColor() const;
	void setTeamColor(BrickColor color);
};


extern const char *const sFlagStandService;

class FlagStandService
	: public DescribedNonCreatable<FlagStandService, Instance, sFlagStandService>
	, public IStepped
	, public Service
{
private:
	typedef DescribedNonCreatable<FlagStandService, Instance, sFlagStandService> Super;
	std::list<FlagStand *> flagStands;

	///////////////////////////////////////////////////////////////////////////
	// Instance
	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
		Super::onServiceProvider(oldProvider, newProvider);
		onServiceProviderIStepped(oldProvider, newProvider);
	}

	// Istepped
	/*override*/ void onStepped(const Stepped& event);

	FlagStand *findRandomEmptyStandForFlag(Flag *f);

public:
	FlagStandService();
	~FlagStandService();

	void affixFlagToRandomEmptyStand(Flag* flag);

	FlagStand *FindStandWithFlag(Flag *f);

	void RegisterFlagStand(FlagStand *fs);
	void UnregisterFlagStand(FlagStand *fs);
};


} // namespace