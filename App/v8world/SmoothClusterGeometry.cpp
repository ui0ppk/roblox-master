#include "stdafx.h"

#include "v8world/SmoothClusterGeometry.h"

#include "v8world/MegaClusterPoly.h"
#include "v8datamodel/MegaCluster.h"

#include "v8world/KDTree.h"

#include "voxel2/MaterialTable.h"
#include "voxel2/Grid.h"
#include "voxel2/Mesher.h"
#include "voxel2/Conversion.h"

#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/BroadPhaseCollision/btDbvt.h"

#include "rbx/Profiler.h"

FASTINTVARIABLE(SmoothTerrainPhysicsCacheSize, 16*1024*1024)

namespace RBX {

struct ChunkMeshShapeData
{
	boost::scoped_array<Vector3> vertexPositions;
	boost::scoped_array<unsigned char> vertexMaterials;
	size_t vertexCount;

	boost::scoped_array<unsigned int> solidIndices;
	size_t solidTriangleCount;

	boost::scoped_array<unsigned int> waterIndices;
	size_t waterTriangleCount;

	ChunkMeshShapeData(const Voxel2::Mesher::BasicMesh& geometry)
	{
        using namespace Voxel2::Mesher;

        size_t triangleCount = geometry.indices.size() / 3;
        size_t waterTriangles = 0;

        std::vector<char> iswater(triangleCount);

        for (size_t i = 0; i < triangleCount; ++i)
        {
            const Vertex& v0 = geometry.vertices[geometry.indices[3 * i + 0]];
            const Vertex& v1 = geometry.vertices[geometry.indices[3 * i + 1]];
            const Vertex& v2 = geometry.vertices[geometry.indices[3 * i + 2]];

            iswater[i] = BasicMesh::isWater(v0, v1, v2);
            waterTriangles += iswater[i];
        }

		vertexCount = geometry.vertices.size();
		vertexPositions.reset(new Vector3[vertexCount]);
		vertexMaterials.reset(new unsigned char[vertexCount]);

		for (size_t i = 0; i < vertexCount; ++i)
        {
            const Vertex& v = geometry.vertices[i];

			vertexPositions[i] = Voxel::cellSpaceToWorldSpace(v.position);
            vertexMaterials[i] = v.material;
        }

        solidTriangleCount = triangleCount - waterTriangles;
		solidIndices.reset(new unsigned int[solidTriangleCount * 3]);

        waterTriangleCount = waterTriangles;
		waterIndices.reset(new unsigned int[waterTriangleCount * 3]);

        size_t solidOffset = 0;
        size_t waterOffset = 0;

		for (size_t i = 0; i < triangleCount; ++i)
        {
            unsigned int i0 = geometry.indices[3 * i + 0];
            unsigned int i1 = geometry.indices[3 * i + 1];
            unsigned int i2 = geometry.indices[3 * i + 2];

            if (iswater[i])
            {
                waterIndices[waterOffset + 0] = i0;
                waterIndices[waterOffset + 1] = i1;
                waterIndices[waterOffset + 2] = i2;

                waterOffset += 3;
            }
            else
            {
                solidIndices[solidOffset + 0] = i0;
                solidIndices[solidOffset + 1] = i1;
                solidIndices[solidOffset + 2] = i2;

                solidOffset += 3;
            }
        }
	}
};

class ChunkMeshShape: public btConcaveShape
{
public:
	ChunkMeshShape(const Voxel2::Mesher::BasicMesh& geometry)
    : data(geometry)
    {
        m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
        
        solidTree.build(data.vertexPositions.get(), data.vertexMaterials.get(), data.vertexCount, data.solidIndices.get(), data.solidTriangleCount);
        waterTree.build(data.vertexPositions.get(), data.vertexMaterials.get(), data.vertexCount, data.waterIndices.get(), data.waterTriangleCount);
        
        Vector3 aabbMin = solidTree.extentsMin.min(waterTree.extentsMin);
        Vector3 aabbMax = solidTree.extentsMax.max(waterTree.extentsMax);

        localAabbMin = btVector3(aabbMin.x, aabbMin.y, aabbMin.z);
        localAabbMax = btVector3(aabbMax.x, aabbMax.y, aabbMax.z);

        RBXPROFILER_COUNTER_ADD("memory/terrain/physics", getMemorySize());
    }
    
    ~ChunkMeshShape()
    {
        RBXPROFILER_COUNTER_SUB("memory/terrain/physics", getMemorySize());
    }

