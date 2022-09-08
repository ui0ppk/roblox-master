/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/SpawnLocation.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/Team.h"
#include "V8DataModel/Forcefield.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/DebrisService.h"

namespace RBX {

const char* const  sSpawnLocation = "SpawnLocation";

REFLECTION_BEGIN();
Reflection::BoundProp<int>  SpawnLocation::prop_SpawnForcefieldDuration("Duration", "Forcefield", &SpawnLocation::forcefieldDuration);
Reflection::BoundProp<bool> SpawnLocation::prop_Neutral("Neutral", "Teams", &SpawnLocation::neutral);
Reflection::BoundProp<bool> SpawnLocation::prop_AllowTeamChangeOnTouch("AllowTeamChangeOnTouch", "Teams", &SpawnLocation::allowTeamChangeOnTouch, &SpawnLocation::onAllowTeamChangeOnTouchChanged);
static const Reflection::PropDescriptor<SpawnLocation, BrickColor> prop_TeamColor("TeamColor", "Teams", &SpawnLocation::getTeamColor, &SpawnLocation::setTeamColor);
static Reflection::PropDescriptor<SpawnLocation, bool> prop_Enabled("Enabled", category_Behavior, &SpawnLocation::getEnabled, &SpawnLocation::setEnabled);
REFLECTION_END();

SpawnLocation::SpawnLocation() :
	neutral(true),
	allowTeamChangeOnTouch(false),
	forcefieldDuration(10),
	enabled(true),
	spawnerTouched(FLog::TouchedSignal)
{
	setName(sSpawnLocation);
}

SpawnLocation::~SpawnLocation()
{
}

BrickColor SpawnLocation::getTeamColor() const { return teamColor; }
void SpawnLocation::setTeamColor(BrickColor color) {
	this->teamColor = color;
	raisePropertyChanged(prop_TeamColor);
}

void SpawnLocation::onEvent_spawnerTouched(shared_ptr<Instance> other)
{
	if (Network::Players::backendProcessing(this) && other) {
		RBXASSERT(allowTeamChangeOnTouch);
		if (Instance* touchingCharacter = other->getParent()) {
			if (Humanoid::modelIsCharacter(touchingCharacter)) {
				if (Network::Player *p = Network::Players::getPlayerFromCharacter(touchingCharacter)) {
 					p->setTeamColor(getTeamColor());
					p->setNeutral(this->neutral);
				}
			}
		}
	}
}

void SpawnLocation::updateSpawnerTouched()
{
	if (allowTeamChangeOnTouch && !Network::Players::clientIsPresent(this, false) && getEnabled())
	{
		if (!spawnerTouched.connected())
		{
			spawnerTouched = this->onDemandWrite()->touchedSignal.connect(boost::bind(&SpawnLocation::onEvent_spawnerTouched, this, _1));
			FASTLOG2(FLog::TouchedSignal, "Connecting Spawner to touched signal, instance: %p, signal: %p", this, &onDemandRead()->touchedSignal);
		}
	}
	else
	{
		if (spawnerTouched.connected())
		{
			FASTLOG2(FLog::TouchedSignal, "Disconnecting Spawner from touched signal, instance: %p, signal: %p", this, &onDemandRead()->touchedSignal);
			spawnerTouched.disconnect();
		}
	}
}

void SpawnLocation::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);		// Note - moved this to first

	SpawnerService *ss = ServiceProvider::find<SpawnerService>(this);
	if (ss) 
	{
		ss->UnregisterSpawner(this);
		if (isDescendantOf(RBX::ServiceProvider::find<RBX::Workspace>(this)) )
		{
			ss->RegisterSpawner(this);
		}
	}
}

void SpawnLocation::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);

	// either entering or leaving the world
	if (newProvider)
	{
		SpawnerService *ss = ServiceProvider::create<SpawnerService>(newProvider);
		RBXASSERT(ss);

		// entering
		if (isDescendantOf(RBX::ServiceProvider::find<RBX::Workspace>(newProvider)) )
		{
			ss->RegisterSpawner(this);
		}
	}

	if (oldProvider)
	{
		// leaving

		SpawnerService *ss = ServiceProvider::find<SpawnerService>(oldProvider);
		RBXASSERT(ss);
		ss->UnregisterSpawner(this);
	}

	updateSpawnerTouched();
}

void SpawnLocation::setEnabled(bool value)
{
	if (value != enabled)
	{
		enabled = value;
		raisePropertyChanged(prop_Enabled);
		updateSpawnerTouched();
	}
}

