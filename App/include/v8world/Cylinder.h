#pragma once

#include "V8World/Geometry.h"
#include "V8World/GeometryPool.h"
#include "V8World/BulletGeometryPoolObjects.h"

namespace RBX {

    class Cylinder: public Geometry
    {
    public:
        typedef GeometryPool<Vector3, BulletCylinderShapeWrapper, Vector3Comparer> BulletCylinderShapePool;
        
    private:
        typedef Geometry Super;

        float realLength, realWidth;

        BulletCylinderShapePool::Token bulletCylinderShape;
        
		void updateBulletCollisionData();

    public:
        Cylinder();
        ~Cylinder();
        
		GeometryType getGeometryType() const override {return GEOMETRY_CYLINDER;}
		CollideType getCollideType() const override {return COLLIDE_BULLET;}

        bool setUpBulletCollisionData() override;

        bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal) override;
        void setSize(const Vector3& _size) override;

        Matrix3 getMoment(float mass) const override;
        float getVolume() const override;

        float getRadius() const override;

        Vector3 getCenterToCorner(const Matrix3& rotation) const override;

        size_t closestSurfaceToPoint(const Vector3& pointInBody) const override;
        Plane getPlaneFromSurface(const size_t surfaceId) const override;
        CoordinateFrame getSurfaceCoordInBody(const size_t surfaceId) const override;
        Vector3 getSurfaceNormalInBody(const size_t surfaceId) const override;
        size_t getMostAlignedSurface(const Vector3& vecInWorld, const G3D::Matrix3& objectR) const override;
        int getNumSurfaces() const override;
        Vector3 getSurfaceVertInBody(const size_t surfaceId, const int vertId) const override;
        int getNumVertsInSurface(const size_t surfaceId) const override;
        bool vertOverlapsFace(const Vector3& pointInBody, const size_t surfaceId) const override;

        bool findTouchingSurfacesConvex(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId) const override;
        bool FacesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const override;
        bool FaceVerticesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const override;
        bool FaceEdgesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const override;
    };

} // namespace
