#include "stdafx.h"
#include "FastCluster.h"

#include "humanoid/Humanoid.h"

#include "GfxBase/AsyncResult.h"
#include "GfxBase/PartIdentifier.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Accoutrement.h"
#include "v8datamodel/PartCookie.h"

#include "v8world/Clump.h"
#include "Util/Math.h"

#include "SceneManager.h"
#include "GeometryGenerator.h"
#include "MaterialGenerator.h"
#include "Util.h"
#include "SceneUpdater.h"
#include "Material.h"

#include "GfxBase/RenderCaps.h"
#include "GfxBase/FrameRateManager.h"

#include "SuperCluster.h"

#include "VisualEngine.h"

#include "rbx/Profiler.h"

LOGVARIABLE(RenderFastCluster, 0)

namespace RBX
{
namespace Graphics
{

// This should be adjusted so that this number + worst-case number of vertices for any single part <= 65536 (16-bit index buffer limit)
const unsigned int kFastClusterBatchMaxVertices = 32768;

// This should be equal to the maximum number of vertices we can draw in a single call
const unsigned int kFastClusterBatchGroupMaxVertices = 65535;

class FastClusterMeshGenerator
{
public:
    FastClusterMeshGenerator(VisualEngine* visualEngine, RBX::Humanoid* humanoid, unsigned int maxBones, bool isFW)
        : visualEngine(visualEngine)
        , humanoidIdentifier(humanoid)
        , maxBonesPerBatch(0)
    {
        bones.reserve(maxBones);
        
        if (isFW || visualEngine->getRenderCaps()->getSkinningBoneCount() == 0)
        {
            maxBonesPerBatch = 1;
        }
        else
        {
            maxBonesPerBatch = visualEngine->getRenderCaps()->getSkinningBoneCount();
        }

		colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;
    }

	~FastClusterMeshGenerator()
	{
		visualEngine->getMaterialGenerator()->invalidateCompositCache();
	}
    
    const HumanoidIdentifier& getHumanoidIdentifier()
    {
        return humanoidIdentifier;
    }
    
    void addBone(RBX::PartInstance* root)
    {
        Bone bone;
        bone.root = root;
        bone.boundsMin = Vector3::maxFinite();
        bone.boundsMax = Vector3::minFinite();
        
        bones.push_back(bone);
    }
    
    void addInstance(size_t boneIndex, RBX::PartInstance* part, RBX::Decal* decal, unsigned int materialFlags, RenderQueue::Id renderQueue, RBX::AsyncResult* asyncResult)
    {
        RBXASSERT(boneIndex < bones.size());
        
        const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : NULL;
        
		MaterialGenerator::Result material = visualEngine->getMaterialGenerator()->createMaterial(part, decal, hi, materialFlags);
        
        if (!material.material)
            return;
        
        MaterialGroup& mg = materialGroups[std::make_pair(material.material.get(), renderQueue)];
        
        if (!mg.material)
        {
            mg.material = material.material;
			mg.renderQueue = renderQueue;
            mg.materialResultFlags = material.flags;
            mg.materialFeatures = material.features;

            if (decal)
			{
                unsigned int opaqueMaterialFlags = materialFlags & ~MaterialGenerator::Flag_Transparent;

				mg.decalMaterialOpaque = visualEngine->getMaterialGenerator()->createMaterial(part, decal, hi, opaqueMaterialFlags).material;
            }
        }
        else
        {
            RBXASSERT(mg.materialResultFlags == material.flags);
        }
        
        // force a batch break on transparent objects so that transparency sorting works
        // don't do this if the geometry is for the same part as the last one - this means a decal on different face,
        // decals on different faces of the same part never have transparency ordering issues
		bool forceBatchBreak = renderQueue == RenderQueue::Id_Transparent && part != getLastPart(mg);
        
        // create new batch on vertex count overflows
        if (mg.batches.empty() || mg.batches.back().counter.getVertexCount() >= kFastClusterBatchMaxVertices || forceBatchBreak)
        {
            mg.batches.push_back(Batch());
        }
        else
        {
            // create new batch on bone count overflow
            const Batch& batch = mg.batches.back();
            
            if (batch.bones.size() >= maxBonesPerBatch && batch.bones.back() != boneIndex)
            {
                mg.batches.push_back(Batch());
            } 
        }
        
        Batch& batch = mg.batches.back();
        
        // fetch any resources the part might need
        unsigned int resourceFlags = ((materialFlags & MaterialGenerator::Flag_UseCompositTexture) || (hi && hi->isPartHead(part)))
                ? GeometryGenerator::Resource_SubstituteBodyParts
                : 0;

        GeometryGenerator::Resources resources = GeometryGenerator::fetchResources(part, hi, resourceFlags, asyncResult);
        
        // accumulate geometry counters in the batch; actual geometry generation is done in finalize()
        GeometryGenerator::Options options;
        
        batch.counter.addInstance(part, decal, options, resources);
        
        // add new bone if needed
        if (batch.bones.empty() || batch.bones.back() != boneIndex)
        {
            RBXASSERT(batch.bones.size() < maxBonesPerBatch);
            
            batch.bones.push_back(boneIndex);
        }
        
        // add batch instance
        BatchInstance bi = {part, decal, batch.bones.size() - 1, material.uvOffsetScale, resources};
        
        batch.instances.push_back(bi);
    }
            
