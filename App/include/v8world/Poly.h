#pragma once

#include "V8World/Geometry.h"
#include "V8World/Mesh.h"

namespace RBX {

	namespace POLY {
		class Mesh;
	}


	class Poly : public Geometry {
	private:
		typedef Geometry Super;
		float centerToCornerDistance;				

	protected:
		const POLY::Mesh* mesh;

		/*override*/ void setSize(const G3D::Vector3& _size);

		float getCenterToCornerDistance() const {return centerToCornerDistance;}

		Vector3 getCenterToCornerWorst() const {return Vector3(centerToCornerDistance, centerToCornerDistance, centerToCornerDistance);}	

		/*implement*/ virtual void buildMesh() = 0;

	public:
		Poly() : mesh(NULL)	{}
		~Poly() {}

		// Geometry Overrides
		/*override*/ virtual CollideType getCollideType() const	{return COLLIDE_POLY;}

		/*override*/ virtual bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal);

		/*override*/ float getRadius() const {return centerToCornerDistance;}

		/*override*/ Vector3 getCenterToCorner(const Matrix3& rotation) const {return getCenterToCornerWorst();}

		/*override*/ Vector3 getCofmOffset() const;

		/*override*/ Matrix3 getMoment(float mass) const;

		/*override*/ bool collidesWithGroundPlane(const CoordinateFrame& c, float yHeight) const;

		const POLY::Mesh* getMesh() const {return mesh;}

		// Dragger/joiner support
		size_t closestSurfaceToPoint( const Vector3& pointInBody ) const;
		Plane getPlaneFromSurface( const size_t surfaceId ) const;
		virtual CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;
		Vector3 getSurfaceNormalInBody( const size_t surfaceId ) const;
		size_t getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const;
		int getNumSurfaces( void ) const { return mesh->numFaces(); }
		Vector3 getSurfaceVertInBody( const size_t surfaceId, const int vertId ) const;
		int getNumVertsInSurface( const size_t surfaceId ) const;
		bool vertOverlapsFace( const Vector3& pointInBody, const size_t surfaceId ) const;

        /*override*/virtual bool findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const;
        /*override*/virtual bool FacesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const;
        /*override*/virtual bool FaceVerticesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const;
        /*override*/virtual bool FaceEdgesOverlapped( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol ) const;


		/*override*/ std::vector<Vector3> polygonIntersectionWithFace( const std::vector<Vector3>& polygonInBody, const size_t surfaceId ) const;

	};

} // namespace
