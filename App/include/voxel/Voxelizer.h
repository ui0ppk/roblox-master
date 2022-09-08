#pragma once
// suffix header file for Grid.h

#include "Util/SpatialRegion.h"
#include "Util/Extents.h"

#include <vector>

namespace RBX {
	class MegaClusterInstance;
	class ContactManager;
	class PartInstance;

	namespace Voxel { class Grid; }
	namespace Voxel2 { class Grid; }

	const int kVoxelChunkSizeXZ = 32;
	const int kVoxelChunkSizeY = 16;

	const Vector3int32 kVoxelChunkSize = Vector3int32(kVoxelChunkSizeXZ, kVoxelChunkSizeY, kVoxelChunkSizeXZ);

namespace Voxel {

struct OccupancyChunk
{
	unsigned int dirty;
	unsigned int age;
	Vector3int32 index;
	unsigned char occupancy[kVoxelChunkSizeY][kVoxelChunkSizeXZ][kVoxelChunkSizeXZ];
	Extents getChunkExtents() const;
};

struct DataModelPartCache;

class Voxelizer
{
public:
	Voxelizer(bool collisionTransparency = false);

	void occupancyUpdateChunk(OccupancyChunk& chunk, MegaClusterInstance* terrain, ContactManager* contactManager);
        
	void occupancyUpdateChunkPrepare(OccupancyChunk& chunk, MegaClusterInstance* terrain, ContactManager* contactManager, std::vector<DataModelPartCache>& partCache);
	void occupancyUpdateChunkPerform(const std::vector<DataModelPartCache>& partCache);

	void setNonFixedPartsEnabled(bool value) { nonFixedPartsEnabled = value; }
	bool getNonFixedPartsEnabled() const { return nonFixedPartsEnabled; }

private:
	void occupancyFillTerrainMega(OccupancyChunk& chunk, Voxel::Grid& terrain, const Vector3int32& chunkOffset, const Extents& chunkExtents);
	void occupancyFillTerrainMegaSIMD(OccupancyChunk& chunk, Voxel::Grid& terrain, const Vector3int32& chunkOffset, const Extents& chunkExtents);

	void occupancyFillTerrainSmooth(OccupancyChunk& chunk, Voxel2::Grid& terrain, const Extents& chunkExtents);
	void occupancyFillTerrainSmoothSIMD(OccupancyChunk& chunk, Voxel2::Grid& terrain, const Extents& chunkExtents);

	void occupancyFillBlock(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillBlockDF(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency);
	void occupancyFillBlockDFAA(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency);
	void occupancyFillBlockDFSIMD(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency);

	void occupancyFillSphere(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillEllipsoid(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillCylinderX(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillCylinderY(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillWedge(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillCornerWedge(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
	void occupancyFillTorso(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);
    void occupancyFillMesh(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);

    void addMeshToPartCache(std::vector<DataModelPartCache>& partCache, PartInstance* part, OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents_, const CoordinateFrame& cframe, float transparency);

	template <typename DistanceFunction> void occupancyFillDF(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, DistanceFunction& df);

	float getEffectiveTransparency(PartInstance* part);

	bool useSIMD;
	bool nonFixedPartsEnabled;
	bool collisionTransparency;
};

struct DataModelPartCache
{
    typedef void (Voxelizer::*pfn)(OccupancyChunk& chunk, const Extents& chunkExtents, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius);

    pfn              fillFunc;
    OccupancyChunk*  chunk;
    Vector3          extents;
    CoordinateFrame  cframe;
    float            transparency;
    float            meshRadius;

    DataModelPartCache(pfn fillFunc, OccupancyChunk& chunk, const Vector3& extents, const CoordinateFrame& cframe, float transparency, float meshRadius = 0)
    : fillFunc(fillFunc)
    , chunk(&chunk)
    , extents(extents)
    , cframe(cframe)
    , transparency(transparency)
    , meshRadius(meshRadius)
    {
    }
};


} }

