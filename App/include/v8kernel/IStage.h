 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/Debug.h"
#include "Util/G3DCore.h"

namespace RBX {

	class Kernel;

	class RBXBaseClass IStage {
	public:
		typedef enum {	CLEAN_STAGE,
						JOINT_STAGE,
						GROUND_STAGE,
						EDGE_STAGE,
						CONTACT_STAGE,
						TREE_STAGE,	
						MOVING_STAGE,
						SPATIAL_FILTER,
						MECH_TO_ASSEMBLY_STAGE,
						ASSEMBLY_STAGE,
						MOVING_ASSEMBLY_STAGE,
						STEP_JOINTS_STAGE,
						HUMANOID_STAGE,
						SLEEP_STAGE,
						SIMULATE_STAGE,
						KERNEL_STAGE} StageType;

	private:
		IStage*		upstream;
		IStage*		downstream;

		const IStage* findStageImpl(StageType stageType) const {
			const IStage* answer = this;
			while (answer->getStageType() != stageType) {
				answer = answer->getDownstream();
			}
			return answer;
		}

	public:
		IStage(IStage* upstream, IStage* downstream) 
			: upstream(upstream), downstream(downstream)
		{}

		virtual ~IStage() {
			if (downstream) {
				delete downstream;
			}
		}

		IStage* getUpstream()				{return upstream;}
		IStage* getDownstream()				{return downstream;}
		const IStage* getDownstream() const	{return downstream;}

		virtual StageType getStageType() const = 0;

		const IStage* findStage(StageType stageType) const {
			return findStageImpl(stageType);
		}

		IStage* findStage(StageType stageType) {
			return const_cast<IStage*>(findStageImpl(stageType));
		}

		virtual Kernel* getKernel()	{
			RBXASSERT(downstream);
			return downstream->getKernel();
		}
	};

}	// namespace