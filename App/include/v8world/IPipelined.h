 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/IWorldStage.h"
#include "rbx/Debug.h"

namespace RBX {

	class Kernel;

	class RBXBaseClass IPipelined
	{
	private:
		IStage*		currentStage;

		void removeFromStage(IStage::StageType stageType);

		IStage* getStage(IStage::StageType stageType) const;	
	public:
		IPipelined() : currentStage(NULL)
		{}
	
		virtual ~IPipelined() {
			RBXASSERT(currentStage == NULL);
			currentStage = static_cast<IStage*>(Debugable::badMemory());
		}

		void putInPipeline(IStage* stage);

		void removeFromPipeline(IStage* stage);

		void putInStage(IStage* stage);

		void removeFromStage(IStage* stage);

		bool inPipeline() const {
			return (currentStage != NULL);
		}

		const IStage* getCurrentStage() const {return currentStage;}

		bool inStage(IStage::StageType stageType) const {
			RBXASSERT(currentStage);
			return (currentStage && (currentStage->getStageType() == stageType));
		}

		bool inStage(IStage* iStage) const {
			RBXASSERT(iStage);
			RBXASSERT(currentStage);
			return (currentStage == iStage);
		}

		bool inOrDownstreamOfStage(IStage::StageType stageType) const {
			RBXASSERT(currentStage);
			return (currentStage && (currentStage->getStageType() >= stageType));
		}

		bool inOrDownstreamOfStage(IStage* iStage) const {
			RBXASSERT(iStage);
			RBXASSERT(currentStage);
			return (currentStage && iStage && (currentStage->getStageType() >= iStage->getStageType()));
		}

		bool downstreamOfStage(IStage* iStage) const {
			RBXASSERT(iStage);
			RBXASSERT(currentStage);
			return (currentStage && iStage && currentStage->getStageType() > iStage->getStageType());
		}

		bool inKernel() const							{return inStage(IStage::KERNEL_STAGE);}
		Kernel* getKernel() const;						// should never fail		

		virtual void putInKernel(Kernel* kernel);
		virtual void removeFromKernel();

		World* findWorld() {
			if (!currentStage) {
				return NULL;
			}
			else {
				IStage* worldStage = (!inKernel()) ? currentStage : currentStage->getUpstream();
				return rbx_static_cast<IWorldStage*>(worldStage)->getWorld();
			}
		}
	};

}	// namespace