    unsigned int finalize(FastCluster* cluster, FastClusterSharedGeometry& sharedGeometry)
    {
        GeometryGenerator::Vertex* sharedVertexData = NULL;			
        unsigned short* sharedIndexData = NULL;
        unsigned int sharedVertexOffset = 0;
        unsigned int sharedIndexOffset = 0;

        // Gather pointers to all batches
        std::vector<std::pair<MaterialGroup*, Batch*> > batches;

        for (MaterialGroupMap::iterator it = materialGroups.begin(); it != materialGroups.end(); ++it)
        {
            MaterialGroup& mg = it->second;

            for (MaterialGroup::BatchList::iterator bit = mg.batches.begin(); bit != mg.batches.end(); ++bit)
            {
                Batch& batch = *bit;
                
                if (batch.instances.empty() || batch.counter.getVertexCount() == 0 || batch.counter.getIndexCount() == 0)
                    continue;

                batches.push_back(std::make_pair(&mg, &batch));
            }
        }

        // Sort batches to make sure batches that can be merged go first
        std::sort(batches.begin(), batches.end(), BatchMaterialPlasticLODComparator());

        // Get total size of the shared geometry data
        unsigned int sharedVertexCount = 0;
        unsigned int sharedIndexCount = 0;

        for (size_t i = 0; i < batches.size(); ++i)
        {
            Batch& batch = *batches[i].second;

            sharedVertexCount += batch.counter.getVertexCount();
            sharedIndexCount += batch.counter.getIndexCount();
        }

        // Update shared geometry so that it has the required amount of memory
        if (sharedVertexCount > 0 && sharedIndexCount > 0)
        {
            setupSharedGeometry(sharedGeometry, sharedVertexCount, sharedIndexCount, cluster->isFW());
            
            sharedVertexData = static_cast<GeometryGenerator::Vertex*>(sharedGeometry.vertexBuffer->lock(GeometryBuffer::Lock_Discard));
            sharedIndexData = static_cast<unsigned short*>(sharedGeometry.indexBuffer->lock(GeometryBuffer::Lock_Discard));
        }
        else
        {
            sharedGeometry.reset();
        }

        // Create as few geometry objects as we can
        shared_ptr<Geometry> geometry;
        unsigned int geometryBaseVertex = 0;
        
        // Create entities in groups
        for (size_t groupBegin = 0; groupBegin < batches.size(); )
        {
            unsigned int groupEnd = groupBegin + 1;

            unsigned int groupVertexCount = batches[groupBegin].second->counter.getVertexCount();

            unsigned int groupVertexOffset = sharedVertexOffset;
            unsigned int groupIndexOffset = sharedIndexOffset;
            Extents groupBounds;

            // Reuse geometry if generated indices stay within 16-bit boundaries
			if (!geometry || groupVertexOffset + groupVertexCount - geometryBaseVertex > kFastClusterBatchGroupMaxVertices)
			{
                geometry = visualEngine->getDevice()->createGeometry(getVertexLayout(), sharedGeometry.vertexBuffer, sharedGeometry.indexBuffer, groupVertexOffset);
                geometryBaseVertex = groupVertexOffset;
			}

            // Generate geometry for high LOD
            for (size_t bi = groupBegin; bi < groupEnd; ++bi)
            {
                const MaterialGroup& mg = *batches[bi].first;
                const Batch& batch = *batches[bi].second;
                
				unsigned int vertexOffset = sharedVertexOffset - geometryBaseVertex;

                // generate geometry
                std::vector<unsigned int> instanceVertexCount;
                
                Extents bounds = generateBatchGeometry(mg, batch, sharedVertexData + geometryBaseVertex, sharedIndexData + sharedIndexOffset, vertexOffset, instanceVertexCount, cluster->isFW());
                
                // create geometry batch
                GeometryBatch geometryBatch(geometry, Geometry::Primitive_Triangles, sharedIndexOffset, batch.counter.getIndexCount(), vertexOffset, vertexOffset + batch.counter.getVertexCount());

                // setup entity
                unsigned char lodMask = (groupEnd - groupBegin > 1) ? ((1 << 0) | (1 << 1)) : 0xff;

                // override render queue if character will cast shadow
                RenderQueue::Id queueId = mg.renderQueue;
                if (mg.renderQueue == RenderQueue::Id_Opaque && cluster->getHumanoidKey())
                    queueId = RenderQueue::Id_OpaqueCasters;
                if (mg.renderQueue == RenderQueue::Id_Transparent && cluster->getHumanoidKey())
                    queueId = RenderQueue::Id_TransparentCasters;

                cluster->addEntity(new FastClusterEntity(cluster, geometryBatch, mg.material, mg.decalMaterialOpaque, queueId, lodMask, batch.bones, bounds, mg.materialFeatures));
                
                // update buffer offsets
                sharedVertexOffset += batch.counter.getVertexCount();
                sharedIndexOffset += batch.counter.getIndexCount();
                
				groupBounds.expandToContain(bounds);
            }

            // Create merged entity for low LOD
            if (groupEnd - groupBegin > 1)
            {
                const MaterialGroup& mg = *batches[groupBegin].first;
                const Batch& batch = *batches[groupBegin].second;

                // create geometry batch
				GeometryBatch geometryBatch(geometry, Geometry::Primitive_Triangles, groupIndexOffset, sharedIndexOffset - groupIndexOffset, groupVertexOffset - geometryBaseVertex, sharedVertexOffset - geometryBaseVertex);

                // setup entity
				cluster->addEntity(new FastClusterEntity(cluster, geometryBatch, mg.material, mg.decalMaterialOpaque, mg.renderQueue, /* lodMask= */ 1 << 2, batch.bones, groupBounds, mg.materialFeatures));
            }

            // Move to next batch portion
            groupBegin = groupEnd;
        }
        
        if (sharedVertexData && sharedIndexData)
        {
            RBXASSERT(sharedVertexOffset == sharedVertexCount && sharedIndexOffset == sharedIndexCount);

			RBXPROFILER_SCOPE("Render", "upload");
            
            sharedGeometry.vertexBuffer->unlock();
            sharedGeometry.indexBuffer->unlock();
        }

        return sharedVertexCount;
    }

