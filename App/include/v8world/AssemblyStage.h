/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/EdgeBuffer.h"

namespace RBX {
	class Assembly;
	class Primitive;

	class AssemblyStage : public EdgeBuffer {
	public:
		AssemblyStage(IStage* upstream, World* world);

		~AssemblyStage();

		/*override*/ IStage::StageType getStageType()  const {return IStage::ASSEMBLY_STAGE;}

		void onFixedAssemblyRootAdded(Assembly* a);
		void onFixedAssemblyRootRemoving(Assembly* a);

		void onNoSimulateAssemblyRootAdded(Assembly* a)			{onFixedAssemblyRootAdded(a);}
		void onNoSimulateAssemblyRootRemoving(Assembly* a)		{onFixedAssemblyRootRemoving(a);}

		void onNoSimulateAssemblyDescendentAdded(Assembly* a);
		void onNoSimulateAssemblyDescendentRemoving(Assembly* a);

		void onSimulateAssemblyRootAdded(Assembly* a);
		void onSimulateAssemblyRootRemoving(Assembly* a);

		void onSimulateAssemblyDescendentAdded(Assembly* a);
		void onSimulateAssemblyDescendentRemoving(Assembly* a);

		Assembly* onEngineChanging(Primitive* p);
		void onEngineChanged(Assembly* a);
	};
} // namespace