    void castRay(KDTree::RayResult& result, const Vector3& raySource, const Vector3& rayTarget, bool ignoreWater)
    {
        solidTree.queryRay(result, raySource, rayTarget);

        if (!ignoreWater)
            waterTree.queryRay(result, raySource, rayTarget);
    }

	void processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const override
    {
        solidTree.queryAABB(callback, Vector3(aabbMin.x(), aabbMin.y(), aabbMin.z()), Vector3(aabbMax.x(), aabbMax.y(), aabbMax.z()));
    }

	void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const override
    {
        RBXCRASH();
    }

	PartMaterial getTriangleMaterial(unsigned int triangleIndex, const Vector3& localHitPoint)
	{
		return Voxel2::Conversion::getMaterialFromVoxelMaterial(solidTree.getMaterial(triangleIndex, localHitPoint));
	}

	void getBoundingSphere(btVector3& center, btScalar& radius) const override
    {
        center = (localAabbMin + localAabbMax) * 0.5f;
        radius = (localAabbMax - localAabbMin).length() * 0.5f;
    }

    void setLocalScaling(const btVector3& scaling) override
    {
        RBXCRASH();
    }

	const btVector3& getLocalScaling() const override
    {
        static const btVector3 result(1, 1, 1);

        return result;
    }

	void calculateLocalInertia(btScalar mass, btVector3& inertia) const override
    {
        inertia = btVector3(0, 0, 0);
    }

	virtual const char*	getName() const override
    {
        return "SmoothClusterChunkMesh";
    }

	size_t getMemorySize() const
	{
        size_t result = 0;

        result += data.vertexCount * (sizeof(data.vertexPositions[0]) + sizeof(data.vertexMaterials[0]));
        result += data.solidTriangleCount * 3 * sizeof(data.solidIndices[0]);
        result += data.waterTriangleCount * 3 * sizeof(data.waterIndices[0]);
        result += solidTree.nodes.size() * sizeof(KDNode);
        result += waterTree.nodes.size() * sizeof(KDNode);

        return result;
	}

    const btVector3& getLocalAabbMin() const
    {
        return localAabbMin;
    }

    const btVector3& getLocalAabbMax() const
    {
        return localAabbMax;
    }

private:
	ChunkMeshShapeData data;

    KDTree solidTree;
    KDTree waterTree;

    btVector3 localAabbMin;
    btVector3 localAabbMax;
};

struct SmoothClusterGeometry::ChunkMesh
{
    enum State
	{
        State_Dummy,
        State_Ready,
	};

    Vector3int32 id;
    State state;

	btDbvt* tree;
	btDbvtNode* treeNode;

	shared_ptr<ChunkMeshShape> shape;

	ChunkMesh(const Vector3int32& id)
		: id(id)
        , state(State_Dummy)
		, tree(NULL)
        , treeNode(NULL)
	{
	}

	~ChunkMesh()
	{
		if (treeNode)
		{
			tree->remove(treeNode);
		}
	}

    Voxel2::Region getRegion() const
	{
        return Voxel2::Region::fromChunk(id, TerrainPartitionSmooth::kChunkSizeLog2).expand(1);
	}

	void generateShape(MegaClusterInstance* mci)
    {
		RBXPROFILER_SCOPE("Physics", "generateShape");

        RBXASSERT(state == State_Dummy);
		state = State_Ready;

        Voxel2::Grid* grid = mci->getSmoothGrid();

		Voxel2::Region region = getRegion();
        Voxel2::Box box = grid->read(region);

        if (box.isEmpty())
            return;

        using namespace Voxel2::Mesher;

        Options options = { mci->getMaterialTable(), /* generateWater= */ true };
        BasicMesh geometry = generateGeometry(box, region.begin(), 0, options);

        if (geometry.indices.empty())
            return;

		shape.reset(new ChunkMeshShape(geometry));
    }