    unsigned int getBoneCount()
    {
        return bones.size();
    }
    
    RBX::PartInstance* getBoneRoot(unsigned int i)
    {
        return bones[i].root;
    }
    
    Extents getBoneBounds(unsigned int i)
    {
        const Bone& bone = bones[i];
        
        return Extents(bone.boundsMin, bone.boundsMax);
    }

private:
    struct BatchInstance
    {
        RBX::PartInstance* part;
        RBX::Decal* decal;
        unsigned int localBoneIndex;
        G3D::Vector4 uvOffsetScale;
        GeometryGenerator::Resources resources;
    };
    
    struct Batch
    {
        GeometryGenerator counter;
        std::vector<BatchInstance> instances;
        std::vector<unsigned int> bones;
    };
    
    struct MaterialGroup
    {
        shared_ptr<Material> material;
        RenderQueue::Id renderQueue;

        shared_ptr<Material> decalMaterialOpaque;
        unsigned int materialResultFlags;
        unsigned int materialFeatures;
        
        typedef std::list<Batch> BatchList;
        BatchList batches;
    };
    
    struct Bone
    {
        RBX::PartInstance* root;
        RBX::Vector3 boundsMin;
        RBX::Vector3 boundsMax;
    };
    
    typedef std::map<std::pair<Material*, RenderQueue::Id>, MaterialGroup> MaterialGroupMap;
    
    VisualEngine* visualEngine;
    HumanoidIdentifier humanoidIdentifier;

    unsigned int maxBonesPerBatch;
    bool colorOrderBGR;
    
    MaterialGroupMap materialGroups;
    std::vector<Bone> bones;

    RBX::PartInstance* getLastPart(const MaterialGroup& mg)
    {
        if (mg.batches.empty())
            return NULL;
            
        const Batch& batch = mg.batches.back();
        
        if (batch.instances.empty())
            return NULL;
            
        return batch.instances.back().part;
    }
    
