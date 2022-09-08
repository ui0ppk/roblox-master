#include "stdafx.h"
#include "LightGrid.h"

#include "FastLog.h"
#include "rbx/Debug.h"

#include "v8world/ContactManagerSpatialHash.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Light.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/BlockMesh.h"
#include "v8datamodel/CylinderMesh.h"
#include "v8datamodel/SpecialMesh.h"

#include "GfxCore/Device.h"

#include "CullableSceneNode.h"
#include "LightObject.h"
#include "VisualEngine.h"
#include "Util.h"
#include "SpatialHashedScene.h"

#include "rbx/DenseHash.h"

#include "rbx/Profiler.h"

#if defined(__APPLE__)
#include "TargetConditionals.h"
#endif

#if (defined(RBX_PLATFORM_IOS) && !TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__)
#include <arm_neon.h>
#endif

LOGVARIABLE(RenderLightGrid, 0)

LOGVARIABLE(RenderLightGridAgeProportion, 5)
LOGVARIABLE(RenderLightGridBorderGlobalCutoff, 32)
LOGVARIABLE(RenderLightGridBorderSkylightCutoff, 32)

#if defined(_WIN32) || (defined(__APPLE__) && !defined(RBX_PLATFORM_IOS))
#define SIMD_SSE2
#endif

#if (defined(RBX_PLATFORM_IOS) && !TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__)
#define SIMD_NEON
#endif

#if defined(SIMD_SSE2)
#define SIMDCALL(func, args) (useSIMD ? func##SIMD args : func args)
#define SIMDCALLSSE(func, args) (useSIMD ? func##SIMD args : func args)
#elif defined(SIMD_NEON)
#define SIMDCALL(func, args) (useSIMD ? func##SIMD args : func args)
#define SIMDCALLSSE(func, args) func args
#else
#define SIMDCALL(func, args) func args
#define SIMDCALLSSE(func, args) func args
#endif

namespace RBX
{
namespace Graphics
{

const int kLightGridDirectionalDirtyThreshold = 4;
const int kLightGridSkylightDirtyThreshold = 4;
const int kLightGridShadowDirtyThreshold = 4;

// branchless equivalent of std::max(0, v)
inline unsigned char max0(int v)
{
    return v & ~(v >> 31);
}

// branchless equivalent of std::min(255, v)
inline unsigned char min255(int v)
{
    return v | ((255 - v) >> 31);
}

// keep track of dirty state by or-ing together abs(difference)
inline void updateWithDirtyFlag(unsigned char& target, int value, unsigned int& dirty)
{
    int diff = target - value;
    int diffsign = diff >> 31;

    target = value;
    dirty |= (diff ^ diffsign) - diffsign;
}

inline Vector3int32 getGridPosition(const Vector3& v)
{
    float gx = v.x / 4.f;
    float gy = v.y / 4.f;
    float gz = v.z / 4.f;
    
    return Vector3int32(fastFloorInt(gx), fastFloorInt(gy), fastFloorInt(gz));
}

inline Vector3int32 getChunkIndexFromGridPosition(const Vector3int32& pos)
{
    BOOST_STATIC_ASSERT(kLightGridChunkSizeXZ == 1 << boost::static_log2<kLightGridChunkSizeXZ>::value);
    BOOST_STATIC_ASSERT(kLightGridChunkSizeY == 1 << boost::static_log2<kLightGridChunkSizeY>::value);
    
    // Use right shift instead of division since right shift rounds down instead of truncating
    int cx = pos.x >> boost::static_log2<kLightGridChunkSizeXZ>::value;
    int cy = pos.y >> boost::static_log2<kLightGridChunkSizeY>::value;
    int cz = pos.z >> boost::static_log2<kLightGridChunkSizeXZ>::value;
    
    return Vector3int32(cx, cy, cz);
}


inline unsigned int getArrayOffset(const Vector3int32& index, const Vector3int32& size)
{
    return index.x + (index.z + index.y * size.z) * size.x;
}

Vector3int32 convertToFixedPointNormalized(float x, float y, float z, int bits)
{
    int targetSum = 1 << bits;

    // Convert value to fixed point
    int resultX = G3D::clamp(x, 0.f, 1.f) * targetSum + 0.5f;
    int resultY = G3D::clamp(y, 0.f, 1.f) * targetSum + 0.5f;
    int resultZ = G3D::clamp(z, 0.f, 1.f) * targetSum + 0.5f;
    
    // Normalize sum
    // If the sum was approximately targetSum, this distributes the error roughly proportionately
    // If the sum was very different (i.e. much larger?), then we may get bogus results.
    // Order is significant here - if the error is not evenly divisible by 3, we'll adjust Y more.
    resultZ = std::max(0, resultZ + (targetSum - (resultX + resultY + resultZ)) / 3);
    resultX = std::max(0, resultX + (targetSum - (resultX + resultY + resultZ)) / 2);
    resultY = std::max(0, resultY + (targetSum - (resultX + resultY + resultZ)) / 1);
        
    return Vector3int32(resultX, resultY, resultZ);
}

Vector3 getLightContributionZ1(float x, float y)
{
    float wedgeVolumeX = x / 2;
    float wedgeVolumeY = y / 2;
    float cornerVolume = x * y / 6;
    
    float contribX = wedgeVolumeX - cornerVolume;
    float contribY = wedgeVolumeY - cornerVolume;
    
    return Vector3(contribX, contribY, 1 - contribX - contribY);
}

Vector3int32 getLightContribution(const Vector3& direction, int bits)
{
    float x = fabsf(direction.x);
    float y = fabsf(direction.y);
    float z = fabsf(direction.z);
    
    if (z >= y && z >= x)
    {
        Vector3 result = getLightContributionZ1(x / z, y / z);
        
        return convertToFixedPointNormalized(result.x, result.y, result.z, bits);
    }
    else if (y >= z && y >= x)
    {
        Vector3 result = getLightContributionZ1(x / y, z / y);
        
        return convertToFixedPointNormalized(result.x, result.z, result.y, bits);
    }
    else
    {
        Vector3 result = getLightContributionZ1(y / x, z / x);
        
        return convertToFixedPointNormalized(result.z, result.x, result.y, bits);
    }
}

Vector3int32 getLightContributionApprox(const Vector3& direction, int bits)
{
    return convertToFixedPointNormalized(direction.x * direction.x, direction.y * direction.y, direction.z * direction.z, bits);
}

void fillDummyLighting(LightGridChunk& chunk, unsigned char global, unsigned char skylight)
{
    unsigned char rowData[kLightGridChunkSizeXZ][4];
    
    for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
    {
        rowData[x][0] = 0;
        rowData[x][1] = 0;
        rowData[x][2] = 0;
        rowData[x][3] = global;
    }
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            memcpy(chunk.lighting[y][z][0], rowData[0], sizeof(rowData));
            
    memset(chunk.lightingSkylight, skylight, sizeof(chunk.lightingSkylight));

    int averageWeight = kLightGridChunkSizeXZ * kLightGridChunkSizeXZ * 255;
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        chunk.lightingAverageGlobal[y] = global * averageWeight;
        chunk.lightingAverageSkylight[y] = skylight * averageWeight;
        chunk.lightingAverageWeight[y] = averageWeight;
    }

    memset(chunk.irradianceFringeX.data, 255, sizeof(chunk.irradianceFringeX.data));
    memset(chunk.irradianceFringeY.data, 255, sizeof(chunk.irradianceFringeY.data));
    memset(chunk.irradianceFringeZ.data, 255, sizeof(chunk.irradianceFringeZ.data));
    
    memset(chunk.irradianceFringeSkyY.data, 255, sizeof(chunk.irradianceFringeSkyY.data));
    memset(chunk.irradianceFringeSkyX0.data, 255, sizeof(chunk.irradianceFringeSkyX0.data));
    memset(chunk.irradianceFringeSkyX1.data, 255, sizeof(chunk.irradianceFringeSkyX1.data));
    memset(chunk.irradianceFringeSkyZ0.data, 255, sizeof(chunk.irradianceFringeSkyZ0.data));
    memset(chunk.irradianceFringeSkyZ1.data, 255, sizeof(chunk.irradianceFringeSkyZ1.data));
}

typedef unsigned char LightingData[kLightGridChunkSizeY][kLightGridChunkSizeXZ][kLightGridChunkSizeXZ][4];

void fillLightingBorderX(LightingData& lighting, int x, const Color3uint8& skyAmbient)
{
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
			lighting[y][z][x][0] = skyAmbient.r;
			lighting[y][z][x][1] = skyAmbient.g;
            lighting[y][z][x][2] = skyAmbient.b;
            lighting[y][z][x][3] = 255;
        }
}

void fillLightingBorderY(LightingData& lighting, int y, const Color3uint8& skyAmbient)
{
    unsigned char rowData[kLightGridChunkSizeXZ][4];
    
    for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
    {
        rowData[x][0] = skyAmbient.r;
        rowData[x][1] = skyAmbient.g;
        rowData[x][2] = skyAmbient.b;
        rowData[x][3] = 255;
    }
    
    for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        memcpy(lighting[y][z][0], rowData[0], sizeof(rowData));
}

void fillLightingBorderZ(LightingData& lighting, int z, const Color3uint8& skyAmbient)
{
    unsigned char rowData[kLightGridChunkSizeXZ][4];
    
    for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
    {
        rowData[x][0] = skyAmbient.r;
        rowData[x][1] = skyAmbient.g;
        rowData[x][2] = skyAmbient.b;
        rowData[x][3] = 255;
    }
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        memcpy(lighting[y][z][0], rowData[0], sizeof(rowData));
}

void fillLightingBorders(LightingData& lighting, const Vector3int32& index, const Vector3int32& corner, const Vector3int32& size, const Color3uint8& skyAmbient)
{
    if (index.x == corner.x)
		fillLightingBorderX(lighting, 0, skyAmbient);

    if (index.x == corner.x + size.x - 1)
		fillLightingBorderX(lighting, kLightGridChunkSizeXZ - 1, skyAmbient);

    if (index.y == corner.y)
	{
        // We fill two bottom rows to make sure that the lookup-table-based filtering samples both values as a border color
		fillLightingBorderY(lighting, 0, skyAmbient);
		fillLightingBorderY(lighting, 1, skyAmbient);
	}

    if (index.y == corner.y + size.y - 1)
		fillLightingBorderY(lighting, kLightGridChunkSizeY - 1, skyAmbient);

    if (index.z == corner.z)
		fillLightingBorderZ(lighting, 0, skyAmbient);

    if (index.z == corner.z + size.z - 1)
		fillLightingBorderZ(lighting, kLightGridChunkSizeXZ - 1, skyAmbient);
}

bool isChunkMoreImportant(const LightGridChunk& lhs, const LightGridChunk& rhs)
{
    return (lhs.distancePriority < rhs.distancePriority) || (lhs.distancePriority == rhs.distancePriority && lhs.dirty > rhs.dirty);
}

inline int isqrt(int value)
{
    int result = 0;
    
    while (result * result < value)
        result++;
        
    return result;
}

static bool hasShadowCastingLights(const std::vector<LightObject*>& lights)
{
    for (size_t i = 0; i < lights.size(); ++i)
        if (lights[i]->getShadowMap())
            return true;
            
    return false;
}

Vector3int32 LightGrid::getChunkIndexForPoint(const Vector3& v)
{
    return getChunkIndexFromGridPosition(getGridPosition(v));
}

LightGridChunk* LightGrid::getChunkByIndex(const Vector3int32& index)
{
    Vector3int32 localIndex = index - cornerChunkIndex;
    
    if (isChunkInsideGridLocal(localIndex))
        return getChunkByLocalIndex(localIndex);
    else
        return NULL;
}

LightGridChunk* LightGrid::getChunkByLocalIndex(const Vector3int32& index)
{
    unsigned int offset = getArrayOffset(index, chunkCount);
    
    RBXASSERT(offset < chunks.size());
    return chunks[offset];
}

bool LightGrid::isChunkInsideGridLocal(const Vector3int32& index)
{
    return
        static_cast<unsigned int>(index.x) < static_cast<unsigned int>(chunkCount.x) && 
        static_cast<unsigned int>(index.y) < static_cast<unsigned int>(chunkCount.y) && 
        static_cast<unsigned int>(index.z) < static_cast<unsigned int>(chunkCount.z);
}

LightGrid* LightGrid::create(VisualEngine* visualEngine, const Vector3int32& chunkCount, TextureMode textureMode)
{
    RBXASSERT(chunkCount.x > 0 && chunkCount.y > 0 && chunkCount.z > 0);
    
    if (textureMode != Texture_None)
    {
        shared_ptr<Texture> texture;
        shared_ptr<Texture> lookupTexture;

        if (textureMode == Texture_3D)
        {
            texture = visualEngine->getDevice()->createTexture(Texture::Type_3D, Texture::Format_RGBA8,
                chunkCount.x * kLightGridChunkSizeXZ, chunkCount.z * kLightGridChunkSizeXZ, chunkCount.y * kLightGridChunkSizeY, 1, Texture::Usage_Dynamic);
        }
        else
        {
            int gridDepth = chunkCount.y * kLightGridChunkSizeY;
            int gridDepthSqrt = isqrt(gridDepth);

            // The wrapping code only works with depth that is a square for now
            RBXASSERT(gridDepthSqrt * gridDepthSqrt == gridDepth);

            texture = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA8,
                chunkCount.x * kLightGridChunkSizeXZ * gridDepthSqrt, chunkCount.z * kLightGridChunkSizeXZ * gridDepthSqrt, 1, 1, Texture::Usage_Dynamic);
                
            // Create a LUT
            std::vector<float> lutData;
            
            for (int slice = 0; slice < gridDepth; ++slice)
            {
                int sliceOffsetX = (slice % gridDepthSqrt);
                int sliceOffsetY = (slice / gridDepthSqrt);
                
                int sliceNext = (slice + 1) % gridDepth;
                int sliceNextOffsetX = (sliceNext % gridDepthSqrt);
                int sliceNextOffsetY = (sliceNext / gridDepthSqrt);

                lutData.push_back(static_cast<float>(sliceOffsetX) / gridDepthSqrt);
                lutData.push_back(static_cast<float>(sliceOffsetY) / gridDepthSqrt);
                lutData.push_back(static_cast<float>(sliceNextOffsetX) / gridDepthSqrt);
                lutData.push_back(static_cast<float>(sliceNextOffsetY) / gridDepthSqrt);
            }
            
            // Upload LUT to a texture
            if (visualEngine->getDevice()->getCaps().supportsTextureHalfFloat)
            {
                std::vector<unsigned short> lutDataHalf;

                for (size_t i = 0; i < lutData.size(); ++i)
                    lutDataHalf.push_back(floatToHalf(lutData[i]));

                lookupTexture = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA16F, gridDepth, 1, 1, 1, Texture::Usage_Static);
                
                lookupTexture->upload(0, 0, TextureRegion(0, 0, gridDepth, 1), &lutDataHalf[0], lutDataHalf.size() * sizeof(lutDataHalf[0]));
            }
            else
            {
                std::vector<unsigned char> lutDataByte;

                for (size_t i = 0; i < lutData.size(); ++i)
                    lutDataByte.push_back(lutData[i] * 255.f + 0.5f);


                lookupTexture = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA8, gridDepth, 1, 1, 1, Texture::Usage_Static);
                
                lookupTexture->upload(0, 0, TextureRegion(0, 0, gridDepth, 1), &lutDataByte[0], lutDataByte.size() * sizeof(lutDataByte[0]));
            }
        }

        return new LightGrid(visualEngine, texture, lookupTexture, chunkCount);
    }
    else
    {
        return new LightGrid(visualEngine, shared_ptr<Texture>(), shared_ptr<Texture>(), chunkCount);
    }
}

LightGrid::LightGrid(VisualEngine* visualEngine, const shared_ptr<Texture>& texture, const shared_ptr<Texture>& lookupTexture, const Vector3int32& chunkCount)
    : Resource(visualEngine->getDevice())
    , visualEngine(visualEngine)
{
    unsigned char defaultBorderGlobal = 255;
    unsigned char defaultBorderSkylight = 255;

    // Pick a sensible default centered around origin
    Vector3int32 corner = Vector3int32(-chunkCount.x / 2, -chunkCount.y / 2, -chunkCount.z / 2);
    
    chunks.resize(chunkCount.x * chunkCount.y * chunkCount.z);
    
    for (int cy = 0; cy < chunkCount.y; ++cy)
        for (int cz = 0; cz < chunkCount.z; ++cz)
            for (int cx = 0; cx < chunkCount.x; ++cx)
            {
                unsigned int offset = getArrayOffset(Vector3int32(cx, cy, cz), chunkCount);
                
                chunks[offset] = new LightGridChunk();
                
                LightGridChunk& chunk = *chunks[offset];
                
                chunk.index = corner + Vector3int32(cx, cy, cz);
                                    
                chunk.distancePriority = 0;
                chunk.dirty = 0;
                chunk.neverDirty = 0;
                chunk.age = 0;

                fillDummyLighting(chunk, defaultBorderGlobal, defaultBorderSkylight);
            }
            
    memset(dummyFringeXZ255.data, 255, sizeof(dummyFringeXZ255.data));
    memset(dummyFringeY255.data, 255, sizeof(dummyFringeY255.data));

    this->chunkCount = chunkCount;
    cornerChunkIndex = corner;
    
    this->texture = texture;
    this->lookupTexture = lookupTexture;

    lightBorderGlobal = defaultBorderGlobal;
    lightBorderSkylight = defaultBorderSkylight;

    lightShadows = false;
    lightDirection = Vector3(0, -1, 0);
    skyAmbient = Color3uint8(0, 0, 0);
    
    dirtyCursor = Vector3int32(0,0,0);
    dirtyCursorDirection = Vector3int32(-1,-1,-1);

    useSIMD = false;
    
#ifdef SIMD_SSE2
	useSIMD = true;
#endif

#ifdef SIMD_NEON
	useSIMD = true;
#endif

    precomputeShadowLUT();
}

LightGrid::~LightGrid()
{
    for (size_t i = 0; i < chunks.size(); ++i)
        delete chunks[i];
}

void LightGrid::setNonFixedPartsEnabled(bool value)
{
    voxelizer.setNonFixedPartsEnabled(value);
}

bool LightGrid::getNonFixedPartsEnabled() const
{
    return voxelizer.getNonFixedPartsEnabled();
}
    
inline bool isPartFixed(RBX::PartInstance* part)
{
    return part->getPartPrimitive()->getAnchoredProperty() || part->getPartPrimitive()->computeIsGrounded();
}

void LightGrid::occupancyUpdateChunkPrepare(Voxel::OccupancyChunk& chunk, MegaClusterInstance* terrain, ContactManager* contactManager, std::vector< DataModelPartCache >& partCache)
{
	RBXPROFILER_SCOPE("Render", "occupancyUpdateChunkPrepare");

    size_t partCacheOffset = partCache.size();

    voxelizer.occupancyUpdateChunkPrepare(chunk, terrain, contactManager, partCache);

    RBXPROFILER_LABELF("Render", "Primitives: %d", int(partCache.size() - partCacheOffset));
}

void LightGrid::occupancyUpdateChunkPerform(const std::vector< DataModelPartCache >& partCache)
{
	RBXPROFILER_SCOPE("Render", "occupancyUpdateChunkPerform");

    voxelizer.occupancyUpdateChunkPerform(partCache);

    RBXPROFILER_LABELF("Render", "Primitives: %d", int(partCache.size()));
}

void LightGrid::lightingUpdateChunkLocal(LightGridChunk& chunk, SpatialHashedScene* spatialHashedScene)
{
	RBXPROFILER_SCOPE("Render", "lightingUpdateChunkLocal");

    Extents chunkExtents = chunk.getChunkExtents();

    std::vector<LightObject*> lights;
    lightingGetLights(lights, chunkExtents, spatialHashedScene);

    // Update 'never dirty' mask based on whether there are shadow casting lights in the chunk
    chunk.neverDirty = hasShadowCastingLights(lights) ? 0 : LightGridChunk::Dirty_LightingLocalShadowed;
    
    // Update lighting contribution in the chunk
    if (!lights.empty())
    {
        // Accumulate lighting to scratch buffer
        memset(lightingScratch, 0, sizeof(lightingScratch));
    
        for (size_t i = 0; i < lights.size(); ++i)
            lightingUpdateLightScratch(chunk, chunkExtents, lights[i]);
        
        // Separable blur along 3 axes
        SIMDCALL(lightingBlurAxisYZScratch, <true>());
        SIMDCALL(lightingBlurAxisYZScratch, <false>());
        SIMDCALL(lightingBlurAxisXScratchToChunk, (chunk)); // note: last blur step copies the lighting to chunk
	}
    else
    {
        lightingClearLocal(chunk);
    }
}

void LightGrid::lightingUpdateChunkGlobal(LightGridChunk& chunk)
{
	RBXPROFILER_SCOPE("Render", "lightingUpdateChunkGlobal");
    
    if (lightShadows)
    {
        lightingUpdateDirectional(chunk, lightDirection);
    }
    else
    {
        lightingClearGlobal(chunk);
    }
}

void LightGrid::lightingUpdateChunkSkylight(LightGridChunk& chunk)
{
	RBXPROFILER_SCOPE("Render", "lightingUpdateChunkSkylight");
    
    if (lightShadows)
    {
        lightingUpdateSkylight(chunk);
    }
    else
    {
        memset(chunk.lightingSkylight, 0, sizeof(chunk.lightingSkylight));
    }
}

void LightGrid::lightingUpdateChunkAverage(LightGridChunk& chunk)
{
	RBXPROFILER_SCOPE("Render", "lightingUpdateChunkAverage");
    
    SIMDCALL(lightingUpdateAverageImpl, (chunk));
}

void LightGrid::invalidateAll(unsigned int status)
{
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        LightGridChunk& chunk = *chunks[i];
        
        chunk.dirty |= (status & ~chunk.neverDirty);
    }
}

