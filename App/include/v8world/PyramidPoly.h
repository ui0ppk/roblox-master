#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/PyramidMesh.h"
#include "V8World/BlockMesh.h"

namespace RBX {

	class PyramidPoly : public Poly {
	private:
		typedef GeometryPool<Vector3_2Ints, POLY::PyramidMesh, Vector3_2IntsComparer> PyramidMeshPool;
		PyramidMeshPool::Token pyramidMesh;

		int numSides;
		int numSlices;

		void setNumSides( int num ); 
		void setNumSlices( int num ); 
		/*override*/ bool isGeometryOrthogonal( void ) const { return false; }

	protected:
		// Geometry Overrides
		/*override*/ GeometryType getGeometryType() const	{return GEOMETRY_PYRAMID;}
		/*override*/ void setGeometryParameter(const std::string& parameter, int value);
		/*override*/ int getGeometryParameter(const std::string& parameter) const;

		/*override*/ Matrix3 getMoment(float mass) const;
		/*override*/ Vector3 getCofmOffset() const;
		/*override*/ CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;
		size_t getFaceFromLegacyNormalId( const NormalId nId ) const;

		// Poly Overrides
		/*override*/ void buildMesh();

	public:
		PyramidPoly() : numSides(0), numSlices(0)
		{}

		/*override*/ bool setUpBulletCollisionData(void) { return false; }
	};

} // namespace
