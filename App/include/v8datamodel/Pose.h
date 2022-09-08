/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/Pose.h"

namespace RBX {

	class Keyframe;
	extern const char *const sPose;

	class Pose
		: public DescribedCreatable<Pose, Instance, sPose>
	{
	public:
		enum PoseEasingStyle
		{
			POSE_EASING_STYLE_LINEAR = 0,
			POSE_EASING_STYLE_CONSTANT,
			POSE_EASING_STYLE_ELASTIC,
			POSE_EASING_STYLE_CUBIC,
			POSE_EASING_STYLE_BOUNCE
		};
		enum PoseEasingDirection
		{
			POSE_EASING_DIRECTION_IN,
			POSE_EASING_DIRECTION_OUT,
			POSE_EASING_DIRECTION_IN_OUT
		};
	private:
		typedef DescribedCreatable<Pose, Instance, sPose> Super;
	protected:
		CoordinateFrame coordinateFrame;
		float weight;
		float maskWeight;
		Keyframe* findKeyframeParent();
		void invalidate();
		PoseEasingStyle easingStyle;
		PoseEasingDirection easingDirection;
	public:
		Pose();
		const CoordinateFrame& getCoordinateFrame() const { return coordinateFrame; }
		void setCoordinateFrame(const CoordinateFrame& value);

		shared_ptr<const Instances> getSubPoses();
		void addSubPose(shared_ptr<Instance>);
		void removeSubPose(shared_ptr<Instance>);

		float getWeight() const { return weight; }
		void setWeight(float value);

		float getMaskWeight() const { return maskWeight; }
		void setMaskWeight(float value);

		PoseEasingStyle getEasingStyle() const { return easingStyle; }
		void setEasingStyle(PoseEasingStyle value);

		PoseEasingDirection getEasingDirection() const { return easingDirection; }
		void setEasingDirection(PoseEasingDirection value);

		/*override*/ bool askAddChild(const Instance* instance) const 
		{ 
			return Instance::fastDynamicCast<Pose>(instance) != NULL; 
		}

		/*override*/ void onChildAdded(Instance* child)
		{
			invalidate();
		}
		/*override*/ void onChildRemoved(Instance* child)
		{
			invalidate();
		}
		/*override*/ void verifySetAncestor(const Instance* const newParent, const Instance* const instanceGettingNewParent) const;
	};

} // namespace