void LightGrid::invalidateExtents(const RBX::Extents& extents, unsigned int status)
{
    Vector3int32 chunkMin = getChunkIndexForPoint(extents.min()).max(cornerChunkIndex);
    Vector3int32 chunkMax = getChunkIndexForPoint(extents.max()).min(cornerChunkIndex + chunkCount - Vector3int32(1, 1, 1));

    for (int y = chunkMin.y; y <= chunkMax.y; ++y)
        for (int z = chunkMin.z; z <= chunkMax.z; ++z)
            for (int x = chunkMin.x; x <= chunkMax.x; ++x)
            {
                FASTLOG4(FLog::RenderLightGrid, "LightGrid: Marking chunk %dx%dx%d as dirty (%d)", x, y, z, status);
                
                LightGridChunk* chunk = getChunkByIndex(Vector3int32(x, y, z));
                RBXASSERT(chunk);

                chunk->dirty |= (status & ~chunk->neverDirty);
            }
}

void LightGrid::invalidatePoint(const Vector3& point, unsigned int status)
{
    Vector3int32 chunkIndex = getChunkIndexForPoint(point);
    
    if (LightGridChunk* chunk = getChunkByIndex(chunkIndex))
    {
        chunk->dirty |= (status & ~chunk->neverDirty);
    }
}

bool LightGrid::updateGridCenter(const Vector3& cameraTarget, bool skipChunkUpdate)
{
    Vector3int32 cameraPos = getGridPosition(cameraTarget);
    Vector3int32 cameraChunk = getChunkIndexFromGridPosition(cameraPos);

    // If camera is in hysteresis region (2x2x2 chunks in the middle of the grid), don't relocate the grid
    Vector3int32 gridCenterLocal = Vector3int32(chunkCount.x / 2, chunkCount.y / 2, chunkCount.z / 2);
    Vector3int32 chunkCenterLocal = Vector3int32(kLightGridChunkSizeXZ / 2, kLightGridChunkSizeY / 2, kLightGridChunkSizeXZ / 2);
    
    Vector3int32 cameraChunkCenterOffset = cameraChunk - (cornerChunkIndex + gridCenterLocal);
    
    if ((cameraChunkCenterOffset.x == -1 || cameraChunkCenterOffset.x == 0) &&
        (cameraChunkCenterOffset.y == -1 || cameraChunkCenterOffset.y == 0) &&
        (cameraChunkCenterOffset.z == -1 || cameraChunkCenterOffset.z == 0))
    {
        return false;
    }
    
    // Figure out the new location from the chunk that's nearest to the camera
    Vector3int32 newCorner = getChunkIndexFromGridPosition(cameraPos + chunkCenterLocal) - gridCenterLocal;
    RBXASSERT(newCorner != cornerChunkIndex);
    
    relocateGrid(newCorner, skipChunkUpdate);

    return true;
}

void LightGrid::updateAgePriorityForChunks(const Vector3& cameraTarget)
{
    Vector3int32 cellPos = getGridPosition(cameraTarget);
    
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        LightGridChunk& chunk = *chunks[i];
        if(chunk.dirty == 0)
            continue;
        
        Vector3int32 chunkCenter = chunk.index * kLightGridChunkSize + Vector3int32(kLightGridChunkSizeXZ / 2, kLightGridChunkSizeY / 2, kLightGridChunkSizeXZ / 2);

        Vector3int32 diff = Vector3int32(
            abs(cellPos.x - chunkCenter.x) / kLightGridChunkSizeXZ ,
            abs(cellPos.y - chunkCenter.y) / (kLightGridChunkSizeY/2),
            abs(cellPos.z - chunkCenter.z) / kLightGridChunkSizeXZ);

        chunk.distancePriority = std::max(diff.x, std::max(diff.y, diff.z));
        chunk.age++;
    }
}

void LightGrid::updateBorderColor(const Vector3& cameraTarget, const Frustum& cameraFrustum)
{
    // We only care about XZ frustum range, so disable far, top and bottom plane culling for chunks
    Frustum cameraFrustumInfinite = cameraFrustum;
   
    if (cameraFrustumInfinite.faceArray.size() > 3)
    {
        // Conveniently, the planes we care about are the first three, so we can just resize the array if it had more planes to test
        BOOST_STATIC_ASSERT(Frustum::kPlaneNear == 0 && Frustum::kPlaneRight == 1 && Frustum::kPlaneLeft == 2);
        cameraFrustumInfinite.faceArray.resize(3);
    }
    
    Vector3int32 cellPos = getGridPosition(cameraTarget);

    unsigned long long borderGlobal = 0;
    unsigned long long borderSkylight = 0;
    unsigned long long borderWeight = 0;

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        LightGridChunk& chunk = *chunks[i];

        bool isBorderXZ =
            !isChunkInsideGridLocal(chunk.index - cornerChunkIndex - Vector3int32(1, 0, 1)) ||
            !isChunkInsideGridLocal(chunk.index - cornerChunkIndex + Vector3int32(1, 0, 1));

        int localY = cellPos.y - chunk.index.y * kLightGridChunkSizeY;

        // We include an average of two Y slices above our level for all visible chunks on XZ border
        const int minOffsetY = 1;
        const int maxOffsetY = 2;
        
        if (isBorderXZ && localY + maxOffsetY >= 0 && localY + minOffsetY < kLightGridChunkSizeY)
        {
            Extents extents = chunk.getChunkExtents();
            
            if (cameraFrustumInfinite.intersectsSphere(extents.center(), extents.size().x / 2))
            {
                for (int offset = minOffsetY; offset <= maxOffsetY; offset++)
                {
                    if (localY + offset >= 0 && localY + offset < kLightGridChunkSizeY)
                    {
                        borderGlobal += chunk.lightingAverageGlobal[localY + offset];
                        borderSkylight += chunk.lightingAverageSkylight[localY + offset];
                        borderWeight += chunk.lightingAverageWeight[localY + offset];
                    }
                }
            }
        }
    }

    if (borderWeight)
    {
        lightBorderGlobal = (borderGlobal + borderWeight - 1) / borderWeight;
        lightBorderSkylight = (borderSkylight + borderWeight - 1) / borderWeight;
    }
}

Color4uint8 LightGrid::computeAverageColor(const Vector3& focusPoint)
{
	RBXPROFILER_SCOPE("Render", "computeAverageColor");
    
    Vector3int32 cellPos = getGridPosition(focusPoint);

    int border[4] = {};
    int borderCount = 0;

    Color3uint8 storageSkyAmbient = transformColor(skyAmbient);
    
    for (int yi = 0; yi < 3; ++yi)
        for (int zi = -4; zi <= 4; ++zi)
            for (int xi = -4; xi <= 4; ++xi)
            {
                Vector3int32 pos = cellPos + Vector3int32(xi, yi, zi);
                Vector3int32 cpos = getChunkIndexFromGridPosition(pos);

                if (LightGridChunk* chunk = getChunkByIndex(cpos))
                {
                    Vector3int32 cidx = pos - chunk->index * kLightGridChunkSize;

                    border[0] += chunk->lighting[cidx.y][cidx.z][cidx.x][0];
                    border[1] += chunk->lighting[cidx.y][cidx.z][cidx.x][1];
                    border[2] += chunk->lighting[cidx.y][cidx.z][cidx.x][2];

                    if (lightShadows) 
                    {
                        unsigned char skylight = chunk->lightingSkylight[cidx.y][cidx.z][cidx.x];
                        border[0] += (skylight * storageSkyAmbient.r + 255) >> 8;
                        border[1] += (skylight * storageSkyAmbient.g + 255) >> 8;
                        border[2] += (skylight * storageSkyAmbient.b + 255) >> 8;

                        // border[3] accumulates skylight factor, shadow factor here intentionally, 
                        // computeAverageColor used where actual voxel data can't be displayed, 
                        // so shadow term would look weird in outdoors when you can't see the shadow
                        border[3] += skylight;
                    }
                    else
                    {
                        border[3] += 255;
                    }

                    borderCount++;
                }
            }

    if (borderCount == 0)
        return transformColor(lightShadows ? Color4uint8(storageSkyAmbient, 255) : Color4uint8(0,0,0,255));
    else
        return transformColor(Color4uint8(border[0] / borderCount, border[1] / borderCount, border[2] / borderCount, border[3] / borderCount));
}
   
void LightGrid::setLightShadows(bool value)
{
    if (value != lightShadows)
    {
        FASTLOG1(FLog::RenderLightGrid, "LightGrid: Changing light shadows to %d", value);
        
        lightShadows = value;

        invalidateAll(LightGridChunk::Dirty_LightingGlobal | LightGridChunk::Dirty_LightingSkylight);
    }
}

void LightGrid::setLightDirection(const Vector3& value)
{
    if (value.fuzzyNe(lightDirection))
    {
        FASTLOG3F(FLog::RenderLightGrid, "LightGrid: Changing light direction to %f %f %f", value.x, value.y, value.z);
        
        lightDirection = value;

        dirtyCursorDirection = Vector3int32(lightDirection.x < 0 ? -1 : 1, lightDirection.y < 0 ? -1 : 1, lightDirection.z < 0 ? -1 : 1);
        
        invalidateAll(LightGridChunk::Dirty_LightingGlobal);
    }
}

void LightGrid::setSkyAmbient(const Color3uint8& value)
{
    if (value != skyAmbient)
    {
        FASTLOG3(FLog::RenderLightGrid, "LightGrid: Changing sky ambient to %d %d %d", value.r, value.g, value.b);
        
        skyAmbient = value;

        invalidateAll(LightGridChunk::Dirty_LightingUpload);
    }
}

void LightGrid::stepCursor(Vector3int32& cursor)
{
    cursor.x += dirtyCursorDirection.x;
    if(cursor.x < 0 || cursor.x >= chunkCount.x)
    {
        cursor.x = dirtyCursorDirection.x < 0 ? chunkCount.x-1 : 0;
        cursor.z += dirtyCursorDirection.z;
        if(cursor.z < 0 || cursor.z >= chunkCount.z)
        {
            cursor.z = dirtyCursorDirection.z < 0 ? chunkCount.z-1 : 0;
            cursor.y += dirtyCursorDirection.y;
            if(cursor.y < 0 || cursor.y >= chunkCount.y)
            {
                cursor.y = dirtyCursorDirection.y < 0 ? chunkCount.y-1 : 0;
            }
        }
    }

    RBXASSERT(cursor.max(chunkCount) == chunkCount);
    RBXASSERT(cursor.min(Vector3int32(0,0,0)) == Vector3int32(0,0,0));
}

LightGridChunk* LightGrid::findDirtyChunk()
{
    LightGridChunk* candidateChunk = NULL;
    
    Vector3int32 newCursor = dirtyCursor;
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        stepCursor(dirtyCursor);
        LightGridChunk* chunk = getChunkByLocalIndex(dirtyCursor);
        if (chunk->dirty && (candidateChunk == NULL || isChunkMoreImportant(*chunk, *candidateChunk)))
        {
            candidateChunk = chunk;
            newCursor = dirtyCursor;
        }
    }

    dirtyCursor = newCursor;
    return candidateChunk;
}

LightGridChunk* LightGrid::findFirstDirtyChunk()
{
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        stepCursor(dirtyCursor);
        LightGridChunk* chunk = getChunkByLocalIndex(dirtyCursor);
        if (chunk->dirty)
            return chunk;
    }

    return NULL;
}

LightGridChunk* LightGrid::findOldestChunk()
{
    LightGridChunk* candidateChunk = NULL;
    Vector3int32 cursor = Vector3int32(0,0,0);
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        stepCursor(cursor);
        LightGridChunk* chunk = getChunkByLocalIndex(cursor);
        if (chunk->dirty && (candidateChunk == NULL || chunk->age > candidateChunk->age))
            candidateChunk = chunk;
    }
    return candidateChunk;
}

Vector3 LightGrid::getGridCornerOffset() const
{
    return Vector3(cornerChunkIndex.x, cornerChunkIndex.y, cornerChunkIndex.z) * Vector3(kLightGridChunkSizeXZ, kLightGridChunkSizeY, kLightGridChunkSizeXZ) * 4.f;
}

Vector3 LightGrid::getWrapSafeOffset() const
{
    Vector3int32 wrappedIndex = getWrappedChunkIndex(cornerChunkIndex);
    
    // wrappedIndex and cornerChunkIndex should map to the same location in the texture
    // So it should be safe to offset by the difference
    Vector3int32 offset = wrappedIndex - cornerChunkIndex;
    
    return Vector3(offset.x, offset.y, offset.z) * Vector3(kLightGridChunkSizeXZ, kLightGridChunkSizeY, kLightGridChunkSizeXZ) * 4.f;
}

Vector3 LightGrid::getGridSize() const
{
    return Vector3(chunkCount.x, chunkCount.y, chunkCount.z) * Vector3(kLightGridChunkSizeXZ, kLightGridChunkSizeY, kLightGridChunkSizeXZ) * 4.f;
}

Color4uint8 LightGrid::getBorderColor() const
{
    unsigned char global = lightBorderGlobal;
    unsigned char skylight = lightBorderSkylight;
    
    if (FLog::RenderLightGridBorderGlobalCutoff)
        global = (global < FLog::RenderLightGridBorderGlobalCutoff) ? 0 : 255;
        
    if (FLog::RenderLightGridBorderSkylightCutoff)
        skylight = (skylight < FLog::RenderLightGridBorderSkylightCutoff) ? 0 : 255;

    return Color4uint8(
        (skyAmbient.r * skylight + 255) >> 8,
        (skyAmbient.g * skylight + 255) >> 8,
        (skyAmbient.b * skylight + 255) >> 8,
        global);
}
   
void LightGrid::lightingUpdateDirectional(LightGridChunk& chunk, const Vector3& lightDirection)
{
    bool negX = lightDirection.x < 0;
    bool negZ = lightDirection.z < 0;
    
    Vector3int32 lightContrib = getLightContribution(lightDirection, 8);
    
    // Run the right version of update kernel
    if (negX)
    {
        if (negZ)
            lightingUpdateDirectionalImpl<true, true>(chunk, lightContrib);
        else
            lightingUpdateDirectionalImpl<true, false>(chunk, lightContrib);
    }
    else
    {
        if (negZ)
            lightingUpdateDirectionalImpl<false, true>(chunk, lightContrib);
        else
            lightingUpdateDirectionalImpl<false, false>(chunk, lightContrib);
    }
}

