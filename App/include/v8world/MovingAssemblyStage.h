/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "V8World/Joint.h"
#include "v8world/Assembly.h"


namespace RBX {
	class Assembly;

	class MovingAssemblyStage : public IWorldStage 
	{
	private:
		///////////////////////////////////////
		typedef boost::intrusive::list<Joint, boost::intrusive::base_hook<MovingAssemblyStageHook> > Joints;
		Joints uiStepJoints;

		boost::unordered_set<Joint*> animatedJoints;
		
		typedef std::set<Assembly*> Assemblies;
		Assemblies movingGroundedAssemblies;
		Assemblies movingAnimatedAssemblies;

		void addJoint(Joint* j);
		void removeJoint(Joint* j);

		void jointsStepUiInternal(double distributedGameTime, Joint* j, bool fromAnimation);

	public:
		MovingAssemblyStage(IStage* upstream, World* world);

		~MovingAssemblyStage();

		/*override*/ IStage::StageType getStageType()  const {return IStage::MOVING_ASSEMBLY_STAGE;}

		/*override*/ void onEdgeAdded(Edge* e);
		/*override*/ void onEdgeRemoving(Edge* e);

		void addAnimatedJoint(Joint* j);
		void removeAnimatedJoint(Joint* j);
		
		void jointsStepUi(double distributedGameTime);

		void onSimulateAssemblyAdded(Assembly* a);
		void onSimulateAssemblyRemoving(Assembly* a);

		void addMovingGroundedAssembly(Assembly* a);
		void removeMovingGroundedAssembly(Assembly* a);

		void addMovingAnimatedAssembly(Assembly* a);
		void removeMovingAnimatedAssembly(Assembly *a);

		int getMovingGroundedAssembliesSize() { return movingGroundedAssemblies.size(); }

		Assemblies::iterator getMovingGroundedAssembliesBegin() {
			return movingGroundedAssemblies.begin();
		}
		Assemblies::iterator getMovingGroundedAssembliesEnd() {
			return movingGroundedAssemblies.end();
		}
		const Assemblies&	getMovingGroundedAssemblies() {
			return movingGroundedAssemblies;
		}

		const Assemblies& getMovingAnimatedAssemblies() {
			return movingAnimatedAssemblies;
		}
	};
} // namespace
