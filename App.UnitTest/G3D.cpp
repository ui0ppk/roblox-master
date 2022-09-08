#include <boost/test/unit_test.hpp>

#include "RbxG3D/RbxRay.h"
#include "G3D/AABox.h"
#include "G3D/Color3.h"
#include "G3D/CoordinateFrame.h"
#include "V8DataModel/Camera.h"
#include "Util/Math.h"
#include "Util/NormalId.h"
#include "Util/Extents.h"


BOOST_AUTO_TEST_SUITE(G3D)

BOOST_AUTO_TEST_CASE(TestRbxRay)
{
	RBX::RbxRay ray = RBX::RbxRay::fromOriginAndDirection(G3D::Vector3(0, 12, 0), G3D::Vector3(2, 0, 0));	
	BOOST_CHECK_EQUAL(ray.closestPoint(G3D::Vector3(1, 0, 0)), G3D::Vector3(4, 12, 0));
	BOOST_CHECK_CLOSE(ray.distance(G3D::Vector3(1, 0, 0)), 12.3693171f, 0.0001f);
}


BOOST_AUTO_TEST_CASE(TestCamera)
{
	RBX::Camera camera;

	// Create a Dummy Viewport Rect to simulate the real window viewport.
	Vector2 vec2(1662.0f, 666.0f);
	G3D::Rect2D viewport(vec2);
	camera.setViewport(vec2);
    // This is an indirect way of setting the imagePlaneDepth
    camera.setFieldOfViewDegrees(70.0f);

    ///Make sure you get the origin & direction rbxRay correct when you pass in the mouse coord & viewport rect.
    RBX::RbxRay rbxRay = camera.worldRay(377.0f, 149.0f);
    Vector3 rayOrigin = rbxRay.origin();

	BOOST_CHECK_CLOSE(rayOrigin.x, -0.681681f, 0.005f);
	BOOST_CHECK_CLOSE(rayOrigin.y, 19.6904f, 0.005f);
	BOOST_CHECK_CLOSE(rayOrigin.z, 19.2997f, 0.005f);

	Vector3 direction = rbxRay.direction();
	BOOST_CHECK_CLOSE(direction.x, -0.664961874f, 0.0005f);
	BOOST_CHECK_CLOSE(direction.y, -0.30197686f, 0.0005f);
	BOOST_CHECK_CLOSE(direction.z, -0.683107376f, 0.0005f);

}

BOOST_AUTO_TEST_CASE(TestAABox)
{
	AABox aaBox(G3D::Vector3(0,0,-8), G3D::Vector3(8,8,0));
	BOOST_CHECK_EQUAL(aaBox.isFinite(), true);
	BOOST_CHECK_EQUAL(aaBox.low(), G3D::Vector3(0, 0, -8));
	BOOST_CHECK_EQUAL(aaBox.high(), G3D::Vector3(8, 8, 0));
	BOOST_CHECK_EQUAL(aaBox.center(), G3D::Vector3(4, 4, -4));
	BOOST_CHECK_EQUAL(aaBox.corner(1), G3D::Vector3(8, 0, 0));
	BOOST_CHECK_EQUAL(aaBox.extent(2),8.0);
	BOOST_CHECK_EQUAL(aaBox.extent(), G3D::Vector3(8, 8, 8));
	BOOST_CHECK_EQUAL(aaBox.area(), 384);
	BOOST_CHECK_EQUAL(aaBox.volume(), 512);
}

BOOST_AUTO_TEST_CASE(TestColor3)
{
	Color3 color(0.2f, 0.4f, 0.8f);
	BOOST_CHECK_EQUAL(color.isZero(), false);
	BOOST_CHECK_EQUAL(color.isFinite(), true);	

	Color3 dummy(0.7f, 0.2f, 0.5f);
	BOOST_CHECK_EQUAL(color.max(dummy), Color3(0.7f,0.4f,0.8f));
	BOOST_CHECK_EQUAL(color.min(dummy), Color3(0.2f,0.2f,0.5f));
	BOOST_CHECK_EQUAL(color.fuzzyEq(dummy), false);
	BOOST_CHECK_EQUAL(color.fuzzyNe(dummy), true);	
}

