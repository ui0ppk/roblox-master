#pragma once

#include "V8World/RigidJoint.h"

namespace RBX {
	
	class SnapJoint : public RigidJoint
	{
	private:
		///////////////////////////////////////////////////
		// Joint
		/*override*/ virtual JointType getJointType() const	{return SNAP_JOINT;}

		///////////////////////////////////////////////////
		// WeldJoint
		static bool compatibleSurfaces(
							Primitive* p0, 
							Primitive* p1, 
							NormalId nId0, 
							NormalId nId1);

	public:
		SnapJoint()	{}

		SnapJoint(Primitive* prim0, Primitive* prim1, const CoordinateFrame& c0, const CoordinateFrame &c1) 
			: RigidJoint(prim0, prim1, c0, c1)
		{}

		~SnapJoint() {}

		static SnapJoint* canBuildJoint(
			Primitive* p0, 
			Primitive* p1, 
			NormalId nId0, 
			NormalId nId1);
	};


} // namespace

