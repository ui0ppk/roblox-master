#pragma once

#include "V8World/Poly.h"
#include "V8World/GeometryPool.h"
#include "V8World/MegaClusterMesh.h"
#include "V8World/Primitive.h"
#include "V8World/TerrainPartition.h"

class btConvexHullShape;

namespace RBX {

	class MegaClusterInstance;
	namespace Voxel {
		class Grid;
	}

    const float MC_SEARCH_RAY_MAX = 2048.0f; // was 500.0f, but normal mouse has range coded to be 2048.0f
    const float MC_RAY_ZERO_SLOPE_TOLERANCE = .0005f;
    const float MC_HUGE_VAL = 9999999;

	class MegaClusterPoly : public Poly
	{
	public:
		MegaClusterPoly(Primitive* p);
		~MegaClusterPoly();

        typedef GeometryPool<Vector3, POLY::MegaClusterMesh, Vector3Comparer> MegaClusterMeshPool;
        typedef Poly Super;

        /*override*/ virtual const G3D::Vector3& getSize() const	{return Super::getSize();}
		/*override*/ bool setUpBulletCollisionData(void) { return false; }

	private:
		MegaClusterMeshPool::Token aMegaClusterMesh;
        Primitive *myPrim;

		scoped_ptr<TerrainPartitionMega> myTerrainPartition;

		/*override*/ bool isGeometryOrthogonal( void ) const { return false; }

		std::vector<btConvexHullShape*> bulletCellShapes;

		void createBulletCellShapes(void);
		void createBulletCubeCell(void);
		void createBulletVerticalWedgeCell(void);
		void createBulletHorizontalWedgeCell(void);
		void createBulletCornerWedgeCell(void);
		void createBulletInverseCornerWedgeCell(void);

        bool hitLocationOnBlockCell(const RbxRay& rayInMe, const Vector3int16& testCell, Vector3& localHitPoint, Vector3& surfaceNormal, int& surfId, CoordinateFrame& surfaceCf) const;
        bool hitLocationOnVerticalWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& surfaceCf) const;
        bool hitLocationOnHorizontalWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& surfaceCf) const;
        bool hitLocationOnCornerWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& surfaceCf) const;
        bool hitLocationOnInverseCornerWedgeCell(const RbxRay& rayInMe, const Vector3int16& testCell, const int& orientation, Vector3& localHitPoint, Vector3& surfaceNormal, CoordinateFrame& surfaceCf) const;

        bool hitTestMC(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal, int& surfId, CoordinateFrame& surfaceCf, float searchRayMax = MC_SEARCH_RAY_MAX, bool treatCellsAsBlocks = false, bool ignoreWater = false);

	protected:
		// Geometry Overrides
		/*override*/ virtual GeometryType getGeometryType() const	{return GEOMETRY_MEGACLUSTER;}
        /*override*/ Matrix3 getMoment(float mass) const { return Matrix3::identity(); }
        /*override*/ Vector3 getCofmOffset() const { return Vector3::zero(); }
		/*override*/ CoordinateFrame getSurfaceCoordInBody( const size_t surfaceId ) const;
		/*override*/ size_t getFaceFromLegacyNormalId( const NormalId nId ) const;
	
		// Poly Overrides
		/*override*/ void buildMesh();
    public:
        /*override*/ bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal, float searchRayMax = MC_SEARCH_RAY_MAX, bool treatCellsAsBlocks = false, bool ignoreWater = false);
        /*override*/ bool findTouchingSurfacesConvex( const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId ) const;

		virtual bool hitTestTerrain(const RbxRay& rayInMe, Vector3& localHitPoint, int& surfId, CoordinateFrame& surfCf);

        void findCellsTouchingGeometry( const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, std::vector<Vector3int16>* found ) const;
        void findCellsTouchingGeometryWithBuffer( const float& buffer, const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, std::vector<Vector3int16>* found ) const;
        bool findPlanarTouchesWithGeom( const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, std::vector<Vector3int16>* found ) const;

        std::vector<Vector3> findCellIntersectionWithGeom( const Vector3int16& cell, const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t & otherFaceId ) const;
        bool hasPlanarTouchWithGeom( const Vector3int16& cellIndex, const CoordinateFrame& myCf, const Geometry& otherGeom, const CoordinateFrame& otherCf ) const;
		bool cellsInBoundingBox(const Vector3& min, const Vector3& max);

		btConvexHullShape* getBulletCellShape(Voxel::CellBlock shape);
	};

} // namespace
