/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Joint.h"
#include "boost/scoped_ptr.hpp"
#include "boost/intrusive/list.hpp"

namespace RBX {
	class Assembly;

	namespace Profiling
	{
		class CodeProfiler;
	}

	class StepJointsStage : public IWorldStage {
	private:
		typedef boost::intrusive::list<Joint, boost::intrusive::base_hook<StepJointsStageHook> > Joints;
		Joints worldStepJoints;

		void addJoint(Joint* j);
		void removeJoint(Joint* j);

	public:
		///////////////////////////////////////////
		// IStage
		StepJointsStage(IStage* upstream, World* world);

		~StepJointsStage();

		/*override*/ IStage::StageType getStageType()  const {return IStage::STEP_JOINTS_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void onSimulateAssemblyAdded(Assembly* a);
		void onSimulateAssemblyRemoving(Assembly* a);

		void jointsStepWorld();

		boost::scoped_ptr<Profiling::CodeProfiler> profilingJointUpdate;
	};
} // namespace