    const shared_ptr<VertexLayout>& getVertexLayout()
    {
        shared_ptr<VertexLayout>& p = visualEngine->getFastClusterLayout();
        
        if (!p)
        {
            std::vector<VertexLayout::Element> elements;

            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, pos), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, normal), VertexLayout::Format_Float3, VertexLayout::Semantic_Normal));
            
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color, 0));

            bool useShaders = visualEngine->getRenderCaps()->getSkinningBoneCount() > 0;
                
            if (useShaders)
			{
                elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, extra), VertexLayout::Format_UByte4, VertexLayout::Semantic_Color, 1));
			}
			else
			{
                elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, extra), VertexLayout::Format_Color, VertexLayout::Semantic_Color, 1));
            }
            
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, uv), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture, 0));
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, uvStuds), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture, 1));
            
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, tangent), VertexLayout::Format_Float3, VertexLayout::Semantic_Texture, 2));
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, edgeDistances), VertexLayout::Format_Float4, VertexLayout::Semantic_Texture, 3));

            p = visualEngine->getDevice()->createVertexLayout(elements);
            RBXASSERT(p);
        }
        
        return p;
    }
    
    bool canUseBuffer(unsigned int currentCount, unsigned int desiredCount)
    {
        // We can use the current buffer if it has enough data and it does not waste >10% of memory
        return
            desiredCount <= currentCount &&
            currentCount - desiredCount < currentCount / 10;
    }

    void setupSharedGeometry(FastClusterSharedGeometry& sharedGeometry, unsigned int vertexCount, unsigned int indexCount, bool isFW)
    {
        // Create vertex buffer
        if (!sharedGeometry.vertexBuffer || !canUseBuffer(sharedGeometry.vertexBuffer->getElementCount(), vertexCount))
        {
            sharedGeometry.vertexBuffer = visualEngine->getDevice()->createVertexBuffer(sizeof(GeometryGenerator::Vertex), vertexCount, GeometryBuffer::Usage_Static);
        }
        
        // Create index buffer
        if (!sharedGeometry.indexBuffer || !canUseBuffer(sharedGeometry.indexBuffer->getElementCount(), indexCount))
        {
            sharedGeometry.indexBuffer = visualEngine->getDevice()->createIndexBuffer(sizeof(unsigned short), indexCount, GeometryBuffer::Usage_Static);
        }
    }

    CoordinateFrame getRelativeTransform(PartInstance* part, PartInstance* root)
    {
        if (part == root)
        {
            static const CoordinateFrame identity;
            
            return identity;
        }
        else if (root)
        {
            const CoordinateFrame& partFrame = part->getCoordinateFrame();
            const CoordinateFrame& rootFrame = root->getCoordinateFrame();
            
            return rootFrame.toObjectSpace(partFrame);
        }
        else
        {
            return part->getCoordinateFrame();
        }
    }
    
    Extents generateBatchGeometry(const MaterialGroup& mg, const Batch& batch, GeometryGenerator::Vertex* vbptr, unsigned short* ibptr, unsigned int vertexOffset, std::vector<unsigned int>& instanceVertexCount, bool isFW)
    {
		RBXPROFILER_SCOPE("Render", "generateBatchGeometry");

        instanceVertexCount.reserve(batch.instances.size());
        
        GeometryGenerator generator(vbptr, ibptr, vertexOffset);
        
        Vector3 boundsMin = Vector3::maxFinite();
        Vector3 boundsMax = Vector3::minFinite();
        
        const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : NULL;

        for (size_t i = 0; i < batch.instances.size(); ++i)
        {
            const BatchInstance& bi = batch.instances[i];
            Bone& bone = bones[batch.bones[bi.localBoneIndex]];

            GeometryGenerator::Options options(visualEngine, *bi.part, bi.decal, getRelativeTransform(bi.part, bone.root), mg.materialResultFlags, bi.uvOffsetScale, bi.localBoneIndex);

            generator.resetBounds();
            
            size_t oldVertexCount = generator.getVertexCount();
            
            generator.addInstance(bi.part, bi.decal, options, bi.resources, hi);
            
            instanceVertexCount.push_back(generator.getVertexCount() - oldVertexCount);

            if (generator.areBoundsValid())
            {
                RBX::AABox partBounds = generator.getBounds();
                
                bone.boundsMin = bone.boundsMin.min(partBounds.low());
                bone.boundsMax = bone.boundsMax.max(partBounds.high());
                boundsMin = boundsMin.min(partBounds.low());
                boundsMax = boundsMax.max(partBounds.high());
            }
        }

        // Patch stud UVs for FFP
		if (visualEngine->getDevice()->getCaps().supportsFFP && (mg.materialResultFlags & MaterialGenerator::Result_UsesTexture) == 0)
			for (unsigned int i = vertexOffset; i < generator.getVertexCount(); ++i)
				vbptr[i].uv = vbptr[i].uvStuds;
        
        RBXASSERT(generator.getVertexCount() == batch.counter.getVertexCount() + vertexOffset && generator.getIndexCount() == batch.counter.getIndexCount());
        
        return getBounds(boundsMin, boundsMax);
    }
    
    Extents getBounds(const Vector3& min, const Vector3& max)
    {
        if (min.x <= max.x)
            return Extents(min, max);
        else
            return Extents();
    }

    struct BatchMaterialPlasticLODComparator
    {
        bool operator()(const std::pair<MaterialGroup*, Batch*>& lhs, const std::pair<MaterialGroup*, Batch*>& rhs) const
        {
            MaterialGroup* ml = lhs.first;
            MaterialGroup* mr = rhs.first;

            return (ml->materialResultFlags & MaterialGenerator::Result_PlasticLOD) > (mr->materialResultFlags & MaterialGenerator::Result_PlasticLOD);

        }
    };
};

struct PartBindingNullPredicate
{
    template <typename T> bool operator()(const T& part) const
    {
        return part.binding == NULL;
    }
};

struct PartClumpSinglePredicate
{
    template <typename T> bool operator()(const T& part) const
    {
        const Primitive* p = part.instance->getConstPartPrimitive();
        const Primitive* clumpRoot = p->getRoot<Primitive>();
        
        return clumpRoot->numChildren() == 0;
    }
};

struct PartClumpGroupPredicate
{
    template <typename T> bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs.instance->getClump() < rhs.instance->getClump();
    }
};