BOOST_AUTO_TEST_CASE(TestCoordinateFrame)
{
	G3D::CoordinateFrame coord(normalIdToMatrix3(RBX::NORM_Y), Vector3(0, -0.5, 0));
	G3D::CoordinateFrame dummy(normalIdToMatrix3(RBX::NORM_X_NEG), Vector3(0.5, 0.5, 0));	

	BOOST_CHECK_EQUAL(coord.fuzzyEq(dummy), false);	

	float x,y,z,yaw,p,r;
	coord.getXYZYPRDegrees(x, y, z, yaw, p, r);

	BOOST_CHECK_EQUAL(x, 0);
	BOOST_CHECK_EQUAL(y, -0.5f);
	BOOST_CHECK_EQUAL(z, 0);
	BOOST_CHECK_EQUAL(yaw, 360);
	BOOST_CHECK_CLOSE(p, -90.0f, 0.0001f);
	BOOST_CHECK_EQUAL(r, 0);

	RBX::RbxRay rbxRay;
	rbxRay.origin() = G3D::Vector3(0, 0, 0);
	rbxRay.direction() = G3D::Vector3(0.5f, 0.5f, 0.5f);

	RBX::RbxRay rbxRayObjectSpace = coord.toObjectSpace(rbxRay);

	BOOST_CHECK_EQUAL(rbxRayObjectSpace.origin(), G3D::Vector3(0, 0, 0.5));
	BOOST_CHECK_EQUAL(rbxRayObjectSpace.direction(), G3D::Vector3(-0.5, 0.5, 0.5));
	
	G3D::CoordinateFrame presnapCFrame(G3D::Matrix3(0,-0.999999881f, 0, 0, 0, 1, -0.999999881f, 0, 0), G3D::Vector3(11, -1.20000076f, 4.50000763f));
	G3D::CoordinateFrame snappedCFrame = RBX::Math::snapToGrid(presnapCFrame, 0.100000001f);
	BOOST_CHECK_EQUAL(snappedCFrame.rotation, G3D::Matrix3(0,-1, 0, 0, 0, 1, -1, 0, 0));
	BOOST_CHECK_EQUAL(snappedCFrame.translation, G3D::Vector3(11, -1.2f, 4.5f));
}

BOOST_AUTO_TEST_CASE(TestVector3)
{
	G3D::Vector3 v1(4, 2, 1);
	G3D::Vector3 v2(3, -4, 6);
	
	BOOST_CHECK_EQUAL(v1+v2, G3D::Vector3(7, -2, 7));
	BOOST_CHECK_EQUAL(v1-v2, G3D::Vector3(1, 6, -5));
	BOOST_CHECK_CLOSE(v1.length(), 4.5825758f, 0.0001f);	

	Vector3 v = v1.direction();
	BOOST_CHECK_CLOSE(v.x, 0.87287152f, 0.0001f);
	BOOST_CHECK_CLOSE(v.y, 0.43643576f, 0.0001f);
	BOOST_CHECK_CLOSE(v.z, 0.21821788f, 0.0001f);
	

	v = v1.reflectAbout(v2);
	BOOST_CHECK_CLOSE(v.x, -3.0163934f, 0.0001f);
	BOOST_CHECK_CLOSE(v.y, -3.3114753f, 0.0001f);
	BOOST_CHECK_CLOSE(v.z, 0.96721303f, 0.0001f);


	v = v1.reflectionDirection(v2);
	BOOST_CHECK_CLOSE(v.x, 0.65823108f, 0.0001f);
	BOOST_CHECK_CLOSE(v.y, 0.72262323f, 0.0001f);
	BOOST_CHECK_CLOSE(v.z, -0.21106321f, 0.0001f);	

	v = v1.refractionDirection(v2, 0.2f, 0.7f);
	BOOST_CHECK_CLOSE(v.x, 0.58810276f, 0.0001f);
	BOOST_CHECK_CLOSE(v.y, -0.32691860f, 0.0001f);
	BOOST_CHECK_CLOSE(v.z, 0.73976976f, 0.0001f);	

	BOOST_CHECK_CLOSE(v1.unitize(), 4.5825758f, 0.0001f);

	
	v = v1.lerp(v2, 0.1f);
	BOOST_CHECK_CLOSE(v.x, 1.0855844f, 0.0001f);
	BOOST_CHECK_CLOSE(v.y, -0.0072078109f, 0.0001f);
	BOOST_CHECK_CLOSE(v.z, 0.79639614f, 0.0001f);	
}

