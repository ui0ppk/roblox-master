 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/RigidJoint.h"
#include "V8World/Primitive.h"

DYNAMIC_FASTFLAG(OrthonormalizeJointCoords)

namespace RBX {

bool RigidJoint::isAligned()
{
	CoordinateFrame c0World = getPrimitive(0)->getCoordinateFrame() * jointCoord0;
	CoordinateFrame c1World = getPrimitive(1)->getCoordinateFrame() * jointCoord1;
	return Math::fuzzyEq(c0World, c1World);
}

CoordinateFrame RigidJoint::align(Primitive* pMove, Primitive* pStay)
{
	const CoordinateFrame& cMove = (pMove == getPrimitive(0)) ? jointCoord0 : jointCoord1;
	const CoordinateFrame& cStay = (pStay == getPrimitive(0)) ? jointCoord0 : jointCoord1;

	CoordinateFrame jointWorld = pStay->getCoordinateFrame() * cStay;
	return jointWorld * cMove.inverse();
}

CoordinateFrame RigidJoint::getChildInParent(Primitive* parent, Primitive* child)
{
	const CoordinateFrame& cParent = (parent == getPrimitive(0)) ? jointCoord0 : jointCoord1;
	const CoordinateFrame& cChild = (child == getPrimitive(0)) ? jointCoord0 : jointCoord1;

	// Note - we saw this return a non-ortho matrix once
	CoordinateFrame answer = cParent * cChild.inverse();
	Math::orthonormalizeIfNecessary(answer.rotation);
	return answer;
}


void RigidJoint::faceIdToCoords(
	Primitive* p0, 
	Primitive* p1, 
	NormalId nId0, 
	NormalId nId1, 
	CoordinateFrame& c0, 
	CoordinateFrame& c1)
{
	c0 = p0->getFaceCoordInObject(nId0);
	if (DFFlag::OrthonormalizeJointCoords)
		Math::orthonormalizeIfNecessary(c0.rotation);
	CoordinateFrame worldC = p0->getCoordinateFrame() * c0;
	c1 = p1->getCoordinateFrame().toObjectSpace(worldC);
	if (DFFlag::OrthonormalizeJointCoords)
		Math::orthonormalizeIfNecessary(c1.rotation);
}


} // namespace