/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Assembly.h"
#include "Util/SimSendFilter.h"
#include "boost/scoped_ptr.hpp"
#include <set>

namespace RBX {
	class Assembly;
	class Mechanism;
	class Joint;
	class Region2;

/*
					Simulate					Physics Service (Send)
Client				NO  --- doesn’t step ---	NO --- doesn’t step --			
Server				ALL							If Sim							
Edit / Visit Solo	ALL							N0								
Dphysics Client:	Region or Address			If Sim							
Dphysics Server;	Address Match (null)		If Awake or Sim					
					(region is empty)		
*/	

	class SpatialFilter : public IWorldStage {
	public:
		typedef std::set<Assembly*> AssemblySet;

		bool inClientSimRegion(Assembly* a);
		bool addressMatch(Assembly* a);

		static bool sendingPhase(Assembly::FilterPhase phase) {return (phase == Assembly::NoSim_Send) || (phase == Assembly::NoSim_Send_Anim);}
		static bool simulatingPhase(Assembly::FilterPhase phase) {return ((phase == Assembly::Sim_SendIfSim) || (phase == Assembly::Sim_BufferZone));}
		static bool noSimPhase(Assembly::FilterPhase phase) {return ((phase == Assembly::NoSim_Send) || (phase == Assembly::NoSim_Send_Anim) || (phase == Assembly::NoSim_SendIfSim) || (phase == Assembly::NoSim_SendIfSim_Anim));}
		static bool animationPhase(Assembly::FilterPhase phase) {return ((phase == Assembly::NoSim_Send_Anim) || (phase == Assembly::NoSim_SendIfSim_Anim)); }

	private:

		class MoveInstructions {
		public:
			Assembly* a;
			Assembly::FilterPhase	from;
			Assembly::FilterPhase	to;

			MoveInstructions() : a(NULL), from(Assembly::NOT_ASSIGNED), to(Assembly::NOT_ASSIGNED)
			{}

			MoveInstructions(Assembly* _a, Assembly::FilterPhase _from, Assembly::FilterPhase _to) : a(_a), from(_from), to(_to)
			{}

			~MoveInstructions()
			{}
		};

		SimSendFilter filter;

		AssemblySet assemblies[Assembly::NUM_PHASES];						// no simulate assemblies and simulate assemblies

		G3D::Array<MoveInstructions> toMove;

		Assembly::FilterPhase filterAssembly(Assembly* a, bool simulating, Time wakeupNow);		// when simulating, this can affect the datamodel

		bool isNotClientAddress(Assembly* a);

		void changePhase(MoveInstructions& mi);
		void moveInto(MoveInstructions& mi);
		void removeFromPhase(Assembly* a);
		void moveAll(Assembly::FilterPhase destination);

		class MechToAssemblyStage* getMechToAssemblyStage();

		void filterAssemblies();

		void insertPrimitiveJoints(Primitive* p);
		void removePrimitiveJoints(Primitive* p);

	public:
		///////////////////////////////////////////
		// IStage
		SpatialFilter(IStage* upstream, World* world);

		~SpatialFilter();

		/*override*/ IStage::StageType getStageType()  const {return IStage::SPATIAL_FILTER;}

		void filterStep();

		void onMovingAssemblyRootAdded(Assembly* a, Time now);
		void onFixedAssemblyRootAdded(Assembly* a);

		void onAssemblyRootRemoving(Assembly* a);

		SimSendFilter& getSimSendFilter() {return filter;}

		const AssemblySet& getAssemblies(Assembly::FilterPhase phase) {
			RBXASSERT(phase < Assembly::NUM_PHASES);
			return assemblies[phase];
		}
	};
} // namespace
