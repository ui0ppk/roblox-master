/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/AnimationTrackState.h"
#include "V8DataModel/Animation.h"
#include "V8DataModel/Animator.h"
#include "V8DataModel/KeyframeSequence.h"
#include "V8Tree/Service.h"
#include "Util/RunStateOwner.h"
#include "V8DataModel/Animation.h"
#include "V8datamodel/KeyframeSequenceProvider.h"

namespace RBX {

const char* const  sAnimationTrackState = "AnimationTrackState";
static const float autoStopFadeTime = 0.3f;

REFLECTION_BEGIN();
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float,float, float)> event_play(&AnimationTrackState::internalPlaySignal, "PlayAnimation", "gameTime", "fadeTime", "weight", "speed" , Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float)> event_stop(&AnimationTrackState::internalStopSignal, "StopAnimation", "gameTime", "fadeTime", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float,float)> event_adjustWeight(&AnimationTrackState::internalAdjustWeightSignal, "AdjustAnimationWeight", "gameTime", "weight", "fadeTime", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float)> event_adjustSpeed(&AnimationTrackState::internalAdjustSpeedSignal, "AdjustAnimation", "gameTime", "speed", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);

static Reflection::RemoteEventDesc<AnimationTrackState, void(std::string)> event_Keyframe(&AnimationTrackState::keyframeReachedSignal, "KeyframeReached", "keyframeName", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void()> event_Stopped(&AnimationTrackState::stoppedSignal, "Stopped", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
REFLECTION_END();

AnimationTrackState::AnimationTrackState(shared_ptr<const KeyframeSequence> keyframeSequence, weak_ptr<const Animator> animator)
: DescribedNonCreatable<AnimationTrackState, Instance, sAnimationTrackState>()
, startTime(0.0f)
, fadeStartTime(0.0f)
, fadeStartWeight(0.0f)
, fadeEndTime(0.0f)
, fadeEndWeight(0.0f)
, speed(1.0f)
, phase(0.0f)
, keyframeSequence(keyframeSequence)
, animator(animator)
, preKeyframe(-1)
, isPlaying(false)
, priority(KeyframeSequence::CORE)
, priorityOverridden(false)
{
	setName(sAnimationTrackState);

	internalPlaySignal.connect(boost::bind(&AnimationTrackState::onPlay, this, _1, _2, _3, _4));
	internalStopSignal.connect(boost::bind(&AnimationTrackState::onStop, this, _1, _2));
	internalAdjustWeightSignal.connect(boost::bind(&AnimationTrackState::onAdjustWeight, this, _1, _2, _3));
	internalAdjustSpeedSignal.connect(boost::bind(&AnimationTrackState::onAdjustSpeed, this, _1, _2));

	lockParent();
}

void AnimationTrackState::setAnimationTrack(shared_ptr<AnimationTrack> animationTrackIn)
{
	animationTrack = animationTrackIn;
}

bool AnimationTrackState::isStopped(double time)
{
	return (time >= fadeEndTime) && G3D::fuzzyEq(fadeEndWeight,0);
}
double AnimationTrackState::getGameTime()
{
	if(shared_ptr<const Animator> safeAnimator = animator.lock()){
		return safeAnimator->getGameTime();
	}
	//We've lost our animator, shut.it.down
	keyframeSequence.reset();
	return 0.0f;
}
double AnimationTrackState::getWeightAtTime(double time)
{
	if(time >= fadeEndTime)
	{
		return fadeEndWeight;
	}
	else if(time <= fadeStartTime)
	{
		return fadeStartWeight;
	}
	else
	{
		// lerp between two.
		double interval = (fadeEndTime - fadeStartTime);
		return ((time - fadeStartTime)/ interval) * fadeEndWeight   +
				((fadeEndTime - time) / interval) * fadeStartWeight;
	}
}

double AnimationTrackState::getKeyframeAtTime(double time)
{
	if (!inReverse())
	{
		return (time - startTime) * speed + phase;
	}
	else
	{
		return (getDuration() - (time - startTime) * -speed) + phase;
	}
}

float AnimationTrackState::getDuration()
{
	if (const KeyframeSequence *pSequence = getKeyframeSequence())
	{
		return pSequence->getDuration();
	}

	return 0.0f;
}

KeyframeSequence::Priority AnimationTrackState::getPriority() const
{
	if (priorityOverridden) {
		return priority;
	}
	else
	{
		if (const KeyframeSequence* sequence = getKeyframeSequence())
		{
			return sequence->getPriority();
		}
		else
		{
			return KeyframeSequence::CORE;
		}
	}
}

void AnimationTrackState::setPriority(KeyframeSequence::Priority priorityIn)
{
	priorityOverridden = true;
	priority = priorityIn;
}

void AnimationTrackState::setKeyframeAtTime(double gameTime, double keyframetime)
{
	double delta = keyframetime - getKeyframeAtTime(gameTime);
	phase += delta;
}

void AnimationTrackState::onPlay(float gameTime, float fadeTime, float weight, float speed)
{
	if(gameTime >= startTime){
		phase = 0;
		startTime = gameTime;
		fadeStartTime = startTime;
		fadeStartWeight = 0.0;
		fadeEndTime = startTime + fadeTime;
		fadeEndWeight = weight;
		this->speed = speed;

		lastKeyframeTime = 0;
		preKeyframe = -1;

		isPlaying = true;

		//automated animation detection does not detect the first keyframe if the animation is running in reverse
		if (inReverse())
		{
			const KeyframeSequence* kfs = getKeyframeSequence();
			if (kfs->numChildren() > 0)
			{
				if (kfs->getChild(kfs->numChildren() - 1))
				{
					event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(kfs->numChildren() - 1)->getName());
				}
			}
		}
	}
}

void AnimationTrackState::play(float fadeTime, float weight, float speed)
{
	event_play.fireAndReplicateEvent(this, (float)getGameTime(), fadeTime, weight, speed);
}

void AnimationTrackState::onStop(float gameTime, float fadeTime)
{
	fadeStartTime = gameTime;
	fadeStartWeight = getWeightAtTime(gameTime);
	fadeEndTime = gameTime + fadeTime;
	fadeEndWeight = 0.0;
	event_Stopped.fireAndReplicateEvent(this);

	isPlaying = false;
}

void AnimationTrackState::stop(float fadeTime)
{
	event_stop.fireAndReplicateEvent(this, (float)getGameTime(), fadeTime);
}

void AnimationTrackState::onAdjustWeight(float gameTime, float weight, float fadeTime)
{
	fadeStartTime = gameTime;
	fadeStartWeight = getWeightAtTime(gameTime);
	fadeEndTime = gameTime + fadeTime;
	fadeEndWeight = weight;
}

void AnimationTrackState::onAdjustSpeed(float gameTime, float speed)
{
	if(speed != this->speed)
	{
		//if we reversed directions, we need to reset the preKeyframe
		if ( (this->speed >= 0 && speed < 0) || (this->speed < 0 && speed >= 0) )
		{
			preKeyframe = -1;
		}

		// must adjust phase to prevent skipping.
		double keyframetime = getKeyframeAtTime(gameTime);
		this->speed = speed;
		setKeyframeAtTime(gameTime, keyframetime);
	}
}

void AnimationTrackState::adjustWeight(float weight, float fadeTime)
{
	event_adjustWeight.fireAndReplicateEvent(this, (float)getGameTime(), weight, fadeTime);
}

void AnimationTrackState::adjustSpeed(float speed)
{
	event_adjustSpeed.fireAndReplicateEvent(this, (float)getGameTime(), speed);
}

void AnimationTrackState::triggerKeyframeReachedSignal(const shared_ptr<Instance>& child, double minKeyframeTime, double maxKeyframeTime)
{
	Keyframe* kf = Instance::fastDynamicCast<Keyframe>(child.get());
	if(kf)
	{
		if(kf->getTime() > minKeyframeTime && kf->getTime() <= maxKeyframeTime)
		{
			event_Keyframe.fireAndReplicateEvent(this, kf->getName());	
		}
	}
}

bool AnimationTrackState::inReverse()
{
	return (speed < 0);
}

double AnimationTrackState::getDurationClampedKeyframeTime(double keyframeTime)
{
	double duration = getDuration();
	if (duration > 0.0)
	{
		if (keyframeTime < 0)
		{
			int durationsOff = (int)(fabs(keyframeTime) / duration) + 1;
			keyframeTime = keyframeTime + durationsOff * duration;
		}
		else if (keyframeTime > duration)
		{
			int durationsOff = (int)(keyframeTime / duration);
			keyframeTime = keyframeTime - durationsOff * duration;
		}
	}
	return keyframeTime;
}

void AnimationTrackState::resetKeyframeReachedDetection(double keyframeTime)
{
	lastKeyframeTime = keyframeTime;
	preKeyframe = -1;
}

void AnimationTrackState::step(std::vector<PoseAccumulator>& jointposes, double time)
{
	double keyframetime = getKeyframeAtTime(time);
	float trackweight = (float)getWeightAtTime(time);

	const KeyframeSequence* kfs = getKeyframeSequence();
	double normKeyframeTime = keyframetime;
	float duration = getKeyframeSequence()->getDuration();

	kfs->apply(jointposes, lastKeyframeTime, keyframetime, trackweight);

	if(!kfs->getLoop()) // play one time. give stop command after trigger of last keyframe.
	{
		//determine if we are finished with the animation
		bool doneWithAnimation;
		if (!inReverse())
			doneWithAnimation = (lastKeyframeTime < duration && keyframetime >= duration);
		else
			doneWithAnimation = (lastKeyframeTime > 0 && keyframetime <= 0);
		if (duration <= 0)
			doneWithAnimation = false;

		if(doneWithAnimation)
		{
			// fire signal for the last frame
			if (!inReverse())
				event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(kfs->numChildren()-1)->getName());
			else
				event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(0)->getName());

			// should we use the internal event? or the external one?
			// we will play it safe and use the internal one. it is easier to notice a problem with a non-stopped animation than with a 
			// double fire.
			onStop((float)time, autoStopFadeTime);
		}
	}
	else
	{
		//find out where in the looping animation we are -- even if we're moving backwards
		normKeyframeTime = getDurationClampedKeyframeTime(keyframetime);
	}

	detectKeyframeReached(normKeyframeTime, getDurationClampedKeyframeTime(lastKeyframeTime));
	
	lastKeyframeTime = keyframetime;
}

