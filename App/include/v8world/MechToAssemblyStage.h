/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"

namespace RBX {
	class Assembly;
	class Mechanism;
	class AssemblyStage;

	class MechToAssemblyStage : public IWorldStage 
	{
	private:
		AssemblyStage* getAssemblyStage();

	public:
		MechToAssemblyStage(IStage* upstream, World* world);

		~MechToAssemblyStage();

		/*override*/ IStage::StageType getStageType()  const {return IStage::MECH_TO_ASSEMBLY_STAGE;}

		void onFixedAssemblyAdded(Assembly* a);
		void onFixedAssemblyRemoving(Assembly* a);

		void onSimulateAssemblyRootAdded(Assembly* a);
		void onSimulateAssemblyRootRemoving(Assembly* a);

		void onNoSimulateAssemblyRootAdded(Assembly* a);
		void onNoSimulateAssemblyRootRemoving(Assembly* a);
	};
} // namespace
