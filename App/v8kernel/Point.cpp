/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8Kernel/Point.h"
#include "V8Kernel/Body.h"


namespace RBX {

// This is private - only created by the kernel
Point::Point(Body* _body) : 
	numOwners(1),
	body(_body ? _body : Body::getWorldBody())

{}







void Point::step() 
{
	worldPos = body->getCoordinateFrame().pointToWorldSpace(localPos);

	force = Vector3::zero();
}

void Point::forceToBody() 
{
	body->accumulateForce(force, worldPos);

}

void Point::setLocalPos(const Vector3& _localPos) 
{
	localPos = _localPos;
	worldPos = body->getCoordinateFrame().pointToWorldSpace(localPos);
}

void Point::setWorldPos(const Vector3& _worldPos) 
{
	worldPos = _worldPos;
	localPos = body->getCoordinateFrame().pointToObjectSpace(worldPos);
}
		

} // namespace RBX

