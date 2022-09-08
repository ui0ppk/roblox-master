#pragma once

#include "V8World/RigidJoint.h"

namespace RBX {
	
	class WeldJoint : public RigidJoint
	{
	private:
		///////////////////////////////////////////////////
		// Joint
		/*override*/ virtual JointType getJointType() const	{return WELD_JOINT;}

		///////////////////////////////////////////////////
		// WeldJoint
		static bool compatibleSurfaces(
							Primitive* p0, 
							Primitive* p1, 
							NormalId nId0, 
							NormalId nId1);

	public:
		WeldJoint()	{}

		WeldJoint(Primitive* prim0, Primitive* prim1, const CoordinateFrame& c0, const CoordinateFrame &c1) 
			: RigidJoint(prim0, prim1, c0, c1)
		{}

		virtual ~WeldJoint() {}

		static WeldJoint* canBuildJoint(
			Primitive* p0, 
			Primitive* p1, 
			NormalId nId0, 
			NormalId nId1);
	};

	class ManualWeldJoint : public WeldJoint
	{
	private:
		size_t surface0;	// surface from primitive 0
		size_t surface1;	// surface from primitive 1

		/*override*/ virtual JointType getJointType() const	{return MANUAL_WELD_JOINT;}

	public:
		ManualWeldJoint()	{surface0 = (size_t)-1; surface1 = (size_t)-1;}
		ManualWeldJoint(size_t s0, size_t s1, Primitive* prim0, Primitive* prim1, const CoordinateFrame& c0, const CoordinateFrame &c1) 
			: WeldJoint(prim0, prim1, c0, c1)
		{surface0 = s0; surface1 = s1;}

		~ManualWeldJoint() {}

		size_t getSurface0(void) const {return surface0;}
		size_t getSurface1(void) const {return surface1;}
		void setSurface0(size_t surfId) {surface0 = surfId;}
		void setSurface1(size_t surfId) {surface1 = surfId;}

        Vector3int16 getCell() const;
        void setCell(const Vector3int16& pos);

		static bool isTouchingTerrain(Primitive* terrain, Primitive* prim);
	};


} // namespace