FastClusterEntity::FastClusterEntity(FastCluster* cluster, const GeometryBatch& geometry, const shared_ptr<Material>& material, const shared_ptr<Material>& decalMaterialOpaque,
     RenderQueue::Id renderQueueId, unsigned char lodMask, const std::vector<unsigned int>& bones, const Extents& localBounds, unsigned int extraFeatures)
    : RenderEntity(cluster, geometry, material, renderQueueId, lodMask)
    , decalMaterialOpaque(decalMaterialOpaque)
    , extraFeatures(extraFeatures)
    , bones(bones)
    , localBoundsCenter(localBounds.center())
    , sortKeyOffset(0)
{
	if (decalMaterialOpaque)
    {
		if (const Technique* technique = decalMaterialOpaque->getBestTechnique(0, RenderQueue::Pass_Default))
            decalTexture = technique->getTexture(MaterialGenerator::getDiffuseMapStage());

        // if decal we want to offset slightly sort key
        sortKeyOffset = -0.01f;
    }
}

FastClusterEntity::~FastClusterEntity()
{
}

unsigned int FastClusterEntity::getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const
{
    if (useCache(cacheKey, this)) return 0;

    RBXASSERT(bones.size() <= maxTransforms);

    FastCluster* cluster = static_cast<FastCluster*>(node);
    
    for (unsigned int i = 0; i < bones.size(); ++i)
    {
        const CoordinateFrame& cframe = cluster->getTransform(bones[i]);

        memcpy(&buffer[0], cframe.rotation[0], sizeof(float) * 3);
        buffer[3] = cframe.translation.x;

        memcpy(&buffer[4], cframe.rotation[1], sizeof(float) * 3);
        buffer[7] = cframe.translation.y;

        memcpy(&buffer[8], cframe.rotation[2], sizeof(float) * 3);
        buffer[11] = cframe.translation.z;

        buffer += 12;
    }

    return bones.size();
}

void FastClusterEntity::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, unsigned int lodIndex, RenderQueue::Pass pass)
{
	TextureRef::Status decalStatus = decalTexture.getStatus();
	
	if (decalStatus != TextureRef::Status_Null)
	{
		if (decalStatus != TextureRef::Status_Loaded)
            return;

		if (renderQueueId == RenderQueue::Id_Decals)
		{
			if (!decalTexture.getInfo().alpha)
            {
                material = decalMaterialOpaque;
                renderQueueId = RenderQueue::Id_OpaqueDecals;
            }
            else
            {
                renderQueueId = RenderQueue::Id_TransparentDecals;
            }
		}

        decalTexture = TextureRef();
	}

	queue.setFeature(extraFeatures);

    RenderEntity::updateRenderQueue(queue, camera, lodIndex, pass);
}

float FastClusterEntity::getViewDepth(const RenderCamera& camera) const
{
    // getViewDepth should only be used for transparency sorting; then there should only be one bone
    RBXASSERT(bones.size() == 1);
        
    const CoordinateFrame& transform = static_cast<FastCluster*>(node)->getTransform(bones[0]);

    Vector3 center = transform.pointToWorldSpace(localBoundsCenter);
    
    return RenderEntity::computeViewDepth(camera, center, sortKeyOffset);
}

FastClusterBinding::FastClusterBinding(FastCluster* cluster, const shared_ptr<PartInstance>& part)
    : GfxBinding(part)
    , cluster(cluster)
{
    bindProperties(part);
    
    RBXASSERT(part->getGfxPart() == NULL);
    part->setGfxPart(cluster);
    
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: bound part %p to binding %p (%d connections)", cluster, part.get(), this, connections.size());

}
    
void FastClusterBinding::invalidateEntity()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests invalidateEntity", cluster, partInstance.get(), this);
    
    cluster->invalidateEntity();
}

void FastClusterBinding::onCoordinateFrameChanged()
{
    if (cluster->isFW())
    {
        FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests coordinate frame change", cluster, partInstance.get(), this);
    
        cluster->invalidateEntity();
        cluster->queueClusterCheck();
        cluster->markLightingDirty();
    }
}

void FastClusterBinding::onSizeChanged()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests size change", cluster, partInstance.get(), this);

    cluster->invalidateEntity();
    cluster->queueClusterCheck();
    cluster->markLightingDirty();
}

void FastClusterBinding::onTransparencyChanged()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests transparency change", cluster, partInstance.get(), this);

    cluster->invalidateEntity();
    cluster->markLightingDirty();
}

void FastClusterBinding::onSpecialShapeChanged()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests special shape change", cluster, partInstance.get(), this);

    cluster->invalidateEntity();
    cluster->markLightingDirty();
}

void FastClusterBinding::unbind()
{
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: unbind part %p from binding %p (%d connections)", cluster, partInstance.get(), this, connections.size());
    
    RBXASSERT(partInstance->getGfxPart() == cluster || partInstance->getGfxPart() == NULL);
    GfxBinding::unbind();
}

FastClusterSharedGeometry::FastClusterSharedGeometry()
{
}

void FastClusterSharedGeometry::reset()
{
    vertexBuffer.reset();
    indexBuffer.reset();
}
    
