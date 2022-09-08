/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/IndexedMesh.h"
#include "V8World/Enum.h"
#include "V8World/Primitive.h"
#include "V8World/IPipelined.h"
#include "Util/ComputeProp.h"
#include "Util/PhysicsCoord.h"
#include "Util/Average.h"
#include "rbx/Debug.h"
#include <set>
#include "boost/intrusive/list.hpp"
#include "Network/CompactCFrame.h"

namespace RBX {

	class Joint;
	class Primitive;
	class Clump;
	class Edge;
	class MotorJoint;
	class Clump;
	class SimulateStage;
	class AssemblyHistory;

	typedef boost::intrusive::list_base_hook< boost::intrusive::tag<SimulateStage> > SimulateStageHook;
	
	class Assembly 
		: public IPipelined
		, public boost::noncopyable
		, public IndexedMesh
		, public SimulateStageHook
	{
		friend class SimJobStage;

	public:
		typedef enum {Sim_SendIfSim, Sim_BufferZone, NoSim_Send, NoSim_SendIfSim, NoSim_Send_Anim, NoSim_SendIfSim_Anim, NUM_PHASES, Fixed, NOT_ASSIGNED} FilterPhase;

	private:
        bool animationControlled;
		mutable bool inCode;

		unsigned char networkHumanoidState;

		AssemblyHistory*				history;

		Sim::AssemblyState				state;	

		G3D::Array<Edge*> assemblyExternalEdges;
		G3D::Array<Joint*>	assemblyMotors;

		// SimJobStage - index into either the stage, or a mechanism
		class SimJob*					simJob;

		// SleepStage - helper for recursive algorithm - elminate need for external std::set
		int								recursivePassId;
		int								recursiveDepth;

		// SpatialFilter - helper for current Phase
		FilterPhase						filterPhase;

		// Compute Props
		ComputeProp<float, Assembly>	maxRadius;		// farthest point on the primitives - used for getting max velocity
		float computeAssemblyMaxRadius();

		void gatherPrimitiveExternalEdges(Primitive* p);

		Clump* getAssemblyClump();
		const Clump* getConstAssemblyClump() const;

		void getAssemblyMotors(G3D::Array<Joint*>& motors, bool nonAnimatedOnly);
		void getConstAssemblyMotors(G3D::Array<const Joint*>& motors, bool nonAnimatedOnly) const;

		/////////////////////////////////////////////////////////////
		// IndexedMesh
		//
		/*override*/ void onLowersChanged();						// Primitives added/removed beneath me

	public:
		Assembly();

		~Assembly();

		Primitive* getAssemblyPrimitive();
		const Primitive* getConstAssemblyPrimitive() const;

		static Assembly* getPrimitiveAssembly(Primitive* p);
		static const Assembly* getConstPrimitiveAssembly(const Primitive* p);
		static Assembly* getPrimitiveAssemblyFast(Primitive* p);		// primitive must have an assembly

		static bool isAssemblyRootPrimitive(const Primitive* p);

		Assembly* otherAssembly(Edge* edge);
		const Assembly* otherConstAssembly(const Edge* edge) const;

		/////////////////////////////////////////////////////////////////

		bool getCanThrottle() const;

		static bool computeCanThrottle(Edge* edge);

		Vector2 get2dPosition() const;

		static bool computeIsGroundingPrimitive(const Primitive* p);			// in the engine, requestFIxed or RigidJoined to a fixed primitive

		bool computeIsGrounded() const;			

		void notifyMovedFromInternalPhysics();

		void notifyMovedFromExternal();

		// From SpatialFilter
		FilterPhase getFilterPhase() const {return filterPhase;}
		void setFilterPhase(FilterPhase value) {filterPhase = value;}

		// From SimJobStage
		void setSimJob(SimJob* s) {simJob = s;}
		SimJob* getSimJob() {return simJob;}
		const SimJob* getConstSimJob() const {return simJob;}

		// From SleepStage
		void reset(Sim::AssemblyState newState);		// resets state, sleep count, running average
		bool sampleAndNotMoving();
		bool preventNeighborSleep();
		void wakeUp();									// moving from sleeping to non-Sleeping state

		Sim::AssemblyState getAssemblyState() const;
		void setAssemblyState(Sim::AssemblyState value)	{state = value;}

		void setRecursivePassId(int value)		{recursivePassId = value;}
		int getRecursivePassId() const			{return recursivePassId;}

		void setRecursiveDepth(int value)		{recursiveDepth= value;}
		int getRecursiveDepth() const			{return recursiveDepth;}

		bool getAssemblyIsMovingState() const {
			return (Sim::isMovingAssemblyState(state));
		}

		float computeMaxRadius()				{return maxRadius.getValue();}
		float getLastComputedRadius() const		{return maxRadius.getLastComputedValue();}
		float isComputedRadiusDirty() const		{return maxRadius.getDirty();}

		// Replicated attributes (essentially used as Humanoid State)
		unsigned char getNetworkHumanoidState() const		{return networkHumanoidState;}
		void setNetworkHumanoidState(unsigned char value)	{networkHumanoidState = value;}

		const G3D::Array<Edge*>& getAssemblyEdges();

		void setPhysics(const G3D::Array<CompactCFrame>& motorAngles, const PV& pv);
		void getPhysics(G3D::Array<CompactCFrame>& motorAngles) const;

		template<class Func>
		inline void visitAssemblies(Func func) {
			this->visitMeAndChildren<Assembly, Func>(func);
		}

		template<class Func>
		inline void visitDescendentAssemblies(Func func) {
			this->visitDescendents<Assembly, Func>(func);
		}

		template<class Func>
		inline void visitConstDescendentAssemblies(Func func) const {
			this->visitConstDescendents<Assembly, Func>(func);
		}

        bool isAnimationControlled() const { return animationControlled; }
        void setAnimationControlled(bool val) { animationControlled = val; }

		// Primitive Visiting Functions

	private:
		template<class Func>
		inline void visitPrimitivesImpl(Func func, Primitive* p) {
			func(p);
			for (int i = 0; i < p->numChildren(); ++i) {
				Primitive* child = p->getTypedChild<Primitive>(i);
				if (!Assembly::isAssemblyRootPrimitive(child)) {
					visitPrimitivesImpl(func, child);
				}
			}
		}


		template<class Func>
		inline Primitive* findFirstPrimitiveImpl(Func func, Primitive* p) {
			if (func(p)) {
				return p;
			}
			for (int i = 0; i < p->numChildren(); ++i) {
				Primitive* child = p->getTypedChild<Primitive>(i);
				if (!Assembly::isAssemblyRootPrimitive(child)) {
					if (findFirstPrimitiveImpl(func, child)) {
						return child;
					}
				}
			}
			return NULL;
		}

	public:
		template<class Func>
		inline void visitPrimitives(Func func) {
			Primitive* p = getAssemblyPrimitive();
			RBXASSERT(p);
			visitPrimitivesImpl(func, p);
		}

		template<class Func>
		inline Primitive* findFirstPrimitive(Func func) {
			Primitive* p = getAssemblyPrimitive();
			RBXASSERT(p);
			return findFirstPrimitiveImpl(func, p);
		}
	};

}// namespace
