/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "AppDraw/HitTest.h"
#include "GfxBase/Part.h"
#include "G3D/CollisionDetection.h"
#include "G3D/Capsule.h"

namespace RBX {

bool HitTest::hitTestBox(const Part& part, 
					  RBX::RbxRay& rayInPartCoords, 
					  Vector3& hitPointInPartCoords,
					  float gridToReal)
{
	Vector3 halfRealSize = part.gridSize * gridToReal * 0.5;

	return G3D::CollisionDetection::collisionLocationForMovingPointFixedAABox(
							rayInPartCoords.origin(),
							rayInPartCoords.direction(),
							G3D::AABox(-halfRealSize, halfRealSize),
							hitPointInPartCoords);
}


bool HitTest::hitTestBall(const Part& part, 
					   RBX::RbxRay& rayInPartCoords, 
					   Vector3& hitPointInPartCoords,
					   float gridToReal)
{
	float radius = part.gridSize.x * gridToReal* 0.5f;

	return (G3D::CollisionDetection::collisionTimeForMovingPointFixedSphere(
							rayInPartCoords.origin(),
							rayInPartCoords.direction(),
							G3D::Sphere(Vector3::zero(), radius),
							hitPointInPartCoords)	
							
							!= G3D::inf()	);
}

// TODO: Big optimization possible here...
// TODO: Clean up hit test here - offset stuff going on
bool HitTest::hitTestCylinder(const Part& part, 
						   RBX::RbxRay& rayInPartCoords, 
						   Vector3& hitPointInPartCoords,
						   float gridToReal)
{
	float radius = part.gridSize.z * gridToReal * 0.5f;
	float axis = part.gridSize.x * gridToReal * 0.5f;		// TODO - Document all of this - World and Grid space

	return (G3D::CollisionDetection::collisionTimeForMovingPointFixedCapsule(
							rayInPartCoords.origin(),
							rayInPartCoords.direction(),
							G3D::Capsule(Vector3::zero(), Vector3(axis, 0, 0), radius),
							hitPointInPartCoords)	
							
							!= G3D::inf()	);
}

bool HitTest::hitTest(const Part& part,
						   RBX::RbxRay& rayInPartCoords, 
						   Vector3& hitPointInPartCoords,
						   float gridToReal)
{
	switch (part.type)
	{
		case RBX::BLOCK_PART:		return hitTestBox(part, rayInPartCoords, hitPointInPartCoords, gridToReal);
		case RBX::BALL_PART:		return hitTestBall(part, rayInPartCoords, hitPointInPartCoords, gridToReal);
		case RBX::CYLINDER_PART:	return hitTestCylinder(part, rayInPartCoords, hitPointInPartCoords, gridToReal);
		default:				debugAssert(0);
	}
	return false;
}

} // namespace