FastCluster::FastCluster(VisualEngine* visualEngine, Humanoid* humanoid, SuperCluster* owner, bool fw)
    : RenderNode(visualEngine, CullMode_SpatialHash, humanoid ? Flags_ShadowCaster : 0)
    , humanoid(humanoid)
    , owner(owner)
    , fw(fw)
    , dirty(false)
{
    if (humanoid)
        FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: create (humanoid %p)", this, humanoid);
    else
	{
		const SpatialGridIndex& spatialIndex = owner->getSpatialIndex();
        FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: create (grid %dx%dx%d, flags %d)", this, spatialIndex.position.x, spatialIndex.position.y, spatialIndex.position.z, spatialIndex.flags);
	}
    
    SceneUpdater* su = visualEngine->getSceneUpdater();

    // register non-FW clusters for updateCoordinateFrame
    if (!fw)
        su->notifyAwake(this);
    
    getStatsBucket().clusters++;
}


FastCluster::~FastCluster()
{
    unbind();
            
    // delete shared geometry
    sharedGeometry.reset();

    getStatsBucket().clusters--;
    
    // notify scene updater about destruction so that the pointer to FastCluster is no longer stored
    getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
}

void FastCluster::addPart(const boost::shared_ptr<PartInstance>& part)
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: addPart %p (part count was %d)", this, part.get(), parts.size());
    FASTLOGS(FLog::RenderFastCluster, "FastCluster part: %s", part->getFullName().c_str());
    
    Part p;
    p.instance = part.get();
    p.binding = new FastClusterBinding(this, part);
    
    parts.push_back(p);
    
    getStatsBucket().parts++;

    lightDirty = true;
}

void FastCluster::checkCluster()
{
    if (!owner) return;

    const SpatialGridIndex& spIndex = getSpatialIndex();

    FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: checking cluster (spatial index %dx%dx%d-%u)", this, spIndex.position.x, spIndex.position.y, spIndex.position.z, spIndex.flags);

    SceneUpdater* sceneUpdater = getVisualEngine()->getSceneUpdater();

    bool selfInvalidate = false;

    for (size_t i = 0; i < parts.size(); ++i)
    {
        Part& part = parts[i];

        if (!part.binding->isBound())
        {
            continue;
        }
        else
        {
            bool isPartFW = SceneUpdater::isPartStatic(part.instance);
            SpatialGridIndex newspIndex;

            newspIndex = owner->getSpatialGrid()->getIndexUnsafe(part.instance, isPartFW ? SpatialGridIndex::fFW : 0);

            static Vector3int16 m(-1,-1,-1),M(1,1,1);

            if ( spIndex.flags != newspIndex.flags || !( spIndex.position - newspIndex.position ).isBetweenInclusive(m,M) )
            {
                if(selfInvalidate == false)
                {
                    bool result = sceneUpdater->checkAddSeenFastClusters(newspIndex);
                    RBXASSERT(result);
                    priorityInvalidateEntity();
                }

                if(!sceneUpdater->checkAddSeenFastClusters(spIndex))
                {
                    RBXASSERT(selfInvalidate == true); // Should never fail on first added cluster
                    queueClusterCheck();
                    break;
                }

                if (spIndex.flags != newspIndex.flags)
                    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed FW status from %d to %d", this, part.instance, spIndex.flags, newspIndex.flags);
                else
                    FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed spatial index to %dx%dx%d", this, part.instance, spIndex.position.x, spIndex.position.y, spIndex.position.z);

                boost::shared_ptr<PartInstance> instance = shared_from(part.instance);

                part.binding->unbind();
                delete part.binding;
                part.binding = NULL;

                getVisualEngine()->getSceneUpdater()->queuePartToCreate(instance);

                getStatsBucket().parts--;

                selfInvalidate = true;

                lightDirty = true;
            }
        }
    }

    if(selfInvalidate)
    {
        parts.erase(std::remove_if(parts.begin(), parts.end(), PartBindingNullPredicate()), parts.end());
        owner->checkCluster( this );
    }
}

void FastCluster::invalidateEntity()
{
    FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: invalidateEntity", this);
        
    if (!dirty)
    {
        dirty = true;
        
        getVisualEngine()->getSceneUpdater()->queueInvalidateFastCluster(this);
    }
}

void FastCluster::priorityInvalidateEntity()
{
    FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: priorityInvalidateEntity", this);

    dirty = true;

    getVisualEngine()->getSceneUpdater()->queuePriorityInvalidateFastCluster(this);
}