template <bool NegX, bool NegZ> void LightGrid::lightingUpdateDirectionalImpl(LightGridChunk& chunk, const Vector3int32& lightContrib)
{
#define IRRADIANCE(x, y, z) irradianceScratch[(y)+1][(z)+1][(x)+1]

    RBXASSERT(lightContrib.x + lightContrib.y + lightContrib.z == 256);
    
    int stepX = NegX ? 1 : -1;
    int stepY = 1;
    int stepZ = NegZ ? 1 : -1;
    
    // Update incoming irradiance
    LightGridChunk* incomingIrradianceXChunk = getChunkByIndex(chunk.index + Vector3int32(stepX, 0, 0));
    LightGridChunkFringeXZ& incomingIrradianceX = incomingIrradianceXChunk ? incomingIrradianceXChunk->irradianceFringeX : dummyFringeXZ255;
    
    LightGridChunk* incomingIrradianceYChunk = getChunkByIndex(chunk.index + Vector3int32(0, stepY, 0));
    LightGridChunkFringeY& incomingIrradianceY = incomingIrradianceYChunk ? incomingIrradianceYChunk->irradianceFringeY : dummyFringeY255;
    
    LightGridChunk* incomingIrradianceZChunk = getChunkByIndex(chunk.index + Vector3int32(0, 0, stepZ));
    LightGridChunkFringeXZ& incomingIrradianceZ = incomingIrradianceZChunk ? incomingIrradianceZChunk->irradianceFringeZ : dummyFringeXZ255;

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            IRRADIANCE(NegX ? kLightGridChunkSizeXZ : -1, y, z) = incomingIrradianceX.data[y][z];
    
    for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            IRRADIANCE(x, kLightGridChunkSizeY, z) = incomingIrradianceY.data[z][x];
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            IRRADIANCE(x, y, NegZ ? kLightGridChunkSizeXZ : -1) = incomingIrradianceZ.data[y][x];

#if defined(SIMD_SSE2) || defined(SIMD_NEON)
    // tableXSat[i] = max(0, i - 256) * lightContrib.x
    unsigned short tableXSat[512];
    
    for (int i = 0; i < 256; ++i) tableXSat[i] = 0;
    for (int i = 0; i < 256; ++i) tableXSat[i + 256] = i * lightContrib.x;
#endif
        
    // Compute lighting for cells
    for (int yi = 0; yi < kLightGridChunkSizeY; ++yi)
    {
        int y = kLightGridChunkSizeY - 1 - yi;
        
        for (int zi = 0; zi < kLightGridChunkSizeXZ; ++zi)
        {
            int z = NegZ ? kLightGridChunkSizeXZ - 1 - zi : zi;
        
        #ifdef SIMD_SSE2
            if (useSIMD)
            {
                __m128i contribY = _mm_set1_epi16(lightContrib.y);
                __m128i contribZ = _mm_set1_epi16(lightContrib.z);
                
                __m128i zero = _mm_setzero_si128();
                __m128i round = _mm_set1_epi16(255);
                
                for (int xi = 0; xi < kLightGridChunkSizeXZ; xi += 16)
                {
                    int x = NegX ? kLightGridChunkSizeXZ - 16 - xi : xi;

                    __m128i occ = _mm_loadu_si128((__m128i*)&chunk.occupancy[y][z][x]);
                    
                    __m128i incomingY = _mm_loadu_si128((__m128i*)&IRRADIANCE(x, y+stepY, z));
                    __m128i incomingZ = _mm_loadu_si128((__m128i*)&IRRADIANCE(x, y, z+stepZ));
                    
                    __m128i incomingY0 = _mm_unpacklo_epi8(incomingY, zero);
                    __m128i incomingY1 = _mm_unpackhi_epi8(incomingY, zero);
                    __m128i incomingZ0 = _mm_unpacklo_epi8(incomingZ, zero);
                    __m128i incomingZ1 = _mm_unpackhi_epi8(incomingZ, zero);
                    
                    __m128i result0 = _mm_add_epi16(round, _mm_add_epi16(_mm_mullo_epi16(incomingY0, contribY), _mm_mullo_epi16(incomingZ0, contribZ)));
                    __m128i result1 = _mm_add_epi16(round, _mm_add_epi16(_mm_mullo_epi16(incomingY1, contribY), _mm_mullo_epi16(incomingZ1, contribZ)));

                    unsigned short result0Buf[8], result1Buf[8];
                    _mm_storeu_si128((__m128i*)result0Buf, result0);
                    _mm_storeu_si128((__m128i*)result1Buf, result1);
                    
                    unsigned char res[16];

                #define STEP(half, i) \
                    do { \
                        int r = (tableXSat[irrprev + 256] + result##half##Buf[i % 8]) >> 8; \
                        chunk.lighting[y][z][x+i][3] = res[i] = r; \
                        irrprev = r - chunk.occupancy[y][z][x+i]; \
                    } while (0)
                    
                    if (NegX)
                    {
                        int irrprev = IRRADIANCE(x+16, y, z);
                    
                        STEP(1, 15);
                        STEP(1, 14);
                        STEP(1, 13);
                        STEP(1, 12);
                        STEP(1, 11);
                        STEP(1, 10);
                        STEP(1, 9);
                        STEP(1, 8);
                        STEP(0, 7);
                        STEP(0, 6);
                        STEP(0, 5);
                        STEP(0, 4);
                        STEP(0, 3);
                        STEP(0, 2);
                        STEP(0, 1);
                        STEP(0, 0);
                    }
                    else
                    {
                        int irrprev = IRRADIANCE(x-1, y, z);

                        STEP(0, 0);
                        STEP(0, 1);
                        STEP(0, 2);
                        STEP(0, 3);
                        STEP(0, 4);
                        STEP(0, 5);
                        STEP(0, 6);
                        STEP(0, 7);
                        STEP(1, 8);
                        STEP(1, 9);
                        STEP(1, 10);
                        STEP(1, 11);
                        STEP(1, 12);
                        STEP(1, 13);
                        STEP(1, 14);
                        STEP(1, 15);
                    }
                        
                #undef STEP
                
                    __m128i irr = _mm_subs_epu8(_mm_loadu_si128((__m128i*)res), occ);
                    
                    _mm_storeu_si128((__m128i*)&IRRADIANCE(x, y, z), irr);
                }
            }
            else
        #endif
        #ifdef SIMD_NEON
            if (useSIMD)
            {
                uint8x8_t contribY = vdup_n_u8(lightContrib.y);
                uint8x8_t contribZ = vdup_n_u8(lightContrib.z);
                
                uint16x8_t round = vdupq_n_u16(255);
                
                for (int xi = 0; xi < kLightGridChunkSizeXZ; xi += 16)
                {
                    int x = NegX ? kLightGridChunkSizeXZ - 16 - xi : xi;

                    uint8x16_t occ = vld1q_u8(&chunk.occupancy[y][z][x]);
                    
                    uint8x16_t incomingY = vld1q_u8(&IRRADIANCE(x, y+stepY, z));
                    uint8x16_t incomingZ = vld1q_u8(&IRRADIANCE(x, y, z+stepZ));
                    
                    uint8x8_t incomingY0 = vget_low_u8(incomingY);
                    uint8x8_t incomingY1 = vget_high_u8(incomingY);
                    uint8x8_t incomingZ0 = vget_low_u8(incomingZ);
                    uint8x8_t incomingZ1 = vget_high_u8(incomingZ);

                    uint16x8_t result0 = vmlal_u8(vmlal_u8(round, incomingY0, contribY), incomingZ0, contribZ);
                    uint16x8_t result1 = vmlal_u8(vmlal_u8(round, incomingY1, contribY), incomingZ1, contribZ);
                    
                    unsigned short result0Buf[8], result1Buf[8];
                    vst1q_u16(result0Buf, result0);
                    vst1q_u16(result1Buf, result1);
                    
                    unsigned char res[16];

                #define STEP(half, i) \
                    do { \
                        int r = (tableXSat[irrprev + 256] + result##half##Buf[i % 8]) >> 8; \
                        chunk.lighting[y][z][x+i][3] = res[i] = r; \
                        irrprev = r - chunk.occupancy[y][z][x+i]; \
                    } while (0)
                    
                    if (NegX)
                    {
                        int irrprev = IRRADIANCE(x+16, y, z);
                    
                        STEP(1, 15);
                        STEP(1, 14);
                        STEP(1, 13);
                        STEP(1, 12);
                        STEP(1, 11);
                        STEP(1, 10);
                        STEP(1, 9);
                        STEP(1, 8);
                        STEP(0, 7);
                        STEP(0, 6);
                        STEP(0, 5);
                        STEP(0, 4);
                        STEP(0, 3);
                        STEP(0, 2);
                        STEP(0, 1);
                        STEP(0, 0);
                    }
                    else
                    {
                        int irrprev = IRRADIANCE(x-1, y, z);

                        STEP(0, 0);
                        STEP(0, 1);
                        STEP(0, 2);
                        STEP(0, 3);
                        STEP(0, 4);
                        STEP(0, 5);
                        STEP(0, 6);
                        STEP(0, 7);
                        STEP(1, 8);
                        STEP(1, 9);
                        STEP(1, 10);
                        STEP(1, 11);
                        STEP(1, 12);
                        STEP(1, 13);
                        STEP(1, 14);
                        STEP(1, 15);
                    }
                        
                #undef STEP
                
                    uint8x16_t irr = vqsubq_u8(vld1q_u8(res), occ);
                    
                    vst1q_u8(&IRRADIANCE(x, y, z), irr);
                }
            }
            else
        #endif
            {
                for (int xi = 0; xi < kLightGridChunkSizeXZ; ++xi)
                {
                    int x = NegX ? kLightGridChunkSizeXZ - 1 - xi : xi;
                    
                    int incomingX = IRRADIANCE(x+stepX, y, z);
                    int incomingY = IRRADIANCE(x, y+stepY, z);
                    int incomingZ = IRRADIANCE(x, y, z+stepZ);
                    
                    int result = (incomingX * lightContrib.x + incomingY * lightContrib.y + incomingZ * lightContrib.z + 255) >> 8;

                    chunk.lighting[y][z][x][3] = result;

                    IRRADIANCE(x, y, z) = max0(result - chunk.occupancy[y][z][x]);
                }
            }
        }
    }
    
    // Update outgoing irradiance
    LightGridChunkFringeXZ& outgoingIrradianceX = chunk.irradianceFringeX;
    unsigned int outgoingIrradianceXDirty = 0;
    
    LightGridChunkFringeY& outgoingIrradianceY = chunk.irradianceFringeY;
    unsigned int outgoingIrradianceYDirty = 0;
    
    LightGridChunkFringeXZ& outgoingIrradianceZ = chunk.irradianceFringeZ;
    unsigned int outgoingIrradianceZDirty = 0;
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            updateWithDirtyFlag(outgoingIrradianceX.data[y][z], IRRADIANCE(NegX ? 0 : kLightGridChunkSizeXZ - 1, y, z), outgoingIrradianceXDirty);
    
    for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            updateWithDirtyFlag(outgoingIrradianceY.data[z][x], IRRADIANCE(x, 0, z), outgoingIrradianceYDirty);
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            updateWithDirtyFlag(outgoingIrradianceZ.data[y][x], IRRADIANCE(x, y, NegZ ? 0 : kLightGridChunkSizeXZ - 1), outgoingIrradianceZDirty);

    if (outgoingIrradianceXDirty > kLightGridDirectionalDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index - Vector3int32(stepX, 0, 0)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingGlobal;
    }

    if (outgoingIrradianceYDirty > kLightGridDirectionalDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index - Vector3int32(0, stepY, 0)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingGlobal;
    }

    if (outgoingIrradianceZDirty > kLightGridDirectionalDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index - Vector3int32(0, 0, stepZ)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingGlobal;
    }
    
#undef IRRADIANCE
}

void LightGrid::lightingUpdateSkylight(LightGridChunk& chunk)
{
#define IRRADIANCE(x, y, z) irradianceScratch[(y)+1][(z)+1][(x)+1]

    // Power of 4 table
    unsigned char powerCurve[256];
    
    for (int i = 0; i < 256; ++i)
    {
        unsigned int r1 = 255 - i;
        unsigned int r2 = r1 * r1;
        unsigned int r4 = r2 * r2;

        // to make sure i=0 goes to zero we have to subtract from the max value of (r4 >> 24), which is 252
        powerCurve[i] = 252 - (r4 >> 24);
    }

    // Update incoming irradiance
    LightGridChunk* incomingIrradianceYChunk = getChunkByIndex(chunk.index + Vector3int32(0, 1, 0));
    LightGridChunkFringeY& incomingIrradianceY = incomingIrradianceYChunk ? incomingIrradianceYChunk->irradianceFringeSkyY : dummyFringeY255;

    LightGridChunk* incomingIrradianceX0Chunk = getChunkByIndex(chunk.index + Vector3int32(-1, 0, 0));
    LightGridChunkFringeXZ& incomingIrradianceX0 = incomingIrradianceX0Chunk ? incomingIrradianceX0Chunk->irradianceFringeSkyX1 : dummyFringeXZ255;

    LightGridChunk* incomingIrradianceX1Chunk = getChunkByIndex(chunk.index + Vector3int32(+1, 0, 0));
    LightGridChunkFringeXZ& incomingIrradianceX1 = incomingIrradianceX1Chunk ? incomingIrradianceX1Chunk->irradianceFringeSkyX0 : dummyFringeXZ255;

    LightGridChunk* incomingIrradianceZ0Chunk = getChunkByIndex(chunk.index + Vector3int32(0, 0, -1));
    LightGridChunkFringeXZ& incomingIrradianceZ0 = incomingIrradianceZ0Chunk ? incomingIrradianceZ0Chunk->irradianceFringeSkyZ1 : dummyFringeXZ255;

    LightGridChunk* incomingIrradianceZ1Chunk = getChunkByIndex(chunk.index + Vector3int32(0, 0, +1));
    LightGridChunkFringeXZ& incomingIrradianceZ1 = incomingIrradianceZ1Chunk ? incomingIrradianceZ1Chunk->irradianceFringeSkyZ0 : dummyFringeXZ255;
    
    for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
        {
            IRRADIANCE(x, kLightGridChunkSizeY, z) = incomingIrradianceY.data[z][x];
            IRRADIANCE(x, kLightGridChunkSizeY, z) = incomingIrradianceY.data[z][x];
        }

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            IRRADIANCE(-1, y, z) = incomingIrradianceX0.data[y][z];

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            IRRADIANCE(x, y, -1) = incomingIrradianceZ0.data[y][x];

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            IRRADIANCE(kLightGridChunkSizeXZ, y, z) = incomingIrradianceX1.data[y][z];

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            IRRADIANCE(x, y, kLightGridChunkSizeXZ) = incomingIrradianceZ1.data[y][x];

    // Fill horizontal edges
    for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
    {
        IRRADIANCE(x, kLightGridChunkSizeY, -1) = (IRRADIANCE(x, kLightGridChunkSizeY-1, -1) + IRRADIANCE(x, kLightGridChunkSizeY, 0) + 1) / 2;
        IRRADIANCE(x, kLightGridChunkSizeY, kLightGridChunkSizeXZ) = (IRRADIANCE(x, kLightGridChunkSizeY-1, kLightGridChunkSizeXZ) + IRRADIANCE(x, kLightGridChunkSizeY, kLightGridChunkSizeXZ-1) + 1) / 2;
    }

    for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
    {
        IRRADIANCE(-1, kLightGridChunkSizeY, z) = (IRRADIANCE(-1, kLightGridChunkSizeY-1, z) + IRRADIANCE(0, kLightGridChunkSizeY, z) + 1) / 2;
        IRRADIANCE(kLightGridChunkSizeXZ, kLightGridChunkSizeY, z) = (IRRADIANCE(kLightGridChunkSizeXZ, kLightGridChunkSizeY-1, z) + IRRADIANCE(kLightGridChunkSizeXZ-1, kLightGridChunkSizeY, z) + 1) / 2;
    }
    
    // Fill vertical edges
    for (int y = 0; y < kLightGridChunkSizeY + 1; ++y)
    {
        IRRADIANCE(-1, y, -1) = (IRRADIANCE(-1, y, 0) + IRRADIANCE(0, y, -1) + 1) / 2;
        IRRADIANCE(kLightGridChunkSizeXZ, y, -1) = (IRRADIANCE(kLightGridChunkSizeXZ, y, 0) + IRRADIANCE(kLightGridChunkSizeXZ-1, y, -1) + 1) / 2;
        IRRADIANCE(-1, y, kLightGridChunkSizeXZ) = (IRRADIANCE(-1, y, kLightGridChunkSizeXZ-1) + IRRADIANCE(0, y, kLightGridChunkSizeXZ) + 1) / 2;
        IRRADIANCE(kLightGridChunkSizeXZ, y, kLightGridChunkSizeXZ) = (IRRADIANCE(kLightGridChunkSizeXZ, y, kLightGridChunkSizeXZ-1) + IRRADIANCE(kLightGridChunkSizeXZ-1, y, kLightGridChunkSizeXZ) + 1) / 2;
    }
    
    // Compute lighting for cells
    for (int yi = 0; yi < kLightGridChunkSizeY; ++yi)
    {
        int y = kLightGridChunkSizeY - 1 - yi;
        
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            SIMDCALL(lightingUpdateSkylightRow, (chunk, y, z, powerCurve));
        }
    }
    
    // Update outgoing irradiance
    LightGridChunkFringeY& outgoingIrradianceY = chunk.irradianceFringeSkyY;
    unsigned int outgoingIrradianceYDirty = 0;
    
    LightGridChunkFringeXZ& outgoingIrradianceX0 = chunk.irradianceFringeSkyX0;
    unsigned int outgoingIrradianceX0Dirty = 0;
    
    LightGridChunkFringeXZ& outgoingIrradianceX1 = chunk.irradianceFringeSkyX1;
    unsigned int outgoingIrradianceX1Dirty = 0;
    
    LightGridChunkFringeXZ& outgoingIrradianceZ0 = chunk.irradianceFringeSkyZ0;
    unsigned int outgoingIrradianceZ0Dirty = 0;
    
    LightGridChunkFringeXZ& outgoingIrradianceZ1 = chunk.irradianceFringeSkyZ1;
    unsigned int outgoingIrradianceZ1Dirty = 0;
    
    for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            updateWithDirtyFlag(outgoingIrradianceY.data[z][x], IRRADIANCE(x, 0, z), outgoingIrradianceYDirty);

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            updateWithDirtyFlag(outgoingIrradianceX0.data[y][z], IRRADIANCE(0, y, z), outgoingIrradianceX0Dirty);

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            updateWithDirtyFlag(outgoingIrradianceX1.data[y][z], IRRADIANCE(kLightGridChunkSizeXZ-1, y, z), outgoingIrradianceX1Dirty);

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            updateWithDirtyFlag(outgoingIrradianceZ0.data[y][x], IRRADIANCE(x, y, 0), outgoingIrradianceZ0Dirty);

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            updateWithDirtyFlag(outgoingIrradianceZ1.data[y][x], IRRADIANCE(x, y, kLightGridChunkSizeXZ-1), outgoingIrradianceZ1Dirty);

    if (outgoingIrradianceYDirty > kLightGridSkylightDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index + Vector3int32(0, -1, 0)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingSkylight;
    }
    
    if (outgoingIrradianceX0Dirty > kLightGridSkylightDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index + Vector3int32(-1, 0, 0)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingSkylight;
    }
    
    if (outgoingIrradianceX1Dirty > kLightGridSkylightDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index + Vector3int32(+1, 0, 0)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingSkylight;
    }
    
    if (outgoingIrradianceZ0Dirty > kLightGridSkylightDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index + Vector3int32(0, 0, -1)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingSkylight;
    }
    
    if (outgoingIrradianceZ1Dirty > kLightGridSkylightDirtyThreshold)
    {
        if (LightGridChunk* neighbor = getChunkByIndex(chunk.index + Vector3int32(0, 0, +1)))
            neighbor->dirty |= LightGridChunk::Dirty_LightingSkylight;
    }
    
#undef IRRADIANCE
}

void LightGrid::lightingUpdateSkylightRow(LightGridChunk& chunk, int y, int z, const unsigned char* powerCurveLUT)
{
#define IRRADIANCE(x, y, z) irradianceScratch[(y)+1][(z)+1][(x)+1]

    for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
    {
        int incoming00 = IRRADIANCE(x+1, y+1, z+1);
        int incoming10 = IRRADIANCE(x+0, y+1, z+1);
        int incoming20 = IRRADIANCE(x-1, y+1, z+1);
        int incoming01 = IRRADIANCE(x+1, y+1, z+0);
        int incoming21 = IRRADIANCE(x-1, y+1, z+0);
        int incoming02 = IRRADIANCE(x+1, y+1, z-1);
        int incoming12 = IRRADIANCE(x+0, y+1, z-1);
        int incoming22 = IRRADIANCE(x-1, y+1, z-1);
        
        int result = (incoming00 + incoming10 + incoming20 + incoming01 + incoming21 + incoming02 + incoming12 + incoming22 + 7) >> 3;
        
        // apply power curve to make dark highlights lighter
        chunk.lightingSkylight[y][z][x] = powerCurveLUT[result];
        
        IRRADIANCE(x, y, z) = max0(result - chunk.occupancy[y][z][x]);
    }

#undef IRRADIANCE
}

#ifdef SIMD_SSE2
void LightGrid::lightingUpdateSkylightRowSIMD(LightGridChunk& chunk, int y, int z, const unsigned char* powerCurveLUT)
{
#define IRRADIANCE(x, y, z) irradianceScratch[(y)+1][(z)+1][(x)+1]

    __m128i zero = _mm_setzero_si128();
    __m128i all255 = _mm_set1_epi8((char)255);
    __m128i all252 = _mm_set1_epi8((char)252);
    
    for (int x = 0; x < kLightGridChunkSizeXZ; x += 16)
    {
        __m128i occ = _mm_loadu_si128(reinterpret_cast<__m128i*>(&chunk.occupancy[y][z][x]));

        __m128i incoming00 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x+1, y+1, z+1));
        __m128i incoming10 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x+0, y+1, z+1));
        __m128i incoming20 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x-1, y+1, z+1));
        __m128i incoming01 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x+1, y+1, z+0));
        __m128i incoming21 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x-1, y+1, z+0));
        __m128i incoming02 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x+1, y+1, z-1));
        __m128i incoming12 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x+0, y+1, z-1));
        __m128i incoming22 = _mm_loadu_si128((__m128i*)&IRRADIANCE(x-1, y+1, z-1));
        
        __m128i avg0 = _mm_avg_epu8(_mm_avg_epu8(incoming10, incoming01), _mm_avg_epu8(incoming21, incoming12));
        __m128i avg1 = _mm_avg_epu8(_mm_avg_epu8(incoming00, incoming20), _mm_avg_epu8(incoming02, incoming22));
        __m128i avgr = _mm_avg_epu8(avg0, avg1);
        
        // apply power curve to make dark highlights lighter
        __m128i avgrinv = _mm_sub_epi8(all255, avgr);
        
        __m128i avgrinv1_0 = _mm_unpacklo_epi8(avgrinv, zero);
        __m128i avgrinv1_1 = _mm_unpackhi_epi8(avgrinv, zero);
        
        __m128i avgrinv2_0 = _mm_mullo_epi16(avgrinv1_0, avgrinv1_0);
        __m128i avgrinv2_1 = _mm_mullo_epi16(avgrinv1_1, avgrinv1_1);
        
        // note: mulhi gets the results right shifted by 16, an extra right shift by 8 is below
        __m128i avgrinv4_0 = _mm_mulhi_epu16(avgrinv2_0, avgrinv2_0);
        __m128i avgrinv4_1 = _mm_mulhi_epu16(avgrinv2_1, avgrinv2_1);

        __m128i avgrinvcurve = _mm_packus_epi16(_mm_srli_epi16(avgrinv4_0, 8), _mm_srli_epi16(avgrinv4_1, 8));
        __m128i avgrcurve = _mm_sub_epi8(all252, avgrinvcurve);

        // store results
        _mm_storeu_si128((__m128i*)&chunk.lightingSkylight[y][z][x], avgrcurve);

        // update irradiance for next slice
        __m128i irr = _mm_subs_epu8(avgr, occ);
        
        _mm_storeu_si128((__m128i*)&IRRADIANCE(x,y,z), irr);
    }
    
#undef IRRADIANCE
}
#endif

#ifdef SIMD_NEON
void LightGrid::lightingUpdateSkylightRowSIMD(LightGridChunk& chunk, int y, int z, const unsigned char* powerCurveLUT)
{
#define IRRADIANCE(x, y, z) irradianceScratch[(y)+1][(z)+1][(x)+1]

    uint8x16_t all255 = vdupq_n_u8(255);
    uint8x16_t all252 = vdupq_n_u8(252);
    
    for (int x = 0; x < kLightGridChunkSizeXZ; x += 16)
    {
        uint8x16_t occ = vld1q_u8(&chunk.occupancy[y][z][x]);

        uint8x16_t incoming00 = vld1q_u8(&IRRADIANCE(x+1, y+1, z+1));
        uint8x16_t incoming10 = vld1q_u8(&IRRADIANCE(x+0, y+1, z+1));
        uint8x16_t incoming20 = vld1q_u8(&IRRADIANCE(x-1, y+1, z+1));
        uint8x16_t incoming01 = vld1q_u8(&IRRADIANCE(x+1, y+1, z+0));
        uint8x16_t incoming21 = vld1q_u8(&IRRADIANCE(x-1, y+1, z+0));
        uint8x16_t incoming02 = vld1q_u8(&IRRADIANCE(x+1, y+1, z-1));
        uint8x16_t incoming12 = vld1q_u8(&IRRADIANCE(x+0, y+1, z-1));
        uint8x16_t incoming22 = vld1q_u8(&IRRADIANCE(x-1, y+1, z-1));
        
        uint8x16_t avg0 = vrhaddq_u8(vrhaddq_u8(incoming10, incoming01), vrhaddq_u8(incoming21, incoming12));
        uint8x16_t avg1 = vrhaddq_u8(vrhaddq_u8(incoming00, incoming20), vrhaddq_u8(incoming02, incoming22));
        uint8x16_t avgr = vrhaddq_u8(avg0, avg1);
        
        // apply power curve to make dark highlights lighter
        uint8x16_t avgrinv = vsubq_u8(all255, avgr);
        
        uint8x8_t avgrinv1_0 = vget_low_u8(avgrinv);
        uint8x8_t avgrinv1_1 = vget_high_u8(avgrinv);
        
        uint16x8_t avgrinv2_0 = vshrq_n_u16(vmull_u8(avgrinv1_0, avgrinv1_0), 8);
        uint16x8_t avgrinv2_1 = vshrq_n_u16(vmull_u8(avgrinv1_1, avgrinv1_1), 8);

        uint16x8_t avgrinv4_0 = vshrq_n_u16(vmulq_u16(avgrinv2_0, avgrinv2_0), 8);
        uint16x8_t avgrinv4_1 = vshrq_n_u16(vmulq_u16(avgrinv2_1, avgrinv2_1), 8);
        
        uint8x16_t avgrinvcurve = vcombine_u8(vmovn_u16(avgrinv4_0), vmovn_u16(avgrinv4_1));
        uint8x16_t avgrcurve = vsubq_u8(all252, avgrinvcurve);

        // store results
        vst1q_u8(&chunk.lightingSkylight[y][z][x], avgrcurve);

        // update irradiance for next slice
        uint8x16_t irr = vqsubq_u8(avgr, occ);
        
        vst1q_u8(&IRRADIANCE(x,y,z), irr);
    }
    
#undef IRRADIANCE
}
#endif

void LightGrid::lightingGetLights(std::vector<LightObject*>& lights, const Extents& chunkExtents, SpatialHashedScene* spatialHashedScene)
{
	RBXPROFILER_SCOPE("Render", "lightingGetLights");

    std::vector<CullableSceneNode*> nodes;
    spatialHashedScene->queryExtents(nodes, chunkExtents, CullableSceneNode::Flags_LightObject);
    
    for (CullableSceneNode* node: nodes)
    {
        LightObject* lobj = static_cast<LightObject*>(node);

        // Cull lights based on brightness
        if (lobj->getType() != LightObject::Type_None && lobj->getBrightness() > 0)
        {
            lights.push_back(lobj);
        }
    }
}

void LightGrid::lightingUpdateLightScratch(const LightGridChunk& chunk, const Extents& chunkExtents, LightObject* lobj)
{
	RBXPROFILER_SCOPE("Render", "lightingUpdateLightScratch");

    // Convert light data into chunk-local space
    Vector3 position = (lobj->getPosition() - chunkExtents.min()) / 4.f;
    Color3uint8 color = transformColor(Color3uint8(lobj->getColor()));
	float intensity = lobj->getBrightness();
	float range = lobj->getRange() / 4;

    // Compute light extents in terms of a chunk with an extra 1-cell wide border
    Extents extents = lobj->getExtents();

    Vector3int32 extentsMin = fastFloorInt32((extents.min() - chunkExtents.min()) / 4.f + Vector3(1.f, 1.f, 1.f)).max(Vector3int32(0, 0, 0));
    // A small negative offset is added to extentsMax before flooring to prevent updating extra layer of cells for perfectly aligned/sized lights
    Vector3int32 extentsMax = fastFloorInt32((extents.max() - chunkExtents.min()) / 4.f + Vector3(0.99f, 0.99f, 0.99f)).min(kLightGridChunkSize + Vector3int32(1, 1, 1));

    if (lobj->getType() == LightObject::Type_Point)
    {
        if (lobj->getShadowMap())
        {
            lightingComputeShadowMask(chunk, extentsMin, extentsMax, position, *lobj->getShadowMap());
            
            SIMDCALL(lightingUpdatePointLightScratch, <true>(extentsMin, extentsMax, position, range, color, intensity));
        }
        else
        {
            SIMDCALL(lightingUpdatePointLightScratch, <false>(extentsMin, extentsMax, position, range, color, intensity));
        }
    }
    else if (lobj->getType() == LightObject::Type_Spot)
    {
        if (lobj->getShadowMap())
        {
            lightingComputeShadowMask(chunk, extentsMin, extentsMax, position, *lobj->getShadowMap());
            
            SIMDCALL(lightingUpdateSpotLightScratch, <true>(extentsMin, extentsMax, position, range, lobj->getDirection(), lobj->getAngle(), color, intensity));
        }
        else
        {
            SIMDCALL(lightingUpdateSpotLightScratch, <false>(extentsMin, extentsMax, position, range, lobj->getDirection(), lobj->getAngle(), color, intensity));
        }
    }
    else if (lobj->getType() == LightObject::Type_Surface)
    {
        Vector4 axisU = lobj->getAxisU() / Vector4(1, 1, 1, 4);
        Vector4 axisV = lobj->getAxisV() / Vector4(1, 1, 1, 4);
        SIMDCALL(lightingUpdateSurfaceLightScratch, <false>(extentsMin, extentsMax, position, range, lobj->getDirection(), lobj->getAngle(), axisU, axisV, color, intensity));
    }
    else
    {
        RBXASSERT(false);
    }
}

