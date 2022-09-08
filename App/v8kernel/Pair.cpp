#include "stdafx.h"

#include "V8Kernel/Pair.h"
#include "V8Kernel/Constants.h"
#include "V8Kernel/Body.h"
#include "Util/Math.h"


namespace RBX {


GeoPair::GeoPair() 
: body0(NULL)
, body1(NULL)
{}



void GeoPair::computePointPlane(PairParams& _params) 
{
	_params.position = body0->getCoordinateFrameFast().pointToWorldSpace(offset0);
	const CoordinateFrame& c1 = body1->getCoordinateFrameFast();
	Vector3 pPlane =  c1.pointToWorldSpace(offset1);
	_params.normal = -Math::getWorldNormal(pairData.normalID1, c1);
	_params.length = _params.normal.dot(pPlane - _params.position);		// length is negative when overlapping
}

// using notation from ODE
// Use the normal from the plane - planeID
// b0 is an edge body
// b1 is an edge body, and the planeNormal body
// if (reversePolarity), this means that
// the normal needs to be switched
// because the points corresponds to opposite bodies
// only need position of pa, the NON-PLANE edge

void GeoPair::computeEdgeEdgePlane(PairParams& _params) 
{
	const CoordinateFrame& ca = body0->getCoordinateFrameFast();
	const CoordinateFrame& cPlane = body1->getCoordinateFrameFast();

	Vector3 pa		= ca.pointToWorldSpace(offset0);
	Vector3 pPlane	= cPlane.pointToWorldSpace(offset1);

	Vector3 ua = Math::getWorldNormal(pairData.normalID0, ca);
	Vector3 ub = Math::getWorldNormal(pairData.normalID1, cPlane);
	
	Vector3 p = pPlane - pa;
	float uaub = ua.dot(ub);
	float q1 = ua.dot(p);
	float q2 = -ub.dot(p);

	float d = 1 - uaub*uaub;

	_params.normal = -Math::getWorldNormal(pairData.planeID, cPlane);

	if (d > 1e-5) {						// work here
		float alpha = (q1 + uaub*q2) / d;
		if (fabs(alpha) > 6.0f)
		{
			alpha = 6.0f * Math::sign(alpha);
		}
		_params.position = pa + alpha * ua;
		_params.length = _params.normal.dot(pPlane - _params.position);
	}
	else {
		_params.position = pa;
		_params.length = 0.0f;
	}
}

void GeoPair::computeEdgeEdgePlane2(PairParams& _params) 
{
	const CoordinateFrame& ca = body0->getCoordinateFrameFast();
	const CoordinateFrame& cPlane = body1->getCoordinateFrameFast();

    Vector3 p1 = offset0;
    Vector3 p2 = ca.pointToObjectSpace(cPlane.pointToWorldSpace(offset1));

	Vector3 u1 = ca.vectorToObjectSpace(Math::getWorldNormal(pairData.normalID0, ca));
	Vector3 u2 = ca.vectorToObjectSpace(Math::getWorldNormal(pairData.normalID1, cPlane));
	
    _params.normal = -Math::getWorldNormal(pairData.planeID, cPlane);

    // Effectively, if they're far enough from being parallel, 
	if( fabs(fabs(u1.dot(u2)) - 1.0) > 1e-5 )
    {
        float a1 = (p1 - p2).dot(u1);
        float a2 = (p1 - p2).dot(u2);  
        float b = u1.dot(u2);

        float t1 = (b*a2 - a1*b*b)/(1 - b*b) - a1;
        float t2 = (a2 - a1*b)/(1 - b*b);

		Vector3 body1ClosestPointInBody1 = p1 + t1*u1;
		
		Vector3 body2ClosestPointInBody2 = cPlane.pointToObjectSpace(ca.pointToWorldSpace(p2 + t2*u2));
		float body1ClosestPointEdgeLocation = body1ClosestPointInBody1[pairData.normalID0 % 3];
		float body2ClosestPointEdgeLocation = body2ClosestPointInBody2[pairData.normalID1 % 3];

		float eps = 0.05;
		if ((fabs(body1ClosestPointEdgeLocation) > 0.5 * edgeLength0 + eps) ||
			(fabs(body2ClosestPointEdgeLocation) > 0.5 * edgeLength1 + eps))
		{
			_params.position = ca.pointToWorldSpace(offset0);
			_params.length = 0.0f;
		}
		else
		{
			_params.position = ca.pointToWorldSpace(body1ClosestPointInBody1);
			_params.length = _params.normal.dot(cPlane.pointToWorldSpace(offset1) - _params.position);
		}
    }
    else
    {
        _params.position = ca.pointToWorldSpace(offset0);
		_params.length = 0.0f;
    }
}

void GeoPair::computeEdgeEdge(PairParams& _params) 
{
	const CoordinateFrame& ca = body0->getCoordinateFrameFast();
	const CoordinateFrame& cb = body1->getCoordinateFrameFast();

	Vector3 pa = ca.pointToWorldSpace(offset0);
	Vector3 pb = cb.pointToWorldSpace(offset1);

	Vector3 ua = Math::getWorldNormal(pairData.normalID0, ca);
	Vector3 ub = Math::getWorldNormal(pairData.normalID1, cb);

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
		_params.position = 0.5 * (pa + pb);
		_params.normal = pa - pb;
		_params.length = -_params.normal.unitize();
	}
	else {
		_params.position = pa;
		_params.normal = ua;
		_params.length = 0.0;	// hopefully will blow out;
	}
}

} // namespace


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag12 = 0;
    };
};