void FastCluster::checkBindings()
{
    for (size_t i = 0; i < parts.size(); ++i)
    {
        Part& part = parts[i];
        
        if (!part.binding->isBound())
        {
            FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: part %p is no longer in workspace", this, part.instance);
    
            delete part.binding;
            part.binding = NULL;
        }
        else
        {
            Humanoid* newHumanoid = SceneUpdater::getHumanoid(part.instance);
            
            if (humanoid != newHumanoid)
            {
                FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed humanoid from %p to %p", this, part.instance, newHumanoid, humanoid);
                
                boost::shared_ptr<PartInstance> instance = shared_from(part.instance);
                
                part.binding->unbind();
                delete part.binding;
                part.binding = NULL;
                
                getVisualEngine()->getSceneUpdater()->queuePartToCreate(instance);
            }
        }
        
        if (!part.binding)
        {
            getStatsBucket().parts--;
            
            lightDirty = true;
        }
    }
    
    parts.erase(std::remove_if(parts.begin(), parts.end(), PartBindingNullPredicate()), parts.end());
}

void FastCluster::updateEntity(bool assetsUpdated)
{
    if(!assetsUpdated && !dirty)
    {
        FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: skipping updateEntity dirty: %d, assetsUpdated: %d", this, dirty, assetsUpdated);
        return;
    }
    
    RBX::Timer<RBX::Time::Precise> timer;

    // cluster is dirty at this point if updateEntity is a reaction to invalidateEntity, but it may not be dirty if
    // scene updater decides to update the cluster after a requested asset is ready.
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: updateEntity for %d parts (dirty: %d, light dirty: %d)", this, parts.size(), dirty, lightDirty);
    
    // reset dirty state before updating to check it after updating finished
    dirty = false;
    
    // update all part bindings
    checkBindings();
    
    // invalidate lighting for the old AABB
    if (lightDirty)
        invalidateLighting(getWorldBounds());
    
    // if we don't need this cluster any more, destroy it
    if (parts.empty())
    {
        FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: destroy (no more parts)", this);
    
        if (owner)
            owner->destroyFastCluster(this); // this call deletes the object, should be the last call in this function
        else
            getVisualEngine()->getSceneUpdater()->destroyFastCluster(this); // this call deletes the object, should be the last call in this function
        return;
    }
    
    // update cluster geometry
    AsyncResult asyncResult;
    
    updateClumpGrouping();
    unsigned int totalVertexCount = updateGeometry(&asyncResult);
    
    // subscribe for updateEntity when some pending assets are done loading
    getVisualEngine()->getSceneUpdater()->notifyWaitingForAssets(this, asyncResult.waitingFor);
        
    // update block count used for FRM-based culling
    setBlockCount(parts.size());

    // this call updates the AABB
    updateCoordinateFrame(false);
    
    // invalidate lighting for the new AABB
    if (lightDirty)
        invalidateLighting(getWorldBounds());
        
    // reset light dirty; this means that any pending changes except for the changes in bone transforms have resulted in light invalidation
    lightDirty = false;

    // we should not do invalidateEntity from updateEntity - this results in excessive invalidations
    RBXASSERT(!dirty);
    
    FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: updated geometry for %d parts in %d usec (%d entities, %d vertices)", this, parts.size(), (int)(timer.delta().msec() * 1000), entities.size(), totalVertexCount);
}

void FastCluster::updateClumpGrouping()
{
    // no need to update grouping for FW
    if (fw) return;
    
    // move all single-clump parts to the beginning (to make sorting cheaper)
    std::vector<Part>::iterator middle = std::partition(parts.begin(), parts.end(), PartClumpSinglePredicate());
    
    // sort remaining parts by clump to use one bone for clump
    std::sort(middle, parts.end(), PartClumpGroupPredicate());
    
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: %d parts, %d single-clump parts, %d multi-clump parts", this, parts.size(), middle - parts.begin(), parts.end() - middle);
}

unsigned int FastCluster::updateGeometry(AsyncResult* asyncResult)
{
	RBXPROFILER_SCOPE("Render", "updateGeometry");

    FastClusterMeshGenerator generator(getVisualEngine(), humanoid, parts.size(), fw);
    
    const HumanoidIdentifier& hi = generator.getHumanoidIdentifier();

    // for FW cluster all parts use one pseudo-bone
    if (fw)
        generator.addBone(NULL);
    
    // generate geometry for parts
    RBX::Clump* lastClump = NULL;
    
    for (size_t parti = 0; parti < parts.size(); ++parti)
    {
        Part& part = parts[parti];
        
		// Material flags
        bool ignoreDecals = false;
		unsigned int materialFlags = MaterialGenerator::createFlags(!fw, part.instance, &hi, ignoreDecals);
		bool partTransparent = (part.instance->getTransparencyUi() > 0);
  
        // add a new bone if necessary
        Clump* clump = part.instance->getClump();
        
        if (!fw && ( !clump || clump != lastClump ) )
        {
            generator.addBone(part.instance);
            
            lastClump = clump;
        }
        
        unsigned int boneIndex = generator.getBoneCount() - 1;
        
        // add part geometry
        if (part.instance->getTransparencyUi() < 1)
			generator.addInstance(boneIndex, part.instance, NULL, materialFlags, partTransparent ? RenderQueue::Id_Transparent : RenderQueue::Id_Opaque, asyncResult);
        
        // add part decals / textures
        if ((part.instance->getCookie() & PartCookie::HAS_DECALS) && part.instance->getChildren() && !ignoreDecals)
        {
            unsigned int decalMaterialFlags = (materialFlags & MaterialGenerator::Flag_Skinned) | MaterialGenerator::Flag_Transparent;
                    
            const Instances& children = *part.instance->getChildren();
            
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (Decal* decal = children[i]->fastDynamicCast<Decal>())
                {
					float decalTransparency = decal->getTransparencyUi();
					if (decalTransparency < 1)
					{
						RenderQueue::Id renderQueue = partTransparent ? RenderQueue::Id_Transparent : decalTransparency > 0 ? RenderQueue::Id_TransparentDecals : RenderQueue::Id_Decals;

						generator.addInstance(boneIndex, part.instance, decal, decalMaterialFlags, renderQueue, asyncResult);
					}
                }
            }
        }
    }

    // Destroy all existing entities
    for (size_t i = 0; i < entities.size(); ++i)
        delete entities[i];
        
    entities.clear();

    // Generate new entities
    unsigned int vertexCount = generator.finalize(this, sharedGeometry);
    
    // Retrieve bone data
    bones.resize(generator.getBoneCount());
    
    for (size_t i = 0; i < bones.size(); ++i)
    {
        bones[i].root = generator.getBoneRoot(i);
        bones[i].localBounds = generator.getBoneBounds(i);
        bones[i].transform = bones[i].root ? bones[i].root->getCoordinateFrame() : CoordinateFrame();
    }

    return vertexCount;
}

