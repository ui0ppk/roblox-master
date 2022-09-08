 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/WeldJoint.h"
#include "V8World/Primitive.h"
#include "V8World/Contact.h"

namespace RBX {

const float kTerrainBreakOverlap = -0.2f;

bool WeldJoint::compatibleSurfaces(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	SurfaceType t0 = p0->getSurfaceType(nId0);
	SurfaceType t1 = p1->getSurfaceType(nId1);

	// weld beats glue - either one is a weld surface, or male/female snap for now
	return ((t0 == WELD) ||	(t1 == WELD));
}

WeldJoint* WeldJoint::canBuildJoint(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1)
{
	if (	WeldJoint::compatibleSurfaces(p0, p1, nId0, nId1) 
		&&	Joint::canBuildJointTight(p0, p1, nId0, nId1)) {

		CoordinateFrame c0, c1;
		
		RigidJoint::faceIdToCoords(p0, p1, nId0, nId1, c0, c1);

        return new WeldJoint(p0, p1, c0, c1);
	}
	else {
		return NULL;
	}
}

// Due to legacy reasons, cell index has to be encoded in surface0 and surface1 with a special encoding
// Cells within the legacy terrain bounds have to have surface0 equal to a linear index within 511x63x511 array (yes, really)
// and surface1 equal to -1.
// We extend that encoding to maintain legacy data.
Vector3int16 ManualWeldJoint::getCell() const
{
    short x, y, z;

    if ((int)surface1 == -1)
    {
        // legacy encoding
        x = surface0 % 511 - 256;
        y = (surface0 / (511 * 511)) % 63;
        z = (surface0 / 511) % 511 - 256;
    }
    else
    {
        // new encoding; y was split into two 8-bit vectors, reassemble them
        x = surface0 & 0xffff;
        y = ((surface0 >> 16) & 0xff) | (((surface1 >> 16) & 0xff) << 8);
        z = surface1 & 0xffff;
    }

    return Vector3int16(x, y, z);
}

void ManualWeldJoint::setCell(const Vector3int16& pos)
{
    // use new encoding: split y into two 8-bit vectors
    surface0 = (pos.x & 0xffff) | ((pos.y & 0xff) << 16);
    surface1 = (pos.z & 0xffff) | ((pos.y & 0xff00) << 8);
}

bool ManualWeldJoint::isTouchingTerrain(Primitive* terrain, Primitive* prim)
{
	for (int i = 0; i < prim->getNumContacts(); ++i)
		if (prim->getContactOther(i) == terrain)
			if (prim->getContact(i)->computeIsCollidingUi(kTerrainBreakOverlap))
				return true;

	return false;
}

} // namespace