void LightGrid::lightingComputeShadowMask(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, LightShadowMap& shadowMap)
{
	RBXPROFILER_SCOPE("Render", "lightingComputeShadowMask");
    
    Vector3int32 lightCell = fastFloorInt32(lightPosition);
    Vector3int32 extentsMinChunk = (extentsMin - Vector3int32(1, 1, 1)).max(Vector3int32(0, 0, 0));
    Vector3int32 extentsMaxChunk = (extentsMax - Vector3int32(1, 1, 1)).min(kLightGridChunkSize - Vector3int32(1, 1, 1));

    // Transfer shadow map data to shadow mask for all three axes
    lightingTransferShadowSliceToShadowMask<0>(extentsMinChunk, extentsMaxChunk, lightCell, shadowMap.size, shadowMap.sliceX, shadowMap.sliceX);
    lightingTransferShadowSliceToShadowMask<1>(extentsMinChunk, extentsMaxChunk, lightCell, shadowMap.size, shadowMap.sliceY0, shadowMap.sliceY1);
    lightingTransferShadowSliceToShadowMask<2>(extentsMinChunk, extentsMaxChunk, lightCell, shadowMap.size, shadowMap.sliceZ, shadowMap.sliceZ);

    // Update interior shadow mask
    lightingComputeShadowMaskXYZ(chunk, extentsMinChunk, extentsMaxChunk, lightCell, lightPosition - lightCell.toVector3());

    // Transfer shadow map slice from shadow mask for all three axes
    lightingTransferShadowMaskToShadowSlice<0>(chunk, extentsMinChunk, extentsMaxChunk, lightCell, shadowMap.size, shadowMap.sliceX, shadowMap.sliceX);
    lightingTransferShadowMaskToShadowSlice<1>(chunk, extentsMinChunk, extentsMaxChunk, lightCell, shadowMap.size, shadowMap.sliceY0, shadowMap.sliceY1);
    lightingTransferShadowMaskToShadowSlice<2>(chunk, extentsMinChunk, extentsMaxChunk, lightCell, shadowMap.size, shadowMap.sliceZ, shadowMap.sliceZ);

    // The code above fills all voxels inside the chunk, but for edge cases (light intersects the outer border) it does not fill three faces out of 6 of the border,
    // and does not fill any edges or corners in the outer border either. Since blurring needs those let's copy/interpolate them from the data that we have.
    lightingFixupShadowMaskBorder(extentsMin, extentsMax, lightCell);
}

void LightGrid::lightingFixupShadowMaskBorder(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell)
{
    // First, let's make sure that faces (minus edges or corners) are complete
    // Note that we have some face data from shadowmap transfer; lightCell checks detect those
    if (extentsMin.x == 0 && lightCell.x >= 0)
        for (int y = extentsMin.y; y <= extentsMax.y; ++y)
            for (int z = extentsMin.z; z <= extentsMax.z; ++z)
                shadowMask[y][z][0] = shadowMask[y][z][1];

    if (extentsMin.y == 0 && lightCell.y >= 0)
        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
                shadowMask[0][z][x] = shadowMask[1][z][x];

    if (extentsMin.z == 0 && lightCell.z >= 0)
        for (int y = extentsMin.y; y <= extentsMax.y; ++y)
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
                shadowMask[y][0][x] = shadowMask[y][1][x];

    if (extentsMax.x == kLightGridChunkSizeXZ + 1 && lightCell.x < kLightGridChunkSizeXZ)
        for (int y = extentsMin.y; y <= extentsMax.y; ++y)
            for (int z = extentsMin.z; z <= extentsMax.z; ++z)
                shadowMask[y][z][kLightGridChunkSizeXZ + 1] = shadowMask[y][z][kLightGridChunkSizeXZ];

    if (extentsMax.y == kLightGridChunkSizeY + 1 && lightCell.y < kLightGridChunkSizeY)
        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
                shadowMask[kLightGridChunkSizeY + 1][z][x] = shadowMask[kLightGridChunkSizeY][z][x];

    if (extentsMax.z == kLightGridChunkSizeXZ + 1 && lightCell.z < kLightGridChunkSizeXZ)
        for (int y = extentsMin.y; y <= extentsMax.y; ++y)
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
                shadowMask[y][kLightGridChunkSizeXZ + 1][x] = shadowMask[y][kLightGridChunkSizeXZ][x];

    // Now let's fill edges
    if (extentsMin.x == 0 || extentsMin.y == 0 || extentsMin.z == 0 || extentsMax.x == kLightGridChunkSizeXZ + 1 || extentsMax.y == kLightGridChunkSizeY + 1 || extentsMax.z == kLightGridChunkSizeXZ + 1)
    {
    #define SHADOW(x, y, z) shadowMask[(y)+1][(z)+1][(x)+1]

        // Fill horizontal edges
        for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
        {
            SHADOW(x, kLightGridChunkSizeY, -1) = (SHADOW(x, kLightGridChunkSizeY-1, -1) + SHADOW(x, kLightGridChunkSizeY, 0) + 1) / 2;
            SHADOW(x, kLightGridChunkSizeY, kLightGridChunkSizeXZ) = (SHADOW(x, kLightGridChunkSizeY-1, kLightGridChunkSizeXZ) + SHADOW(x, kLightGridChunkSizeY, kLightGridChunkSizeXZ-1) + 1) / 2;

            SHADOW(x, -1, -1) = (SHADOW(x, 0, -1) + SHADOW(x, -1, 0) + 1) / 2;
            SHADOW(x, -1, kLightGridChunkSizeXZ) = (SHADOW(x, 0, kLightGridChunkSizeXZ) + SHADOW(x, -1, kLightGridChunkSizeXZ-1) + 1) / 2;
        }

        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            SHADOW(-1, kLightGridChunkSizeY, z) = (SHADOW(-1, kLightGridChunkSizeY-1, z) + SHADOW(0, kLightGridChunkSizeY, z) + 1) / 2;
            SHADOW(kLightGridChunkSizeXZ, kLightGridChunkSizeY, z) = (SHADOW(kLightGridChunkSizeXZ, kLightGridChunkSizeY-1, z) + SHADOW(kLightGridChunkSizeXZ-1, kLightGridChunkSizeY, z) + 1) / 2;

            SHADOW(-1, -1, z) = (SHADOW(-1, 0, z) + SHADOW(0, -1, z) + 1) / 2;
            SHADOW(kLightGridChunkSizeXZ, -1, z) = (SHADOW(kLightGridChunkSizeXZ, 0, z) + SHADOW(kLightGridChunkSizeXZ-1, -1, z) + 1) / 2;
        }
        
        // Fill vertical edges & corners
        for (int y = -1; y < kLightGridChunkSizeY + 1; ++y)
        {
            SHADOW(-1, y, -1) = (SHADOW(-1, y, 0) + SHADOW(0, y, -1) + 1) / 2;
            SHADOW(kLightGridChunkSizeXZ, y, -1) = (SHADOW(kLightGridChunkSizeXZ, y, 0) + SHADOW(kLightGridChunkSizeXZ-1, y, -1) + 1) / 2;
            SHADOW(-1, y, kLightGridChunkSizeXZ) = (SHADOW(-1, y, kLightGridChunkSizeXZ-1) + SHADOW(0, y, kLightGridChunkSizeXZ) + 1) / 2;
            SHADOW(kLightGridChunkSizeXZ, y, kLightGridChunkSizeXZ) = (SHADOW(kLightGridChunkSizeXZ, y, kLightGridChunkSizeXZ-1) + SHADOW(kLightGridChunkSizeXZ-1, y, kLightGridChunkSizeXZ) + 1) / 2;
        }

    #undef SHADOW
    }
}

template <int Axis> void LightGrid::lightingTransferShadowSliceToShadowMask(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, int shadowMapSize, const LightShadowSlice& shadowSlice0, const LightShadowSlice& shadowSlice1)
{
    if (shadowMapSize == 0 || !shadowSlice0.data || !shadowSlice1.data)
        return;

    // YZX; X axis => YZ, Y axis => ZX, Z axis => YX
    const int PlaneU = (Axis == 1) ? 2 : 1;
    const int PlaneV = (Axis == 0) ? 2 : 0;

    if (lightCell[Axis] < 0 || lightCell[Axis] >= kLightGridChunkSize[Axis])
    {
        const LightShadowSlice& shadowSlice = (lightCell[Axis] < 0 ? shadowSlice1 : shadowSlice0);
        int axisOffset = (lightCell[Axis] < 0 ? -1 : kLightGridChunkSize[Axis]);

        // Shadow map is odd and symmetric around light center; makes math easier
        RBXASSERT(shadowMapSize % 2 == 1);

        // Get shadow region offsets (relative to array start, not light center)
        int shadowOffsetU = lightCell[PlaneU] - shadowMapSize / 2;
        int shadowOffsetV = lightCell[PlaneV] - shadowMapSize / 2;

        // Intersect shadow region and extents in UV projection
        int minU = std::max(lightCell[PlaneU] - shadowMapSize / 2, extentsMin[PlaneU]);
        int minV = std::max(lightCell[PlaneV] - shadowMapSize / 2, extentsMin[PlaneV]);

        int maxU = std::min(lightCell[PlaneU] + shadowMapSize / 2, extentsMax[PlaneU]);
        int maxV = std::min(lightCell[PlaneV] + shadowMapSize / 2, extentsMax[PlaneV]);

        for (int u = minU; u <= maxU; ++u)
        {
            const unsigned char* shadowRow = shadowSlice.data.get() + (u - shadowOffsetU) * shadowMapSize;

            for (int v = minV; v <= maxV; ++v)
            {
                int y = (Axis == 1) ? axisOffset : u;
                int z = (Axis == 0) ? v : (Axis == 1) ? u : axisOffset;
                int x = (Axis == 0) ? axisOffset : v;

                shadowMask[y+1][z+1][x+1] = shadowRow[v - shadowOffsetV];
            }
        }
    }
}

template <int Axis> void LightGrid::lightingTransferShadowMaskToShadowSlice(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, int shadowMapSize, LightShadowSlice& shadowSlice0, LightShadowSlice& shadowSlice1)
{
    if (shadowMapSize == 0 || !shadowSlice0.data || !shadowSlice1.data)
        return;

    // YZX; X axis => YZ, Y axis => ZX, Z axis => YX
    const int PlaneU = (Axis == 1) ? 2 : 1;
    const int PlaneV = (Axis == 0) ? 2 : 0;

    bool intersectsMin = extentsMin[Axis] == 0 && lightCell[Axis] >= 0;
    bool intersectsMax = extentsMax[Axis] == kLightGridChunkSize[Axis] - 1 && lightCell[Axis] < kLightGridChunkSize[Axis];

    // If there is just one shadowmap slice, there should be at most one intersection
    RBXASSERT(!(&shadowSlice0 == &shadowSlice1 && intersectsMin && intersectsMax));

    for (int slice = 0; slice < 2; ++slice)
    {
        bool intersects = (slice == 0) ? intersectsMin : intersectsMax;

        if (intersects)
        {
            LightShadowSlice& shadowSlice = (slice == 0) ? shadowSlice0 : shadowSlice1;
            int axisOffset = (slice == 0) ? 0 : kLightGridChunkSize[Axis]-1;

            // Shadow map is odd and symmetric around light center; makes math easier
            RBXASSERT(shadowMapSize % 2 == 1);

            // Get shadow region offsets (relative to array start, not light center)
            int shadowOffsetU = lightCell[PlaneU] - shadowMapSize / 2;
            int shadowOffsetV = lightCell[PlaneV] - shadowMapSize / 2;

            // Intersect shadow region and extents in UV projection
            int minU = std::max(lightCell[PlaneU] - shadowMapSize / 2, extentsMin[PlaneU]);
            int minV = std::max(lightCell[PlaneV] - shadowMapSize / 2, extentsMin[PlaneV]);

            int maxU = std::min(lightCell[PlaneU] + shadowMapSize / 2, extentsMax[PlaneU]);
            int maxV = std::min(lightCell[PlaneV] + shadowMapSize / 2, extentsMax[PlaneV]);

            unsigned int dirty = 0;

            for (int u = minU; u <= maxU; ++u)
            {
                unsigned char* shadowRow = shadowSlice.data.get() + (u - shadowOffsetU) * shadowMapSize;

                for (int v = minV; v <= maxV; ++v)
                {
                    int y = (Axis == 1) ? axisOffset : u;
                    int z = (Axis == 0) ? v : (Axis == 1) ? u : axisOffset;
                    int x = (Axis == 0) ? axisOffset : v;

                    updateWithDirtyFlag(shadowRow[v - shadowOffsetV], shadowMask[y+1][z+1][x+1], dirty);
                }
            }

            if (dirty > kLightGridShadowDirtyThreshold)
            {
                Vector3int32 neighborIndex = chunk.index;
                neighborIndex[Axis] += (slice == 0) ? -1 : +1;

                if (LightGridChunk* neighbor = getChunkByIndex(neighborIndex))
                    neighbor->dirty |= LightGridChunk::Dirty_LightingLocalShadowed;
            }
        }
    }
}

void LightGrid::lightingComputeShadowMaskXYZ(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
    if (lightCell.x >= extentsMin.x)
    {
        lightingComputeShadowMaskYZ<true>(chunk, extentsMin, Vector3int32(std::min(extentsMax.x, lightCell.x), extentsMax.y, extentsMax.z), lightCell, lightFraction);
    }

    if (lightCell.x < extentsMax.x)
    {
        lightingComputeShadowMaskYZ<false>(chunk, Vector3int32(std::max(extentsMin.x, lightCell.x + 1), extentsMin.y, extentsMin.z), extentsMax, lightCell, lightFraction);
    }
}

template <bool NegX> void LightGrid::lightingComputeShadowMaskYZ(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
    if (lightCell.y >= extentsMin.y)
    {
        lightingComputeShadowMaskZ<NegX, true>(chunk, extentsMin, Vector3int32(extentsMax.x, std::min(extentsMax.y, lightCell.y), extentsMax.z), lightCell, lightFraction);
    }

    if (lightCell.y < extentsMax.y)
    {
        lightingComputeShadowMaskZ<NegX, false>(chunk, Vector3int32(extentsMin.x, std::max(extentsMin.y, lightCell.y + 1), extentsMin.z), extentsMax, lightCell, lightFraction);
    }
}

template <bool NegX, bool NegY> void LightGrid::lightingComputeShadowMaskZ(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
    if (lightCell.z >= extentsMin.z)
    {
        lightingComputeShadowMask<NegX, NegY, true>(chunk, extentsMin, Vector3int32(extentsMax.x, extentsMax.y, std::min(extentsMax.z, lightCell.z)), lightCell, lightFraction);
    }

    if (lightCell.z < extentsMax.z)
    {
        lightingComputeShadowMask<NegX, NegY, false>(chunk, Vector3int32(extentsMin.x, extentsMin.y, std::max(extentsMin.z, lightCell.z + 1)), extentsMax, lightCell, lightFraction);
    }
}

template <bool NegX, bool NegY, bool NegZ> void LightGrid::lightingComputeShadowMask(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
    // Only use smooth SIMD version since smooth scalar version is 2x slower than LUT
#ifdef SIMD_SSE2
    if (useSIMD)
        lightingComputeShadowMaskImplSmoothSIMD<NegX, NegY, NegZ>(chunk, extentsMin, extentsMax, lightCell, lightFraction);
    else
#endif
        lightingComputeShadowMaskImplLUT<NegX, NegY, NegZ>(chunk, extentsMin, extentsMax, lightCell, lightFraction);
}

static int getLUTOffset(const Vector3int32& lightCell, int axis)
{
    if (lightCell[axis] < 0)
        return -lightCell[axis];
    else if (lightCell[axis] >= kLightGridChunkSize[axis])
        return lightCell[axis] - kLightGridChunkSize[axis] + 1;
    else
        return 0;
}

template <bool NegX, bool NegY, bool NegZ> void LightGrid::lightingComputeShadowMaskImplLUT(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
#define SHADOW(x, y, z) shadowMask[(y)+1][(z)+1][(x)+1]
    
    int stepX = NegX ? 1 : -1;
    int stepY = NegY ? 1 : -1;
    int stepZ = NegZ ? 1 : -1;

    // Verify that all accesses in the loops don't go out of chunk bounds
    RBXASSERT(extentsMin.x >= 0 && extentsMin.y >= 0 && extentsMin.z >= 0);
    RBXASSERT(extentsMax.x < kLightGridChunkSizeXZ && extentsMax.y < kLightGridChunkSizeY && extentsMax.z < kLightGridChunkSizeXZ);

    // It's critical that we only use edge (xi/yi/zi == 0) values if the light source is outside the chunk
    int lutOffsetX = getLUTOffset(lightCell, 0);
    int lutOffsetY = getLUTOffset(lightCell, 1);
    int lutOffsetZ = getLUTOffset(lightCell, 2);

    Vector3int32 size = extentsMax - extentsMin;

    for (int yi = 0; yi <= size.y; ++yi)
    {
        int y = NegY ? extentsMax.y - yi : extentsMin.y + yi;
        int lutY = yi + lutOffsetY;
        
        for (int zi = 0; zi <= size.z; ++zi)
        {
            int z = NegZ ? extentsMax.z - zi : extentsMin.z + zi;
            int lutZ = zi + lutOffsetZ;
            
            for (int xi = 0; xi <= size.x; ++xi)
            {
                int x = NegX ? extentsMax.x - xi : extentsMin.x + xi;
                int lutX = xi + lutOffsetX;
                
                int shadowX = SHADOW(x + stepX, y, z);
                int shadowY = SHADOW(x, y + stepY, z);
                int shadowZ = SHADOW(x, y, z + stepZ);

                const ShadowLUTEntry& contrib = shadowLUT[lutY][lutZ][lutX];

                int result = ((shadowX * contrib.x + shadowY * contrib.y + shadowZ * contrib.z + 127) >> 7) + contrib.add;

                SHADOW(x, y, z) = max0(result - chunk.occupancy[y][z][x]);
            }
        }
    }

#undef SHADOW
}

template <bool NegX, bool NegY, bool NegZ> void LightGrid::lightingComputeShadowMaskImplSmooth(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
#define SHADOW(x, y, z) shadowMask[(y)+1][(z)+1][(x)+1]
    
    int stepX = NegX ? 1 : -1;
    int stepY = NegY ? 1 : -1;
    int stepZ = NegZ ? 1 : -1;

    // Verify that all accesses in the loops don't go out of chunk bounds
    RBXASSERT(extentsMin.x >= 0 && extentsMin.y >= 0 && extentsMin.z >= 0);
    RBXASSERT(extentsMax.x < kLightGridChunkSizeXZ && extentsMax.y < kLightGridChunkSizeY && extentsMax.z < kLightGridChunkSizeXZ);

    // It's critical that we only use edge (xi/yi/zi == 0) values if the light source is outside the chunk
    int lutOffsetX = getLUTOffset(lightCell, 0);
    int lutOffsetY = getLUTOffset(lightCell, 1);
    int lutOffsetZ = getLUTOffset(lightCell, 2);

    Vector3int32 size = extentsMax - extentsMin;

    float fractionX = NegX ? lightFraction.x : -lightFraction.x;
    float fractionY = NegY ? lightFraction.y : -lightFraction.y;
    float fractionZ = NegZ ? lightFraction.z : -lightFraction.z;
    
    for (int yi = 0; yi <= size.y; ++yi)
    {
        int y = NegY ? extentsMax.y - yi : extentsMin.y + yi;
        int lutY = yi + lutOffsetY;

        float contribY = (lutY == 0) ? 0 : (lutY + fractionY) * (lutY + fractionY);
        
        for (int zi = 0; zi <= size.z; ++zi)
        {
            int z = NegZ ? extentsMax.z - zi : extentsMin.z + zi;
            int lutZ = zi + lutOffsetZ;
            
            float contribZ = (lutZ == 0) ? 0 : (lutZ + fractionZ) * (lutZ + fractionZ);

            for (int xi = 0; xi <= size.x; ++xi)
            {
                int x = NegX ? extentsMax.x - xi : extentsMin.x + xi;
                int lutX = xi + lutOffsetX;

                float contribX = (lutX == 0) ? 0 : (lutX + fractionX) * (lutX + fractionX);
                
                int shadowX = SHADOW(x + stepX, y, z);
                int shadowY = SHADOW(x, y + stepY, z);
                int shadowZ = SHADOW(x, y, z + stepZ);

                float contribSum = contribX + contribY + contribZ;

                // We never use the 0,0,0 cell from any of (up to 8) regions of update
                // This is mostly to guarantee that the part owner of the light does not occlude the light if it's small
                int result =
                    (lutX == 0 && lutY == 0 && lutZ == 0)
                    ? 255
                    : ((shadowX * contribX + shadowY * contribY + shadowZ * contribZ) / contribSum + 0.5f);

                SHADOW(x, y, z) = max0(result - chunk.occupancy[y][z][x]);
            }
        }
    }

#undef SHADOW
}