ClusterStats& FastCluster::getStatsBucket()
{
    RenderStats* stats = getVisualEngine()->getRenderStats();
    
    if (humanoid)
        return stats->clusterFastHumanoid;
    else if (fw)
        return stats->clusterFastFW;
    else
        return stats->clusterFast;
}

void FastCluster::updateCoordinateFrame(bool recalcLocalBounds)
{
    Extents oldWorldBB = getWorldBounds();
    Extents newWorldBB;
    
    bool bonesChanged = false;

	RBX::Time now = RBX::Time::nowFast();

    for (size_t i = 0; i < bones.size(); ++i)
    {
        Bone& bone = bones[i];
        
        if (bone.root && !bone.root->getSleeping())
        {
            CoordinateFrame frame = bone.root->calcRenderingCoordinateFrame(now);

            if (!Math::fuzzyEq(bone.transform, frame))
                bonesChanged = true;

            bone.transform = frame;
            
            if ( owner && owner->getSpatialGrid()->getIndexUnsafe(bone.root, fw ? SpatialGridIndex::fFW : 0) != owner->getSpatialIndex() )
            {
                // Bone moved significantly, re-cluster
                queueClusterCheck();
            }
        }

        newWorldBB.expandToContain(bone.localBounds.toWorldSpace(bone.transform));
    }

    updateWorldBounds(newWorldBB);

    if (bonesChanged)
    {
        invalidateLighting(oldWorldBB);
        invalidateLighting(newWorldBB);
    }
}

void FastCluster::invalidateLighting(const Extents& bbox)
{
    if (humanoid) return;

    if (!bbox.isNull())
    {
        getVisualEngine()->getSceneUpdater()->lightingInvalidateOccupancy(bbox, bbox.center(), fw);
    }
}

void FastCluster::unbind()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: unbind (%d parts, %d own connections)", this, parts.size(), connections.size());
        
    GfxPart::unbind();
    
    getStatsBucket().parts -= parts.size();
    
    for (size_t i = 0; i < parts.size(); ++i)
    {
        Part& part = parts[i];
        
        part.binding->unbind();
        delete part.binding;
    }
    
    bones.clear();
    parts.clear();
}

void FastCluster::onSleepingChanged(bool sleeping, PartInstance* part)
{
    if (!owner) return;
    
    // FW parts react on wakeup, non-FW parts react on sleep
    if ((fw && !sleeping) || (!fw && sleeping))
    {
        bool isPartFW = SceneUpdater::isPartStatic(part);
        
        if (isPartFW != fw)
        {
            FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: part %p, sleeping %d (fw %d -> %d)", this, part, sleeping, fw, isPartFW);
            
            queueClusterCheck();
        }
    }
}

void FastCluster::onClumpChanged(PartInstance* part)
{
    if (!fw)
	{
        FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: part %p requests clump change", this, part);
    
        invalidateEntity();
	}
}

void FastCluster::queueClusterCheck()
{
    if (!owner) return;
    getVisualEngine()->getSceneUpdater()->queueFastClusterCheck(this, isFW());
}

void FastCluster::markLightingDirty()
{
    lightDirty = true;
}

unsigned int FastCluster::getPartCount()
{
    return parts.size();
}

const SpatialGridIndex& FastCluster::getSpatialIndex() const
{
    return owner->getSpatialIndex();
}

void FastCluster::getAllParts(std::vector< shared_ptr<PartInstance> >& retParts) const
{
    retParts.resize(parts.size());
    for( int j=0,e=parts.size(); j<e; ++j )
    {
        retParts[j] = shared_from( parts[j].instance );
    }
}

}
}
