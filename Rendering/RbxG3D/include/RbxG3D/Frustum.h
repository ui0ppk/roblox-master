#ifndef RBXG3D_FRUSTUM_H
#define RBXG3D_FRUSTUM_H

#include "G3D/Plane.h"
#include "G3D/Vector3.h"

namespace G3D
{
	class CoordinateFrame;
}

namespace RBX {

class Extents;

class Frustum 
{
public:
	Frustum() {};

	// fovx, fovy are in radians
	Frustum(const G3D::Vector3& apex, const G3D::Vector3& dir, const G3D::Vector3& up, float nearDist, float farDist, float fovx, float fovy);

	enum FrustumPlane
	{
		kPlaneNear = 0,
		kPlaneRight,
		kPlaneLeft,
		kPlaneBottom,
		kPlaneTop,
		kPlaneFar,
		kPlaneInvalid,
	};

	/** The faces in the frustum.  When the
		far plane is at infinity, there are 5 faces,
		otherwise there are 6.  The faces are in the order
		N,R,L,B,T,[F].
		*/
	G3D::Array<G3D::Plane>         faceArray;

	bool containsPoint(const G3D::Vector3& point) const;
	bool intersectsSphere(const G3D::Vector3& center, float radius) const;

	bool containsAABB(const RBX::Extents& aabb) const;
    bool intersectsAABB(const RBX::Extents& aabb, const G3D::CoordinateFrame& extentsFrame) const;
	bool containsAABB(const RBX::Extents& aabb, const G3D::CoordinateFrame& extentsFrame) const;
};

}	// namespace RBX

#endif
