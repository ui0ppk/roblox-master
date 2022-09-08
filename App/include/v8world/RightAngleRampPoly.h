#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/RightAngleRampMesh.h"
#include "V8World/BlockMesh.h"

namespace RBX {

	class RightAngleRampPoly : public Poly {
	public:
		typedef GeometryPool<Vector3, POLY::RightAngleRampMesh, Vector3Comparer> RightAngleRampMeshPool;

		/*override*/ Matrix3 getMoment(float mass) const;
		/*override*/ Vector3 getCofmOffset() const;
		/*override*/ bool isGeometryOrthogonal( void ) const { return false; }
		/*override*/ bool setUpBulletCollisionData(void) { return false; }

	private:
		RightAngleRampMeshPool::Token aRightAngleRampMesh;

	protected:
		// Geometry Overrides
		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_RIGHTANGLERAMP;}
		
		// Poly Overrides
		/*override*/ void buildMesh();
		/*override*/ size_t getFaceFromLegacyNormalId( const NormalId nId ) const;

	};

} // namespace
