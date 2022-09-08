/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/MultiJoint.h"
#include <vector>

namespace RBX {
    class Constraint;
	class GlueJoint : public MultiJoint
	{
	private:
		typedef MultiJoint Super;
		Face faceInJointSpace;

        // Used when PGS is on
        std::vector< Constraint* > constraints;

		float getMaxForce();

		// Joint
		/*override*/ JointType getJointType() const	{return Joint::GLUE_JOINT;}
		/*override*/ bool isBreakable() const {return true;}
        /*override*/ bool isBroken() const;

		static bool compatibleSurfaces(
						Primitive* p0, 
						Primitive* p1, 
						NormalId nId0, 
						NormalId nId1);
	protected:
		// Edge
		/*override*/ void putInKernel(Kernel* kernel);
        /*override*/ void removeFromKernel();
	public:
		GlueJoint();

		GlueJoint(
			Primitive*	p0,
			Primitive*	p1,
			const CoordinateFrame& jointCoord0,
			const CoordinateFrame& jointCoord1,
			const Face& faceInJointSpace);

		const Vector3& getFacePoint(int i) const {			// in joint space (common to both P0 and P1)
			RBXASSERT(i >= 0 && i < 4);
			return faceInJointSpace[i];
		}

		void setFacePoint(int i, const Vector3& value) {		// in joint space
			RBXASSERT(i >= 0 && i < 4);
			faceInJointSpace[i] = value;
		}

		static GlueJoint* canBuildJoint(
				Primitive* p0, 
				Primitive* p1, 
				NormalId nId0, 
				NormalId nId1);

	};

	class ManualGlueJoint : public GlueJoint
	{
	private:
		typedef GlueJoint Super;
		size_t surface0;	// surface from primitive 0
		size_t surface1;	// surface from primitive 1

		/*override*/ virtual JointType getJointType() const	{return MANUAL_GLUE_JOINT;}
		/*override*/ void putInKernel(Kernel* kernel);
		/*override*/ void computeIntersectingSurfacePoints(void);

	public:
		ManualGlueJoint()	{surface0 = (size_t)-1; surface1 = (size_t)-1;}
		ManualGlueJoint(size_t s0, size_t s1, Primitive* prim0, Primitive* prim1, const CoordinateFrame& c0, const CoordinateFrame &c1, const Face& faceInJointSpace) 
			: GlueJoint(prim0, prim1, c0, c1, faceInJointSpace)
		{surface0 = s0; surface1 = s1;}

		~ManualGlueJoint() {}
		
		size_t getSurface0(void) const {return surface0;}
		size_t getSurface1(void) const {return surface1;}
		void setSurface0(size_t surfId) {surface0 = surfId;}
		void setSurface1(size_t surfId) {surface1 = surfId;}
	};


} // namespace
