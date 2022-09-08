/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/KeyframeSequence.h"
#include "V8DataModel/Keyframe.h"
#include "V8DataModel/Workspace.h"

namespace RBX {

const char* const sKeyframe = "Keyframe";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Keyframe, shared_ptr<const Instances>()> func_getPoses(&Keyframe::getPoses, "GetPoses", Security::None);
static Reflection::BoundFuncDesc<Keyframe, void(shared_ptr<Instance>)> func_addPose(&Keyframe::addPose, "AddPose", "pose", Security::None);
static Reflection::BoundFuncDesc<Keyframe, void(shared_ptr<Instance>)> func_removePose(&Keyframe::removePose, "RemovePose", "pose", Security::None);

const Reflection::PropDescriptor<Keyframe, float> prop_Time("Time", category_Data, &Keyframe::getTime, &Keyframe::setTime);
REFLECTION_END();

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

Keyframe::Keyframe() 
: DescribedCreatable<Keyframe, Instance, sKeyframe>()
, time(0.0f)
{
	setName(sKeyframe);
}

shared_ptr<const Instances> Keyframe::getPoses()
{
	return getChildren2();
}
void Keyframe::addPose(shared_ptr<Instance> pose)
{
	if (pose != NULL) {
		pose->setParent(this);
	}
}
void Keyframe::removePose(shared_ptr<Instance> pose)
{
	if (pose != NULL) {
		if(pose->getParent() == this){
			pose->setParent(NULL);
		}
	}
}
void Keyframe::setTime(float value)
{
	if(time != value){
		time = value;
		raisePropertyChanged(prop_Time);
		invalidate();
	}
}

void Keyframe::invalidate()
{
	KeyframeSequence* keyframeSequence = Instance::fastDynamicCast<KeyframeSequence>(getParent());
	if(keyframeSequence)
	{
		keyframeSequence->invalidateCache();
	}
}

void Keyframe::verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const
{
	Super::verifySetAncestor(newParent, instanceGettingNewParent);
}

}
