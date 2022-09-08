/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/KeyframeSequence.h"

namespace RBX {
	class Animator;
	class AnimationTrackState;
    class Animation;
	extern const char *const sAnimationTrack;

	//A controller class that wraps an AnimationTrackState and allows for interaction with Lua
	class AnimationTrack
		: public DescribedNonCreatable<AnimationTrack, Instance, sAnimationTrack>
	{
	protected:
		//Keeps animationTrackState alive (in case we need to restart it)
		shared_ptr<AnimationTrackState> animationTrackState;

		//Keeps a weak ptr to our animator, so we can restart an animation if it has become stopped
		weak_ptr<Animator> animator;

        shared_ptr<Animation> animation;

		rbx::signals::connection keyframeReachedConnection;
		rbx::signals::connection stoppedConnection;

		void forwardKeyframeReached(std::string);
		void forwardStopped();

		double getGameTime() const;

	public:
		AnimationTrack(shared_ptr<AnimationTrackState> animationTrackState, weak_ptr<Animator> animator, shared_ptr<Animation> anim);
		~AnimationTrack();

		void play(float fadeTime, float weight, float speed);
        void localPlay(float fadeTime, float weight, float speed);
		void stop(float fadeTime);
        void localStop(float fadeTime);
		void adjustWeight(float weight, float fadeTime);
		void adjustSpeed(float speed);
		void localAdjustSpeed(float speed);

		float getLength() const;
		bool getIsPlaying() const;
		Animation* getAnimation() const;

		double getTimePosition() const;
		void setTimePosition(double timePosition);
		void localSetTimePosition(double timePosition);

		double getTimeOfKeyframe(std::string keyframeName);

		KeyframeSequence::Priority getPriority() const;
		void setPriority(KeyframeSequence::Priority priority);

        const std::string getAnimationName() const;

		rbx::signal<void(std::string)> keyframeReachedSignal;
		rbx::signal<void()> stoppedSignal;
	};

} // namespace