#ifdef SIMD_SSE2
template <bool NegX, bool NegY, bool NegZ> void LightGrid::lightingComputeShadowMaskImplSmoothSIMD(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction)
{
#define SHADOW(x, y, z) shadowMask[(y)+1][(z)+1][(x)+1]
    
    int stepX = NegX ? 1 : -1;
    int stepY = NegY ? 1 : -1;
    int stepZ = NegZ ? 1 : -1;

    // Verify that all accesses in the loops don't go out of chunk bounds
    RBXASSERT(extentsMin.x >= 0 && extentsMin.y >= 0 && extentsMin.z >= 0);
    RBXASSERT(extentsMax.x < kLightGridChunkSizeXZ && extentsMax.y < kLightGridChunkSizeY && extentsMax.z < kLightGridChunkSizeXZ);

    // It's critical that we only use edge (xi/yi/zi == 0) values if the light source is outside the chunk
    int lutOffsetX = getLUTOffset(lightCell, 0);
    int lutOffsetY = getLUTOffset(lightCell, 1);
    int lutOffsetZ = getLUTOffset(lightCell, 2);

    Vector3int32 size = extentsMax - extentsMin;

    float fractionX = NegX ? lightFraction.x : -lightFraction.x;
    float fractionY = NegY ? lightFraction.y : -lightFraction.y;
    float fractionZ = NegZ ? lightFraction.z : -lightFraction.z;

    __m128 contribXStart1 =
        _mm_add_ps(
            NegX ? _mm_setr_ps(4, 3, 2, 1) : _mm_setr_ps(1, 2, 3, 4),
            _mm_set1_ps(lutOffsetX + fractionX));

    __m128 contribXAdd = _mm_set1_ps(4.f);

    for (int yi = 0; yi <= size.y; ++yi)
    {
        int y = NegY ? extentsMax.y - yi : extentsMin.y + yi;
        int lutY = yi + lutOffsetY;

        float contribY = (lutY == 0) ? 0 : (lutY + fractionY) * (lutY + fractionY);
        __m128 contribY4 = _mm_set1_ps(contribY);
        
        for (int zi = 0; zi <= size.z; ++zi)
        {
            int z = NegZ ? extentsMax.z - zi : extentsMin.z + zi;
            int lutZ = zi + lutOffsetZ;
            
            float contribZ = (lutZ == 0) ? 0 : (lutZ + fractionZ) * (lutZ + fractionZ);

            {
                int xi = 0;
                int x = NegX ? extentsMax.x - xi : extentsMin.x + xi;
                int lutX = xi + lutOffsetX;

                float contribX = (lutX == 0) ? 0 : (lutX + fractionX) * (lutX + fractionX);
                
                int shadowX = SHADOW(x + stepX, y, z);
                int shadowY = SHADOW(x, y + stepY, z);
                int shadowZ = SHADOW(x, y, z + stepZ);

                float contribSum = contribX + contribY + contribZ;

                // We never use the 0,0,0 cell from any of (up to 8) regions of update
                // This is mostly to guarantee that the part owner of the light does not occlude the light if it's small
                int result =
                    (lutX == 0 && lutY == 0 && lutZ == 0)
                    ? 255
                    : ((shadowX * contribX + shadowY * contribY + shadowZ * contribZ) / contribSum + 0.5f);

                SHADOW(x, y, z) = max0(result - chunk.occupancy[y][z][x]);
            }

            __m128 contribZ4 = _mm_set1_ps(contribZ);
            __m128 contribX = contribXStart1;

            for (int xi = 1; xi <= size.x; xi += 4)
            {
                int x = NegX ? extentsMax.x - xi - 3 : extentsMin.x + xi;

                __m128i shadowY1 = _mm_loadu_si128((const __m128i*)&SHADOW(x, y + stepY, z));
                __m128i shadowY2 = _mm_unpacklo_epi8(shadowY1, _mm_setzero_si128());
                __m128i shadowY3 = _mm_unpacklo_epi8(shadowY2, _mm_setzero_si128());
                __m128 shadowY = _mm_cvtepi32_ps(shadowY3);

                __m128i shadowZ1 = _mm_loadu_si128((const __m128i*)&SHADOW(x, y, z + stepZ));
                __m128i shadowZ2 = _mm_unpacklo_epi8(shadowZ1, _mm_setzero_si128());
                __m128i shadowZ3 = _mm_unpacklo_epi8(shadowZ2, _mm_setzero_si128());
                __m128 shadowZ = _mm_cvtepi32_ps(shadowZ3);

                __m128i occupancy1 = _mm_loadu_si128((const __m128i*)&chunk.occupancy[y][z][x]);
                __m128i occupancy2 = _mm_unpacklo_epi8(occupancy1, _mm_setzero_si128());
                __m128i occupancy3 = _mm_unpacklo_epi8(occupancy2, _mm_setzero_si128());
                __m128 occupancy = _mm_cvtepi32_ps(occupancy3);

                __m128 contribX4 = _mm_mul_ps(contribX, contribX);

                __m128 contribSum = _mm_add_ps(contribX4, _mm_add_ps(contribY4, contribZ4));

                __m128 shadowYContrib = _mm_mul_ps(shadowY, contribY4);
                __m128 shadowZContrib = _mm_mul_ps(shadowZ, contribZ4);
                __m128 shadowYZContrib = _mm_sub_ps(_mm_div_ps(_mm_add_ps(shadowYContrib, shadowZContrib), contribSum), occupancy);

                __m128 contribX4Norm = _mm_div_ps(contribX4, contribSum);

            #define VSPLAT(v, c) _mm_shuffle_ps(v, v, _MM_SHUFFLE(c,c,c,c))
            #define STEP(i) \
                do { \
                    __m128 result = _mm_add_ss(_mm_mul_ss(shadow, VSPLAT(contribX4Norm, i)), VSPLAT(shadowYZContrib, i)); \
                    shadow = _mm_max_ss(result, _mm_setzero_ps()); \
                    SHADOW(x + i, y, z) = _mm_cvtss_si32(shadow); \
                } while (0)

                if (NegX)
                {
                    __m128 shadow = _mm_set_ss(SHADOW(x + 4, y, z));

                    STEP(3); STEP(2); STEP(1); STEP(0);
                }
                else
                {
                    __m128 shadow = _mm_set_ss(SHADOW(x - 1, y, z));

                    STEP(0); STEP(1); STEP(2); STEP(3);
                }

            #undef STEP
            #undef VSPLAT

                contribX = _mm_add_ps(contribX, contribXAdd);
            }
        }
    }

#undef SHADOW
}
#endif

void LightGrid::precomputeShadowLUT()
{
    for (int y = 0; y < kShadowLUTSize; ++y)
        for (int z = 0; z < kShadowLUTSize; ++z)
            for (int x = 0; x < kShadowLUTSize; ++x)
            {
                ShadowLUTEntry& e = shadowLUT[y][z][x];

                Vector3 contribf = Vector3(x + 0.5f, y + 0.5f, z + 0.5f);

                // cancel out contributions from edges
                // note; it's better to do it after integer renormalization, but this is easier
                if (x == 0)
                    contribf.x = 0;

                if (y == 0)
                    contribf.y = 0;

                if (z == 0)
                    contribf.z = 0;

                // Use approximate weights so that LUT and Smooth versions match
                Vector3int32 contrib = getLightContributionApprox(normalize(contribf), 7);

                // We never use the 0,0,0 cell from any of (up to 8) regions of update
                // This is mostly to guarantee that the part owner of the light does not occlude the light if it's small
                if (x == 0 && y == 0 && z == 0)
                {
                    e.x = 0;
                    e.y = 0;
                    e.z = 0;
                    e.add = 255;
                }
                else
                {
                    e.x = contrib.x;
                    e.y = contrib.y;
                    e.z = contrib.z;
                    e.add = 0;
                }
            }
}

template <bool Shadows> void LightGrid::lightingUpdatePointLightScratch(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Color3uint8& lightColor, float lightIntensity)
{
    float lightRadiusInvSq = 1 / (lightRadius * lightRadius);
    
    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
            {
                Vector3 cellPosition = Vector3((x - 1) + 0.5f, (y - 1) + 0.5f, (z - 1) + 0.5f);
            
                float distAttenuation = 1 - (lightPosition - cellPosition).squaredLength() * lightRadiusInvSq;
                float attenuation = saturate(distAttenuation * lightIntensity);
                
                if (Shadows)
                    attenuation *= shadowMask[y][z][x] / 255.f;
                
                int factor = attenuation * 65536.0f + 0.5f;
                int color = (((lightColor.r * factor) >> 16) << 20) + (((lightColor.g * factor) >> 16) << 10) + (((lightColor.b * factor) >> 16) << 0);
                
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                int value = lightingScratch[y][z][x] + color;
                int carry = value & 0x10040100;
                
                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                lightingScratch[y][z][x] = (value & 0xff3fcff) | (carry - (carry >> 8));
            }
        }
    }
}

#ifdef SIMD_SSE2
template <bool Shadows> void LightGrid::lightingUpdatePointLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Color3uint8& lightColor, float lightIntensity_)
{
    __m128 zero = _mm_setzero_ps();
    __m128 one = _mm_set1_ps(1.f);
    __m128 half = _mm_set1_ps(0.5f);
    __m128 offsetX = _mm_setr_ps(-0.5f, 0.5f, 1.5f, 2.5f);

    __m128 lightRadiusInvSq = _mm_set1_ps(1 / (lightRadius * lightRadius));
    __m128 lightIntensity = _mm_set1_ps(lightIntensity_);
    __m128 lightPositionX = _mm_set1_ps(lightPosition.x);
    __m128 lightPositionY = _mm_set1_ps(lightPosition.y);
    __m128 lightPositionZ = _mm_set1_ps(lightPosition.z);
    
    __m128 colorR = _mm_set1_ps(lightColor.r << 20);
    __m128 colorG = _mm_set1_ps(lightColor.g << 10);
    __m128 colorB = _mm_set1_ps(lightColor.b);

    __m128i maskR = _mm_set1_epi32(0xff00000);
    __m128i maskG = _mm_set1_epi32(0x3fc00);
    __m128i maskB = _mm_set1_epi32(0xff);

    __m128i carryMask = _mm_set1_epi32(0x10040100);
    __m128i mask = _mm_set1_epi32(0xff3fcff);

    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        // lightPosition.y - cellPosition.y
        __m128 cellVectorY = _mm_sub_ps(_mm_sub_ps(_mm_cvtepi32_ps(_mm_set1_epi32(y)), half), lightPositionY);

        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            // lightPosition.z - cellPosition.z
            __m128 cellVectorZ = _mm_sub_ps(_mm_sub_ps(_mm_cvtepi32_ps(_mm_set1_epi32(z)), half), lightPositionZ);

            __m128 cellVectorYZDSq = _mm_add_ps(_mm_mul_ps(cellVectorY, cellVectorY), _mm_mul_ps(cellVectorZ, cellVectorZ));

            for (int x = extentsMin.x; x <= extentsMax.x; x += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                __m128 cellVectorX = _mm_sub_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(x)), offsetX), lightPositionX);

                __m128 distSquared = _mm_add_ps(_mm_mul_ps(cellVectorX, cellVectorX), cellVectorYZDSq);

                __m128 distAttenuation = _mm_sub_ps(one, _mm_mul_ps(distSquared, lightRadiusInvSq));
                __m128 attenuation = _mm_max_ps(zero, _mm_min_ps(one, _mm_mul_ps(distAttenuation, lightIntensity)));

                if (Shadows)
                {
                    // Load first 4 bytes and expand to 4 floats
                    __m128i shadowMask1 = _mm_loadu_si128((__m128i*)&shadowMask[y][z][x]);
                    __m128i shadowMask2 = _mm_unpacklo_epi8(shadowMask1, _mm_setzero_si128());
                    __m128i shadowMask3 = _mm_unpacklo_epi8(shadowMask2, _mm_setzero_si128());
                    __m128 shadowMask = _mm_cvtepi32_ps(shadowMask3);

                    // Combine shadow mask with attenuation
                    attenuation = _mm_mul_ps(attenuation, _mm_mul_ps(shadowMask, _mm_set1_ps(1.f / 255.f)));
                }

                __m128i chanR = _mm_and_si128(maskR, _mm_cvttps_epi32(_mm_mul_ps(colorR, attenuation)));
                __m128i chanG = _mm_and_si128(maskG, _mm_cvttps_epi32(_mm_mul_ps(colorG, attenuation)));
                __m128i chanB = _mm_and_si128(maskB, _mm_cvttps_epi32(_mm_mul_ps(colorB, attenuation)));

                __m128i color = _mm_or_si128(chanR, _mm_or_si128(chanG, chanB));
            
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                __m128i lighting = _mm_loadu_si128((__m128i*)&lightingScratch[y][z][x]);

                __m128i value = _mm_add_epi32(lighting, color);
                __m128i carry = _mm_and_si128(value, carryMask);

                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                __m128i result = _mm_or_si128(_mm_and_si128(value, mask), _mm_sub_epi32(carry, _mm_srli_epi32(carry, 8)));

                _mm_storeu_si128((__m128i*)&lightingScratch[y][z][x], result);
            }
        }
    }
}
#endif

#ifdef SIMD_NEON
template <bool Shadows> void LightGrid::lightingUpdatePointLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Color3uint8& lightColor, float lightIntensity_)
{
    float32x4_t zero = vdupq_n_f32(0);
    float32x4_t one = vdupq_n_f32(1.f);
    float32x4_t half = vdupq_n_f32(0.5f);
    float32x4_t offsetX = {-0.5f, 0.5f, 1.5f, 2.5f};

    float32x4_t lightRadiusInvSq = vdupq_n_f32(1 / (lightRadius * lightRadius));
    float32x4_t lightIntensity = vdupq_n_f32(lightIntensity_);
    float32x4_t lightPositionX = vdupq_n_f32(lightPosition.x);
    float32x4_t lightPositionY = vdupq_n_f32(lightPosition.y);
    float32x4_t lightPositionZ = vdupq_n_f32(lightPosition.z);
    
    float32x4_t colorR = vdupq_n_f32(lightColor.r << 20);
    float32x4_t colorG = vdupq_n_f32(lightColor.g << 10);
    float32x4_t colorB = vdupq_n_f32(lightColor.b);

    uint32x4_t maskR = vdupq_n_u32(0xff00000);
    uint32x4_t maskG = vdupq_n_u32(0x3fc00);
    uint32x4_t maskB = vdupq_n_u32(0xff);

    uint32x4_t carryMask = vdupq_n_u32(0x10040100);
    uint32x4_t mask = vdupq_n_u32(0xff3fcff);

    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        // lightPosition.y - cellPosition.y
        float32x2_t yy = vcvt_f32_s32(vdup_n_s32(y));
        float32x4_t cellVectorY = vsubq_f32(vsubq_f32(vcombine_f32(yy, yy), half), lightPositionY);

        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            // lightPosition.z - cellPosition.z
            float32x2_t zz = vcvt_f32_s32(vdup_n_s32(z));
            float32x4_t cellVectorZ = vsubq_f32(vsubq_f32(vcombine_f32(zz, zz), half), lightPositionZ);

            float32x4_t cellVectorYZDSq = vmlaq_f32(vmulq_f32(cellVectorY, cellVectorY), cellVectorZ, cellVectorZ);

            for (int x = extentsMin.x; x <= extentsMax.x; x += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                float32x2_t xx = vcvt_f32_s32(vdup_n_s32(x));
                float32x4_t cellVectorX = vsubq_f32(vaddq_f32(vcombine_f32(xx, xx), offsetX), lightPositionX);

                float32x4_t distSquared = vmlaq_f32(cellVectorYZDSq, cellVectorX, cellVectorX);

                float32x4_t distAttenuation = vmlsq_f32(one, distSquared, lightRadiusInvSq);
                float32x4_t attenuation = vmaxq_f32(zero, vminq_f32(one, vmulq_f32(distAttenuation, lightIntensity)));

                if (Shadows)
                {
                    uint16x8_t shadowMask1 = vmovl_u8(vld1_u8(&shadowMask[y][z][x]));
                    uint32x4_t shadowMask2 = vmovl_u16(vget_low_u16(shadowMask1));
                    float32x4_t shadowMask = vcvtq_f32_u32(shadowMask2);

                    attenuation = vmulq_f32(attenuation, vmulq_n_f32(shadowMask, 1.f / 255.f));
                }

                uint32x4_t chanR = vandq_u32(maskR, vcvtq_u32_f32(vmulq_f32(colorR, attenuation)));
                uint32x4_t chanG = vandq_u32(maskG, vcvtq_u32_f32(vmulq_f32(colorG, attenuation)));
                uint32x4_t chanB = vandq_u32(maskB, vcvtq_u32_f32(vmulq_f32(colorB, attenuation)));

                uint32x4_t color = vorrq_u32(chanR, vorrq_u32(chanG, chanB));
            
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                uint32x4_t lighting = vld1q_u32(&lightingScratch[y][z][x]);

                uint32x4_t value = vaddq_u32(lighting, color);
                uint32x4_t carry = vandq_u32(value, carryMask);

                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                uint32x4_t result = vorrq_u32(vandq_u32(value, mask), vsubq_u32(carry, vshrq_n_u32(carry, 8)));

                vst1q_u32(&lightingScratch[y][z][x], result);
            }
        }
    }
}
#endif

template <bool Shadows> void LightGrid::lightingUpdateSpotLightScratch(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Color3uint8& lightColor, float lightIntensity)
{
    float lightRadiusInvSq = 1 / (lightRadius * lightRadius);
    float lightConeHalfAngleCos = cosf(G3D::toRadians(lightConeAngle / 2));
    float lightConeAttenuationScale = lightConeHalfAngleCos < 1.f ? 1.f / (1.f - lightConeHalfAngleCos) : 0.f;
    
    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
            {
                Vector3 cellPosition = Vector3((x - 1) + 0.5f, (y - 1) + 0.5f, (z - 1) + 0.5f);
            
                float distAttenuation = 1 - (lightPosition - cellPosition).squaredLength() * lightRadiusInvSq;
                float coneAttenuation = (dot(lightDirection, normalize(cellPosition - lightPosition)) - lightConeHalfAngleCos) * lightConeAttenuationScale;
                float attenuation = saturate(distAttenuation * std::max(0.f, coneAttenuation) * lightIntensity);

                if (Shadows)
                    attenuation *= shadowMask[y][z][x] / 255.f;
                
                int factor = attenuation * 65536.0f + 0.5f;
                int color = (((lightColor.r * factor) >> 16) << 20) + (((lightColor.g * factor) >> 16) << 10) + (((lightColor.b * factor) >> 16) << 0);
                
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                int value = lightingScratch[y][z][x] + color;
                int carry = value & 0x10040100;
                
                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                lightingScratch[y][z][x] = (value & 0xff3fcff) | (carry - (carry >> 8));
            }
        }
    }
}

#ifdef SIMD_SSE2
template <bool Shadows> void LightGrid::lightingUpdateSpotLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Color3uint8& lightColor, float lightIntensity_)
{
    __m128 zero = _mm_setzero_ps();
    __m128 one = _mm_set1_ps(1.f);
    __m128 half = _mm_set1_ps(0.5f);
    __m128 offsetX = _mm_setr_ps(-0.5f, 0.5f, 1.5f, 2.5f);

    float lightConeHalfAngleCos_ = cosf(G3D::toRadians(lightConeAngle / 2));

    __m128 lightRadiusInvSq = _mm_set1_ps(1 / (lightRadius * lightRadius));
    __m128 lightConeHalfAngleCos = _mm_set1_ps(lightConeHalfAngleCos_);
    __m128 lightConeAttenuationScale = _mm_set1_ps(lightConeHalfAngleCos_ < 1.f ? 1.f / (1.f - lightConeHalfAngleCos_) : 0.f);

    __m128 lightIntensity = _mm_set1_ps(lightIntensity_);
    __m128 lightPositionX = _mm_set1_ps(lightPosition.x);
    __m128 lightPositionY = _mm_set1_ps(lightPosition.y);
    __m128 lightPositionZ = _mm_set1_ps(lightPosition.z);
    
    __m128 lightDirectionX = _mm_set1_ps(lightDirection.x);
    __m128 lightDirectionY = _mm_set1_ps(lightDirection.y);
    __m128 lightDirectionZ = _mm_set1_ps(lightDirection.z);

    __m128 colorR = _mm_set1_ps(lightColor.r << 20);
    __m128 colorG = _mm_set1_ps(lightColor.g << 10);
    __m128 colorB = _mm_set1_ps(lightColor.b);

    __m128i maskR = _mm_set1_epi32(0xff00000);
    __m128i maskG = _mm_set1_epi32(0x3fc00);
    __m128i maskB = _mm_set1_epi32(0xff);

    __m128i carryMask = _mm_set1_epi32(0x10040100);
    __m128i mask = _mm_set1_epi32(0xff3fcff);

    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        // lightPosition.y - cellPosition.y
        __m128 cellVectorY = _mm_sub_ps(_mm_sub_ps(_mm_cvtepi32_ps(_mm_set1_epi32(y)), half), lightPositionY);

        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            // lightPosition.z - cellPosition.z
            __m128 cellVectorZ = _mm_sub_ps(_mm_sub_ps(_mm_cvtepi32_ps(_mm_set1_epi32(z)), half), lightPositionZ);

            __m128 cellVectorYZDSq = _mm_add_ps(_mm_mul_ps(cellVectorY, cellVectorY), _mm_mul_ps(cellVectorZ, cellVectorZ));
            __m128 cellVectorDotYZ = _mm_add_ps(_mm_mul_ps(cellVectorY, lightDirectionY), _mm_mul_ps(cellVectorZ, lightDirectionZ));

            for (int x = extentsMin.x; x <= extentsMax.x; x += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                __m128 cellVectorX = _mm_sub_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(x)), offsetX), lightPositionX);

                __m128 distSquared = _mm_add_ps(_mm_mul_ps(cellVectorX, cellVectorX), cellVectorYZDSq);
                __m128 distInv = _mm_rsqrt_ps(distSquared);
                
                __m128 coneDot = _mm_mul_ps(distInv, _mm_add_ps(_mm_mul_ps(lightDirectionX, cellVectorX), cellVectorDotYZ));

                __m128 distAttenuation = _mm_sub_ps(one, _mm_mul_ps(distSquared, lightRadiusInvSq));
                __m128 coneAttenuation = _mm_max_ps(zero, _mm_mul_ps(_mm_sub_ps(coneDot, lightConeHalfAngleCos), lightConeAttenuationScale));

                __m128 attenuation = _mm_max_ps(zero, _mm_min_ps(one, _mm_mul_ps(distAttenuation, _mm_mul_ps(coneAttenuation, lightIntensity))));

                if (Shadows)
                {
                    // Load first 4 bytes and expand to 4 floats
                    __m128i shadowMask1 = _mm_loadu_si128((__m128i*)&shadowMask[y][z][x]);
                    __m128i shadowMask2 = _mm_unpacklo_epi8(shadowMask1, _mm_setzero_si128());
                    __m128i shadowMask3 = _mm_unpacklo_epi8(shadowMask2, _mm_setzero_si128());
                    __m128 shadowMask = _mm_cvtepi32_ps(shadowMask3);

                    // Combine shadow mask with attenuation
                    attenuation = _mm_mul_ps(attenuation, _mm_mul_ps(shadowMask, _mm_set1_ps(1.f / 255.f)));
                }

                __m128i chanR = _mm_and_si128(maskR, _mm_cvttps_epi32(_mm_mul_ps(colorR, attenuation)));
                __m128i chanG = _mm_and_si128(maskG, _mm_cvttps_epi32(_mm_mul_ps(colorG, attenuation)));
                __m128i chanB = _mm_and_si128(maskB, _mm_cvttps_epi32(_mm_mul_ps(colorB, attenuation)));

                __m128i color = _mm_or_si128(chanR, _mm_or_si128(chanG, chanB));
            
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                __m128i lighting = _mm_loadu_si128((__m128i*)&lightingScratch[y][z][x]);

                __m128i value = _mm_add_epi32(lighting, color);
                __m128i carry = _mm_and_si128(value, carryMask);

                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                __m128i result = _mm_or_si128(_mm_and_si128(value, mask), _mm_sub_epi32(carry, _mm_srli_epi32(carry, 8)));

                _mm_storeu_si128((__m128i*)&lightingScratch[y][z][x], result);
            }
        }
    }
}
#endif	

