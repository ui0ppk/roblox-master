#pragma once

#include "rbx/Memory.h"
#include "GfxBase/GfxPart.h"
#include "RenderNode.h"
#include "SpatialGrid.h"
#include "TextureRef.h"

namespace RBX
{
	class PartInstance;
	class Humanoid;
	struct ClusterStats;
}

namespace RBX
{
namespace Graphics
{

class SceneManager;

class FastCluster;
class FastClusterEntity;
class SuperCluster;

class FastClusterEntity: public RenderEntity
{
public:
    FastClusterEntity(FastCluster* cluster, const GeometryBatch& geometry, const shared_ptr<Material>& material, const shared_ptr<Material>& decalMaterialOpaque,
		RenderQueue::Id renderQueueId, unsigned char lodMask, const std::vector<unsigned int>& bones, const Extents& localBounds, unsigned int extraFeatures);
    ~FastClusterEntity() override;

    // Renderable overrides
    unsigned int getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const override;
    
    // RenderNode overrides
    void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, unsigned int lodIndex, RenderQueue::Pass pass) override;
    float getViewDepth(const RenderCamera& camera) const override;
    
private:
    TextureRef decalTexture;
    shared_ptr<Material> decalMaterialOpaque;

    unsigned int extraFeatures;

    std::vector<unsigned int> bones;
    
    Vector3 localBoundsCenter;
    float sortKeyOffset;
};

class FastClusterBinding: public GfxBinding, public RBX::Allocator<FastClusterBinding>
{
public:
    FastClusterBinding(FastCluster* cluster, const boost::shared_ptr<PartInstance>& part);
    
    virtual void invalidateEntity();
    virtual void onCoordinateFrameChanged();
    virtual void onSizeChanged();
    virtual void onTransparencyChanged();
    virtual void onSpecialShapeChanged();
    virtual void unbind();
    
private:
    FastCluster* cluster;
};

struct FastClusterSharedGeometry
{
    FastClusterSharedGeometry();
    
    void reset();
    
    shared_ptr<VertexBuffer> vertexBuffer;
    shared_ptr<IndexBuffer> indexBuffer;
};

class FastCluster: public RenderNode
{
public:
    FastCluster(VisualEngine* visualEngine, Humanoid* humanoid, SuperCluster* owner, bool fw);
    virtual ~FastCluster();
    
    void addPart(const boost::shared_ptr<PartInstance>& part);

    // Used by SceneUpdater to determine cluster location in the grid
    void* getHumanoidKey() const { return humanoid; }
    const SpatialGridIndex& getSpatialIndex() const;
    bool isFW() const { return fw; }
	SuperCluster* getOwner() const { return owner; }

    // Transform access
    const CoordinateFrame& getTransform(unsigned int id) const
    {
        RBXASSERT(id < bones.size());
        return bones[id].transform;
    }
    
    void queueClusterCheck();
    void checkCluster();

    void markLightingDirty();
    void priorityInvalidateEntity();

    // GfxBinding overrides
    virtual void invalidateEntity();
    virtual void updateEntity(bool assetsUpdated);
    virtual void unbind();
    
    // GfxPart overrides
    virtual void updateCoordinateFrame(bool recalcLocalBounds);
    virtual unsigned int getPartCount();
    virtual void onSleepingChanged(bool sleeping, PartInstance* part);
    virtual void onClumpChanged(PartInstance* part);
	
	void getAllParts(std::vector< shared_ptr<PartInstance> >& retParts) const;
    PartInstance* getPart() const { if (parts.empty()) return NULL; return parts[0].instance; }
    
private:
    void checkBindings();
    void updateClumpGrouping();
    unsigned int updateGeometry(AsyncResult* asyncResult);

    void invalidateLighting(const Extents& bbox);
    
    ClusterStats& getStatsBucket();
    
    struct Bone
    {
        PartInstance* root;
        Extents localBounds;
        CoordinateFrame transform;
    };
    
    struct Part
    {
        PartInstance* instance;
        FastClusterBinding* binding;
    };

    std::vector<Bone> bones;
    std::vector<Part> parts;
    
    FastClusterSharedGeometry sharedGeometry;

    Humanoid* humanoid;
    SuperCluster*  owner;
    bool fw;
    
    bool dirty;
    bool lightDirty;
};


}
}
