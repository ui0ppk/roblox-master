/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/JointBuilder.h"
#include "V8World/WeldJoint.h"
#include "V8World/SnapJoint.h"
#include "V8World/GlueJoint.h"
#include "V8World/RotateJoint.h"
#include "V8World/Primitive.h"
#include "V8World/Tolerance.h"
#include "Util/Units.h"
#include "Util/Math.h"

namespace RBX {


Joint* JointBuilder::canJoin(Primitive* p0, Primitive* p1) 
{
	const Extents& e0 = p0->getFastFuzzyExtents();
	const Extents& e1 = p1->getFastFuzzyExtents();

	// 1. If not next to each other or overlapping, bail out
	if (e0.separatedByMoreThan(e1, Tolerance::jointMaxUnaligned())) {
		return NULL;
	}
	
	const Matrix3& r0 = p0->getCoordinateFrame().rotation; 
	const Matrix3& r1 = p1->getCoordinateFrame().rotation; 

	for (int i = 0; i < 6; ++i) 
	{
		NormalId nId0 = NormalId(i);
		Vector3 n0 = Math::getWorldNormal(nId0, r0);
		NormalId nId1 = Math::getClosestObjectNormalId(-n0, r1);

		if (RotateJoint* rotateJoint = RotateJoint::canBuildJoint(p0, p1, nId0, nId1)) {
			return rotateJoint;
		}
		if (WeldJoint* weldJoint = WeldJoint::canBuildJoint(p0, p1, nId0, nId1)) {
			return weldJoint;
		}
		if (SnapJoint* snapJoint = SnapJoint::canBuildJoint(p0, p1, nId0, nId1)) {
			return snapJoint;
		}
		if (GlueJoint* glueJoint = GlueJoint::canBuildJoint(p0, p1, nId0, nId1)) {
			return glueJoint;
		}
	}
	return NULL;
}
		

} // namespace