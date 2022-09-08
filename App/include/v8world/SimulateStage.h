/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
// Note - used to be called "SimJobStage.h"

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Assembly.h"
#include <map>
#include "boost/intrusive/list.hpp"
#include <boost/unordered_map.hpp>

namespace RBX {

	class SimulateStage	: public IWorldStage
	{
	public:
		typedef boost::intrusive::list<Assembly, boost::intrusive::base_hook<SimulateStageHook> > Assemblies;

	private:
#if 0
		typedef boost::unordered_map<Assembly*, int> AssemblyMap;
#else
		typedef std::map<Assembly*, int> AssemblyMap;
#endif
		AssemblyMap movingAssemblyRoots;

		Assemblies movingDynamicAssemblies;
		Assemblies realTimeAssemblies;

		bool validateEdge(Edge* e);

		void putFirstMovingRootInSendPhysics(Assembly* a);
		void removeLastMovingRootFromSendPhysics(Assembly* a);
		bool removeFromSendPhysics(Assembly* a);

	public:
		SimulateStage(IStage* upstream, World* world);

		~SimulateStage();

		/*override*/ IStage::StageType getStageType() const {return IStage::SIMULATE_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void onAssemblyAdded(Assembly* assembly);
		void onAssemblyRemoving(Assembly* assembly);

		int getMovingDynamicAssembliesSize() { return movingDynamicAssemblies.size(); }
		Assemblies::iterator getMovingDynamicAssembliesBegin() {
			return movingDynamicAssemblies.begin();
		}
		Assemblies::iterator getMovingDynamicAssembliesEnd() {
			return movingDynamicAssemblies.end();
		}
		Assemblies::iterator getRealTimeAssembliesBegin() {
			return realTimeAssemblies.begin();
		}
		Assemblies::iterator getRealtimeAssembliesEnd() {
			return realTimeAssemblies.end();
		}
	};
} // namespace
