#include "RbxG3D/Frustum.h"
#include "rbx/Debug.h"
#include "util/Math.h"
#include "util/Extents.h"

using G3D::Plane;
using G3D::Vector3;

using namespace RBX;

// creates a frustum in a local coordinate space with the apex at the origin
// translates the frustum into the space defined by apex, dir and up
// performing the floating point operations in a local space helps reduce numerical errors when all of the points are far from the origin
Frustum::Frustum(const Vector3& apex, const Vector3& dir, const Vector3& up, const float nearDist, const float farDist, const float fovx, const float fovy)
{
	Matrix3 rotation;

	// up and dir should already be normalized and perpendicular to each other
	RBXASSERT(dir.isUnit());
	RBXASSERT(up.isUnit());

	// due to rounding errors the cross product may not be a unit vector even though it should be.
	const Vector3 right = up.cross(dir).unit();

	rotation.setColumn(0, right);
	rotation.setColumn(1, up);
	rotation.setColumn(2, -dir);

	// Near plane (wind backwards so normal faces into frustum)
	// Recall that nearPlane, farPlane are positive numbers, so
	// we need to negate them to produce actual z values.
	faceArray.append(Plane(Vector3(0.0f, 0.0f, -1.0f), Vector3(0,0,-nearDist)));

	// Right plane
	faceArray.append(Plane(Vector3(-cosf(fovx/2.0f), 0.0f, -sinf(fovx/2.0f)), Vector3::zero()));

	// Left plane
	faceArray.append(Plane(Vector3(-faceArray.last().normal().x, 0.0f, faceArray.last().normal().z), Vector3::zero()));

	// Bottom plane
	faceArray.append(Plane(Vector3(0.0f, cosf(fovy/2.0f), -sinf(fovy/2.0f)), Vector3::zero()));

	// Top plane
	faceArray.append(Plane(Vector3(0.0f, -faceArray.last().normal().y, faceArray.last().normal().z), Vector3::zero()));

	// Far plane
	if (farDist < inf()) 
	{
		faceArray.append(Plane(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, -farDist)));
	}

	// Transform planes to world space
	for (int face = 0; face < faceArray.size(); ++face) 
	{
		// Since there is no scale factor, we don't have to 
		// worry about the inverse transpose of the normal.
		Vector3 normal;
		float d;

		faceArray[face].getEquation(normal, d);

		Vector3 newNormal = rotation * normal;

		if (G3D::isFinite(d)) {
			d = (newNormal * -d + apex).dot(newNormal);
			faceArray[face] = Plane(newNormal, newNormal * d);
		} else {
			// When d is infinite, we can't multiply 0's by it without
			// generating NaNs.
			faceArray[face] = Plane::fromEquation(newNormal.x, newNormal.y, newNormal.z, d);
		}
	}
}

bool Frustum::containsAABB(const Extents& aabb) const
{
	for (int ii = 0; ii < 8; ++ii)
	{
		Vector3 corner = aabb.getCorner(ii);
		if (!containsPoint(corner))
		{
			return false;
		}
	}
	return true;
}

bool Frustum::intersectsAABB(const RBX::Extents& aabb, const G3D::CoordinateFrame& extentsFrame) const
{
    RBX::Extents worldBox = aabb.toWorldSpace(extentsFrame);
    Vector3 extent = worldBox.size() * 0.5f;
    Vector3 center = worldBox.center();

    for (int ii = 0; ii < 6; ++ii)
    {
        const Plane& plane = faceArray[ii];
        
        float d = plane.normal().dot(center) - plane.distance();
        float r = Vector3(fabsf(plane.normal().x), fabsf(plane.normal().y), fabsf(plane.normal().z)).dot(extent);

        // box is in negative half-space => outside the frustum
        if (d + r < 0)
            return false;
    }
    return true;
}

bool Frustum::containsAABB(const Extents& aabb, const G3D::CoordinateFrame& extentsFrame) const
{
	for (int ii = 0; ii < 8; ++ii)
	{
		Vector3 corner = aabb.getCorner(ii);
		Vector3 world = extentsFrame.pointToWorldSpace(corner);
		if (!containsPoint(world))
		{
			return false;
		}
	}
	return true;
}

bool Frustum::containsPoint(const Vector3& point) const
{
	for (int ii = 0; ii < faceArray.size(); ++ii)			
	{
		const Plane& plane = faceArray[ii];
		if (!plane.halfSpaceContains(point))
		{
			return false;
		}
	}
	return true;
}


bool Frustum::intersectsSphere(const Vector3& center, float radius) const
{
	for (int ii = 0; ii < faceArray.size(); ++ii)			
	{
		const Plane& p = faceArray[ii];
		Plane offsetplane(p.normal(), p.distance() - radius);
		if (!offsetplane.halfSpaceContains(center))
		{
			return false;
		}
	}
	return true;
}
