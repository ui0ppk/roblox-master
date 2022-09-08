#pragma once

#include "Util/G3DCore.h"
#include "Util/Units.h"
#include "Util/Face.h"

#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"

namespace RBX {

	class Geometry 
	{
	private:
		Vector3	size;	

	protected:
		boost::scoped_ptr<btCollisionObject> bulletCollisionObject;
	
	public:
		btCollisionObject* getBulletCollisionObject(void) { return bulletCollisionObject.get(); }
		virtual bool setUpBulletCollisionData(void) = 0;
		typedef enum {	GEOMETRY_UNDEFINED=0,
                        GEOMETRY_BALL, 
						GEOMETRY_BLOCK,
						GEOMETRY_CYLINDER,
						GEOMETRY_WEDGE, 
						GEOMETRY_PRISM, 
						GEOMETRY_PYRAMID, 
						GEOMETRY_PARALLELRAMP, 
						GEOMETRY_RIGHTANGLERAMP,
						GEOMETRY_CORNERWEDGE,
                        GEOMETRY_MEGACLUSTER,
                        GEOMETRY_SMOOTHCLUSTER,
						GEOMETRY_TRI_MESH	} GeometryType;

		typedef enum {	COLLIDE_BALL=1, 
						COLLIDE_BLOCK, 
						COLLIDE_POLY,
						COLLIDE_BULLET } CollideType;

		Geometry() : bulletCollisionObject(NULL)
		{}

		virtual ~Geometry() 
		{}
		
		virtual GeometryType getGeometryType() const = 0;

		virtual CollideType getCollideType() const = 0;

		///////////////////////////////////////////
		// Size and Extents
		//

		// Grid Size
		virtual void setSize(const G3D::Vector3& _size)	{
			size = _size;
		}
		const G3D::Vector3& getSize() const	{return size;}

		// Parameters
		virtual void setGeometryParameter(const std::string& parameter, int value) {
			RBXASSERT(0);			// stock geometry does not handle parameters.
		}
		virtual int getGeometryParameter(const std::string& parameter) const {
			RBXASSERT(0);			// stock geometry does not handle parameters.
			return 0;
		}

		// Radius
		virtual float getRadius() const	= 0;

		// Dragger support
		virtual size_t closestSurfaceToPoint( const Vector3& pointInBody ) const = 0;
		virtual Plane getPlaneFromSurface( const size_t surfaceId ) const = 0;
		virtual CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const = 0;
		virtual Vector3 getSurfaceNormalInBody( const size_t surfaceId ) const = 0;
		virtual size_t getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const = 0;
		virtual int getNumSurfaces( void ) const = 0;
		virtual Vector3 getSurfaceVertInBody( const size_t surfaceId, const int vertId ) const = 0;
		virtual int getNumVertsInSurface( const size_t surfaceId ) const = 0;
		virtual bool vertOverlapsFace( const Vector3& pointInBody, const size_t surfaceId ) const = 0;
		virtual size_t getFaceFromLegacyNormalId( const NormalId nId ) const { return nId; }
		virtual bool isGeometryOrthogonal( void ) const { return true; }

        // Relative proximity
        /*override*/virtual bool findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const = 0;
        /*override*/virtual bool FacesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const = 0;
        /*override*/virtual bool FaceVerticesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const = 0;
        /*override*/virtual bool FaceEdgesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const = 0;

		// Corner (better than radius for big blocks)
		virtual Vector3 getCenterToCorner(const Matrix3& rotation) const {return Vector3::zero();}

		// CofmOffset
		virtual Vector3 getCofmOffset() const {return Vector3::zero();}

		// Moment
		virtual Matrix3 getMoment(float mass) const {return Matrix3::zero();}

		// Volume
		virtual float getVolume() const {return size.x * size.y * size.z;}

		// Hit Test
		virtual bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal) {return false;}

		virtual bool collidesWithGroundPlane(const CoordinateFrame& c, float yHeight) const {
			return ((c.translation.y - getRadius()) < yHeight);
		}

		virtual std::vector<Vector3> polygonIntersectionWithFace( const std::vector<Vector3>& polygonInBody, const size_t surfaceId ) const {
			std::vector<Vector3> empty;
			return empty;
		}

        // Cluster RTTI helper
        bool isTerrain() const
        {
            GeometryType type = getGeometryType();

            return type == GEOMETRY_MEGACLUSTER || type == GEOMETRY_SMOOTHCLUSTER;
        }

		// Cluster dragger helper
		virtual bool hitTestTerrain(const RbxRay& rayInMe, Vector3& localHitPoint, int& surfId, CoordinateFrame& surfCf) { return false; }
	};

} // namespace