    void updateTree()
	{
		RBXASSERT(state == State_Ready);

        if (shape)
		{
            btDbvtVolume volume = btDbvtVolume::FromMM(shape->getLocalAabbMin(), shape->getLocalAabbMax());

			tree->update(treeNode, volume);
		}
		else
		{
			tree->remove(treeNode);

            treeNode = NULL;
		}
	}
};

SmoothClusterGeometry::SmoothClusterGeometry(Primitive* p)
	: myPrim(p)
    , grid(NULL)
	, bulletChunksTree(NULL)
	, gcChunkCountLast(0)
	, gcUnusedMemory(0)
	, gcUnusedMemoryNext(0)
{
	grid = rbx_static_cast<MegaClusterInstance*>(p->getOwner())->getSmoothGrid();

	partition.reset(new TerrainPartitionSmooth(grid));

	bulletChunksTree = new (btAlignedAlloc(sizeof(btDbvt), 16)) btDbvt();
}

SmoothClusterGeometry::~SmoothClusterGeometry()
{
    for (ChunkMap::iterator it = bulletChunks.begin(); it != bulletChunks.end(); ++it)
        delete it->second;

    if (bulletChunksTree)
    {
        bulletChunksTree->~btDbvt();
        btAlignedFree(bulletChunksTree);
    }
}

Geometry::GeometryType SmoothClusterGeometry::getGeometryType() const
{
	return GEOMETRY_SMOOTHCLUSTER;
}

Geometry::CollideType SmoothClusterGeometry::getCollideType() const
{
	return COLLIDE_BULLET;
}

float SmoothClusterGeometry::getRadius() const
{
    return 0;
}

size_t SmoothClusterGeometry::closestSurfaceToPoint(const Vector3& pointInBody) const
{
    return 0;
}

Plane SmoothClusterGeometry::getPlaneFromSurface(const size_t surfaceId) const
{
    return Plane();
}

CoordinateFrame SmoothClusterGeometry::getSurfaceCoordInBody(const size_t surfaceId) const
{
	return CoordinateFrame();
}

Vector3 SmoothClusterGeometry::getSurfaceNormalInBody(const size_t surfaceId) const
{
    return Vector3();
}

size_t SmoothClusterGeometry::getMostAlignedSurface( const Vector3& vecInWorld, const G3D::Matrix3& objectR ) const
{
    return 0;
}

int SmoothClusterGeometry::getNumSurfaces() const
{
    return 0;
}

Vector3 SmoothClusterGeometry::getSurfaceVertInBody(const size_t surfaceId, const int vertId) const
{
    return Vector3();
}

int SmoothClusterGeometry::getNumVertsInSurface(const size_t surfaceId) const
{
    return 0;
}

bool SmoothClusterGeometry::vertOverlapsFace(const Vector3& pointInBody, const size_t surfaceId) const
{
    return false;
}

bool SmoothClusterGeometry::findTouchingSurfacesConvex(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId) const
{
    return false;
}

bool SmoothClusterGeometry::FacesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const
{
    return false;
}

bool SmoothClusterGeometry::FaceVerticesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const
{
	return false;
}

bool SmoothClusterGeometry::FaceEdgesOverlapped(const CoordinateFrame& myCf, size_t& myFaceId, const Geometry& otherGeom, const CoordinateFrame& otherCf, size_t& otherFaceId, float tol) const
{
	return false;
}

bool SmoothClusterGeometry::hitTest(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal)
{
    unsigned char material;
    
    return castRay(rayInMe, localHitPoint, surfaceNormal, material, MC_SEARCH_RAY_MAX, /* ignoreWater= */ true);
}

bool SmoothClusterGeometry::hitTestTerrain(const RbxRay& rayInMe, Vector3& localHitPoint, int& surfId, CoordinateFrame& surfCf)
{
	Vector3 normal;
    unsigned char material;

	if (castRay(rayInMe, localHitPoint, normal, material,  MC_SEARCH_RAY_MAX, /* ignoreWater= */ true))
	{
		surfId = Math::getClosestObjectNormalId(normal, Matrix3::identity());
		RBXASSERT(surfId != NORM_UNDEFINED);

		surfCf.rotation = Math::getWellFormedRotForZVector(normal);
        surfCf.translation = Math::toGrid(localHitPoint, Voxel::kCELL_SIZE);
		// do not change CFrame origin in Y
		surfCf.translation[surfId%3] = localHitPoint[surfId%3];
		return true;
	}
	else
	{
		surfId = -1;

		return false;
	}
}

bool SmoothClusterGeometry::collidesWithGroundPlane(const CoordinateFrame& c, float yHeight) const
{
    return false;
}

bool SmoothClusterGeometry::setUpBulletCollisionData()
{
    return false;
}

struct DbvhRayTest
{
	MegaClusterInstance* mci;
	std::vector<SmoothClusterGeometry::ChunkMesh*> updatedChunks;

    KDTree::RayResult result;

    Vector3 rayFrom;
    Vector3 rayTo;

    bool ignoreWater;

    struct DbvhNode
    {
        const btDbvtNode* node;
        float tmin;

        DbvhNode()
        {
        }

        DbvhNode(const btDbvtNode* node, float tmin)
        : node(node)
        , tmin(tmin)
        {
        }
    };