void AnimationTrackState::detectKeyframeReached(double animationTime, double lastAnimationTime)
{
	if (!isPlaying)
		return;

	const KeyframeSequence* kfs = getKeyframeSequence();
	int numKf = kfs->numChildren();
    
    if (numKf == 0)
        return;

	//did we just loop? if so, fire the keyframes relevant. of course, only if the animation does loop
	if (kfs->getLoop())
	{
		if (!inReverse())
		{
			if (lastAnimationTime > animationTime)
			{
				event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(kfs->numChildren() - 1)->getName());
				event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(0)->getName());
			}
		}
		else
		{
			if (animationTime > lastAnimationTime)
			{
				event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(0)->getName());
				event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(kfs->numChildren() - 1)->getName());
			}
		}
	}

	//now we need to check the midway keyframes
	for (int index = 0; index < numKf; index++)
	{
		const Keyframe* keyframe = Instance::fastDynamicCast<const Keyframe>(kfs->getChild(index));
		RBXASSERT(keyframe);

		bool passedKeyframe;
		if (!inReverse())
			passedKeyframe = (lastAnimationTime <= keyframe->getTime() && keyframe->getTime() < animationTime);
		else
			passedKeyframe = (lastAnimationTime >= keyframe->getTime() && keyframe->getTime() > animationTime);

		if (passedKeyframe)
		{
			if (preKeyframe == index)
				continue;

			event_Keyframe.fireAndReplicateEvent(this, keyframe->getName());
			preKeyframe = index;
			break;
		}
	}
}

}
