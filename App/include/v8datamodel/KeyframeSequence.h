/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/Keyframe.h"
#include "V8DataModel/IAnimatableJoint.h"

namespace RBX {
	extern const char *const sKeyframeSequence;
	class AnimationTrackState;
	class JointInstance;

	typedef std::pair<weak_ptr<Instance>, IAnimatableJoint *> JointPair;

	struct PoseAccumulator
	{
		PoseAccumulator(const JointPair &joint) 
			: joint(joint) {};
		JointPair joint;
		CachedPose pose;
	};

	class KeyframeSequence
		: public DescribedCreatable<KeyframeSequence, Instance, sKeyframeSequence>
	{
	private:
		typedef DescribedCreatable<KeyframeSequence, Instance, sKeyframeSequence> Super;
	public:
		enum Priority
		{
			CORE = 1000,			// This is lowest priority.  Reflection prevents use of negative numbers.
			IDLE = 0,
			MOVEMENT = 1,
			ACTION = 2,
		};

	private:
		struct CachedKeyframe
		{
			float time;
			std::vector<CachedPose*> poses; // animatedJoints.Count == poses.Count;
			bool operator<(const CachedKeyframe& rhs) const
			{
				return time < rhs.time;
			}
		};
		struct Cache
		{
			bool isValid;
			float duration;
			std::vector<std::string> namedParts;
			std::vector<std::pair<size_t, size_t> > animatedJoints; //ints index into namedParts;
			std::vector<CachedPose> allPoses; // here mostly for memory management. 
			size_t poseCount; 
			std::vector<CachedKeyframe> keyframes; // sorted keyframes.
			Cache()
				: isValid(false)
				, duration(0.0)
			{};
		};
		mutable Cache cache;
		void cacheData() const;
		const Cache& getCachedData() const;
		
		void AppendPosePass0(const shared_ptr<Instance>& child) const;
		void AppendPosePass1(const shared_ptr<Instance>& child, std::vector<CachedPose*>* poses) const;

		void cacheKeyframePass0(const shared_ptr<Instance>& child) const;
		void cacheKeyframePass1(const shared_ptr<Instance>& child) const;
		CachedKeyframe makeKeyframe(Keyframe* kf) const;
	protected:
		bool loop;
		Priority priority;
	public:
		KeyframeSequence();
		
		bool getLoop() const { return loop; }
		void setLoop(bool value);

		Priority getPriority() const { return priority; }
		void setPriority(Priority value);

		/*override*/ bool askSetParent(const Instance* instance) const { return false; }
		/*override*/ bool askAddChild(const Instance* instance) const 
		{ 
			return Instance::fastDynamicCast<Keyframe>(instance)  != NULL; 
		}

		void apply(std::vector<PoseAccumulator>& jointposes, double lastkeyframetime, double keyframetime, float trackweight) const;

		/*override*/ void onChildAdded(Instance* child);
		/*override*/ void onChildRemoved(Instance* child);

		/*override*/ void verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const;

		shared_ptr<const Instances> getKeyframes();
		void addKeyframe(shared_ptr<Instance>);
		void removeKeyframe(shared_ptr<Instance>);

		void copyKeyframeSequence(KeyframeSequence* other);

		void invalidateCache();

		float getDuration() const;
		
	};

} // namespace
