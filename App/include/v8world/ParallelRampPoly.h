#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/ParallelRampMesh.h"
#include "V8World/BlockMesh.h"

namespace RBX {

	class ParallelRampPoly : public Poly {
	public:
		typedef GeometryPool<Vector3, POLY::ParallelRampMesh, Vector3Comparer> ParallelRampMeshPool;

		/*override*/ Matrix3 getMoment(float mass) const;
		/*override*/ Vector3 getCofmOffset() const;
		/*override*/ bool isGeometryOrthogonal( void ) const { return false; }
		/*override*/ bool setUpBulletCollisionData(void) { return false; }

	private:
		ParallelRampMeshPool::Token aParallelRampMesh;

	protected:
		// Geometry Overrides
		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_PARALLELRAMP;}
		
		// Poly Overrides
		/*override*/ void buildMesh();
	};

} // namespace
