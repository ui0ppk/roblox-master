#pragma once

#include "V8World/Joint.h"

namespace RBX {

	class D6Link;

	class Motor6DJoint : public Joint
	{
	private:
		D6Link* link;

		///////////////////////////////////////////////////
		// Joint
		/*override*/ JointType getJointType() const	{return Joint::MOTOR_6D_JOINT;}
		/*override*/ bool isBroken() const {return false;}
		/*override*/ bool isAligned();

		Vector3 poseOffsetDelta;
		Vector3 poseAxisAngleDelta;
		float poseMaskWeight;
		int poseFreshness;
		Vector3 currentOffset;
		Vector3 currentAxisAngle;

		int getParentId() const;

		void setJointOffsetCFrame(const Vector3 offset, const Vector3 axisAngle);

	public:
		float maxZAngleVelocity; // for support of legacy animate scripts
		float desiredZAngle;     // 
		float getCurrentZAngle() const;//
		void setCurrentZAngle(float value); 
		Vector3 getCurrentOffset() const {return currentOffset;}
		Vector3 getCurrentAngle() const {return currentAxisAngle;}
		bool setCurrentOffsetAngle(const Vector3 offset, const Vector3 axisAngle);
		void applyPose(const Vector3& poseOffset, const Vector3& poseAxisAngle, float poseWeight, float maskWeight);

		CoordinateFrame getMeInOther(Primitive* me);

		/*override*/ bool canStepUi() const {return true;}
		/*override*/ bool stepUi(double distributedGameTime);

		Motor6DJoint();

		~Motor6DJoint();

		size_t hashCode() const;

		/*override*/ Link* resetLink();

		static bool isMotor6DJoint(const Edge* e) {
			return (	isJoint(e) 
					&&	rbx_static_cast<const Joint*>(e)->getJointType() == Joint::MOTOR_6D_JOINT);
		}
	};

} // namespace

