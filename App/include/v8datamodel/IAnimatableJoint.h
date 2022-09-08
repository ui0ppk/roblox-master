#pragma once

#include "Pose.h"

namespace RBX 
{
	class Pose;

	struct CachedPose
	{
		CachedPose() 
			: weight(0.0)
			, maskWeight(1.0)
			, initialized(false)
			, easingStyle(Pose::POSE_EASING_STYLE_LINEAR)
			, easingDirection(Pose::POSE_EASING_DIRECTION_IN)
		{};
		CachedPose(const Vector3& translation, const Vector3& rotaxisangle)
			: weight(1.0)
			, maskWeight(0.0)
			, translation(translation)
			, rotaxisangle(rotaxisangle)
			, initialized(true)
			, easingStyle(Pose::POSE_EASING_STYLE_LINEAR)
			, easingDirection(Pose::POSE_EASING_DIRECTION_IN)
		{}

		Vector3 translation; 
		Vector3 rotaxisangle; 
		float weight;
		float maskWeight;
		Pose::PoseEasingStyle easingStyle;
		Pose::PoseEasingDirection easingDirection;
		bool initialized;

		CoordinateFrame getCFrame() const;
		void setCFrame(const CoordinateFrame& cframe); 

		static CachedPose interpolatePoses(const CachedPose& p0, const CachedPose& p1, float w0, float w1);
		static CachedPose blendPoses(const CachedPose& p0, const CachedPose& p1);
	};



	class  IAnimatableJoint
	{
    protected:
        bool isAnimatedJoint;
        IAnimatableJoint() {isAnimatedJoint = false;}
	public:
		static const std::string sNULL;
		static const std::string sROOT;

		virtual const std::string& getParentName() = 0;
		virtual const std::string& getPartName() = 0;
		virtual void applyPose(const CachedPose& pose) = 0;

        virtual void setIsAnimatedJoint(bool val) {isAnimatedJoint = val;}
        bool getIsAnimatedJoint() const {return isAnimatedJoint;}
	};



}