bool FailsPlane(const G3D::Vector3& pt, const RBX::Frustum& fr, RBX::Frustum::FrustumPlane plane)
{
	for (int ii = 0; ii < fr.faceArray.size(); ++ii)
	{
		bool passes = fr.faceArray[ii].halfSpaceContains(pt);
		if (!passes && static_cast<int>(plane) != ii)
		{
			return false;
		}
		if (passes && static_cast<int>(plane) == ii)
		{
			return false;
		}
	}

	return true;
}

bool FailsPlane(const RBX::Extents& ex, const RBX::Frustum& fr, RBX::Frustum::FrustumPlane plane)
{
	bool retVal = false;
	for (int ii = 0; ii < 8; ++ii)
	{
		Vector3 corner = ex.getCorner(ii);
		if (FailsPlane(corner, fr, plane))
		{
			retVal = true;
		}
	}
	return retVal;
}

BOOST_AUTO_TEST_CASE(TestFrustum)
{
	const float kNearPlaneDist = 0.5f;
	const float kFarPlaneDist = 3.f;
	const float kFortyFiveDegInRad = 0.707107f;
	const float kNinetyDegInRad = 1.57079633f;

	// at a distance of 1 from the apex the planes will intersect +/- 0.707 in the x/y direction

	RBX::Extents ex(G3D::Vector3(-0.5f, -0.5f, 1.f), G3D::Vector3(0.5f, 0.5f, 2.f));
	RBX::Frustum fr1(G3D::Vector3::zero(), G3D::Vector3(0, 0, 1), G3D::Vector3(0, 1, 0), kNearPlaneDist, kFarPlaneDist, kNinetyDegInRad, kNinetyDegInRad);
	RBX::Frustum fr2(G3D::Vector3::zero(), G3D::Vector3(0, 0, 1), G3D::Vector3(0, 1, 0), kNearPlaneDist, RBX::Math::inf(), kNinetyDegInRad, kNinetyDegInRad);

	// extents completely in frustum
	BOOST_CHECK(fr1.containsAABB(ex));
	BOOST_CHECK(fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneInvalid));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneInvalid));

	// crosses far plane
	ex = RBX::Extents(G3D::Vector3(-0.5f, -0.5f, kFarPlaneDist - 1.f), G3D::Vector3(0.5f, 0.5f, kFarPlaneDist + 1.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(fr2.containsAABB(ex));	// no far plane
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneFar));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneInvalid));

	// beyond far plane
	ex = RBX::Extents(G3D::Vector3(-0.5f, -0.5f, kFarPlaneDist + 1.f), G3D::Vector3(0.5f, 0.5f, kFarPlaneDist + 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(fr2.containsAABB(ex));	// no far plane
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneFar));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneInvalid));

	// between apex and near plane
	ex = RBX::Extents(G3D::Vector3(-0.1f, -0.1f, kNearPlaneDist - 0.2f), G3D::Vector3(0.1f, 0.1f, kNearPlaneDist - 0.1f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneNear));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneNear));

	// crosses near plane
	ex = RBX::Extents(G3D::Vector3(-0.1f, -0.1f, kNearPlaneDist - 0.1f), G3D::Vector3(0.1f, 0.1f, kNearPlaneDist + 0.1f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneNear));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneNear));

	// opposite side of apex
	ex = RBX::Extents(G3D::Vector3(-0.5f, -0.5f, -2.f), G3D::Vector3(0.5f, 0.5f, -1.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));

	// above top plane
	ex = RBX::Extents(G3D::Vector3(-0.5f, 5.f, 1.f), G3D::Vector3(0.5f, 6.f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneTop));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneTop));

	// crosses top plane
	ex = RBX::Extents(G3D::Vector3(-0.5f, 0.f, 1.f), G3D::Vector3(0.5f, 6.f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneTop));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneTop));

	// below bottom plane
	ex = RBX::Extents(G3D::Vector3(-0.5f, -6.f, 1.f), G3D::Vector3(0.5f, -5.f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneBottom));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneBottom));

	// crosses bottom plane
	ex = RBX::Extents(G3D::Vector3(-0.5f, -6.f, 1.f), G3D::Vector3(0.5f, 0.f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneBottom));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneBottom));

	// outside left plane
	ex = RBX::Extents(G3D::Vector3(-6.f, -0.5f, 1.f), G3D::Vector3(-5.f, 0.5f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneLeft));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneLeft));

	// crosses left plane
	ex = RBX::Extents(G3D::Vector3(-6.f, -0.5f, 1.f), G3D::Vector3(0.f, 0.5f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneLeft));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneLeft));

	// outside right plane
	ex = RBX::Extents(G3D::Vector3(5.f, -0.5f, 1.f), G3D::Vector3(6.f, 0.5f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneRight));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneRight));

	// crosses right plane
	ex = RBX::Extents(G3D::Vector3(0.f, -0.5f, 1.f), G3D::Vector3(6.f, 0.5f, 2.f));
	BOOST_CHECK(!fr1.containsAABB(ex));
	BOOST_CHECK(!fr2.containsAABB(ex));
	BOOST_CHECK(FailsPlane(ex, fr1, RBX::Frustum::kPlaneRight));
	BOOST_CHECK(FailsPlane(ex, fr2, RBX::Frustum::kPlaneRight));

	// sphere enclosed by frustum
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(0.f, 0.f, 1.f), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, 0.f, 1.f), 0.25f));

	// frustum enclosed by sphere
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(0.f, 0.f, 1.f), 10.f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, 0.f, 1.f), 10.f));

	// outside near plane
	BOOST_CHECK(!fr1.intersectsSphere(G3D::Vector3(0.f, 0.f, 0.f), 0.25f));
	BOOST_CHECK(!fr2.intersectsSphere(G3D::Vector3(0.f, 0.f, 0.f), 0.25f));

	// intersects near plane
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(0.f, 0.f, kNearPlaneDist), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, 0.f, kNearPlaneDist), 0.25f));

	// outside far plane
	BOOST_CHECK(!fr1.intersectsSphere(G3D::Vector3(0.f, 0.f, kFarPlaneDist + 1.f), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, 0.f, kFarPlaneDist + 1.f), 0.25f));	// no far plane

	// intersects far plane
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(0.f, 0.f, kFarPlaneDist), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, 0.f, kFarPlaneDist), 0.25f));

	// outside top plane
	BOOST_CHECK(!fr1.intersectsSphere(G3D::Vector3(0.f, 10.f, 1.f), 0.25f));
	BOOST_CHECK(!fr2.intersectsSphere(G3D::Vector3(0.f, 10.f, 1.f), 0.25f));

	// intersects top plane
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(0.f, kFortyFiveDegInRad, 1.f), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, kFortyFiveDegInRad, 1.f), 0.25f));

	// outside bottom plane
	BOOST_CHECK(!fr1.intersectsSphere(G3D::Vector3(0.f, -10.f, 1.f), 0.25f));
	BOOST_CHECK(!fr2.intersectsSphere(G3D::Vector3(0.f, -10.f, 1.f), 0.25f));

	// intersects bottom plane
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(0.f, -kFortyFiveDegInRad, 1.f), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(0.f, -kFortyFiveDegInRad, 1.f), 0.25f));

	// outside left plane
	BOOST_CHECK(!fr1.intersectsSphere(G3D::Vector3(-10.f, 0.f, 1.f), 0.25f));
	BOOST_CHECK(!fr2.intersectsSphere(G3D::Vector3(-10.f, 0.f, 1.f), 0.25f));

	// intersects left plane
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(-kFortyFiveDegInRad, 0.f, 1.f), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(-kFortyFiveDegInRad, 0.f, 1.f), 0.25f));

	// outside right plane
	BOOST_CHECK(!fr1.intersectsSphere(G3D::Vector3(10.f, 0.f, 1.f), 0.25f));
	BOOST_CHECK(!fr2.intersectsSphere(G3D::Vector3(10.f, 0.f, 1.f), 0.25f));

	// intersects right plane
	BOOST_CHECK(fr1.intersectsSphere(G3D::Vector3(kFortyFiveDegInRad, 0.f, 1.f), 0.25f));
	BOOST_CHECK(fr2.intersectsSphere(G3D::Vector3(kFortyFiveDegInRad, 0.f, 1.f), 0.25f));

	// check with more complicated starting position
	RBX::Frustum fr3(G3D::Vector3(0,0,10000), G3D::Vector3(0, 0, 1), G3D::Vector3(0, 1, 0), kNearPlaneDist, kFarPlaneDist, kNinetyDegInRad, kNinetyDegInRad);
	BOOST_CHECK(fr3.intersectsSphere(G3D::Vector3(kFortyFiveDegInRad, 0.f, 10001.f), .25f));

	// check for far off points that should still be just inside the frustum
	RBX::Frustum fr4(G3D::Vector3::zero(), G3D::Vector3(0, 0, 1), G3D::Vector3(0, 1, 0), kNearPlaneDist, RBX::Math::inf(), kNinetyDegInRad, kNinetyDegInRad);
	BOOST_CHECK(fr4.containsPoint(G3D::Vector3(999.f, 0.f, 1000.f)));
	BOOST_CHECK(fr4.containsPoint(G3D::Vector3(-999.f, 0.f, 1000.f)));
	BOOST_CHECK(fr4.containsPoint(G3D::Vector3(0.f, 999.f, 1000.f)));
	BOOST_CHECK(fr4.containsPoint(G3D::Vector3(0.f, -999.f, 1000.f)));
}

BOOST_AUTO_TEST_CASE(TestFrustumTransformed)
{
	const float kNinetyDegInRad = 1.57079633f;
	
	RBX::Frustum fr1(G3D::Vector3(100, 200, 300), G3D::Vector3(+1, 0, 0), G3D::Vector3(0, -1, 0), 1.f, 1000.f, kNinetyDegInRad, kNinetyDegInRad);
	RBX::Frustum fr2(G3D::Vector3(100, 200, 300), G3D::Vector3(-1, 0, 0), G3D::Vector3(0, -1, 0), 1.f, 1000.f, kNinetyDegInRad, kNinetyDegInRad);
	
	BOOST_CHECK( fr1.containsPoint(G3D::Vector3(150, 200, 300)));
	BOOST_CHECK(!fr1.containsPoint(G3D::Vector3( 50, 200, 300)));
	
	BOOST_CHECK(!fr2.containsPoint(G3D::Vector3(150, 200, 300)));
	BOOST_CHECK( fr2.containsPoint(G3D::Vector3( 50, 200, 300)));
}

BOOST_AUTO_TEST_SUITE_END()
