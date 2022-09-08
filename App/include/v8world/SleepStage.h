/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Enum.h"
#include "V8World/Contact.h"
#include "Util/IndexArray.h"
#include "Util/G3DCore.h"
#include "boost/scoped_ptr.hpp"
#include "Util/RunningAverage.h"
#include <set>
#include <deque>


namespace RBX {

	class Assembly;
	class Edge;
	class Joint;
	class Contact;
	class Kernel;
	class SleepStage;

	namespace Profiling
	{
		class CodeProfiler;
	}


	class SleepStage : public IWorldStage {
	public:
		typedef std::set<Assembly*> AssemblySet;
		typedef std::set<Joint*> JointSet;
	private:
		typedef IWorldStage Super;
		// utility - prevent extra allocs on resize
		std::vector<Assembly*> toDeep;
		std::vector<Assembly*> toWake;
		std::vector<Assembly*> toSleepingChecking;
		std::vector<Contact*> toSleeping;
		std::vector<Contact*> toStepping;
		std::vector<Contact*> toContacting;
		std::vector<Contact*> toContactingSleeping;
		std::vector<Joint*> toSleepingJoint;

		int numContactsInStage;
		int numContactsInKernel;
		bool throttling;
		bool debugReentrant;
		int longStepId;

		typedef AssemblySet::iterator AssemblySetIt;
		typedef AssemblySet::const_iterator CAssemblySetIt;
		typedef IndexArray<Contact, &Contact::steppingIndexFunc> ContactList;
		typedef ContactList ContactLists[Sim::NUM_THROTTLE_TYPE];

		// defining objects

		AssemblySet			recursiveWakePending;				// only on impact...
		AssemblySet			wakePending;
		AssemblySet			awake;
		AssemblySet			sleepingChecking;					// Edges that are awake
		AssemblySet			sleepingDeeply;						// no Edges that are awake
		AssemblySet			removing;
		ContactLists		steppingContacts;
		ContactLists		touchingContacts;
		JointSet			steppingJoints;

		// Main Stepping functions
		int recursivePassId;
		bool externalRecursiveWake;

		void stepAssembliesRecursiveWakePending();

		void stepAssembliesWakePending();

		void doContacts(ContactLists& contactLists);

		void stepContacts(ContactList& contactList);

		void stepJoints();

		void stepAssembliesAwake();
		void stepAssembliesSleepingChecking();

		////////////////////////////////////////////////////////
		// Supporting Stepping functions
		//
		static float highVelocityContact();

		void wakeAssemblies(AssemblySet& wakeSet, int maxDepth, Sim::AssemblyState checkState);
		void traverse(Assembly* assembly, std::deque<Assembly*>& aDeque, int maxDepth);

		void wakeEdge(Edge* e);

		Sim::EdgeState computeContactState(bool assembliesMoving, bool inContact, bool canCollide, bool wasTouching);

		bool highVelocityNewTouch(Contact* c);

		void wakeEvent(Edge* e);
		void recursiveWakeEvent(Contact* c);
		void wakeEvent(Assembly* a);
		void recursiveWakeEvent(Assembly* a);

		void changeContactState(const std::vector<Contact*>& contacts, Sim::EdgeState newState);
		void changeJointState(const std::vector<Joint*>& joints, Sim::EdgeState newState);
		void changeAssemblyState(const std::vector<Assembly*>& assemblies, Sim::AssemblyState newState);

		void changeContactState(Contact* c, Sim::EdgeState newState);
		void changeJointState(Joint* j, Sim::EdgeState newState);
		void changeAssemblyState(Assembly* a, Sim::AssemblyState newState);

		AssemblySet& stateToSet(Sim::AssemblyState state);

		bool edgeIsAwake(Edge* e);
		bool isAffecting(Edge* e);
		
		bool atLeastOneAssemblyMoving(Assembly* a0, Assembly* a1);

		/////////////////////////////////////////////////////////////////
		//
		// Assembly functions

		bool shouldSleep(Assembly* a);
		bool preventNeighborSleep(Assembly* a);

		Sim::AssemblyState computeStateFromNeighbors(Assembly* a);

		bool forceNeighborAwake(Assembly* a);
		bool movingTooMuchToSleep(Assembly* a);

		bool validate();
		bool validateJoints();
	public:
		SleepStage(IStage* upstream, World* world);

		~SleepStage();


		///////////////////////////////////////////
		// IStage
		/*override*/ IStage::StageType getStageType() const {return IStage::SLEEP_STAGE;}

		/*override*/ int getMetric(IWorldStage::MetricType metricType);

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void stepSleepStage(int worldStepId, int uiStepId, bool _throttling);

		/////////////////////////////////////////////
		// From Upstream Collision Stage, World
		void onAssemblyAdded(Assembly* a);
		void onAssemblyRemoving(Assembly* a);
		void onExternalTickleAssembly(Assembly* a, bool recursive);

		int numTouchingContacts();
		const AssemblySet& getAwakeAssemblies() const {return awake;}

		///////////////////////////////////////////
		// Profiler
		boost::scoped_ptr<Profiling::CodeProfiler> profilingCollision;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingJointSleep;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingWake;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingSleep;
	};
} // namespace

