#pragma once

#include "V8World/Joint.h"

namespace RBX {

	class RevoluteLink;

	class MotorJoint : public Joint
	{
	private:
		RevoluteLink* link;

		///////////////////////////////////////////////////
		// Joint
		/*override*/ JointType getJointType() const	{return Joint::MOTOR_1D_JOINT;}
		/*override*/ bool isBroken() const {return false;}
		/*override*/ bool isAligned();

		float currentAngle;
		float poseAngleDelta;
		float poseMaskWeight;
		int poseFreshness;

		int getParentId() const;

		void setJointAngle(float value);

	public:
		// tweak this to adjust how long a pose stays applied in the absence of a fresh call to applyPose()
		static const int poseDuration = 32;

		float maxVelocity;
		float desiredAngle;
		float getCurrentAngle() const {return currentAngle;}
		bool setCurrentAngle(float value);
		void applyPose(float poseAngle, float poseWeight, float maskWeight);

		CoordinateFrame getMeInOther(Primitive* me);

		/*override*/ bool canStepUi() const {return true;}
		/*override*/ bool stepUi(double distributedGameTime);

		MotorJoint();

		~MotorJoint();

		size_t hashCode() const;

		/*override*/ Link* resetLink();

		static bool isMotorJoint(const Edge* e) {
			return (	isJoint(e) 
					&&	rbx_static_cast<const Joint*>(e)->getJointType() == Joint::MOTOR_1D_JOINT);
		}
	};

} // namespace

