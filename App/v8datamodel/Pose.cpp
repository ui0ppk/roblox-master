/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Pose.h"
#include "V8DataModel/Keyframe.h"
#include "V8DataModel/Workspace.h"

namespace RBX {

const char* const sPose = "Pose";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Pose, shared_ptr<const Instances>()> func_getPoses(&Pose::getSubPoses, "GetSubPoses", Security::None);
static Reflection::BoundFuncDesc<Pose, void(shared_ptr<Instance>)> func_addPose(&Pose::addSubPose, "AddSubPose", "pose", Security::None);
static Reflection::BoundFuncDesc<Pose, void(shared_ptr<Instance>)> func_removePose(&Pose::removeSubPose, "RemoveSubPose", "pose", Security::None);


const Reflection::PropDescriptor<Pose, CoordinateFrame> prop_CFrame("CFrame", category_Data, &Pose::getCoordinateFrame, &Pose::setCoordinateFrame);
const Reflection::PropDescriptor<Pose, float> prop_Weight("Weight", category_Data, &Pose::getWeight, &Pose::setWeight);
const Reflection::PropDescriptor<Pose, float> prop_MaskWeight("MaskWeight", category_Data, &Pose::getMaskWeight, &Pose::setMaskWeight);
REFLECTION_END();

// DFFlag::AnimationEasingStylesEnabled
// static Reflection::EnumPropDescriptor<Pose, Pose::PoseEasingStyle> prop_EasingStyle("EasingStyle", category_Data, &Pose::getEasingStyle, &Pose::setEasingStyle);
// static Reflection::EnumPropDescriptor<Pose, Pose::PoseEasingDirection> prop_EasingDirection("EasingDirection", category_Data, &Pose::getEasingDirection, &Pose::setEasingDirection);

namespace Reflection
{
	template<>
	EnumDesc<RBX::Pose::PoseEasingStyle>::EnumDesc()
		:EnumDescriptor("PoseEasingStyle")
	{
		addPair(Pose::POSE_EASING_STYLE_LINEAR, "Linear");
		addPair(Pose::POSE_EASING_STYLE_CONSTANT, "Constant");
		addPair(Pose::POSE_EASING_STYLE_ELASTIC, "Elastic");
		addPair(Pose::POSE_EASING_STYLE_CUBIC, "Cubic");
		addPair(Pose::POSE_EASING_STYLE_BOUNCE, "Bounce");
	}

	template<>
	EnumDesc<RBX::Pose::PoseEasingDirection>::EnumDesc()
		:EnumDescriptor("PoseEasingDirection")
	{
		addPair(Pose::POSE_EASING_DIRECTION_OUT, "Out");
		addPair(Pose::POSE_EASING_DIRECTION_IN_OUT, "InOut");
		addPair(Pose::POSE_EASING_DIRECTION_IN, "In");

	}
}

Pose::Pose() 
: DescribedCreatable<Pose, Instance, sPose>()
, weight(1.0f)
, maskWeight(0.0f)
, easingStyle(Pose::POSE_EASING_STYLE_LINEAR)
, easingDirection(Pose::POSE_EASING_DIRECTION_IN)
{
	setName(sPose);
}

shared_ptr<const Instances> Pose::getSubPoses()
{
	return getChildren2();
}
void Pose::addSubPose(shared_ptr<Instance> pose)
{
	if (pose != NULL) {
		pose->setParent(this);
	}
}
void Pose::removeSubPose(shared_ptr<Instance> pose)
{
	if (pose != NULL) {
		if(pose->getParent() == this){
			pose->setParent(NULL);
		}
	}
}

Keyframe* Pose::findKeyframeParent()
{
	Keyframe* kf = Instance::fastDynamicCast<Keyframe>(getParent());
	if(kf)
		return kf;

	Pose* pose = Instance::fastDynamicCast<Pose>(getParent());
	if(pose)
		return pose->findKeyframeParent();
	else
		return NULL;
}
void Pose::invalidate()
{
	Keyframe* kf = findKeyframeParent();
	if(kf)
	{
		kf->invalidate();
	}
}


void Pose::setCoordinateFrame(const CoordinateFrame& value)
{
	if(coordinateFrame != value){
		coordinateFrame = value;
		raisePropertyChanged(prop_CFrame);
		invalidate();
	}
}

void Pose::setWeight(float value)
{
	if(weight != value){
		weight = value;
		raisePropertyChanged(prop_Weight);
		invalidate();
	}
}
void Pose::setMaskWeight(float value)
{
	if(maskWeight != value){
		maskWeight = value;
		raisePropertyChanged(prop_MaskWeight);
		invalidate();
	}
}

void Pose::setEasingStyle(PoseEasingStyle value)
{
	if (easingStyle != value)
	{
		easingStyle = value;
// DFFlag::AnimationEasingStylesEnabled
//		raisePropertyChanged(prop_EasingStyle);
		invalidate();
	}
}

void Pose::setEasingDirection(Pose::PoseEasingDirection value)
{
	if (easingDirection != value)
	{
		easingDirection = value;
// DFFlag::AnimationEasingStylesEnabled
//		raisePropertyChanged(prop_EasingDirection);
		invalidate();
	}
}

void Pose::verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const
{
	Super::verifySetAncestor(newParent, instanceGettingNewParent);
}


}
