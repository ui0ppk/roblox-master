/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/IPipelined.h"
#include "V8Kernel/Kernel.h"

namespace RBX {


Kernel* IPipelined::getKernel() const				
{
	RBXASSERT(this->inKernel());
	IStage* answer = getStage(IStage::KERNEL_STAGE);
	return rbx_static_cast<Kernel*>(answer);
}

void IPipelined::putInKernel(Kernel* kernel)
{
	putInStage(kernel);
}

void IPipelined::removeFromKernel()
{
	RBXASSERT(currentStage);
	RBXASSERT(currentStage->getStageType() == IStage::KERNEL_STAGE);
	removeFromStage(currentStage);
}



IStage* IPipelined::getStage(IStage::StageType stageType) const
{
	RBXASSERT(currentStage);
	IStage* tryStage = currentStage;
	do {
		if (tryStage->getStageType() == stageType) {
			return tryStage;
		}
		tryStage = (tryStage->getStageType() > stageType)
						?	tryStage->getUpstream()
						:	tryStage->getDownstream();
	}
	while (1);
}

void IPipelined::putInPipeline(IStage* stage)
{
	RBXASSERT(stage);
	RBXASSERT(!currentStage);
	currentStage = stage;
}

void IPipelined::removeFromPipeline(IStage* stage)
{
	RBXASSERT(stage);
	RBXASSERT(currentStage);
	RBXASSERT(currentStage == stage);
	currentStage = NULL;
}


void IPipelined::putInStage(IStage* stage)
{
	RBXASSERT(stage);
	RBXASSERT(currentStage);
	RBXASSERT(stage->getUpstream() == currentStage);
	RBXASSERT(currentStage->getDownstream() == stage);
	currentStage = stage;
}

void IPipelined::removeFromStage(IStage* stage)
{
	RBXASSERT(currentStage);
	RBXASSERT(stage);
	RBXASSERT(stage == currentStage);
	RBXASSERT(stage->getUpstream());
	currentStage = currentStage->getUpstream();
}

} // namespace