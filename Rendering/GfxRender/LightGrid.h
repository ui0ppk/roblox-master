#pragma once

#include "rbx/Boost.hpp"

#include "Util/G3DCore.h"
#include "Util/Vector3int32.h"

#include "GfxCore/Resource.h"

#include "voxel/Voxelizer.h"

namespace RBX
{
	class PartInstance;
	class Frustum;
	class Extents;
	class MegaClusterInstance;
	class ContactManager;

    namespace Voxel
    {
        class Grid;
    }
}

namespace RBX
{
namespace Graphics
{

using RBX::Voxel::DataModelPartCache;
class VisualEngine;
class SpatialHashedScene;

struct LightShadowSlice;
struct LightShadowMap; 
class LightObject;

class Texture;

const int kLightGridChunkSizeXZ = kVoxelChunkSizeXZ;
const int kLightGridChunkSizeY = kVoxelChunkSizeY;

const Vector3int32 kLightGridChunkSize = kVoxelChunkSize;

struct LightGridChunkFringeY
{
    unsigned char data[kLightGridChunkSizeXZ][kLightGridChunkSizeXZ];
};

struct LightGridChunkFringeXZ
{
    unsigned char data[kLightGridChunkSizeY][kLightGridChunkSizeXZ];
};


struct LightGridChunk : Voxel::OccupancyChunk
{
    enum DirtyStatus
    {
        Dirty_LightingLocalShadowed = 1 << 1,
        Dirty_LightingLocal = 1 << 2,
        Dirty_LightingSkylight = 1 << 3,
        Dirty_LightingGlobal = 1 << 4,
        Dirty_LightingUpload = 1 << 5,
        Dirty_Occupancy = 1 << 6,
        Dirty_HighPriority = 1 << 7,
        
        Dirty_OccupancyAndDependents = Dirty_Occupancy | Dirty_LightingGlobal | Dirty_LightingSkylight | Dirty_LightingLocalShadowed
    };
    
    
    unsigned int distancePriority;
    unsigned int neverDirty;
    
    // 3D array order: [y][z][x]
    unsigned char lighting[kLightGridChunkSizeY][kLightGridChunkSizeXZ][kLightGridChunkSizeXZ][4];
    unsigned char lightingSkylight[kLightGridChunkSizeY][kLightGridChunkSizeXZ][kLightGridChunkSizeXZ];
    
    // Per-slice average of lighting data
    unsigned int lightingAverageGlobal[kLightGridChunkSizeY];
    unsigned int lightingAverageSkylight[kLightGridChunkSizeY];
    unsigned int lightingAverageWeight[kLightGridChunkSizeY];
    
    // fringe data for inter-chunk luminance propagation
    LightGridChunkFringeXZ irradianceFringeX;
    LightGridChunkFringeY irradianceFringeY;
    LightGridChunkFringeXZ irradianceFringeZ;
    
    LightGridChunkFringeY irradianceFringeSkyY;
    LightGridChunkFringeXZ irradianceFringeSkyX0;
    LightGridChunkFringeXZ irradianceFringeSkyX1;
    LightGridChunkFringeXZ irradianceFringeSkyZ0;
    LightGridChunkFringeXZ irradianceFringeSkyZ1;
    
};

class LightGrid: public Resource
{
public:
    enum TextureMode
    {
        Texture_None,
        Texture_2D,
        Texture_3D
    };

    static LightGrid* create(VisualEngine* visualEngine, const Vector3int32& chunkCount, TextureMode textureMode);

    LightGrid(VisualEngine* visualEngine, const shared_ptr<Texture>& texture, const shared_ptr<Texture>& lookupTexture, const Vector3int32& chunkCount);
    ~LightGrid();
    
    void occupancyUpdateChunkPrepare(Voxel::OccupancyChunk& chunk, MegaClusterInstance* terrain, ContactManager* contactManager, std::vector< DataModelPartCache >& partCache);
    void occupancyUpdateChunkPerform(const std::vector< DataModelPartCache >& partCache);
    
