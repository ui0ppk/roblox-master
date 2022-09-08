#include "stdafx.h"

#include "V8Kernel/PolyConnectors.h"
#include "V8Kernel/Body.h"
#include "Util/Math.h"

namespace RBX {



//////////////////////////////////////////////////////////////////////////////////////////

/*
	Position == position in world coordinates of deepest penetration point.
	Length == (negative) value - amount of penetration
	Normal == points "away" from the b0 object, into the b1 object
*/

void BallPlaneConnector::updateContactPoint() 
{
	const Vector3& p0 = geoPair.body0->getPosFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();

	Vector3 p1 = c1.pointToWorldSpace(offset);
//	contactPoint.normal = -Math::getWorldNormal(normalId1, c1);
	contactPoint.normal = -c1.vectorToWorldSpace(normal);			// points "away from b0"

	Vector3 delta = p1 - p0;			// vector from p1 to p0, edge point to center of ball
	Vector3 centerToPlanePoint = contactPoint.normal * contactPoint.normal.dot(delta);	
	contactPoint.position = p0 + centerToPlanePoint;
	contactPoint.length = centerToPlanePoint.length() - radius;
	ContactConnector::updateContactPoint();
}


void BallEdgeConnector::updateContactPoint() 
{
	const Vector3& p0 = geoPair.body0->getPosFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();

	Vector3 p1 = c1.pointToWorldSpace(offset);
	Vector3 edgeNormal = c1.vectorToWorldSpace(normal);
	Vector3 delta = p1 - p0;	// vector from p0 to p1, center of ball to edge point
	Vector3 projection = edgeNormal * edgeNormal.dot(delta);
	contactPoint.normal = delta - projection;			// right now normal is not unitized
	contactPoint.position = p0 + contactPoint.normal;
	contactPoint.length = contactPoint.normal.unitize() - radius;
	ContactConnector::updateContactPoint();
}


void BallVertexConnector::updateContactPoint() 
{
	const Vector3& pCenter = geoPair.body0->getPosFast();
	contactPoint.position = geoPair.body1->getCoordinateFrameFast().pointToWorldSpace(offset);
	contactPoint.normal = contactPoint.position - pCenter;
	contactPoint.length = contactPoint.normal.unitize() - radius;
	ContactConnector::updateContactPoint();
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


void FaceVertexConnector::updateContactPoint()
{
	Plane planeInWorld = geoPair.body0->getCoordinateFrameFast().toWorldSpace(facePlane);

	contactPoint.position = geoPair.body1->getCoordinateFrameFast().pointToWorldSpace(vertexOffset);

	contactPoint.normal = planeInWorld.normal();
	contactPoint.length = planeInWorld.distance(contactPoint.position);
	ContactConnector::updateContactPoint();
}
/*

void FaceEdgeConnector::updateContactPoint()
{
	const CoordinateFrame& c0 = geoPair.body0->getCoordinateFrameFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();

	Line line0World = c0.toWorldSpace(faceLine);
	Line line1World = c1.toWorldSpace(edgeLine);

	Vector3 pa = line0World.point();
	Vector3 pb = line1World.point();

	Vector3 ua = line0World.direction();
	Vector3 ub = line1World.direction();

	Vector3 p = pb - pa;
	float uaub = ua.dot(ub);
	float q1 = ua.dot(p);
	float q2 = -ub.dot(p);
	float d = 1 - uaub*uaub;
	if (d > 1e-6f) {
		d = 1.0f / d;
		float alpha = (q1 + uaub*q2)*d;
		float beta = (uaub*q1 + q2)*d;
		pa = pa + alpha * ua;
		pb = pb + beta * ub;

		contactPoint.position = pa;
		contactPoint.normal = pa - pb;		// backwards
		contactPoint.length = -contactPoint.normal.unitize();

		Vector3 outwards = c0.vectorToWorldSpace(facePlane.normal());

		if (params.normal.dot(outwards) > 0.0) {			// lines are overlapping
			RBXASSERT(params.length <= 0.0);
		}
		else {
			contactPoint.normal = -contactPoint.normal;		// not overlapping
			contactPoint.length = -contactPoint.length;
			RBXASSERT(contactPoint.length >= 0.0);
		}
	}
	else {
		contactPoint.position = pa;
		contactPoint.normal = ua;
		contactPoint.length = 0.0;	// hopefully will blow out;
	}
	ContactConnector::updateContactPoint();
}
*/


void FaceEdgeConnector::updateContactPoint()
{
	const CoordinateFrame& c0 = geoPair.body0->getCoordinateFrameFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();

	Line line0World = c0.toWorldSpace(faceLine);
	Line line1World = c1.toWorldSpace(edgeLine);

	Vector3 p0, p1;

	if (Line::closestPoints(line0World, line1World, p0, p1))
	{
		contactPoint.position = p0;
		contactPoint.normal = p0 - p1;		// backwards
		contactPoint.length = -contactPoint.normal.unitize(1e-20);

		Vector3 outwards = c0.vectorToWorldSpace(facePlane.normal());

		if (contactPoint.normal.dot(outwards) > 0.0) {			// lines are overlapping
			RBXASSERT(contactPoint.length <= 0.0);
		}
		else {
			contactPoint.normal = -contactPoint.normal;		// not overlapping
			contactPoint.length = -contactPoint.length;
			RBXASSERT(contactPoint.length >= 0.0);
		}
	}
	else {
		contactPoint.position = p0;
		contactPoint.normal = c0.vectorToWorldSpace(facePlane.normal());
		contactPoint.length = 0.0;	// hopefully will blow out;
	}
	ContactConnector::updateContactPoint();
}




void EdgeEdgeConnector::updateContactPoint()
{
	const CoordinateFrame& c0 = geoPair.body0->getCoordinateFrameFast();
	const CoordinateFrame& c1 = geoPair.body1->getCoordinateFrameFast();

	Line line0World = c0.toWorldSpace(edgeLine0);
	Line line1World = c1.toWorldSpace(edgeLine1);

	Vector3 pa = line0World.point();
	Vector3 pb = line1World.point();

	Vector3 ua = line0World.direction();
	Vector3 ub = line1World.direction();

	Vector3 p = pb - pa;
	float uaub = ua.dot(ub);
	float q1 = ua.dot(p);
	float q2 = -ub.dot(p);
	float d = 1 - uaub*uaub;
	if (d > 1e-6f) {
		d = 1.0f / d;
		float alpha = (q1 + uaub*q2)*d;
		float beta = (uaub*q1 + q2)*d;
		pa = pa + alpha * ua;
		pb = pb + beta * ub;

//		contactPoint.position = 0.5 * (pa + pb);
//		contactPoint.normal = pa - pb;
//		contactPoint.length = -params.normal.unitize();

		contactPoint.position = pa;
		contactPoint.normal = pa - pb;		// backwards
		contactPoint.length = -contactPoint.normal.unitize();

		Vector3 b0ToB1 = c1.translation - c0.translation;
		if (contactPoint.normal.dot(b0ToB1) > 0.0) {			// lines are overlapping
			RBXASSERT(contactPoint.length <= 0.0);
		}
		else {
			contactPoint.normal = -contactPoint.normal;		// not overlapping
			contactPoint.length = -contactPoint.length;
			RBXASSERT(contactPoint.length >= 0.0);
		}
	}
	else {
		contactPoint.position = pa;
		contactPoint.normal = ua;
		contactPoint.length = 0.0;	// hopefully will blow out;
	}
	ContactConnector::updateContactPoint();
}


} // namespace