#ifdef SIMD_NEON
template <bool Shadows> void LightGrid::lightingUpdateSpotLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Color3uint8& lightColor, float lightIntensity_)
{
    float32x4_t zero = vdupq_n_f32(0);
    float32x4_t one = vdupq_n_f32(1.f);
    float32x4_t half = vdupq_n_f32(0.5f);
    float32x4_t offsetX = {-0.5f, 0.5f, 1.5f, 2.5f};

    float lightConeHalfAngleCos_ = cosf(G3D::toRadians(lightConeAngle / 2));

    float32x4_t lightConeHalfAngleCos = vdupq_n_f32(lightConeHalfAngleCos_);
    float32x4_t lightConeAttenuationScale = vdupq_n_f32(lightConeHalfAngleCos_ < 1.f ? 1.f / (1.f - lightConeHalfAngleCos_) : 0.f);
    
    float32x4_t lightRadiusInvSq = vdupq_n_f32(1 / (lightRadius * lightRadius));
    float32x4_t lightIntensity = vdupq_n_f32(lightIntensity_);
    float32x4_t lightPositionX = vdupq_n_f32(lightPosition.x);
    float32x4_t lightPositionY = vdupq_n_f32(lightPosition.y);
    float32x4_t lightPositionZ = vdupq_n_f32(lightPosition.z);
    
    float32x4_t lightDirectionX = vdupq_n_f32(lightDirection.x);
    float32x4_t lightDirectionY = vdupq_n_f32(lightDirection.y);
    float32x4_t lightDirectionZ = vdupq_n_f32(lightDirection.z);

    float32x4_t colorR = vdupq_n_f32(lightColor.r << 20);
    float32x4_t colorG = vdupq_n_f32(lightColor.g << 10);
    float32x4_t colorB = vdupq_n_f32(lightColor.b);

    uint32x4_t maskR = vdupq_n_u32(0xff00000);
    uint32x4_t maskG = vdupq_n_u32(0x3fc00);
    uint32x4_t maskB = vdupq_n_u32(0xff);

    uint32x4_t carryMask = vdupq_n_u32(0x10040100);
    uint32x4_t mask = vdupq_n_u32(0xff3fcff);

    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        // lightPosition.y - cellPosition.y
        float32x2_t yy = vcvt_f32_s32(vdup_n_s32(y));
        float32x4_t cellVectorY = vsubq_f32(vsubq_f32(vcombine_f32(yy, yy), half), lightPositionY);

        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            // lightPosition.z - cellPosition.z
            float32x2_t zz = vcvt_f32_s32(vdup_n_s32(z));
            float32x4_t cellVectorZ = vsubq_f32(vsubq_f32(vcombine_f32(zz, zz), half), lightPositionZ);

            float32x4_t cellVectorYZDSq = vmlaq_f32(vmulq_f32(cellVectorY, cellVectorY), cellVectorZ, cellVectorZ);
            float32x4_t cellVectorDotYZ = vmlaq_f32(vmulq_f32(cellVectorY, lightDirectionY), cellVectorZ, lightDirectionZ);

            for (int x = extentsMin.x; x <= extentsMax.x; x += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                float32x2_t xx = vcvt_f32_s32(vdup_n_s32(x));
                float32x4_t cellVectorX = vsubq_f32(vaddq_f32(vcombine_f32(xx, xx), offsetX), lightPositionX);

                float32x4_t distSquared = vmlaq_f32(cellVectorYZDSq, cellVectorX, cellVectorX);
                float32x4_t distInv = vrsqrteq_f32(distSquared);
                
                float32x4_t coneDot = vmulq_f32(distInv, vmlaq_f32(cellVectorDotYZ, lightDirectionX, cellVectorX));

                float32x4_t distAttenuation = vmlsq_f32(one, distSquared, lightRadiusInvSq);
                float32x4_t coneAttenuation = vmaxq_f32(zero, vmulq_f32(vsubq_f32(coneDot, lightConeHalfAngleCos), lightConeAttenuationScale));

                float32x4_t attenuation = vmaxq_f32(zero, vminq_f32(one, vmulq_f32(distAttenuation, vmulq_f32(coneAttenuation, lightIntensity))));

                if (Shadows)
                {
                    uint16x8_t shadowMask1 = vmovl_u8(vld1_u8(&shadowMask[y][z][x]));
                    uint32x4_t shadowMask2 = vmovl_u16(vget_low_u16(shadowMask1));
                    float32x4_t shadowMask = vcvtq_f32_u32(shadowMask2);

                    attenuation = vmulq_f32(attenuation, vmulq_n_f32(shadowMask, 1.f / 255.f));
                }

                uint32x4_t chanR = vandq_u32(maskR, vcvtq_u32_f32(vmulq_f32(colorR, attenuation)));
                uint32x4_t chanG = vandq_u32(maskG, vcvtq_u32_f32(vmulq_f32(colorG, attenuation)));
                uint32x4_t chanB = vandq_u32(maskB, vcvtq_u32_f32(vmulq_f32(colorB, attenuation)));

                uint32x4_t color = vorrq_u32(chanR, vorrq_u32(chanG, chanB));
            
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                uint32x4_t lighting = vld1q_u32(&lightingScratch[y][z][x]);

                uint32x4_t value = vaddq_u32(lighting, color);
                uint32x4_t carry = vandq_u32(value, carryMask);

                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                uint32x4_t result = vorrq_u32(vandq_u32(value, mask), vsubq_u32(carry, vshrq_n_u32(carry, 8)));

                vst1q_u32(&lightingScratch[y][z][x], result);
            }
        }
    }
}
#endif	

template <bool Shadows> void LightGrid::lightingUpdateSurfaceLightScratch(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Vector4& axisU, const Vector4& axisV, const Color3uint8& lightColor, float lightIntensity)
{
    float lightRadiusInvSq = 1 / (lightRadius * lightRadius);
    float lightConeHalfAngleCos = cosf(G3D::toRadians(lightConeAngle / 2));
    float lightConeAttenuationScale = lightConeHalfAngleCos < 1.f ? 1.f / (1.f - lightConeHalfAngleCos) : 0.f;
    
    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            for (int x = extentsMin.x; x <= extentsMax.x; ++x)
            {
                Vector3 cellPosition = Vector3((x - 1) + 0.5f, (y - 1) + 0.5f, (z - 1) + 0.5f);

                // Clamped projection of cellPosition onto a rectangle defined by lightPosition and axisU/axisV
                float projU = dot(cellPosition - lightPosition, axisU.xyz());
                float projUClamped = G3D::clamp(projU, -axisU.w, axisU.w);

                float projV = dot(cellPosition - lightPosition, axisV.xyz());
				float projVClamped = G3D::clamp(projV, -axisV.w, axisV.w);

				Vector3 lightProjectedPosition = lightPosition + projUClamped * axisU.xyz() + projVClamped * axisV.xyz();
            
                float distAttenuation = 1 - (lightProjectedPosition - cellPosition).squaredLength() * lightRadiusInvSq;
                float coneAttenuation = (dot(lightDirection, normalize(cellPosition - lightProjectedPosition)) - lightConeHalfAngleCos) * lightConeAttenuationScale;
                float attenuation = saturate(distAttenuation * std::max(0.f, coneAttenuation) * lightIntensity);

                if (Shadows)
                    attenuation *= shadowMask[y][z][x] / 255.f;
                
                int factor = attenuation * 65536.0f + 0.5f;
                int color = (((lightColor.r * factor) >> 16) << 20) + (((lightColor.g * factor) >> 16) << 10) + (((lightColor.b * factor) >> 16) << 0);
                
                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                int value = lightingScratch[y][z][x] + color;
                int carry = value & 0x10040100;
                
                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                lightingScratch[y][z][x] = (value & 0xff3fcff) | (carry - (carry >> 8));
            }
        }
    }
}

#ifdef SIMD_SSE2
template <bool Shadows> void LightGrid::lightingUpdateSurfaceLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Vector4& axisU, const Vector4& axisV, const Color3uint8& lightColor, float lightIntensity_)
{
    __m128 zero = _mm_setzero_ps();
    __m128 one = _mm_set1_ps(1.f);
    __m128 half = _mm_set1_ps(0.5f);
    __m128 offsetX = _mm_setr_ps(-0.5f, 0.5f, 1.5f, 2.5f);

    float lightConeHalfAngleCos_ = cosf(G3D::toRadians(lightConeAngle / 2));

    __m128 lightRadiusInvSq = _mm_set1_ps(1 / (lightRadius * lightRadius));
    __m128 lightConeHalfAngleCos = _mm_set1_ps(lightConeHalfAngleCos_);
    __m128 lightConeAttenuationScale = _mm_set1_ps(lightConeHalfAngleCos_ < 1.f ? 1.f / (1.f - lightConeHalfAngleCos_) : 0.f);

    __m128 lightIntensity = _mm_set1_ps(lightIntensity_);
    __m128 lightPositionX = _mm_set1_ps(lightPosition.x);
    __m128 lightPositionY = _mm_set1_ps(lightPosition.y);
    __m128 lightPositionZ = _mm_set1_ps(lightPosition.z);

    __m128 lightDirectionX = _mm_set1_ps(lightDirection.x);
    __m128 lightDirectionY = _mm_set1_ps(lightDirection.y);
    __m128 lightDirectionZ = _mm_set1_ps(lightDirection.z);

    __m128 axisUX = _mm_set1_ps(axisU.x);
    __m128 axisUY = _mm_set1_ps(axisU.y);
    __m128 axisUZ = _mm_set1_ps(axisU.z);
    __m128 axisULength = _mm_set1_ps(axisU.w);
    __m128 axisUInvLength = _mm_set1_ps(-axisU.w);

    __m128 axisVX = _mm_set1_ps(axisV.x);
    __m128 axisVY = _mm_set1_ps(axisV.y);
    __m128 axisVZ = _mm_set1_ps(axisV.z);
    __m128 axisVLength = _mm_set1_ps(axisV.w);
    __m128 axisVInvLength = _mm_set1_ps(-axisV.w);

    __m128 colorR = _mm_set1_ps(lightColor.r << 20);
    __m128 colorG = _mm_set1_ps(lightColor.g << 10);
    __m128 colorB = _mm_set1_ps(lightColor.b);

    __m128i maskR = _mm_set1_epi32(0xff00000);
    __m128i maskG = _mm_set1_epi32(0x3fc00);
    __m128i maskB = _mm_set1_epi32(0xff);

    __m128i carryMask = _mm_set1_epi32(0x10040100);
    __m128i mask = _mm_set1_epi32(0xff3fcff);

    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        // lightPosition.y - cellPosition.y
        __m128 yy = _mm_sub_ps(_mm_cvtepi32_ps(_mm_set1_epi32(y)), half);
        __m128 cellVectorY = _mm_sub_ps(yy, lightPositionY);

        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            // lightPosition.z - cellPosition.z
            __m128 zz = _mm_sub_ps(_mm_cvtepi32_ps(_mm_set1_epi32(z)), half);
            __m128 cellVectorZ = _mm_sub_ps(zz, lightPositionZ);
            
            __m128 projUYZ = _mm_add_ps(_mm_mul_ps(axisUY, cellVectorY), _mm_mul_ps(axisUZ, cellVectorZ));
            __m128 projVYZ = _mm_add_ps(_mm_mul_ps(axisVY, cellVectorY), _mm_mul_ps(axisVZ, cellVectorZ));

            for (int x = extentsMin.x; x <= extentsMax.x; x += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                __m128 xx = _mm_add_ps(_mm_cvtepi32_ps(_mm_set1_epi32(x)), offsetX);
                __m128 cellVectorX = _mm_sub_ps(xx, lightPositionX);

                // clamp(dot(axisU,cellVector), -axisULength, +axisULength)
                __m128 projU = _mm_add_ps(projUYZ, _mm_mul_ps(axisUX, cellVectorX));
                __m128 projUDistClamped = _mm_min_ps(_mm_max_ps(axisUInvLength, projU), axisULength);

                __m128 projV = _mm_add_ps(projVYZ, _mm_mul_ps(axisVX, cellVectorX));
                __m128 projVDistClamped = _mm_min_ps(_mm_max_ps(axisVInvLength, projV), axisVLength);

                __m128 cellVectorXProjected = _mm_sub_ps(cellVectorX, _mm_add_ps(_mm_mul_ps(projUDistClamped, axisUX), _mm_mul_ps(projVDistClamped, axisVX)));
                __m128 cellVectorYProjected = _mm_sub_ps(cellVectorY, _mm_add_ps(_mm_mul_ps(projUDistClamped, axisUY), _mm_mul_ps(projVDistClamped, axisVY)));
                __m128 cellVectorZProjected = _mm_sub_ps(cellVectorZ, _mm_add_ps(_mm_mul_ps(projUDistClamped, axisUZ), _mm_mul_ps(projVDistClamped, axisVZ)));
                
                __m128 cellVectorYZDSqProjected = _mm_add_ps(_mm_mul_ps(cellVectorYProjected, cellVectorYProjected), _mm_mul_ps(cellVectorZProjected, cellVectorZProjected));
                __m128 distSquared = _mm_add_ps(_mm_mul_ps(cellVectorXProjected, cellVectorXProjected), cellVectorYZDSqProjected);
                __m128 distInv = _mm_rsqrt_ps(distSquared);

                __m128 cellVectorDotYZProjected = _mm_add_ps(_mm_mul_ps(cellVectorYProjected, lightDirectionY), _mm_mul_ps(cellVectorZProjected, lightDirectionZ));
                __m128 coneDot = _mm_mul_ps(distInv, _mm_add_ps(_mm_mul_ps(lightDirectionX, cellVectorXProjected), cellVectorDotYZProjected));

                __m128 distAttenuation = _mm_sub_ps(one, _mm_mul_ps(distSquared, lightRadiusInvSq));
                __m128 coneAttenuation = _mm_max_ps(zero, _mm_mul_ps(_mm_sub_ps(coneDot, lightConeHalfAngleCos), lightConeAttenuationScale));

                __m128 attenuation = _mm_max_ps(zero, _mm_min_ps(one, _mm_mul_ps(distAttenuation, _mm_mul_ps(coneAttenuation, lightIntensity))));

                if (Shadows)
                {
                    // Load first 4 bytes and expand to 4 floats
                    __m128i shadowMask1 = _mm_loadu_si128((__m128i*)&shadowMask[y][z][x]);
                    __m128i shadowMask2 = _mm_unpacklo_epi8(shadowMask1, _mm_setzero_si128());
                    __m128i shadowMask3 = _mm_unpacklo_epi8(shadowMask2, _mm_setzero_si128());
                    __m128 shadowMask = _mm_cvtepi32_ps(shadowMask3);

                    // Combine shadow mask with attenuation
                    attenuation = _mm_mul_ps(attenuation, _mm_mul_ps(shadowMask, _mm_set1_ps(1.f / 255.f)));
                }

                __m128i chanR = _mm_and_si128(maskR, _mm_cvttps_epi32(_mm_mul_ps(colorR, attenuation)));
                __m128i chanG = _mm_and_si128(maskG, _mm_cvttps_epi32(_mm_mul_ps(colorG, attenuation)));
                __m128i chanB = _mm_and_si128(maskB, _mm_cvttps_epi32(_mm_mul_ps(colorB, attenuation)));

                __m128i color = _mm_or_si128(chanR, _mm_or_si128(chanG, chanB));

                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                __m128i lighting = _mm_loadu_si128((__m128i*)&lightingScratch[y][z][x]);

                __m128i value = _mm_add_epi32(lighting, color);
                __m128i carry = _mm_and_si128(value, carryMask);

                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                __m128i result = _mm_or_si128(_mm_and_si128(value, mask), _mm_sub_epi32(carry, _mm_srli_epi32(carry, 8)));

                _mm_storeu_si128((__m128i*)&lightingScratch[y][z][x], result);
            }
        }
    }
}
#endif	

