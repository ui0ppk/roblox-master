#pragma once

#include "V8World/Joint.h"


namespace RBX {

	class RigidJoint : public Joint
	{
	private:
		///////////////////////////////////////////////////
		// Joint
		/*override*/ virtual JointType getJointType() const	{RBXASSERT(0); return Joint::NO_JOINT;}
		/*override*/ virtual bool isBroken() const {return false;}

		///////////////////////////////////////////////////
		// KinematicJoint
		// TODO: This assumes two function (one virtual) calls are way better than a dynamic cast?...
		static bool jointIsRigid(Joint* j) {
			JointType jt = j->getJointType();
			return ((jt == Joint::WELD_JOINT) || (jt == Joint::SNAP_JOINT) || (jt == Joint::MANUAL_WELD_JOINT));
		}

	protected:
		static void faceIdToCoords(
			Primitive* p0, 
			Primitive* p1, 
			NormalId nId0, 
			NormalId nId1, 
			CoordinateFrame& c0, 
			CoordinateFrame& c1);

	public:
		RigidJoint()
		{}

		RigidJoint(
			Primitive* prim0, 
			Primitive* prim1, 
			const CoordinateFrame& c0, 
			const CoordinateFrame &c1)
			: Joint(prim0, prim1, c0, c1)
		{}

		~RigidJoint() {}

		/*override*/ bool isAligned();

		/*override*/ CoordinateFrame align(Primitive* pMove, Primitive* pStay);

		CoordinateFrame getChildInParent(Primitive* parent, Primitive* child);
	};

} // namespace

