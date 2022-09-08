/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Animation.h"
#include "V8DataModel/KeyframeSequenceProvider.h"
#include "V8DataModel/AnimationTrackState.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"

namespace RBX {

const char* const sAnimation = "Animation";

const Reflection::PropDescriptor<Animation, AnimationId> prop_AnimationId("AnimationId", category_Data, &Animation::getAssetId, &Animation::setAssetId);
//const Reflection::PropDescriptor<Animation, bool> prop_Loop("Loop", category_Data, &Animation::getLoop, &Animation::setLoop);
//const Reflection::EnumPropDescriptor<Animation, Animation::Priority> prop_Priority("Priority", category_Data, &Animation::getPriority, &Animation::setPriority);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

Animation::Animation() 
: DescribedCreatable<Animation, Instance, sAnimation>()
{
	setName(sAnimation);
}

bool Animation::isEmbeddedAsset() const
{
	return !assetId.isHttp() && !assetId.isAsset();
}

shared_ptr<const KeyframeSequence> Animation::getKeyframeSequence() const
{
	return getKeyframeSequence(this);
}

shared_ptr<const KeyframeSequence> Animation::getKeyframeSequence(const Instance* context) const
{
	if(assetId.isNull()){
		return shared_ptr<KeyframeSequence>();
	}

	if(KeyframeSequenceProvider* keyframeSequenceProvider = ServiceProvider::create<KeyframeSequenceProvider>(context)){
		return keyframeSequenceProvider->getKeyframeSequence(assetId, this);
	}
	return shared_ptr<KeyframeSequence>();
}

void Animation::setAssetId(AnimationId value)
{
	if(assetId != value) {
		assetId = value;
		raisePropertyChanged(prop_AnimationId);
	}
}

} // RBX
