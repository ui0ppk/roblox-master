#pragma once

#include "V8Tree/Service.h"
#include "Util/Runstateowner.h"
#include "Voxel/Cell.h"
#include "Voxel/CellChangeListener.h"
#include "Util/SpatialRegion.h"
#include <queue>

#include "Voxel2/GridListener.h"

namespace RBX {

class DataModel;
class MegaClusterInstance;

extern const char* const sChangeHistoryService;
class ChangeHistoryService 
	: public DescribedCreatable<ChangeHistoryService, Instance, sChangeHistoryService, Reflection::ClassDescriptor::INTERNAL>
	, public Service
	, public Voxel::CellChangeListener
	, public Voxel2::GridListener
{
private:
	typedef DescribedCreatable<ChangeHistoryService, Instance, sChangeHistoryService, Reflection::ClassDescriptor::INTERNAL> Super;
	DataModel* dataModel;

	class Item;
	class Waypoint;
	
	shared_ptr<MegaClusterInstance> megaClusterInstance;

	Waypoint* recording;

	typedef std::list<Waypoint*> Waypoints; // must be a list and not a vector because we maintain iterators after reallocs
	Waypoints waypoints;

	Waypoints::iterator     playWaypoint;	    // playing nextWaypoint is a "redo"
	Waypoints::iterator     unplayWaypoint;	    // unplaying unplayWaypoint is an "undo"
	Waypoints::iterator     runStartWaypoint;   // unplaying up to runStartWaypoint is a "reset"
	bool                    playing;
	bool                    enabled;
    int                     dataSize;
	shared_ptr<RunService>  runService;
    shared_ptr<Instance>    statsItem;

	rbx::signals::scoped_connection itemAddedConnection;
	rbx::signals::scoped_connection itemRemovedConnection;
	rbx::signals::scoped_connection itemChangedConnection;
	rbx::signals::scoped_connection runTransitionConnection;
public:
    static const int minWaypoints = 3;
	static const int maxWaypoints = 250;
    static const int maxMemoryUsage = 250 * 1024 * 1024;

	typedef enum { 
		Aggregate, // Don't allow waypoints while running. All changes are recorded and made part of the reset waypoint
		Snapshot,  // When recording waypoints while running, take a snapshot of positions and velocities
		Hybrid     // When recording waypoints while running, ignore unrelated position and velocity changes
	} RuntimeUndoBehavior;
	static RuntimeUndoBehavior runtimeUndoBehavior;

	ChangeHistoryService();
	~ChangeHistoryService();

	static void requestWaypoint(const char* name, const Instance* context);
	void resetBaseWaypoint();
	void clearWaypoints();

	void setEnabled(bool state);
	bool isEnabled() const { return enabled; }
	void requestWaypoint(const char* name);
	void requestWaypoint2(std::string name) { requestWaypoint(name.c_str()); }

	// TODO: rename play-->redo unplay-->undo???
	bool getPlayWaypoint(std::string& name, int steps = 0) const;
	bool getUnplayWaypoint(std::string& name, int steps = 0) const;
	bool canPlay() const { std::string name; return getPlayWaypoint(name); }
	bool canUnplay() const { std::string name; return getUnplayWaypoint(name); }
	
	// returns a tuple:  bool state, [string name]
	shared_ptr<const Reflection::Tuple> canUnplay2();
	shared_ptr<const Reflection::Tuple> canPlay2();
	//std::string getPlayName() const { std::string name; getPlayWaypoint(name); return name; }
	//std::string getUnplayName() const { std::string name; getUnplayWaypoint(name); return name; }

	void play();
	void playLua();
	void unplay();
	void unplayLua();
	
	//to show progress dialog in studio
	boost::function<void(boost::function<void()>, std::string)> withLongRunningOperation;

	bool isResetEnabled() const;
	void reset();

    int getWaypointDataSize() const { return dataSize; }
    int getWaypointCount() const    { return waypoints.size(); }

    rbx::signal<void()> waypointChangedSignal;
    rbx::signal<void(std::string)> undoSignal;
    rbx::signal<void(std::string)> redoSignal;

	MegaClusterInstance* getTerrain() const { return megaClusterInstance.get(); }

protected:
	virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
private:
	void attach();
	void dettach();
	void trimWaypoints();
	void mergeFirstTwoWaypoints();
    void computeDataSize();

	virtual void onItemAdded(shared_ptr<RBX::Instance> item);
	virtual void onItemRemoved(shared_ptr<RBX::Instance> item);
	virtual void onItemChanged(shared_ptr<RBX::Instance> item, const Reflection::PropertyDescriptor* descriptor);
	bool isRecordable(Instance* instance);

	/*override*/ virtual void terrainCellChanged(const Voxel::CellChangeInfo& cell);
	/*override*/ virtual void onTerrainRegionChanged(const Voxel2::Region& region);

	void setCell(const SpatialRegion::Id& chunkPos, const Vector3int16& cellInChunk,
		Voxel::Cell detail, Voxel::CellMaterial material);

	typedef enum { Accept, Reject } CheckResult;
	CheckResult checkSettingWaypoint();

	void onRunTransition(RunTransition event);
	void setWaypoint(const char* name);
	void setRunWaypoint();
	void reportMissedPhysicsChanges(shared_ptr<RBX::Instance> instance);
};

}
