#include "stdafx.h"

#include "v8Kernel/BuoyancyConnector.h"
#include "V8Kernel/Body.h"

namespace RBX
{
	const Vector3 BuoyancyConnector::getWorldPosition()
	{
		const CoordinateFrame& cf(getBody(body1)->getPvSafe().position); 
		return cf.pointToWorldSpace(position);
	}

	void BuoyancyConnector::computeForce( bool throttling )
	{
		getBody(body1)->accumulateForce(force, getWorldPosition());
		getBody(body1)->accumulateTorque(torque);
	}

	BuoyancyConnector::BuoyancyConnector(Body* b0, Body* b1, const Vector3& pos) :
	ContactConnector(b0, b1, ContactParams()),
		position(pos),
		force(Vector3::zero()),
		torque(Vector3::zero()),
		floatDistance(0.0f),
		sinkDistance(0.0f),
		submergeRatio(0.0f)
	{
	}

	void BuoyancyConnector::updateContactPoint()
	{
		// This is used for debug rendering only
		const CoordinateFrame& cf(getBody(body1) ->getPvUnsafe().position); 
		contactPoint.position = cf.pointToWorldSpace(position);
		contactPoint.normal = force.direction();
		contactPoint.length = -force.magnitude() / 3000.0f;
	}
}