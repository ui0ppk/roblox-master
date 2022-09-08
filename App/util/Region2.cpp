/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/Region2.h"
#include "Util/quadedge.h"

namespace RBX {


// static
float Region2::getRelativeError(const Vector2& pos2d, const WeightedPoint& owner)
{
	float distanceSquared = (pos2d - owner.point).squaredLength();
	float radiusSquared = owner.radius * owner.radius;

	return distanceSquared/radiusSquared;		// alternately could return only distance squared
}



// static
bool Region2::pointInRange(const Vector2& pos2d, const WeightedPoint& owner, const float slop)
{
	float adjustedRadius = owner.radius * slop;
	float adjustedRadiusSquared = adjustedRadius * adjustedRadius;

	float distanceSquared = (pos2d - owner.point).squaredLength();
	return (distanceSquared < adjustedRadiusSquared);
}


bool Region2::contains(const Vector2& pos2d, const float slop) const
{
	if (!pointInRange(pos2d, owner, slop))				// 1.  Distance must be inside the circle
	{	
		return false;
	}
	else												// 2.	True unless we find someone closer!
	{
		return !findCloserOther(pos2d, slop);
	}
}


bool Region2::findCloserOther(const Vector2& point, const float slop) const
{
	for (int i = 0; i < others.size(); ++i) 		// 2.  Point closer to a secondary Point - must be false
	{	
		if (closerToOtherPoint(point, owner, others[i], slop))
		{
			return true;
		}
	}
	return false;
}

bool Region2::closerToOtherPoint(const Vector2& pos2d,
								const WeightedPoint& owner,
								const WeightedPoint& other,
								const float slop)
{
	if (owner.point == other.point) {
		return false;
	}

	float ownerSquaredDistance = (pos2d - owner.point).squaredLength();
	float otherSquaredDistance = (pos2d - other.point).squaredLength();

	if (otherSquaredDistance < ownerSquaredDistance)
	{
		float radiusDelta = owner.radius * (slop - 1.0f);
		float radiusDeltaSquared = radiusDelta * radiusDelta;
		if (ownerSquaredDistance - otherSquaredDistance > radiusDeltaSquared)
			return true;
	}
	return false;
}

} // namespace