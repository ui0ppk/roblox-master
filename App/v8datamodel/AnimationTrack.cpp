/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/AnimationTrack.h"
#include "V8DataModel/Animator.h"
#include "V8DataModel/AnimationTrackState.h"
#include "v8datamodel/Animation.h"
#include "V8DataModel/Workspace.h"

namespace RBX {

namespace Reflection
{
	template<>
	const Type& Type::getSingleton<shared_ptr<AnimationTrack> >()
	{
		static TType<shared_ptr<AnimationTrack> > type("AnimationTrack");
		return type;
	}
}

const char* const sAnimationTrack = "AnimationTrack";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<AnimationTrack, void(float,float,float)> desc_Play(&AnimationTrack::play, "Play","fadeTime", 0.1f, "weight", 1.0f, "speed", 1.0f, Security::None);
static Reflection::BoundFuncDesc<AnimationTrack, void(float)> desc_Stop(&AnimationTrack::stop, "Stop", "fadeTime", 0.1f, Security::None);
static Reflection::BoundFuncDesc<AnimationTrack, void(float,float)> desc_AdjustWeight(&AnimationTrack::adjustWeight, "AdjustWeight", "weight", 1.0f, "fadeTime", 0.1f, Security::None);
static Reflection::BoundFuncDesc<AnimationTrack, void(float)> desc_AdjustSpeed(&AnimationTrack::adjustSpeed, "AdjustSpeed", "speed", 1.0f, Security::None);
static Reflection::BoundFuncDesc<AnimationTrack, double(std::string)> desc_GetTimeOfKeyframe(&AnimationTrack::getTimeOfKeyframe, "GetTimeOfKeyframe", "keyframeName", Security::None);

static Reflection::PropDescriptor<AnimationTrack, float> prop_Length("Length", category_Data, &AnimationTrack::getLength, NULL, Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<AnimationTrack, bool> prop_IsPlaying("IsPlaying", category_Data, &AnimationTrack::getIsPlaying, NULL, Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<AnimationTrack, float> prop_TimePosition("TimePosition", category_Data, &AnimationTrack::getTimePosition, &AnimationTrack::setTimePosition, Reflection::PropertyDescriptor::UI);

static Reflection::RefPropDescriptor<AnimationTrack, Animation> prop_Animation("Animation", category_Data, &AnimationTrack::getAnimation, NULL, Reflection::PropertyDescriptor::SCRIPTING);

const Reflection::EnumPropDescriptor<AnimationTrack, KeyframeSequence::Priority> prop_Priority("Priority", category_Data, &AnimationTrack::getPriority, &AnimationTrack::setPriority);

static Reflection::EventDesc<AnimationTrack, void(std::string)> event_Keyframe(&AnimationTrack::keyframeReachedSignal, "KeyframeReached", "keyframeName");
static Reflection::EventDesc<AnimationTrack, void()> event_Stopped(&AnimationTrack::stoppedSignal, "Stopped");
REFLECTION_END();

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

AnimationTrack::AnimationTrack(shared_ptr<AnimationTrackState> animationTrackState, weak_ptr<Animator> animator, shared_ptr<Animation> anim) 
: DescribedNonCreatable<AnimationTrack, Instance, sAnimationTrack>()
, animationTrackState(animationTrackState)
, animator(animator)
, animation(anim)
{
	setName(sAnimationTrack);

	keyframeReachedConnection = animationTrackState->keyframeReachedSignal.connect(boost::bind(&AnimationTrack::forwardKeyframeReached, this, _1));
	stoppedConnection = animationTrackState->stoppedSignal.connect(boost::bind(&AnimationTrack::forwardStopped, this));

	lockParent();
}
AnimationTrack::~AnimationTrack()
{
	if(keyframeReachedConnection.connected())
		keyframeReachedConnection.disconnect();

	if(stoppedConnection.connected())
		stoppedConnection.disconnect();
}

Animation* AnimationTrack::getAnimation() const
{
	return animation.get();
}

float AnimationTrack::getLength() const
{
	return animationTrackState->getDuration();
}

bool AnimationTrack::getIsPlaying() const
{
	return animationTrackState->getIsPlaying();
}

KeyframeSequence::Priority AnimationTrack::getPriority() const
{
	return animationTrackState->getPriority();
}

void AnimationTrack::setPriority(KeyframeSequence::Priority priority)
{
	animationTrackState->setPriority(priority);
}

void AnimationTrack::forwardKeyframeReached(std::string keyFrame)
{
	keyframeReachedSignal(keyFrame);
}

void AnimationTrack::forwardStopped()
{
	stoppedSignal();
}

void AnimationTrack::play(float fadeTime, float weight, float speed)
{
    localPlay(fadeTime, weight, speed);

    if(shared_ptr<Animator> safeAnimator = animator.lock())
    {
		shared_ptr<Instance> me = shared_from<Instance>(this);
        safeAnimator->replicateAnimationPlay(animation->getAssetId(), fadeTime, weight, speed, me);
    }
}

void AnimationTrack::localPlay(float fadeTime, float weight, float speed)
{
    if(shared_ptr<Animator> safeAnimator = animator.lock()){
        safeAnimator->reloadAnimation(animationTrackState);
        animationTrackState->play(fadeTime, weight, speed);
    }
}

void AnimationTrack::stop(float fadeTime)
{
	localStop(fadeTime);

    if(shared_ptr<Animator> safeAnimator = animator.lock())
    {
        safeAnimator->replicateAnimationStop(animation->getAssetId(), fadeTime);
    }
}

void AnimationTrack::localStop(float fadeTime)
{
    animationTrackState->stop(fadeTime);
}

void AnimationTrack::adjustWeight(float weight, float fadeTime)
{
	if(shared_ptr<Animator> safeAnimator = animator.lock()){
		safeAnimator->reloadAnimation(animationTrackState);
		animationTrackState->adjustWeight(weight, fadeTime);
	}
}
void AnimationTrack::adjustSpeed(float speed)
{
	double oldSpeed = animationTrackState->getSpeed();
	localAdjustSpeed(speed);

	if (oldSpeed != animationTrackState->getSpeed())
	{
		if(shared_ptr<Animator> safeAnimator = animator.lock())
		{
			safeAnimator->replicateAnimationSpeed(animation->getAssetId(), speed);
		}
	}
}

void AnimationTrack::localAdjustSpeed(float speed)
{
	if(shared_ptr<Animator> safeAnimator = animator.lock())
	{
		safeAnimator->reloadAnimation(animationTrackState);
		animationTrackState->adjustSpeed(speed);
	}
}

const std::string AnimationTrack::getAnimationName() const
{
    if (animation)
    {
        return animation->getName();
    }
    else
    {
        return "";
    }
}

double AnimationTrack::getGameTime() const
{
	shared_ptr<Animator> safeAnimator = animator.lock();
	if (safeAnimator)
	{
		return safeAnimator->getGameTime();
	}
	else
	{
		return 0.0f;
	}
}

double AnimationTrack::getTimePosition() const
{
	return animationTrackState->getDurationClampedKeyframeTime(animationTrackState->getKeyframeAtTime(getGameTime()));
}

void AnimationTrack::setTimePosition(double timePosition)
{
	double oldTime = getTimePosition();
	localSetTimePosition(timePosition);

	if (oldTime != getTimePosition())
	{
		shared_ptr<Animator> safeAnimator = animator.lock();
		if (safeAnimator)
		{
			safeAnimator->replicateAnimationTimePosition(animation->getAssetId(), timePosition);
		}
	}
}

void AnimationTrack::localSetTimePosition(double timePosition)
{
	double clampedKeyframeTime = animationTrackState->getDurationClampedKeyframeTime(timePosition);
	animationTrackState->setKeyframeAtTime(getGameTime(), clampedKeyframeTime);
	animationTrackState->resetKeyframeReachedDetection(clampedKeyframeTime);
}

double AnimationTrack::getTimeOfKeyframe(std::string keyframeName)
{
	const KeyframeSequence* kfs = animationTrackState->getKeyframeSequence();
	for (unsigned int index = 0; index < kfs->numChildren(); index++)
	{
		if (kfs->getChild(index)->getName() == keyframeName)
		{
			const Keyframe* keyframe = Instance::fastDynamicCast<const Keyframe>(kfs->getChild(index));
			RBXASSERT(keyframe);
			return keyframe->getTime();
		}
	}
	throw RBX::runtime_error("Could not find a keyframe by that name!");
}

}
