/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/BasicPartInstance.h"
#include "G3D/Vector3.h"
#include "v8tree/instance.h"
#include "v8tree/service.h"


#include <vector>

namespace RBX {

	namespace Network {
		class Player;
	}

	class ModelInstance;
	class Workspace;

extern const char* const sSpawnLocation;
class SpawnLocation 
	: public DescribedCreatable<SpawnLocation, BasicPartInstance, sSpawnLocation>
{
private:
	friend class SpawnerService;
	typedef DescribedCreatable<SpawnLocation, BasicPartInstance, sSpawnLocation> Super;
	
	BrickColor teamColor;
	bool enabled;

	rbx::signals::scoped_connection_logged spawnerTouched;
	void onEvent_spawnerTouched(shared_ptr<Instance> other);
	void updateSpawnerTouched();

public:
	
	SpawnLocation();								
	~SpawnLocation();								
	
	void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	void onAncestorChanged(const AncestorChanged& event);

	void onAllowTeamChangeOnTouchChanged(const Reflection::PropertyDescriptor&)
	{
		updateSpawnerTouched();
	}

	bool neutral;
	bool allowTeamChangeOnTouch;
	int forcefieldDuration;

	static Reflection::BoundProp<bool> prop_Neutral;
	static Reflection::BoundProp<bool> prop_AllowTeamChangeOnTouch;
	static Reflection::BoundProp<int>  prop_SpawnForcefieldDuration;


	BrickColor getTeamColor() const;
	void setTeamColor(BrickColor color);

	bool getEnabled() const			{return enabled; }
	void setEnabled(bool value);

};


extern const char *const sSpawnerService;

class SpawnerService
	: public DescribedNonCreatable<SpawnerService, Instance, sSpawnerService>
	, public Service
{

public:
	SpawnerService();
	~SpawnerService();
	void RegisterSpawner(SpawnLocation *s);
	void UnregisterSpawner(SpawnLocation *s);
	SpawnLocation* GetSpawnLocation(Network::Player *player, std::string preferedSpawnName);
	bool SpawnPlayer(shared_ptr<ModelInstance> model, Network::Player *player, std::string preferedSpawnName);
	static void SpawnPlayer(Workspace* workspace, shared_ptr<ModelInstance> model, CoordinateFrame location, int forceFieldDuration);
	void ClearContents();
private:
	std::list<SpawnLocation *> spawners;
};



} // namespace