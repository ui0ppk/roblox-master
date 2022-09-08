/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "Util/SteppedInstance.h"

#include "V8DataModel/KeyframeSequence.h" //for Animation::Priority, CachedPose

namespace RBX {

	class AnimatableRootJoint;
	class AnimationTrackState;
	class PartInstance;
    class AnimationTrack;

	extern const char *const sAnimator;

	class Animator
		: public DescribedCreatable<Animator, Instance, sAnimator>
		, public IStepped
	{
	private:
		typedef DescribedCreatable<Animator, Instance, sAnimator> Super;

		RBX::Time serverLockTimer;

		Instance* getRootInstance();
		void setupClumpChangedListener(Instance* rootInstance);

	protected:
        std::map<ContentId, shared_ptr<AnimationTrack> > animationTrackMap;
        std::string activeAnimation;

        typedef std::vector<JointPair> AnimatableJointSet;
		AnimatableJointSet animatableJoints;
		void calcAnimatableJoints(Instance* rootInstance, shared_ptr<Instance> descendant = shared_ptr<Instance>());
		void appendAnimatableJointsRec(shared_ptr<Instance> instance, shared_ptr<Instance> exclude);

		scoped_ptr<AnimatableRootJoint> animatableRootJoint;

		rbx::signals::scoped_connection descentdantAdded;		
		rbx::signals::scoped_connection descentdantRemoved;				
		rbx::signals::scoped_connection ancestorChanged;
		rbx::signals::scoped_connection clumpChangedConnection;
		void onEvent_DescendantAdded(shared_ptr<Instance> descendant);
		void onEvent_DescendantRemoving(shared_ptr<Instance> descendant);
		void onEvent_AncestorModified();
		void onEvent_ClumpChanged(shared_ptr<Instance> instance);

		std::list<shared_ptr<AnimationTrackState> > activeAnimations;

	public:
		Animator();
		Animator(Instance* replicatingContainer); // use this to add Animator behavor to another Instance in the tree, like Humanoid.

		virtual ~Animator();

		float getGameTime() const;
		shared_ptr<Instance> loadAnimation(shared_ptr<Instance> animation);
		void reloadAnimation(shared_ptr<AnimationTrackState> animationTrackState);

		shared_ptr<PartInstance> testForServerLockPart;

		/*implement*/ void onStepped(const Stepped& event);

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

        /*override*/ void verifySetParent(const Instance* instance) const;
		/*override*/ bool askSetParent(const Instance* instance) const { return true; }
		/*override*/ bool askAddChild(const Instance* instance) const;
		/*override*/ void onAncestorChanged(const AncestorChanged& event);

        rbx::remote_signal<void(ContentId, float, float, float)> onPlaySignal;
        rbx::remote_signal<void(ContentId, float)> onStopSignal;
		rbx::remote_signal<void(ContentId, float)> onAdjustSpeedSignal;
		rbx::remote_signal<void(ContentId, float)> onSetTimePositionSignal;
        void onPlay(ContentId animation, float fadeTime, float weight, float speed);
        void onStop(ContentId animation, float fadeTime);
		void onAdjustSpeed(ContentId animation, float speed);
		void onSetTimePosition(ContentId animation, float timePosition);
        void passiveLoadAnimation(ContentId animation);
        void replicateAnimationPlay(ContentId animation, float fadeTime, float weight, float speed, shared_ptr<Instance> track);
        void replicateAnimationStop(ContentId animation, float fadeTime);
		void replicateAnimationSpeed(ContentId animation, float speed);
		void replicateAnimationTimePosition(ContentId animation, float timePosition);
        std::string getActiveAnimation() const {return activeAnimation;}

		void tellParentAnimationPlayed(shared_ptr<Instance> animationTrack);
		shared_ptr<const Reflection::ValueArray> getPlayingAnimationTracks();
	private:
		void onTrackStepped(shared_ptr<AnimationTrackState> trackinst, double time, KeyframeSequence::Priority priority, 
								std::vector<PoseAccumulator>* poses
								);
	};

} // namespace