#ifdef SIMD_NEON
template <bool Shadows> void LightGrid::lightingUpdateSurfaceLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Vector4& axisU, const Vector4& axisV, const Color3uint8& lightColor, float lightIntensity_)
{
    float32x4_t zero = vdupq_n_f32(0);
    float32x4_t one = vdupq_n_f32(1.f);
    float32x4_t half = vdupq_n_f32(0.5f);
    float32x4_t offsetX = {-0.5f, 0.5f, 1.5f, 2.5f};

    float lightConeHalfAngleCos_ = cosf(G3D::toRadians(lightConeAngle / 2));

    float32x4_t lightConeHalfAngleCos = vdupq_n_f32(lightConeHalfAngleCos_);
    float32x4_t lightConeAttenuationScale = vdupq_n_f32(lightConeHalfAngleCos_ < 1.f ? 1.f / (1.f - lightConeHalfAngleCos_) : 0.f);

    float32x4_t lightRadiusInvSq = vdupq_n_f32(1 / (lightRadius * lightRadius));
    float32x4_t lightIntensity = vdupq_n_f32(lightIntensity_);
    float32x4_t lightPositionX = vdupq_n_f32(lightPosition.x);
    float32x4_t lightPositionY = vdupq_n_f32(lightPosition.y);
    float32x4_t lightPositionZ = vdupq_n_f32(lightPosition.z);

    float32x4_t lightDirectionX = vdupq_n_f32(lightDirection.x);
    float32x4_t lightDirectionY = vdupq_n_f32(lightDirection.y);
    float32x4_t lightDirectionZ = vdupq_n_f32(lightDirection.z);

    float32x4_t axisUX = vdupq_n_f32(axisU.x);
    float32x4_t axisUY = vdupq_n_f32(axisU.y);
    float32x4_t axisUZ = vdupq_n_f32(axisU.z);
    float32x4_t axisULength = vdupq_n_f32(axisU.w);
    float32x4_t axisUInvLength = vdupq_n_f32(-axisU.w);

    float32x4_t axisVX = vdupq_n_f32(axisV.x);
    float32x4_t axisVY = vdupq_n_f32(axisV.y);
    float32x4_t axisVZ = vdupq_n_f32(axisV.z);
    float32x4_t axisVLength = vdupq_n_f32(axisV.w);
    float32x4_t axisVInvLength = vdupq_n_f32(-axisV.w);

    float32x4_t colorR = vdupq_n_f32(lightColor.r << 20);
    float32x4_t colorG = vdupq_n_f32(lightColor.g << 10);
    float32x4_t colorB = vdupq_n_f32(lightColor.b);

    uint32x4_t maskR = vdupq_n_u32(0xff00000);
    uint32x4_t maskG = vdupq_n_u32(0x3fc00);
    uint32x4_t maskB = vdupq_n_u32(0xff);

    uint32x4_t carryMask = vdupq_n_u32(0x10040100);
    uint32x4_t mask = vdupq_n_u32(0xff3fcff);

    for (int y = extentsMin.y; y <= extentsMax.y; ++y)
    {
        // lightPosition.y - cellPosition.y
        float32x2_t yy = vcvt_f32_s32(vdup_n_s32(y));
        float32x4_t cellY = vsubq_f32(vcombine_f32(yy, yy), half);
        float32x4_t cellVectorY = vsubq_f32(cellY, lightPositionY);

        for (int z = extentsMin.z; z <= extentsMax.z; ++z)
        {
            // lightPosition.z - cellPosition.z
            float32x2_t zz = vcvt_f32_s32(vdup_n_s32(z));
            float32x4_t cellZ = vsubq_f32(vcombine_f32(zz, zz), half);
            float32x4_t cellVectorZ = vsubq_f32(cellZ, lightPositionZ);

            float32x4_t projUYZ = vaddq_f32(vmulq_f32(axisUY, cellVectorY), vmulq_f32(axisUZ, cellVectorZ));
            float32x4_t projVYZ = vaddq_f32(vmulq_f32(axisVY, cellVectorY), vmulq_f32(axisVZ, cellVectorZ));
           
            for (int x = extentsMin.x; x <= extentsMax.x; x += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                float32x2_t xx = vcvt_f32_s32(vdup_n_s32(x));
                float32x4_t cellX = vaddq_f32(vcombine_f32(xx, xx), offsetX);
                float32x4_t cellVectorX = vsubq_f32(cellX, lightPositionX);

                float32x4_t projU = vaddq_f32(projUYZ, vmulq_f32(axisUX, cellVectorX));
                float32x4_t projUDistClamped = vminq_f32(vmaxq_f32(axisUInvLength, projU), axisULength);

                float32x4_t projV = vaddq_f32(projVYZ, vmulq_f32(axisVX, cellVectorX));
                float32x4_t projVDistClamped = vminq_f32(vmaxq_f32(axisVInvLength, projV), axisVLength);
                
                float32x4_t cellVectorXProjected = vsubq_f32(cellVectorX, vmlaq_f32(vmulq_f32(projUDistClamped, axisUX), projVDistClamped, axisVX));
                float32x4_t cellVectorYProjected = vsubq_f32(cellVectorY, vmlaq_f32(vmulq_f32(projUDistClamped, axisUY), projVDistClamped, axisVY));
                float32x4_t cellVectorZProjected = vsubq_f32(cellVectorZ, vmlaq_f32(vmulq_f32(projUDistClamped, axisUZ), projVDistClamped, axisVZ));

                float32x4_t cellVectorYZDSqProjected = vmlaq_f32(vmulq_f32(cellVectorYProjected, cellVectorYProjected), cellVectorZProjected, cellVectorZProjected);
                float32x4_t distSquared = vmlaq_f32(cellVectorYZDSqProjected, cellVectorXProjected, cellVectorXProjected);
                float32x4_t distInv = vrsqrteq_f32(distSquared);

                float32x4_t cellVectorDotYZProjected = vmlaq_f32(vmulq_f32(cellVectorYProjected, lightDirectionY), cellVectorZProjected, lightDirectionZ);
                float32x4_t coneDot = vmulq_f32(distInv, vmlaq_f32(cellVectorDotYZProjected, lightDirectionX, cellVectorXProjected));

                float32x4_t distAttenuation = vmlsq_f32(one, distSquared, lightRadiusInvSq);
                float32x4_t coneAttenuation = vmaxq_f32(zero, vmulq_f32(vsubq_f32(coneDot, lightConeHalfAngleCos), lightConeAttenuationScale));

                float32x4_t attenuation = vmaxq_f32(zero, vminq_f32(one, vmulq_f32(distAttenuation, vmulq_f32(coneAttenuation, lightIntensity))));

                if (Shadows)
                {
                    uint16x8_t shadowMask1 = vmovl_u8(vld1_u8(&shadowMask[y][z][x]));
                    uint32x4_t shadowMask2 = vmovl_u16(vget_low_u16(shadowMask1));
                    float32x4_t shadowMask = vcvtq_f32_u32(shadowMask2);

                    attenuation = vmulq_f32(attenuation, vmulq_n_f32(shadowMask, 1.f / 255.f));
                }

                uint32x4_t chanR = vandq_u32(maskR, vcvtq_u32_f32(vmulq_f32(colorR, attenuation)));
                uint32x4_t chanG = vandq_u32(maskG, vcvtq_u32_f32(vmulq_f32(colorG, attenuation)));
                uint32x4_t chanB = vandq_u32(maskB, vcvtq_u32_f32(vmulq_f32(colorB, attenuation)));

                uint32x4_t color = vorrq_u32(chanR, vorrq_u32(chanG, chanB));

                // We're adding two values with a possibility of overflow. Carry bits are 0, so overflow will go into the first carry bit.
                uint32x4_t lighting = vld1q_u32(&lightingScratch[y][z][x]);

                uint32x4_t value = vaddq_u32(lighting, color);
                uint32x4_t carry = vandq_u32(value, carryMask);

                // Form a mask of 8 ones for each carry bit that is set (10...0 -> 10...0 - 1 = 01...1)
                uint32x4_t result = vorrq_u32(vandq_u32(value, mask), vsubq_u32(carry, vshrq_n_u32(carry, 8)));

                vst1q_u32(&lightingScratch[y][z][x], result);
            }
        }
    }
}
#endif	

template <bool AxisY> void LightGrid::lightingBlurAxisYZScratch()
{
#define LIGHTING(k) (AxisY ? lightingScratch[k][i][j] : lightingScratch[i][k][j])
            
    for (int i = 0; i < (AxisY ? kLightGridChunkSizeXZ : kLightGridChunkSizeY) + 2; ++i)
        for (int j = 0; j < kLightGridChunkSizeXZ + 2; ++j)
        {
            unsigned int prev = LIGHTING(0);
            unsigned int cur = LIGHTING(1);
            
            for (int k = 1; k < (AxisY ? kLightGridChunkSizeY : kLightGridChunkSizeXZ) + 1; ++k)
            {
                unsigned int newCur = LIGHTING(k+1);
                
                // ff3fcff mask masks the value bits from packed representation, setting carry bits to zero
                // Note: do /4 instead of /3 since can't really do /3 fast enough in packed representation
                LIGHTING(k) = ((prev + cur + newCur) / 4) & 0xff3fcff;
                
                prev = cur;
                cur = newCur;
            }
        }
            
#undef LIGHTING
}

#ifdef SIMD_SSE2
template <bool AxisY> void LightGrid::lightingBlurAxisYZScratchSIMD()
{
#define LIGHTING(k) (AxisY ? lightingScratch[k][i][j] : lightingScratch[i][k][j])
            
    __m128i mask = _mm_set1_epi32(0xff3fcff);

    for (int i = 0; i < (AxisY ? kLightGridChunkSizeXZ : kLightGridChunkSizeY) + 2; ++i)
        for (int j = 0; j < kLightGridChunkSizeXZ + 2; j += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
        {
            __m128i prev = _mm_loadu_si128((__m128i*)&LIGHTING(0));
            __m128i cur = _mm_loadu_si128((__m128i*)&LIGHTING(1));
            
            for (int k = 1; k < (AxisY ? kLightGridChunkSizeY : kLightGridChunkSizeXZ) + 1; ++k)
            {
                __m128i newCur = _mm_loadu_si128((__m128i*)&LIGHTING(k+1));

                // ff3fcff mask masks the value bits from packed representation, setting carry bits to zero
                // Note: do /4 instead of /3 since can't really do /3 fast enough in packed representation
                __m128i value = _mm_and_si128(_mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(prev, cur), newCur), 2), mask);
                
                _mm_storeu_si128((__m128i*)&LIGHTING(k), value);
                
                prev = cur;
                cur = newCur;
            }
        }
            
#undef LIGHTING
}
#endif

#ifdef SIMD_NEON
template <bool AxisY> void LightGrid::lightingBlurAxisYZScratchSIMD()
{
#define LIGHTING(k) (AxisY ? lightingScratch[k][i][j] : lightingScratch[i][k][j])
            
    uint32x4_t mask = vdupq_n_u32(0xff3fcff);

    for (int i = 0; i < (AxisY ? kLightGridChunkSizeXZ : kLightGridChunkSizeY) + 2; ++i)
        for (int j = 0; j < kLightGridChunkSizeXZ + 2; j += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
        {
            uint32x4_t prev = vld1q_u32(&LIGHTING(0));
            uint32x4_t cur = vld1q_u32(&LIGHTING(1));
            
            for (int k = 1; k < (AxisY ? kLightGridChunkSizeY : kLightGridChunkSizeXZ) + 1; ++k)
            {
                uint32x4_t newCur = vld1q_u32(&LIGHTING(k+1));

                // ff3fcff mask masks the value bits from packed representation, setting carry bits to zero
                // Note: do /4 instead of /3 since can't really do /3 fast enough in packed representation
                uint32x4_t value = vandq_u32(vshrq_n_u32(vaddq_u32(vaddq_u32(prev, cur), newCur), 2), mask);
                
                vst1q_u32(&LIGHTING(k), value);
                
                prev = cur;
                cur = newCur;
            }
        }
            
#undef LIGHTING
}
#endif

void LightGrid::lightingBlurAxisXScratchToChunk(LightGridChunk& chunk)
{
#define LIGHTING(k) lightingScratch[i][j][k]
            
    for (int i = 1; i < kLightGridChunkSizeY + 1; ++i)
        for (int j = 1; j < kLightGridChunkSizeXZ + 1; ++j)
        {
            unsigned int prev = LIGHTING(0);
            unsigned int cur = LIGHTING(1);
            
            for (int k = 1; k < kLightGridChunkSizeXZ + 1; ++k)
            {
                unsigned int newCur = LIGHTING(k+1);
                
                // ff3fcff mask masks the value bits from packed representation, setting carry bits to zero
                // Note: we had two steps before, and we're doing the 3rd blur step; the filter weights should be 1/27
                // Due to fixed point restriction, we divided by 4 in first two steps, yielding a weight of 1/16
                // Divide by two here to have a weight of 1/32, which is close enough to 1/27
                unsigned int value = ((prev + cur + newCur) / 2) & 0xff3fcff;
                
                chunk.lighting[i-1][j-1][k-1][0] = value >> 20;
                chunk.lighting[i-1][j-1][k-1][1] = value >> 10;
                chunk.lighting[i-1][j-1][k-1][2] = value >> 0;
                
                prev = cur;
                cur = newCur;
            }
        }
            
#undef LIGHTING
}

#ifdef SIMD_SSE2
void LightGrid::lightingBlurAxisXScratchToChunkSIMD(LightGridChunk& chunk)
{
#define LIGHTING(k) lightingScratch[i][j][k]
            
    __m128i maskR = _mm_set1_epi32(0xff);
    __m128i maskG = _mm_set1_epi32(0xff00);
    __m128i maskB = _mm_set1_epi32(0xff0000);
    __m128i maskA = _mm_set1_epi32(0xff000000);

    for (int i = 1; i < kLightGridChunkSizeY + 1; ++i)
        for (int j = 1; j < kLightGridChunkSizeXZ + 1; ++j)
        {
            for (int k = 1; k < kLightGridChunkSizeXZ + 1; k += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                __m128i prev = _mm_loadu_si128((__m128i*)&LIGHTING(k-1));
                __m128i cur = _mm_loadu_si128((__m128i*)&LIGHTING(k));
                __m128i newCur = _mm_loadu_si128((__m128i*)&LIGHTING(k+1));
                
                // ff3fcff mask masks the value bits from packed representation, setting carry bits to zero
                // Note: we had two steps before, and we're doing the 3rd blur step; the filter weights should be 1/27
                // Due to fixed point restriction, we divided by 4 in first two steps, yielding a weight of 1/16
                // Divide by two here to have a weight of 1/32, which is close enough to 1/27
                __m128i value = _mm_srli_epi32(_mm_add_epi32(_mm_add_epi32(prev, cur), newCur), 1);

                // now we have to repack from 10 10 10 format to 8 8 8
                __m128i chanR = _mm_and_si128(_mm_srli_epi32(value, 20), maskR);
                __m128i chanG = _mm_and_si128(_mm_srli_epi32(value, 2), maskG);
                __m128i chanB = _mm_and_si128(_mm_slli_epi32(value, 16), maskB);
                
                // lighting has arbitrary rgb, but chanX should have alpha as zero - should be enough to or values together if we mask lighting
                __m128i lighting = _mm_loadu_si128((__m128i*)chunk.lighting[i-1][j-1][k-1]);
                __m128i result = _mm_or_si128(_mm_or_si128(chanR, chanG), _mm_or_si128(chanB, _mm_and_si128(lighting, maskA)));
                
                _mm_storeu_si128((__m128i*)chunk.lighting[i-1][j-1][k-1], result);
            }
        }
            
#undef LIGHTING
}
#endif

#ifdef SIMD_NEON
void LightGrid::lightingBlurAxisXScratchToChunkSIMD(LightGridChunk& chunk)
{
#define LIGHTING(k) lightingScratch[i][j][k]
            
    uint32x4_t maskR = vdupq_n_u32(0xff);
    uint32x4_t maskG = vdupq_n_u32(0xff00);
    uint32x4_t maskB = vdupq_n_u32(0xff0000);
    uint32x4_t maskA = vdupq_n_u32(0xff000000);

    for (int i = 1; i < kLightGridChunkSizeY + 1; ++i)
        for (int j = 1; j < kLightGridChunkSizeXZ + 1; ++j)
        {
            for (int k = 1; k < kLightGridChunkSizeXZ + 1; k += 4) // note: this loop processes a few extra values - it's ok since we pad X in lighting scratch by 4 pixels
            {
                uint32x4_t prev = vld1q_u32(&LIGHTING(k-1));
                uint32x4_t cur = vld1q_u32(&LIGHTING(k));
                uint32x4_t newCur = vld1q_u32(&LIGHTING(k+1));
                
                // ff3fcff mask masks the value bits from packed representation, setting carry bits to zero
                // Note: we had two steps before, and we're doing the 3rd blur step; the filter weights should be 1/27
                // Due to fixed point restriction, we divided by 4 in first two steps, yielding a weight of 1/16
                // Divide by two here to have a weight of 1/32, which is close enough to 1/27
                uint32x4_t value = vshrq_n_u32(vaddq_u32(vaddq_u32(prev, cur), newCur), 1);

                // now we have to repack from 10 10 10 format to 8 8 8
                uint32x4_t chanR = vandq_u32(vshrq_n_u32(value, 20), maskR);
                uint32x4_t chanG = vandq_u32(vshrq_n_u32(value, 2), maskG);
                uint32x4_t chanB = vandq_u32(vshlq_n_u32(value, 16), maskB);
                
                // lighting has arbitrary rgb, but chanX should have alpha as zero - should be enough to or values together if we mask lighting
                uint32x4_t lighting = vreinterpretq_u32_u8(vld1q_u8(chunk.lighting[i-1][j-1][k-1]));
                uint32x4_t result = vorrq_u32(vorrq_u32(chanR, chanG), vorrq_u32(chanB, vandq_u32(lighting, maskA)));
                
                vst1q_u8(chunk.lighting[i-1][j-1][k-1], vreinterpretq_u8_u32(result));
            }
        }
            
#undef LIGHTING
}
#endif

void LightGrid::lightingUpdateAverageImpl(LightGridChunk& chunk)
{
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        unsigned int global = 0;
        unsigned int skylight = 0;
        unsigned int weight = 0;

        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            {
                int w = 255 - chunk.occupancy[y][z][x];
                
                global += chunk.lighting[y][z][x][3] * w;
                skylight += chunk.lightingSkylight[y][z][x] * w;
                weight += w;
            }
            
        chunk.lightingAverageGlobal[y] = global;
        chunk.lightingAverageSkylight[y] = skylight;
        chunk.lightingAverageWeight[y] = weight;
    }
}

#ifdef SIMD_SSE2
void LightGrid::lightingUpdateAverageImplSIMD(LightGridChunk& chunk)
{
    __m128i zero = _mm_setzero_si128();
    __m128i b255 = _mm_set1_epi8((char)255);

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        __m128i global4[4] = {zero, zero, zero, zero};
        __m128i skylight4[4] = {zero, zero, zero, zero};
        __m128i weight4[4] = {zero, zero, zero, zero};

        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            for (int x = 0; x < kLightGridChunkSizeXZ; x += 16)
            {
                __m128i occ = _mm_loadu_si128((__m128i*)&chunk.occupancy[y][z][x]);
                
                __m128i light0 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+0]);
                __m128i light1 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+4]);
                __m128i light2 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+8]);
                __m128i light3 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+12]);
                
                __m128i skylight = _mm_loadu_si128((__m128i*)&chunk.lightingSkylight[y][z][x]);

                __m128i skylight01 = _mm_unpacklo_epi8(skylight, zero);
                __m128i skylight23 = _mm_unpackhi_epi8(skylight, zero);
                
                __m128i skylight0 = _mm_unpacklo_epi8(skylight01, zero);
                __m128i skylight1 = _mm_unpackhi_epi8(skylight01, zero);
                __m128i skylight2 = _mm_unpacklo_epi8(skylight23, zero);
                __m128i skylight3 = _mm_unpackhi_epi8(skylight23, zero);
                
                // compute weights (255 - occupancy) into weight0...weight3
                __m128i weight = _mm_sub_epi8(b255, occ);
                
                __m128i weight01 = _mm_unpacklo_epi8(weight, zero);
                __m128i weight23 = _mm_unpackhi_epi8(weight, zero);
                
                __m128i weight0 = _mm_unpacklo_epi8(weight01, zero);
                __m128i weight1 = _mm_unpackhi_epi8(weight01, zero);
                __m128i weight2 = _mm_unpacklo_epi8(weight23, zero);
                __m128i weight3 = _mm_unpackhi_epi8(weight23, zero);
                
                // multiply global lighting and skylight by weight (result fits in 16 bits)
                __m128i globalw0 = _mm_mullo_epi16(weight0, _mm_srli_epi32(light0, 24));
                __m128i globalw1 = _mm_mullo_epi16(weight1, _mm_srli_epi32(light1, 24));
                __m128i globalw2 = _mm_mullo_epi16(weight2, _mm_srli_epi32(light2, 24));
                __m128i globalw3 = _mm_mullo_epi16(weight3, _mm_srli_epi32(light3, 24));
                
                __m128i skylightw0 = _mm_mullo_epi16(weight0, skylight0);
                __m128i skylightw1 = _mm_mullo_epi16(weight1, skylight1);
                __m128i skylightw2 = _mm_mullo_epi16(weight2, skylight2);
                __m128i skylightw3 = _mm_mullo_epi16(weight3, skylight3);
                
                // accumulate
                global4[0] = _mm_add_epi32(global4[0], globalw0);
                global4[1] = _mm_add_epi32(global4[1], globalw1);
                global4[2] = _mm_add_epi32(global4[2], globalw2);
                global4[3] = _mm_add_epi32(global4[3], globalw3);

                skylight4[0] = _mm_add_epi32(skylight4[0], skylightw0);
                skylight4[1] = _mm_add_epi32(skylight4[1], skylightw1);
                skylight4[2] = _mm_add_epi32(skylight4[2], skylightw2);
                skylight4[3] = _mm_add_epi32(skylight4[3], skylightw3);

                weight4[0] = _mm_add_epi32(weight4[0], weight0);
                weight4[1] = _mm_add_epi32(weight4[1], weight1);
                weight4[2] = _mm_add_epi32(weight4[2], weight2);
                weight4[3] = _mm_add_epi32(weight4[3], weight3);
            }
        }
            
        // merge accumulated results
        __m128i global = _mm_add_epi32(_mm_add_epi32(global4[0], global4[1]), _mm_add_epi32(global4[2], global4[3]));
        __m128i skylight = _mm_add_epi32(_mm_add_epi32(skylight4[0], skylight4[1]), _mm_add_epi32(skylight4[2], skylight4[3]));
        __m128i weight = _mm_add_epi32(_mm_add_epi32(weight4[0], weight4[1]), _mm_add_epi32(weight4[2], weight4[3]));
        
        unsigned int globalBuf[4];
        unsigned int skylightBuf[4];
        unsigned int weightBuf[4];
        _mm_storeu_si128((__m128i*)globalBuf, global);
        _mm_storeu_si128((__m128i*)skylightBuf, skylight);
        _mm_storeu_si128((__m128i*)weightBuf, weight);

        chunk.lightingAverageGlobal[y] = globalBuf[0] + globalBuf[1] + globalBuf[2] + globalBuf[3];
        chunk.lightingAverageSkylight[y] = skylightBuf[0] + skylightBuf[1] + skylightBuf[2] + skylightBuf[3];
        chunk.lightingAverageWeight[y] = weightBuf[0] + weightBuf[1] + weightBuf[2] + weightBuf[3];
    }
}
#endif

