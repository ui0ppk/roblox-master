#pragma once

#include "V8World/Geometry.h"
#include "V8World/Primitive.h"
#include "V8World/TerrainPartition.h"
#include "Util/PartMaterial.h"

class btConvexHullShape;
struct btDbvt;

namespace RBX {

	namespace Voxel2 { class Grid; }

	class SmoothClusterGeometry: public Geometry
	{
    public:
        struct ChunkMesh;

		SmoothClusterGeometry(Primitive* p);
		~SmoothClusterGeometry();

        // Geometry overrides
		GeometryType getGeometryType() const override;
		CollideType getCollideType() const override;
		float getRadius() const override;
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
		bool hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal) override;
		bool collidesWithGroundPlane(const CoordinateFrame& c, float yHeight) const override;
		bool setUpBulletCollisionData() override;

		bool hitTestTerrain(const RbxRay& rayInMe, Vector3& localHitPoint, int& surfId, CoordinateFrame& surfCf) override;

        // Terrain specific API
		bool castRay(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal, unsigned char& surfaceMaterial, float maxDistance, bool ignoreWater);
		bool findCellsInBoundingBox(const Vector3& min, const Vector3& max);

        void updateChunk(const Vector3int32& id);
        void updateAllChunks();

		void garbageCollectIncremental();

        shared_ptr<btCollisionShape> getBulletChunkShape(const Vector3int32& id);
		TerrainPartitionSmooth* getTerrainPartition() { return partition.get(); }

		static PartMaterial getTriangleMaterial(btCollisionShape* collisionShape, unsigned int triangleIndex, const Vector3& localHitPoint);

	private:
        Primitive* myPrim;

        Voxel2::Grid* grid;

		scoped_ptr<TerrainPartitionSmooth> partition;

		typedef boost::unordered_map<Vector3int32, ChunkMesh*> ChunkMap;
        ChunkMap bulletChunks;

		Vector3int32 gcChunkKeyNext;
		size_t gcChunkCountLast;
		size_t gcUnusedMemory;
		size_t gcUnusedMemoryNext;

		btDbvt* bulletChunksTree;
	};

} // namespace
