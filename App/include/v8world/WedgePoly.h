#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/WedgeMesh.h"
#include "V8World/BlockMesh.h"
#include "V8World/BulletGeometryPoolObjects.h"

namespace RBX {

	class WedgePoly : public Poly {
	public:
		typedef GeometryPool<Vector3, POLY::WedgeMesh, Vector3Comparer> WedgeMeshPool;
		typedef GeometryPool<Vector3, BulletWedgeShapeWrapper, Vector3Comparer> BulletWedgeShapePool;

		/*override*/ Matrix3 getMoment(float mass) const;
		/*override*/ Vector3 getCofmOffset() const;
		/*override*/ CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;
		/*override*/ size_t getFaceFromLegacyNormalId( const NormalId nId ) const;
		/*override*/ bool isGeometryOrthogonal( void ) const { return false; }
		/*override*/ bool setUpBulletCollisionData(void);
		/*override*/ void setSize(const G3D::Vector3& _size);

	private:
		typedef Poly Super;

		WedgeMeshPool::Token wedgeMesh;
		BulletWedgeShapePool::Token bulletWedgeShape;

		/*override*/ virtual Vector3 getCenterToCorner(const Matrix3& rotation) const;

        void updateBulletCollisionData();

	protected:
		// Geometry Overrides
		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_WEDGE;}
		
		// Poly Overrides
		/*override*/ void buildMesh();
	};

} // namespace