	DbvhRayTest(MegaClusterInstance* mci, const Vector3& rayFrom, const Vector3& rayTo, bool ignoreWater)
		: mci(mci)
        , rayFrom(rayFrom)
        , rayTo(rayTo)
        , ignoreWater(ignoreWater)
	{
	}

    void processLeaf(const btDbvtNode* leaf)
    {
		SmoothClusterGeometry::ChunkMesh* chunk = static_cast<SmoothClusterGeometry::ChunkMesh*>(leaf->data);

		if (chunk->state == SmoothClusterGeometry::ChunkMesh::State_Dummy)
		{
			chunk->generateShape(mci);

			updatedChunks.push_back(chunk);
		}

        if (chunk->shape)
        {
            chunk->shape->castRay(result, rayFrom, rayTo, ignoreWater);
        }
    }

    void processTree(const btDbvtNode* root)
    {
        btVector3 rayOrigin = btVector3(rayFrom.x, rayFrom.y, rayFrom.z);
        btVector3 rayDir = btVector3(rayTo.x, rayTo.y, rayTo.z) - rayOrigin;

        // what about division by zero? --> just set rayDirection[i] to INF/BT_LARGE_FLOAT
        btVector3 rayDirectionInverse;
        rayDirectionInverse[0] = rayDir[0] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[0];
        rayDirectionInverse[1] = rayDir[1] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[1];
        rayDirectionInverse[2] = rayDir[2] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[2];
        unsigned int raySigns[3] = { rayDirectionInverse[0] < 0.0, rayDirectionInverse[1] < 0.0, rayDirectionInverse[2] < 0.0};
        
        btAlignedObjectArray<DbvhNode> stack;
        int stackOffset = 0;

        stack.resize(btDbvt::DOUBLE_STACKSIZE);

        stack[stackOffset++] = DbvhNode(root, 0);

        btVector3 bounds[2];

        while (stackOffset > 0)
        {
            DbvhNode node = stack[--stackOffset];

            if (node.tmin >= result.fraction)
                continue;

            if (node.node->isinternal())
            {
                btScalar tmin[2] = {};
                bool hit[2] = {};

                bounds[0] = node.node->childs[0]->volume.Mins();
                bounds[1] = node.node->childs[0]->volume.Maxs();
                hit[0] = btRayAabb2(rayOrigin, rayDirectionInverse, raySigns, bounds, tmin[0], 0.f, 1.f);

                bounds[0] = node.node->childs[1]->volume.Mins();
                bounds[1] = node.node->childs[1]->volume.Maxs();
                hit[1] = btRayAabb2(rayOrigin, rayDirectionInverse, raySigns, bounds, tmin[1], 0.f, 1.f);

                if (stackOffset + 2 > stack.size())
                    stack.resize(stack.size() * 2);

                // start with the node that's closer to the ray origin
                int i0 = (tmin[0] < tmin[1]) ? 0 : 1;
                int i1 = 1 - i0;

                if (hit[i1])
                    stack[stackOffset++] = DbvhNode(node.node->childs[i1], tmin[i1]);

                if (hit[i0])
                    stack[stackOffset++] = DbvhNode(node.node->childs[i0], tmin[i0]);
            }
            else
            {
                processLeaf(node.node);
            }
        }
    }
};

bool SmoothClusterGeometry::castRay(const RbxRay& rayInMe, Vector3& localHitPoint, Vector3& surfaceNormal, unsigned char& surfaceMaterial, float maxDistance, bool ignoreWater)
{
	Vector3 from = rayInMe.origin();
	Vector3 to = rayInMe.origin() + maxDistance * rayInMe.direction();

    MegaClusterInstance* mci = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner());

    DbvhRayTest test(mci, from, to, ignoreWater);

    if (bulletChunksTree->m_root)
        test.processTree(bulletChunksTree->m_root);

	for (size_t i = 0; i < test.updatedChunks.size(); ++i)
		test.updatedChunks[i]->updateTree();

    if (test.result.hasHit())
    {
		localHitPoint = from + test.result.fraction * (to - from);
        surfaceNormal = test.result.tree->getTriangleNormal(test.result.triangle);
        surfaceMaterial = test.result.tree->getMaterial(test.result.triangle, localHitPoint);

        return true;
    }

    return false;
}

bool SmoothClusterGeometry::findCellsInBoundingBox(const Vector3& min, const Vector3& max)
{
	Voxel2::Grid* grid = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner())->getSmoothGrid();

	Voxel2::Region region = Voxel2::Region::fromExtents(min, max);

    if (region.empty())
        return false;

    Voxel2::Box box = grid->read(region);

	return !box.isEmpty();
}