#ifdef SIMD_NEON
void LightGrid::lightingUpdateAverageImplSIMD(LightGridChunk& chunk)
{
    uint32x4_t zero = vdupq_n_u32(0);
    uint8x16_t b255 = vdupq_n_u8(255);

    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        uint32x4_t global4[4] = {zero, zero, zero, zero};
        uint32x4_t skylight4[4] = {zero, zero, zero, zero};
        uint32x4_t weight4[4] = {zero, zero, zero, zero};

        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            for (int x = 0; x < kLightGridChunkSizeXZ; x += 16)
            {
                uint8x16_t occ = vld1q_u8(&chunk.occupancy[y][z][x]);

                uint32x4_t light0 = vreinterpretq_u32_u8(vld1q_u8(chunk.lighting[y][z][x+0]));
                uint32x4_t light1 = vreinterpretq_u32_u8(vld1q_u8(chunk.lighting[y][z][x+4]));
                uint32x4_t light2 = vreinterpretq_u32_u8(vld1q_u8(chunk.lighting[y][z][x+8]));
                uint32x4_t light3 = vreinterpretq_u32_u8(vld1q_u8(chunk.lighting[y][z][x+12]));

                uint8x16_t skylight = vld1q_u8(&chunk.lightingSkylight[y][z][x]);

                uint16x8_t skylight01 = vmovl_u8(vget_low_u8(skylight));
                uint16x8_t skylight23 = vmovl_u8(vget_high_u8(skylight));

                uint32x4_t skylight0 = vmovl_u16(vget_low_u16(skylight01));
                uint32x4_t skylight1 = vmovl_u16(vget_high_u16(skylight01));
                uint32x4_t skylight2 = vmovl_u16(vget_low_u16(skylight23));
                uint32x4_t skylight3 = vmovl_u16(vget_high_u16(skylight23));
                
                // compute weights (255 - occupancy) into weight0...weight3
                uint8x16_t weight = vsubq_u8(b255, occ);

                uint16x8_t weight01 = vmovl_u8(vget_low_u8(weight));
                uint16x8_t weight23 = vmovl_u8(vget_high_u8(weight));

                uint32x4_t weight0 = vmovl_u16(vget_low_u16(weight01));
                uint32x4_t weight1 = vmovl_u16(vget_high_u16(weight01));
                uint32x4_t weight2 = vmovl_u16(vget_low_u16(weight23));
                uint32x4_t weight3 = vmovl_u16(vget_high_u16(weight23));
                
                // accumulate
                global4[0] = vmlaq_u32(global4[0], weight0, vshrq_n_u32(light0, 24));
                global4[1] = vmlaq_u32(global4[1], weight1, vshrq_n_u32(light1, 24));
                global4[2] = vmlaq_u32(global4[2], weight2, vshrq_n_u32(light2, 24));
                global4[3] = vmlaq_u32(global4[3], weight3, vshrq_n_u32(light3, 24));

                skylight4[0] = vmlaq_u32(skylight4[0], weight0, skylight0);
                skylight4[1] = vmlaq_u32(skylight4[1], weight1, skylight1);
                skylight4[2] = vmlaq_u32(skylight4[2], weight2, skylight2);
                skylight4[3] = vmlaq_u32(skylight4[3], weight3, skylight3);

                weight4[0] = vaddq_u32(weight4[0], weight0);
                weight4[1] = vaddq_u32(weight4[1], weight1);
                weight4[2] = vaddq_u32(weight4[2], weight2);
                weight4[3] = vaddq_u32(weight4[3], weight3);
            }
        }
            
        // merge accumulated results
        uint32x4_t global = vaddq_u32(vaddq_u32(global4[0], global4[1]), vaddq_u32(global4[2], global4[3]));
        uint32x4_t skylight = vaddq_u32(vaddq_u32(skylight4[0], skylight4[1]), vaddq_u32(skylight4[2], skylight4[3]));
        uint32x4_t weight = vaddq_u32(vaddq_u32(weight4[0], weight4[1]), vaddq_u32(weight4[2], weight4[3]));
        
        unsigned int globalBuf[4];
        unsigned int skylightBuf[4];
        unsigned int weightBuf[4];
        vst1q_u32(globalBuf, global);
        vst1q_u32(skylightBuf, skylight);
        vst1q_u32(weightBuf, weight);

        chunk.lightingAverageGlobal[y] = globalBuf[0] + globalBuf[1] + globalBuf[2] + globalBuf[3];
        chunk.lightingAverageSkylight[y] = skylightBuf[0] + skylightBuf[1] + skylightBuf[2] + skylightBuf[3];
        chunk.lightingAverageWeight[y] = weightBuf[0] + weightBuf[1] + weightBuf[2] + weightBuf[3];
    }
}
#endif

void LightGrid::lightingClearLocal(LightGridChunk& chunk)
{
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            for (int x = 0; x < kLightGridChunkSizeXZ; x += 4)
            {
                chunk.lighting[y][z][x+0][0] = 0;
                chunk.lighting[y][z][x+0][1] = 0;
                chunk.lighting[y][z][x+0][2] = 0;
                chunk.lighting[y][z][x+1][0] = 0;
                chunk.lighting[y][z][x+1][1] = 0;
                chunk.lighting[y][z][x+1][2] = 0;
                chunk.lighting[y][z][x+2][0] = 0;
                chunk.lighting[y][z][x+2][1] = 0;
                chunk.lighting[y][z][x+2][2] = 0;
                chunk.lighting[y][z][x+3][0] = 0;
                chunk.lighting[y][z][x+3][1] = 0;
                chunk.lighting[y][z][x+3][2] = 0;
            }
}

void LightGrid::lightingClearGlobal(LightGridChunk& chunk)
{
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            for (int x = 0; x < kLightGridChunkSizeXZ; x += 8)
            {
                chunk.lighting[y][z][x+0][3] = 255;
                chunk.lighting[y][z][x+1][3] = 255;
                chunk.lighting[y][z][x+2][3] = 255;
                chunk.lighting[y][z][x+3][3] = 255;
                chunk.lighting[y][z][x+4][3] = 255;
                chunk.lighting[y][z][x+5][3] = 255;
                chunk.lighting[y][z][x+6][3] = 255;
                chunk.lighting[y][z][x+7][3] = 255;
            }
}

void LightGrid::lightingComposit(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch)
{
	RBXPROFILER_SCOPE("Render", "lightingComposit");

    if (!lightShadows || (skyAmbient.r == 0 && skyAmbient.g == 0 && skyAmbient.b == 0))
    {
        unsigned char* dataSlice = data;
        
        for (int y = 0; y < kLightGridChunkSizeY; ++y)
        {
            unsigned char* dataRow = dataSlice;
            
            for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
            {
                memcpy(dataRow, chunk.lighting[y][z][0], kLightGridChunkSizeXZ * 4);
                
                dataRow += rowPitch;
            }
            
            dataSlice += slicePitch;
        }
    }
    else
    {
        Color3uint8 storageSkyAmbient = transformColor(skyAmbient);
    
    #if defined(__ANDROID__) && __GNUC__ == 4 && __GNUC_MINOR__ == 6
        // GCC 4.6 miscompiles SIMD implementation in Release, causing lighting bugs
        lightingCompositImplSIMD(chunk, data, rowPitch, slicePitch, storageSkyAmbient);
    #else
        SIMDCALL(lightingCompositImpl, (chunk, data, rowPitch, slicePitch, storageSkyAmbient));
    #endif
    }
}
    
void LightGrid::lightingCompositImpl(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch, const Color3uint8& skyAmbient)
{
    unsigned char skyTable[256][4];
    
    for (int i = 0; i < 256; ++i)
    {
        skyTable[i][0] = (i * skyAmbient.r + 255) >> 8;
        skyTable[i][1] = (i * skyAmbient.g + 255) >> 8;
        skyTable[i][2] = (i * skyAmbient.b + 255) >> 8;
        skyTable[i][3] = 0;
    }
    
    unsigned char* dataSlice = data;
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        unsigned char* dataRow = dataSlice;
        
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            for (int x = 0; x < kLightGridChunkSizeXZ; ++x)
            {
                unsigned char skylight = chunk.lightingSkylight[y][z][x];
                
                unsigned int a = *reinterpret_cast<const unsigned int*>(chunk.lighting[y][z][x]);
                unsigned int b = *reinterpret_cast<unsigned int*>(skyTable[skylight]);
                
                // lose low bits, parallel add (carry goes into high bit of each byte)
                unsigned int res7 = ((a >> 1) & 0x7f7f7f7f) + ((b >> 1) & 0x7f7f7f7f);
                
                // convert carry bits into 7-bit mask
                unsigned int carry = (res7 & 0x80808080);
                unsigned int carryMask = carry - (carry >> 7);
                
                // carry means overflow -> max255
                unsigned int res = (res7 | carryMask) << 1;
                
                *reinterpret_cast<unsigned int*>(dataRow + x * 4) = res;
            }
            
            dataRow += rowPitch;
        }
        
        dataSlice += slicePitch;
    }
}

#ifdef SIMD_SSE2
void LightGrid::lightingCompositImplSIMD(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch, const Color3uint8& skyAmbient)
{
    __m128i zero = _mm_setzero_si128();
    __m128i round = _mm_set1_epi16(255);
    
    __m128i skyAmbientR = _mm_set1_epi16(skyAmbient.r);
    __m128i skyAmbientG = _mm_set1_epi16(skyAmbient.g);
    __m128i skyAmbientB = _mm_set1_epi16(skyAmbient.b);
    __m128i maskHighByte = _mm_set1_epi16((short)0xff00);
    
    unsigned char* dataSlice = data;
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        unsigned char* dataRow = dataSlice;
        
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            for (int x = 0; x < kLightGridChunkSizeXZ; x += 16)
            {
                __m128i skylight = _mm_loadu_si128((__m128i*)&chunk.lightingSkylight[y][z][x]);
                
                __m128i light0 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+0]);
                __m128i light1 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+4]);
                __m128i light2 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+8]);
                __m128i light3 = _mm_loadu_si128((__m128i*)chunk.lighting[y][z][x+12]);
                
                __m128i skylight01 = _mm_unpacklo_epi8(skylight, zero);
                __m128i skylight23 = _mm_unpackhi_epi8(skylight, zero);
                
                __m128i skyR01 = _mm_add_epi16(_mm_mullo_epi16(skylight01, skyAmbientR), round);
                __m128i skyR23 = _mm_add_epi16(_mm_mullo_epi16(skylight23, skyAmbientR), round);

                __m128i skyG01 = _mm_add_epi16(_mm_mullo_epi16(skylight01, skyAmbientG), round);
                __m128i skyG23 = _mm_add_epi16(_mm_mullo_epi16(skylight23, skyAmbientG), round);

                __m128i skyB01 = _mm_add_epi16(_mm_mullo_epi16(skylight01, skyAmbientB), round);
                __m128i skyB23 = _mm_add_epi16(_mm_mullo_epi16(skylight23, skyAmbientB), round);
                
                // xform . r0 . r1 . r2 . r3 and . g0 . g1 . g2 . g3 to . . r0 g0 . . r1 g1
                // shift right to get to r0 g0 . . r1 g1 . . (little endian)
                __m128i skyRG0 = _mm_srli_epi32(_mm_unpacklo_epi8(skyR01, skyG01), 16);
                __m128i skyRG1 = _mm_srli_epi32(_mm_unpackhi_epi8(skyR01, skyG01), 16);
                __m128i skyRG2 = _mm_srli_epi32(_mm_unpacklo_epi8(skyR23, skyG23), 16);
                __m128i skyRG3 = _mm_srli_epi32(_mm_unpackhi_epi8(skyR23, skyG23), 16);
                
                // xform . b0 . b1 . b2 . b3 to . . b0 . . . b1 .
                __m128i skyB01_masked = _mm_and_si128(skyB01, maskHighByte);
                __m128i skyB23_masked = _mm_and_si128(skyB23, maskHighByte);
                
                __m128i skyB0 = _mm_unpacklo_epi8(skyB01_masked, zero);
                __m128i skyB1 = _mm_unpackhi_epi8(skyB01_masked, zero);
                __m128i skyB2 = _mm_unpacklo_epi8(skyB23_masked, zero);
                __m128i skyB3 = _mm_unpackhi_epi8(skyB23_masked, zero);
                
                // add everything together
                __m128i result0 = _mm_adds_epu8(_mm_adds_epu8(skyRG0, light0), skyB0);
                __m128i result1 = _mm_adds_epu8(_mm_adds_epu8(skyRG1, light1), skyB1);
                __m128i result2 = _mm_adds_epu8(_mm_adds_epu8(skyRG2, light2), skyB2);
                __m128i result3 = _mm_adds_epu8(_mm_adds_epu8(skyRG3, light3), skyB3);
                
                // store!
                __m128i* dest = (__m128i*)(dataRow + x * 4);
                
                _mm_storeu_si128(dest + 0, result0);
                _mm_storeu_si128(dest + 1, result1);
                _mm_storeu_si128(dest + 2, result2);
                _mm_storeu_si128(dest + 3, result3);
            }
            
            dataRow += rowPitch;
        }
        
        dataSlice += slicePitch;
    }
}
#endif

#ifdef SIMD_NEON
void LightGrid::lightingCompositImplSIMD(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch, const Color3uint8& skyAmbient)
{
    uint16x8_t round = vdupq_n_u16(255);
    
    uint8x8_t skyAmbientR = vdup_n_u8(skyAmbient.r);
    uint8x8_t skyAmbientG = vdup_n_u8(skyAmbient.g);
    uint8x8_t skyAmbientB = vdup_n_u8(skyAmbient.b);

    unsigned char* dataSlice = data;
    
    for (int y = 0; y < kLightGridChunkSizeY; ++y)
    {
        unsigned char* dataRow = dataSlice;
        
        for (int z = 0; z < kLightGridChunkSizeXZ; ++z)
        {
            for (int x = 0; x < kLightGridChunkSizeXZ; x += 16)
            {
                uint8x16_t skylight = vld1q_u8(&chunk.lightingSkylight[y][z][x]);
                
                uint8x16x4_t light = vld4q_u8(chunk.lighting[y][z][x]);

                uint8x8_t skylight01 = vget_low_u8(skylight);
                uint8x8_t skylight23 = vget_high_u8(skylight);

                uint16x8_t skyR01 = vmlal_u8(round, skylight01, skyAmbientR);
                uint16x8_t skyR23 = vmlal_u8(round, skylight23, skyAmbientR);

                uint16x8_t skyG01 = vmlal_u8(round, skylight01, skyAmbientG);
                uint16x8_t skyG23 = vmlal_u8(round, skylight23, skyAmbientG);

                uint16x8_t skyB01 = vmlal_u8(round, skylight01, skyAmbientB);
                uint16x8_t skyB23 = vmlal_u8(round, skylight23, skyAmbientB);

                uint8x16_t skyR = vuzpq_u8(vreinterpretq_u8_u16(skyR01), vreinterpretq_u8_u16(skyR23)).val[1];
                uint8x16_t skyG = vuzpq_u8(vreinterpretq_u8_u16(skyG01), vreinterpretq_u8_u16(skyG23)).val[1];
                uint8x16_t skyB = vuzpq_u8(vreinterpretq_u8_u16(skyB01), vreinterpretq_u8_u16(skyB23)).val[1];

                uint8x16x4_t result;
                result.val[0] = vqaddq_u8(skyR, light.val[0]);
                result.val[1] = vqaddq_u8(skyG, light.val[1]);
                result.val[2] = vqaddq_u8(skyB, light.val[2]);
                result.val[3] = light.val[3];

                // store!
                vst4q_u8(dataRow + x * 4, result);
            }
            
            dataRow += rowPitch;
        }
        
        dataSlice += slicePitch;
    }
}
#endif

void LightGrid::lightingUploadChunk(LightGridChunk& chunk)
{
    RBXPROFILER_SCOPE("Render", "lightingUploadChunk");
    
    if (!texture) return;
    
    Vector3int32 wrappedIndex = getWrappedChunkIndex(chunk.index);
    Vector3int32 gridPos = wrappedIndex * kLightGridChunkSize;
        
    if (texture->getType() == Texture::Type_3D)
    {
        TextureRegion region(gridPos.x, gridPos.z, gridPos.y, kLightGridChunkSizeXZ, kLightGridChunkSizeXZ, kLightGridChunkSizeY);

        if (texture->supportsLocking())
        {
            Texture::LockResult locked = texture->lock(0, 0, region);

            lightingComposit(chunk, static_cast<unsigned char*>(locked.data), locked.rowPitch, locked.slicePitch);
            
            texture->unlock(0, 0);
        }
        else
        {
            lightingComposit(chunk, uploadScratch[0][0][0], kLightGridChunkSizeXZ * 4, kLightGridChunkSizeXZ * kLightGridChunkSizeXZ * 4);

            texture->upload(0, 0, region, uploadScratch, sizeof(uploadScratch));
		}
	}
    else
    {
        // Assume a square total depth of the texture, with sqrt x sqrt tile packing
        int gridDepth = chunkCount.y * kLightGridChunkSizeY;
        int gridDepthSqrt = isqrt(gridDepth);
        
        int gridWidth = chunkCount.x * kLightGridChunkSizeXZ;
        int gridHeight = chunkCount.z * kLightGridChunkSizeXZ;
        
        lightingComposit(chunk, uploadScratch[0][0][0], kLightGridChunkSizeXZ * 4, kLightGridChunkSizeXZ * kLightGridChunkSizeXZ * 4);

        // Fill border row/columns with a border color
		fillLightingBorders(uploadScratch, chunk.index, cornerChunkIndex, chunkCount, transformColor(skyAmbient));

        for (int y = 0; y < kLightGridChunkSizeY; ++y)
        {
            int slice = (gridPos.y + y) % gridDepth;
            int sliceOffsetX = (slice % gridDepthSqrt) * gridWidth;
            int sliceOffsetY = (slice / gridDepthSqrt) * gridHeight;
            
            // We need to wrap-around here - chunk can't cross texture boundary, but chunks from the same slice can wrap around the boundary
            int boxLeft = (gridPos.x + sliceOffsetX) % texture->getWidth();
            int boxTop = (gridPos.z + sliceOffsetY) % texture->getHeight();

            TextureRegion sliceRegion(boxLeft, boxTop, kLightGridChunkSizeXZ, kLightGridChunkSizeXZ);

            texture->upload(0, 0, sliceRegion, uploadScratch[y], sizeof(uploadScratch[y]));
        }
    }
}

void LightGrid::lightingUploadCommit()
{
    RBXPROFILER_SCOPE("Render", "lightingUploadCommit");
    
    if (texture)
		texture->commitChanges();
}

void LightGrid::lightingClearAll()
{
    lightShadows = false;
	skyAmbient = Color3uint8();

    lightBorderGlobal = 255;
    lightBorderSkylight = 255;

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        LightGridChunk& chunk = *chunks[i];
        
        chunk.distancePriority = 0;
        chunk.dirty = 0;
        chunk.neverDirty = 0;
        chunk.age = 0;

        fillDummyLighting(chunk, lightBorderGlobal, lightBorderSkylight);
    }
}

void LightGrid::lightingUploadAll()
{
    if (!texture) return;
    
    if (texture->getType() == Texture::Type_3D && texture->supportsLocking())
    {
        // This is much faster than the per-chunk upload method
        TextureRegion region(0, 0, 0, texture->getWidth(), texture->getHeight(), texture->getDepth());
        Texture::LockResult locked = texture->lock(0, 0, region);
        
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            LightGridChunk& chunk = *chunks[i];
            
            Vector3int32 wrappedIndex = ((chunk.index % chunkCount) + chunkCount) % chunkCount;
            Vector3int32 gridPos = wrappedIndex * kLightGridChunkSize;
            
            RBXASSERT(locked.rowPitch >= kLightGridChunkSizeXZ * 4);
            
            unsigned char* dataSlice = static_cast<unsigned char*>(locked.data) + locked.slicePitch * gridPos.y + locked.rowPitch * gridPos.z + gridPos.x * 4;
            
            lightingComposit(chunk, dataSlice, locked.rowPitch, locked.slicePitch);
        }
        
        texture->unlock(0, 0);
    }
    else
    {
        for (size_t i = 0; i < chunks.size(); ++i)
            lightingUploadChunk(*chunks[i]);
    }
}

void LightGrid::relocateGrid(const Vector3int32& newCorner, bool skipChunkUpdate)
{
	RBXPROFILER_SCOPE("Render", "relocateGrid");
    
    // Gather all chunks and clear out pointers to them (we'll fill them back)
    std::vector<LightGridChunk*> oldChunks = chunks;
    
    for (size_t i = 0; i < chunks.size(); ++i)
        chunks[i] = NULL;
                
    // Put all old chunks that are in the new grid to their old locations
    for (size_t i = 0; i < oldChunks.size(); )
    {
        Vector3int32 localIndex = oldChunks[i]->index - newCorner;
        
        if (isChunkInsideGridLocal(localIndex))
        {
            unsigned int offset = getArrayOffset(localIndex, chunkCount);
            
            RBXASSERT(chunks[offset] == NULL);
            chunks[offset] = oldChunks[i];
            
            // Fast remove from array
            oldChunks[i] = oldChunks.back();
            oldChunks.pop_back();
        }
        else
        {
            i++;
        }
    }
    
    std::vector<LightGridChunk*> newChunks;
    
    // Now fill all remaining chunks with old chunks
    for (int cy = 0; cy < chunkCount.y; ++cy)
        for (int cz = 0; cz < chunkCount.z; ++cz)
            for (int cx = 0; cx < chunkCount.x; ++cx)
            {
                unsigned int offset = getArrayOffset(Vector3int32(cx, cy, cz), chunkCount);
                
                if (!chunks[offset])
                {
                    RBXASSERT(!oldChunks.empty());
                    
                    LightGridChunk* chunk = oldChunks.back();
                    oldChunks.pop_back();
                    
                    chunks[offset] = chunk;
                    
                    chunk->index = newCorner + Vector3int32(cx, cy, cz);
                    chunk->dirty = LightGridChunk::Dirty_Occupancy | LightGridChunk::Dirty_LightingGlobal | LightGridChunk::Dirty_LightingLocal | LightGridChunk::Dirty_LightingSkylight;
                    chunk->neverDirty = 0;
                    
                    newChunks.push_back(chunk);
                }
            }
    
    RBXASSERT(oldChunks.empty());

    // Update grid corner index
    cornerChunkIndex = newCorner;
                    
    if (!skipChunkUpdate)
    {
        // Fill new chunks with dummy data
        for (size_t i = 0; i < newChunks.size(); ++i)
            fillDummyLighting(*newChunks[i], lightBorderGlobal, lightBorderSkylight);

		if (texture && texture->getType() == Texture::Type_2D)
		{
            // Upload the entire grid since we bake border color in the texture
			lightingUploadAll();
		}
		else
		{
            // Upload new chunks so that we don't see stale data after relocation
            for (size_t i = 0; i < newChunks.size(); ++i)
				lightingUploadChunk(*newChunks[i]);
        }

		lightingUploadCommit();
    }
}

Color3uint8 LightGrid::transformColor(const Color3uint8& color) const
{
    return (visualEngine->getDevice()->getCaps().colorOrderBGR) ? color.bgr() : color;
}

Color4uint8 LightGrid::transformColor(const Color4uint8& color) const
{
    return Color4uint8(transformColor(color.rgb()), color.a);
}

Vector3int32 LightGrid::getWrappedChunkIndex(const Vector3int32& index) const
{
    if (texture && texture->getType() == Texture::Type_2D)
		return index - cornerChunkIndex;
    else
		return ((index % chunkCount) + chunkCount) % chunkCount;
}

void LightGrid::onDeviceRestored()
{
    FASTLOG(FLog::RenderLightGrid, "LightGrid: Device restored, reuploading the texture");
    
    lightingUploadAll();
}

}
}