const char *const sSpawnerService = "SpawnerService";

SpawnerService::SpawnerService()
{
	setName(sSpawnerService);
	Instance::propArchivable.setValue(this, false); // archivable(false) in cstor def. doesn't work for this.
	
}
SpawnerService::~SpawnerService() 
{

}

void SpawnerService::RegisterSpawner(SpawnLocation *s)
{
	spawners.push_back(s);
}

void SpawnerService::UnregisterSpawner(SpawnLocation *s)
{
	spawners.remove(s);
}

void SpawnerService::ClearContents()
{
	spawners.clear();
}

SpawnLocation* SpawnerService::GetSpawnLocation(Network::Player *p, std::string preferedSpawnName)
{
	std::vector<SpawnLocation *> possibleLocations;
	std::list<SpawnLocation *>::iterator iter;

	if (SpawnLocation* spawn = p->getDangerousRespawnLocation())
	{
		if  (preferedSpawnName.empty() && (spawn->neutral || p->getTeamColor() == spawn->getTeamColor()))
		{
			if (RBX::Workspace* pWorkspace = RBX::Workspace::findWorkspace(this))
			{
				if (spawn->isDescendantOf(pWorkspace) && spawn->getEnabled())
					return spawn;
			}
		}
	}

	for(iter = spawners.begin(); iter != spawners.end(); iter++)
	{
		if ((*iter)->getEnabled()) 
		{
			if ((*iter)->neutral) {
				// this spawn point is neutral, anyone can use it
				possibleLocations.push_back(*iter);
			} 
			else {
				// this is a team colored spawn point, only members of that team can use it
				if (!p->getNeutral() && (p->getTeamColor() == (*iter)->getTeamColor()))
				{
					possibleLocations.push_back(*iter);
				}
			}
		}
	}

	if (possibleLocations.empty()) 
	{
		return NULL;
	}
	else
	{
		SpawnLocation *sl = NULL;

		if (!preferedSpawnName.empty())
		{
			for (size_t i = 0;i < possibleLocations.size();i ++) 
			{
				if (possibleLocations[i]->getName() == preferedSpawnName)
				{
					sl = possibleLocations[i];
					break;
				}						
			}
		}

		if (sl == NULL) 
		{
			sl = possibleLocations[rand() % possibleLocations.size()];
		}

		return sl;
	}
}

bool SpawnerService::SpawnPlayer(shared_ptr<ModelInstance> model, Network::Player *p, std::string preferedSpawnName)
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	if (!workspace)		// should only be called when Player is in Datamodel
		throw std::runtime_error("SpawnPlayer couroldn't get the workspace");

	if (SpawnLocation* sl = GetSpawnLocation(p, preferedSpawnName))
	{
		SpawnPlayer(workspace, model, sl->getCoordinateFrame(), sl->forcefieldDuration);
		return true;
	}

	return false;
}

void SpawnerService::SpawnPlayer(Workspace* workspace, shared_ptr<ModelInstance> model, CoordinateFrame location, int forceFieldDuration)
{
	RBXASSERT(workspace);
	if (!workspace)		// should only be called when Player is in Datamodel
		return;

	Vector3 dropPoint = location.translation;
	dropPoint.y += 7; // hard-coded height of character (very bad) - use model:getExtents

	if (forceFieldDuration > 0) 
	{
		// conjure up a forcefield for this foo
		shared_ptr<ForceField> f = Creatable<Instance>::create<ForceField>();
		f->setParent(model.get());

		// hook it into the debris service so it expires
		DebrisService *ds = ServiceProvider::create<DebrisService>(workspace);
		RBXASSERT(ds);
		ds->addItem(f, forceFieldDuration);
	}

	workspace->moveToPoint(model.get(), dropPoint, DRAG::UNJOIN_NO_JOIN);

	ModelInstance *pModel = model.get();
	if (pModel) 
	{
		Vector3 lookAt = location.lookVector();
		if (lookAt.fuzzyEq(Vector3::unitY())) 
		{
			lookAt = location.upVector();
		}
		lookAt.y = 0.0f;
		lookAt = lookAt.unit();

		CoordinateFrame rotated;
		rotated.lookAt(lookAt, Vector3::unitY());

		rotated.translation = pModel->getPrimaryCFrame().translation;
		pModel->setPrimaryCFrame(rotated);
	}
}

} // namespace
