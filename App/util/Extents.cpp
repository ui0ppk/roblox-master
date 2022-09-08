/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/Extents.h"
#include "Util/Math.h"
#include "V8DataModel/Camera.h"
#include "rbx/Debug.h"

namespace RBX {

Extents Extents::clampInsideOf(const Extents& other) const { //		Extents& operator&= (const Extents& other) {
	Extents answer;
	answer.low = low.clamp(other.low, other.high);
	answer.high = high.clamp(other.low, other.high);
	return answer;
}


NormalId Extents::closestFace(const Vector3& point)
{
	float x = std::abs(point.x - high.x);	
	float y = std::abs(point.y - high.y);	
	float z = std::abs(point.z - high.z);	
	float nx = std::abs(point.x - low.x);
	float ny = std::abs(point.y - low.y);	
	float nz = std::abs(point.z - low.z);	
	
	NormalId best = NORM_X; 
	float bestV = x;
	if (y < bestV)	{best = NORM_Y; bestV = y;}
	if (z < bestV)	{best = NORM_Z; bestV = z;}
	if (nx < bestV)	{best = NORM_X_NEG; bestV = nx;}
	if (ny < bestV)	{best = NORM_Y_NEG; bestV = ny;}
	if (nz < bestV)	{best = NORM_Z_NEG;}
	return best;
}



// Looking along -Z axis
// +x to the right
//	Front (Z)			Back (-Z)
/*
				^
				+y
	3	7				2	6
									+x -->
	1	5				0	4		
*/


Vector3int16 Extents::getCornerIndex(int i) const
{
	RBXASSERT(i >= 0);
	RBXASSERT(i <= 7);
	int x_id = i / 4;			// 0,0,0,0,1,1,1,1
	int y_id = (i / 2) % 2;		// 0,0,1,1,0,0,1,1
	int z_id = i % 2;			// 0,1,0,1,0,1,0,1
	return Vector3int16(x_id, y_id, z_id);
}

Vector3 Extents::getCorner(int i) const
{
	const Vector3int16 index = getCornerIndex(i);
	float x = (&low)[index.x].x;
	float y = (&low)[index.y].y;
	float z = (&low)[index.z].z;
	return Vector3(x, y, z);
}


void Extents::getFaceCorners(
	NormalId			faceId,
	Vector3&            v0,
	Vector3&            v1,
	Vector3&            v2,
	Vector3&            v3) const
{
    switch (faceId) {
    case 0:
        v0 = getCorner(4); v1 = getCorner(6); v2 = getCorner(7); v3 = getCorner(5);
        break;

    case 1:
        v0 = getCorner(2); v1 = getCorner(3); v2 = getCorner(7); v3 = getCorner(6);
        break;

    case 2:
        v0 = getCorner(1); v1 = getCorner(5); v2 = getCorner(7); v3 = getCorner(3);
        break;

    case 3:
        v0 = getCorner(0); v1 = getCorner(1); v2 = getCorner(3); v3 = getCorner(2);
        break;

    case 4:
        v0 = getCorner(0); v1 = getCorner(4); v2 = getCorner(5); v3 = getCorner(1);
        break;

    case 5:
        v0 = getCorner(0); v1 = getCorner(2); v2 = getCorner(6); v3 = getCorner(4);
        break;

    default:
		RBXASSERT(0);
		break;
    }
}

Extents Extents::express(const CoordinateFrame& myFrame, 
						 const CoordinateFrame& expressInFrame) const
{
	Vector3 minC(Math::inf(), Math::inf(), Math::inf());
	Vector3 maxC(-Math::inf(), -Math::inf(), -Math::inf());

	for (int i = 0; i < 8; ++i) {
		Vector3 corner = this->getCorner(i);
		Vector3 world = myFrame.pointToWorldSpace(corner);
		Vector3 inOther = expressInFrame.pointToObjectSpace(world);
		minC = minC.min(inOther);
		maxC = maxC.max(inOther);
	}
	return Extents::vv(minC, maxC);
}

Extents Extents::toWorldSpace(const CoordinateFrame& localCoord) const
{
    if (isNull())
        return Extents();

    Vector3 center = (low + high) * 0.5f;
    Vector3 halfSize = (high - low) * 0.5f;

    Vector3 newCenter = localCoord.pointToWorldSpace(center);
    Vector3 newHalfSize = Vector3(
        fabs(localCoord.rotation[0][0]) * halfSize[0] + fabs(localCoord.rotation[0][1]) * halfSize[1] + fabs(localCoord.rotation[0][2]) * halfSize[2],
        fabs(localCoord.rotation[1][0]) * halfSize[0] + fabs(localCoord.rotation[1][1]) * halfSize[1] + fabs(localCoord.rotation[1][2]) * halfSize[2],
        fabs(localCoord.rotation[2][0]) * halfSize[0] + fabs(localCoord.rotation[2][1]) * halfSize[1] + fabs(localCoord.rotation[2][2]) * halfSize[2]);

    return Extents(newCenter - newHalfSize, newCenter + newHalfSize);
}

Plane Extents::getPlane(NormalId normalId) const
{
	Vector3 point = faceCenter(normalId);
	Vector3 normal = normalIdToVector3(normalId);
	return Plane(normal, point);
}


Vector3 Extents::faceCenter(NormalId faceId) const
{
	Vector3 answer = center();
	int xyz = faceId % 3;
	if (faceId < 3)
	{						// i.e., positive face x, y, z
		answer[xyz] = high[xyz];
	}
	else
	{						// i.e., negative face -x, -y, -z
		answer[xyz] = low[xyz];
	}
	return answer;
}


Vector3 Extents::clamp(const Extents& innerExtents) const
{
	Vector3 answer(Vector3::zero());

	// for x..z
	for (int i = 0; i < 3; i++) {
		RBXASSERT(innerExtents.size()[i] <= this->size()[i]);
		if (innerExtents.max()[i] > this->max()[i]) {
			answer[i] = this->max()[i] - innerExtents.max()[i];
		}
		else if (innerExtents.min()[i] < this->min()[i]) {
			answer[i] = this->min()[i] - innerExtents.min()[i];
		}
	}
	return answer;
}

float Extents::computeClosestSqDistanceToPoint(const Vector3& point) const
{
	Vector3 center = (high + low) / 2;
	Vector3 extents = (high - low) / 2;

	return ClosestSqDistanceToAABB(point, center, extents);
}

bool Extents::separatedByMoreThan(const Extents& other, float distance) const
{
	RBXASSERT(distance > 0.0);
	Extents thisExpanded(*this);
	thisExpanded.expand(distance);
	return !(thisExpanded.overlapsOrTouches(other));
}

} // namespace