void SmoothClusterGeometry::updateChunk(const Vector3int32& id)
{
    partition->updateChunk(id);

	ChunkMesh* chunk = new ChunkMesh(id);

	Voxel2::Region region = chunk->getRegion();

	btDbvtVolume bounds = btDbvtVolume::FromMM(btVector3(region.begin().x, region.begin().y, region.begin().z) * Voxel::kCELL_SIZE, btVector3(region.end().x, region.end().y, region.end().z) * Voxel::kCELL_SIZE);

    chunk->tree = bulletChunksTree;
    chunk->treeNode = bulletChunksTree->insert(bounds, chunk);

    ChunkMesh*& slot = bulletChunks[id];

    delete slot;
    slot = chunk;
}

void SmoothClusterGeometry::updateAllChunks()
{
	for (ChunkMap::iterator it = bulletChunks.begin(); it != bulletChunks.end(); ++it)
        delete it->second;

	bulletChunks.clear();

    MegaClusterInstance* mci = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner());
	Voxel2::Grid* grid = mci->getSmoothGrid();

	std::vector<Voxel2::Region> regions = grid->getNonEmptyRegions();

    for (size_t i = 0; i < regions.size(); ++i)
	{
		std::vector<Vector3int32> chunks = regions[i].expand(1).getChunkIds(TerrainPartitionSmooth::kChunkSizeLog2);

        for (auto cid: chunks)
		{
			if (bulletChunks.find(cid) == bulletChunks.end())
                updateChunk(cid);
		}
	}
}

void SmoothClusterGeometry::garbageCollectIncremental()
{
	// To catch up with allocation rate we need to visit the number of allocated elements since last run plus a small constant
    size_t allocatedCount = std::max(bulletChunks.size(), gcChunkCountLast) - gcChunkCountLast;
	size_t visitCount = std::min(bulletChunks.size(), allocatedCount + 64);

	ChunkMap::iterator it = bulletChunks.find(gcChunkKeyNext);

	// This should only happen if last GC was ran with no items
	if (it == bulletChunks.end())
		it = bulletChunks.begin();

	for (size_t j = 0; j < visitCount; ++j)
	{
		ChunkMesh* mesh = it->second;

		if (mesh->shape && mesh->shape.unique())
		{
			size_t memorySize = mesh->shape->getMemorySize();

			if (gcUnusedMemory > size_t(FInt::SmoothTerrainPhysicsCacheSize))
			{
				// Prevent overflow during subtracting since unused memory is an approximate measurement
				gcUnusedMemory = std::max(gcUnusedMemory, memorySize) - memorySize;

				RBXASSERT(mesh->state == ChunkMesh::State_Ready);
				mesh->state = ChunkMesh::State_Dummy;
				mesh->shape.reset();
			}
			else
			{
				gcUnusedMemoryNext += memorySize;
			}
		}

		++it;

		if (it == bulletChunks.end())
		{
			// We traversed all chunks, update our unused memory estimate
			gcUnusedMemory = gcUnusedMemoryNext;
			gcUnusedMemoryNext = 0;

			it = bulletChunks.begin();
		}
	}

    // If we stopped at the end, start at the beginning next time (INT_MAX is not a valid chunk id)
	gcChunkCountLast = bulletChunks.size();
	gcChunkKeyNext = (it == bulletChunks.end()) ? Vector3int32(INT_MAX, INT_MAX, INT_MAX) : it->first;
}

shared_ptr<btCollisionShape> SmoothClusterGeometry::getBulletChunkShape(const Vector3int32& id)
{
	ChunkMap::iterator it = bulletChunks.find(id);
    RBXASSERT(it != bulletChunks.end());

	if (it == bulletChunks.end())
		return shared_ptr<btCollisionShape>();

	if (it->second->state == ChunkMesh::State_Dummy)
	{
		MegaClusterInstance* mci = rbx_static_cast<MegaClusterInstance*>(myPrim->getOwner());

		it->second->generateShape(mci);
		it->second->updateTree();
	}
	
	return it->second->shape;
}

PartMaterial SmoothClusterGeometry::getTriangleMaterial(btCollisionShape* collisionShape, unsigned int triangleIndex, const Vector3& localHitPoint)
{
	ChunkMeshShape *chunkShape = boost::polymorphic_downcast<ChunkMeshShape*>(collisionShape);
	return chunkShape->getTriangleMaterial(triangleIndex, localHitPoint);
}

} // namespace RBX
