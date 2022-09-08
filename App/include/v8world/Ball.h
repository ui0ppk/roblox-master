#pragma once

#include "V8World/Geometry.h"
#include "V8World/GeometryPool.h"
#include "V8World/BulletGeometryPoolObjects.h"

namespace RBX {

	class Ball : public Geometry {
	public:
		typedef GeometryPool<float, BulletSphereShapeWrapper, FloatComparer> BulletSphereShapePool;

	private:
		typedef Geometry Super;
		float		realRadius;			// in real world units, == size.x/2
		BulletSphereShapePool::Token bulletSphereShape;


		Matrix3 getMomentSolid(float mass) const;

		/*override*/ void setSize(const G3D::Vector3& _size);

		void updateBulletCollisionData();

	public:
		Ball() : realRadius(0.0) {}			
		~Ball()	{}

		// Primitive Overrides
		/*override*/ virtual bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal);

		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_BALL;}
		/*override*/ virtual CollideType getCollideType() const	{return COLLIDE_BALL;}

		// Real Radius
		/*override*/ virtual float getRadius() const {return realRadius;}

		// Real Corner
		/*override*/ virtual Vector3 getCenterToCorner(const Matrix3& rotation) const {
			return Vector3(realRadius, realRadius, realRadius);
		}

		// Moment
		/*override*/ virtual Matrix3 getMoment(float mass) const {
			return getMomentSolid(mass);
		}
		
		// Volume
		/*override*/ float getVolume() const;

		// Dragger support
		size_t closestSurfaceToPoint( const Vector3& pointInBody ) const;
		Plane getPlaneFromSurface( const size_t surfaceId ) const;
		CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;
		Vector3 getSurfaceNormalInBody( const size_t surfaceId ) const;
		size_t getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const;
		int getNumSurfaces( void ) const { return 6; }
		Vector3 getSurfaceVertInBody( const size_t surfaceId, const int vertId ) const;
		int getNumVertsInSurface( const size_t surfaceId ) const;
		bool vertOverlapsFace( const Vector3& pointInBody, const size_t surfaceId ) const;

        bool findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const {return false;}
        bool FacesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const {RBXASSERT(0); return false;}
        bool FaceVerticesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const{RBXASSERT(0); return false;}
        bool FaceEdgesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const {RBXASSERT(0); return false;}
		
		/*override*/ bool setUpBulletCollisionData(void);
	};

} // namespace