    void lightingUpdateChunkLocal(LightGridChunk& chunk, SpatialHashedScene* spatialHashedScene);
    void lightingUpdateChunkGlobal(LightGridChunk& chunk);
    void lightingUpdateChunkSkylight(LightGridChunk& chunk);
    void lightingUpdateChunkAverage(LightGridChunk& chunk);
    void lightingUploadChunk(LightGridChunk& chunk);
    void lightingUploadCommit();
    
    void lightingClearAll();
    void lightingUploadAll();
    
    bool updateGridCenter(const Vector3& cameraTarget, bool skipChunkUpdate);
    void updateBorderColor(const Vector3& cameraTarget, const Frustum& cameraFrustum);

    void updateAgePriorityForChunks(const Vector3& cameraTarget);
    
    void setLightShadows(bool value);
    void setLightDirection(const Vector3& value);
    void setSkyAmbient(const Color3uint8& value);
    
    void invalidateAll(unsigned int status);
    void invalidateExtents(const Extents& extents, unsigned int status);
    void invalidatePoint(const Vector3& point, unsigned int status);
    
    Color4uint8 computeAverageColor(const Vector3& focusPoint);
    
    Vector3 getGridCornerOffset() const;
    Vector3 getWrapSafeOffset() const;
    Vector3 getGridSize() const;
    Color4uint8 getBorderColor() const;
    Vector3int32 getChunkCount() const { return chunkCount; }
    
    bool hasTexture() const { return !!texture; }
    const shared_ptr<Texture>& getTexture() const { return texture; }
    const shared_ptr<Texture>& getLookupTexture() const { return lookupTexture; }

    LightGridChunk* findDirtyChunk();
    LightGridChunk* findFirstDirtyChunk();
    LightGridChunk* findOldestChunk();

    void setNonFixedPartsEnabled(bool value);
    bool getNonFixedPartsEnabled() const;

    virtual void onDeviceRestored();

private:
    LightGridChunk* getChunkByIndex(const Vector3int32& index);
    LightGridChunk* getChunkByLocalIndex(const Vector3int32& index);
    
    bool isChunkInsideGridLocal(const Vector3int32& index);

    Vector3int32 getChunkIndexForPoint(const Vector3& v);

    Vector3int32 getWrappedChunkIndex(const Vector3int32& index) const;

   
    void lightingUpdateDirectional(LightGridChunk& chunk, const Vector3& lightDirection);
    template <bool NegX, bool NegZ> void lightingUpdateDirectionalImpl(LightGridChunk& chunk, const Vector3int32& lightContrib);
    
    void lightingUpdateSkylight(LightGridChunk& chunk);
    void lightingUpdateSkylightRow(LightGridChunk& chunk, int y, int z, const unsigned char* powerCurveLUT);
    void lightingUpdateSkylightRowSIMD(LightGridChunk& chunk, int y, int z, const unsigned char* powerCurveLUT);

    void lightingUpdateAverageImpl(LightGridChunk& chunk);
    void lightingUpdateAverageImplSIMD(LightGridChunk& chunk);

    void lightingGetLights(std::vector<LightObject*>& lights, const Extents& chunkExtents, SpatialHashedScene* spatialHashedScene);

    void lightingUpdateLightScratch(const LightGridChunk& chunk, const Extents& chunkExtents, LightObject* lobj);
    
    void lightingComputeShadowMask(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, LightShadowMap& shadowMap);

    void lightingFixupShadowMaskBorder(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell);

    template <int Axis> void lightingTransferShadowSliceToShadowMask(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, int shadowMapSize, const LightShadowSlice& shadowSlice0, const LightShadowSlice& shadowSlice1);
    template <int Axis> void lightingTransferShadowMaskToShadowSlice(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, int shadowMapSize, LightShadowSlice& shadowSlice0, LightShadowSlice& shadowSlice1);

    void lightingComputeShadowMaskXYZ(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);
    template <bool NegX> void lightingComputeShadowMaskYZ(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);
    template <bool NegX, bool NegY> void lightingComputeShadowMaskZ(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);
    template <bool NegX, bool NegY, bool NegZ> void lightingComputeShadowMask(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);
    template <bool NegX, bool NegY, bool NegZ> void lightingComputeShadowMaskImplLUT(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);
    template <bool NegX, bool NegY, bool NegZ> void lightingComputeShadowMaskImplSmooth(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);
    template <bool NegX, bool NegY, bool NegZ> void lightingComputeShadowMaskImplSmoothSIMD(const LightGridChunk& chunk, const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3int32& lightCell, const Vector3& lightFraction);

