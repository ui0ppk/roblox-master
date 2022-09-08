 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/SnapJoint.h"
#include "V8World/Primitive.h"

namespace RBX {

bool SnapJoint::compatibleSurfaces(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	SurfaceType t0 = p0->getSurfaceType(nId0);
	SurfaceType t1 = p1->getSurfaceType(nId1);

	return (	((t0 == STUDS) && (t1 == INLET))
			||	((t0 == INLET) && (t1 == STUDS))	
			||  ((t0 == UNIVERSAL) && (t1 == UNIVERSAL))
			||  ((t0 == UNIVERSAL) && (t1 == INLET || t1 == STUDS))
			||  ((t0 == INLET || t0 == STUDS) && (t1 == UNIVERSAL)));
}

SnapJoint* SnapJoint::canBuildJoint(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	if (	SnapJoint::compatibleSurfaces(p0, p1, nId0, nId1) 
		&&	Joint::canBuildJointTight(p0, p1, nId0, nId1)) 
	{

		CoordinateFrame c0, c1;
		RigidJoint::faceIdToCoords(p0, p1, nId0, nId1, c0, c1);

		return new SnapJoint(p0, p1, c0, c1);
	}
	else {
		return NULL;
	}
}


} // namespace