    template <bool Shadows> void lightingUpdatePointLightScratch(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Color3uint8& lightColor, float lightIntensity);
    template <bool Shadows> void lightingUpdatePointLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Color3uint8& lightColor, float lightIntensity);
    
    template <bool Shadows> void lightingUpdateSpotLightScratch(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Color3uint8& lightColor, float lightIntensity);
    template <bool Shadows> void lightingUpdateSpotLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Color3uint8& lightColor, float lightIntensity);
    
    template <bool Shadows> void lightingUpdateSurfaceLightScratch(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Vector4& axisU, const Vector4& axisV, const Color3uint8& lightColor, float lightIntensity);
    template <bool Shadows> void lightingUpdateSurfaceLightScratchSIMD(const Vector3int32& extentsMin, const Vector3int32& extentsMax, const Vector3& lightPosition, float lightRadius, const Vector3& lightDirection, float lightConeAngle, const Vector4& axisU, const Vector4& axisV, const Color3uint8& lightColor, float lightIntensity);


    template <bool AxisY> void lightingBlurAxisYZScratch();
    template <bool AxisY> void lightingBlurAxisYZScratchSIMD();

    void lightingBlurAxisXScratchToChunk(LightGridChunk& chunk);
    void lightingBlurAxisXScratchToChunkSIMD(LightGridChunk& chunk);
    
    void lightingClearLocal(LightGridChunk& chunk);
    void lightingClearGlobal(LightGridChunk& chunk);
    
    void lightingComposit(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch);
    
    void lightingCompositImpl(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch, const Color3uint8& skyAmbient);
    void lightingCompositImplSIMD(const LightGridChunk& chunk, unsigned char* data, unsigned int rowPitch, unsigned int slicePitch, const Color3uint8& skyAmbient);
    
    void relocateGrid(const Vector3int32& newCorner, bool skipChunkUpdate);

    void stepCursor(Vector3int32& cursor);

    void precomputeShadowLUT();
    
    Color3uint8 transformColor(const Color3uint8& color) const;
    Color4uint8 transformColor(const Color4uint8& color) const;
    
    // 3D array order: [y][z][x]
    std::vector<LightGridChunk*> chunks;
    Vector3int32 chunkCount;
    Vector3int32 cornerChunkIndex;

    VisualEngine* visualEngine;
    shared_ptr<Texture> texture;
    shared_ptr<Texture> lookupTexture;
    
    bool useSIMD;
    
    bool lightShadows;
    Vector3 lightDirection;
    Color3uint8 skyAmbient;
    
    Vector3int32 dirtyCursor;
    Vector3int32 dirtyCursorDirection;

    unsigned char lightBorderGlobal;
    unsigned char lightBorderSkylight;
    
    LightGridChunkFringeXZ dummyFringeXZ255;
    LightGridChunkFringeY dummyFringeY255;

    struct ShadowLUTEntry
    {
        unsigned char x, y, z;
        unsigned char add;
    };

    enum { kShadowLUTSize = 16 };

    ShadowLUTEntry shadowLUT[kShadowLUTSize][kShadowLUTSize][kShadowLUTSize];
    
    unsigned char irradianceScratch[kLightGridChunkSizeY+2][kLightGridChunkSizeXZ+2][kLightGridChunkSizeXZ+2];
    
    // Each value here uses 10 bits per channel, with 8 bits for the value and 2 bits as carry bits (that are always zero except for intermediate values)
    unsigned int lightingScratch[kLightGridChunkSizeY+2][kLightGridChunkSizeXZ+2][kLightGridChunkSizeXZ+6];
    
    unsigned char shadowMask[kLightGridChunkSizeY+2][kLightGridChunkSizeXZ+2][kLightGridChunkSizeXZ+6];
    
    unsigned char uploadScratch[kLightGridChunkSizeY][kLightGridChunkSizeXZ][kLightGridChunkSizeXZ][4];

	Voxel::Voxelizer voxelizer;
};

}
}
