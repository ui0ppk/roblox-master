#include "stdafx.h"
#include "SmoothCluster.h"

#include "v8datamodel/MegaCluster.h"

#include "SceneUpdater.h"

#include "Util.h"
#include "FastLog.h"

#include "Water.h"

#include "Material.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "LightGrid.h"
#include "SceneManager.h"
#include "VisualEngine.h"
#include "EnvMap.h"

#include "GfxBase/RenderCaps.h"

#include "voxel2/MaterialTable.h"
#include "voxel2/Grid.h"
#include "voxel2/Mesher.h"

#include "rbx/Profiler.h"

using namespace RBX::Voxel;

FASTFLAGVARIABLE(SmoothTerrainRenderLOD, false)
FASTFLAGVARIABLE(DebugSmoothTerrainRenderFixedLOD, false)

namespace RBX
{
namespace Graphics
{

static std::pair<Vector4, Vector4> getPackInfo(const Voxel2::Region& region)
{
    int voxelSize = Voxel::kCELL_SIZE;
    int size = std::max(region.size().x, std::max(region.size().y, region.size().z)) * voxelSize;

    // we round the factor to power-of-two to avoid fp rounding issues
    // technically max int16 is 32767, but after rounding we have a bit of slack so we can use 32768
    Vector3 unpackOffset = region.begin().toVector3() * voxelSize;
    float unpackScale = float(G3D::ceilPow2(size)) / 32768.f;
    
    // our source position data is not scaled by voxel size so we'd have to scale it before packing
    float packScale = float(voxelSize) / unpackScale;

    // offset is applied after scale so we have to scale it as well
    // additionally we use floor to resolve micro-gaps due to different offsets in different chunks
    Vector3 packOffset = Vector3(floorf(-unpackOffset.x / unpackScale), floorf(-unpackOffset.y / unpackScale), floorf(-unpackOffset.z / unpackScale));
    
    return std::make_pair(Vector4(packOffset, packScale), Vector4(unpackOffset, unpackScale));
}

class SmoothClusterRenderEntity: public RenderEntity
{
public:
	SmoothClusterRenderEntity(RenderNode* node, const GeometryBatch& geometry, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, std::vector<float>* constantTable, const Vector4& unpackInfo)
        : RenderEntity(node, geometry, material, renderQueueId)
        , constantTable(constantTable)
        , unpackInfo(unpackInfo)
	{
	}

	unsigned int getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const override
	{
		if (useCache(cacheKey, constantTable))
		{
			memcpy(buffer, &unpackInfo, 4 * sizeof(float));
			return 1;
		}

		RBXASSERT(constantTable->size() <= maxTransforms * 3 * 4);
		
		memcpy(buffer, &(*constantTable)[0], constantTable->size() * sizeof(float));
		memcpy(buffer, &unpackInfo, 4 * sizeof(float));
		
		return constantTable->size() / 4 / 3;
	}

private:
    std::vector<float>* constantTable;
    Vector4 unpackInfo;
};

SmoothClusterBase::SmoothClusterBase(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part)
    : visualEngine(visualEngine)
    , grid(NULL)
{
    RBXASSERT(part->getPartType() == MEGACLUSTER_PART);
    partInstance = part;

    RBXASSERT(partInstance->getGfxPart() == NULL);
    partInstance->setGfxPart(this);

    MegaClusterInstance* mci = boost::polymorphic_downcast<MegaClusterInstance*>(part.get());

	grid = mci->getSmoothGrid();

    grid->connectListener(this);
    
    connections.push_back(part->ancestryChangedSignal.connect(boost::bind(&SmoothClusterBase::zombify, this)));

	materialTable = mci->getMaterialTable();

	updateMaterialConstants();
}

SmoothClusterBase::~SmoothClusterBase()
{
    unbind();
    
    // notify scene updater about destruction so that the pointer to cluster is no longer stored
	visualEngine->getSceneUpdater()->notifyDestroyed(this);
}

void SmoothClusterBase::unbind()
{
	GfxPart::unbind();

	if (grid)
	{
		grid->disconnectListener(this);

		grid = NULL;
	}
}

void SmoothClusterBase::invalidateEntity()
{
	visualEngine->getSceneUpdater()->queueFullInvalidateMegaCluster(this);
}

static shared_ptr<IndexBuffer> uploadIndices(Device* device, const std::vector<unsigned int>& indices, bool needIndex32)
{
	if (needIndex32)
	{
		shared_ptr<IndexBuffer> ibuf = device->createIndexBuffer(sizeof(unsigned int), indices.size(), GeometryBuffer::Usage_Static);

        ibuf->upload(0, &indices[0], sizeof(unsigned int) * indices.size());

		return ibuf;
	}
	else
	{
		shared_ptr<IndexBuffer> ibuf = device->createIndexBuffer(sizeof(unsigned short), indices.size(), GeometryBuffer::Usage_Static);

		std::vector<unsigned short> ibdata(indices.begin(), indices.end());

        ibuf->upload(0, &ibdata[0], sizeof(unsigned short) * indices.size());

		return ibuf;
	}
}

std::pair<RenderEntity*, RenderEntity*> SmoothClusterBase::uploadGeometry(RenderNode* node, const Vector4& unpackInfo,
    size_t vertexSize, const void* vertices, size_t vertexCount,
    const std::vector<unsigned int>& solidIndices, const std::vector<unsigned int>& waterIndices)
{
	RBXPROFILER_SCOPE("Render", "uploadGeometry");

	using namespace Voxel2::Mesher;

    Device* device = visualEngine->getDevice();
    const DeviceCaps& caps = device->getCaps();

	bool needIndex32 = vertexCount > 65534;

	if (!caps.supportsShaders || (needIndex32 && !caps.supportsIndex32))
		return std::pair<RenderEntity*, RenderEntity*>();

	shared_ptr<VertexBuffer> vbuf = device->createVertexBuffer(vertexSize, vertexCount, GeometryBuffer::Usage_Static);

	vbuf->upload(0, vertices, vertexSize * vertexCount);

	RenderEntity* solidEntity = NULL;

	if (!solidIndices.empty())
	{
		shared_ptr<IndexBuffer> ibuf = uploadIndices(device, solidIndices, needIndex32);
		shared_ptr<Material> material = getSolidMaterial();
        shared_ptr<Geometry> geometry = device->createGeometry(getVertexLayout(), vbuf, ibuf);

        solidEntity = new SmoothClusterRenderEntity(node, GeometryBatch(geometry, Geometry::Primitive_Triangles, ibuf->getElementCount(), vbuf->getElementCount()), material, RenderQueue::Id_Opaque, &materialConstants, unpackInfo);
	}

	RenderEntity* waterEntity = NULL;

	if (!waterIndices.empty())
	{
		shared_ptr<IndexBuffer> ibuf = uploadIndices(visualEngine->getDevice(), waterIndices, needIndex32);
        shared_ptr<Material> material = visualEngine->getWater()->getSmoothMaterial();
        shared_ptr<Geometry> geometry = device->createGeometry(getVertexLayout(), vbuf, ibuf);

        waterEntity = new SmoothClusterRenderEntity(node, GeometryBatch(geometry, Geometry::Primitive_Triangles, ibuf->getElementCount(), vbuf->getElementCount()), material, RenderQueue::Id_TransparentUnsorted, &materialConstants, unpackInfo);
	}
    
	return std::make_pair(solidEntity, waterEntity);
}

const shared_ptr<VertexLayout>& SmoothClusterBase::getVertexLayout()
{
    if (!vertexLayout)
    {
        std::vector<VertexLayout::Element> elements;

		using namespace Voxel2::Mesher;
        
		elements.push_back(VertexLayout::Element(0, offsetof(GraphicsVertexPacked, position), VertexLayout::Format_Short4, VertexLayout::Semantic_Position));
		elements.push_back(VertexLayout::Element(0, offsetof(GraphicsVertexPacked, normal), VertexLayout::Format_UByte4, VertexLayout::Semantic_Normal));
		elements.push_back(VertexLayout::Element(0, offsetof(GraphicsVertexPacked, material0), VertexLayout::Format_UByte4, VertexLayout::Semantic_Texture, 0));
		elements.push_back(VertexLayout::Element(0, offsetof(GraphicsVertexPacked, material1), VertexLayout::Format_UByte4, VertexLayout::Semantic_Texture, 1));

        vertexLayout = visualEngine->getDevice()->createVertexLayout(elements);
        RBXASSERT(vertexLayout);
    }
    
    return vertexLayout;
}

#ifdef RBX_PLATFORM_IOS
static const std::string kTextureExtension = ".pvr";
#elif defined(__ANDROID__)
static const std::string kTextureExtension = ".pvr";
#else
static const std::string kTextureExtension = ".dds";
#endif

static void setupTextures(VisualEngine* visualEngine, Technique& technique)
{
    TextureManager* tm = visualEngine->getTextureManager();
    LightGrid* lightGrid = visualEngine->getLightGrid();
	SceneManager* sceneManager = visualEngine->getSceneManager();

	int lodIndex = technique.getLodIndex();

	technique.setTexture(0, tm->load(ContentId("rbxasset://terrain/diffuse" + kTextureExtension), TextureManager::Fallback_White), lodIndex < 2 ? SamplerState::Filter_Anisotropic : SamplerState::Filter_Linear);

	if (lodIndex < 1)
		technique.setTexture(1, tm->load(ContentId("rbxasset://terrain/normal" + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Linear);

	if (lodIndex < 2)
		technique.setTexture(2, tm->load(ContentId("rbxasset://terrain/specular" + kTextureExtension), TextureManager::Fallback_Black), SamplerState::Filter_Linear);

	technique.setTexture(3, sceneManager->getEnvMap()->getTexture(), SamplerState::Filter_Linear);

    if (lightGrid)
    {
        technique.setTexture(4, lightGrid->getTexture(), SamplerState::Filter_Linear);
		technique.setTexture(5, lightGrid->getLookupTexture(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
    }

	technique.setTexture(6, sceneManager->getShadowMap(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
}

const shared_ptr<Material>& SmoothClusterBase::getSolidMaterial()
{
    if (solidMaterial)
        return solidMaterial;

	const Voxel2::MaterialTable::Atlas& atlas = materialTable->getAtlas();

	Vector4 layerScale = Vector4(float(atlas.tileSize) / float(atlas.width), float(atlas.tileSize) / float(atlas.height), 0, 0);

    solidMaterial = shared_ptr<Material>(new Material());
    
    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SmoothClusterHQVS", "SmoothClusterHQGBufferFS"))
    {
        Technique technique(program, 0);

		setupTextures(visualEngine, technique);

		technique.setConstant("LayerScale", layerScale);

        solidMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("SmoothClusterHQVS", "SmoothClusterHQFS"))
    {
        Technique technique(program, 1);

		setupTextures(visualEngine, technique);

		technique.setConstant("LayerScale", layerScale);

        solidMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP("SmoothClusterVS", "SmoothClusterFS"))
    {
        Technique technique(program, 2);

		setupTextures(visualEngine, technique);

		technique.setConstant("LayerScale", layerScale);

        solidMaterial->addTechnique(technique);
    }

    return solidMaterial;
}

void SmoothClusterBase::reloadMaterialTable()
{
    MegaClusterInstance* mci = boost::polymorphic_downcast<MegaClusterInstance*>(partInstance.get());

	materialTable = mci->getMaterialTable();

	updateMaterialConstants();

	visualEngine->getSceneUpdater()->queueFullInvalidateMegaCluster(this);
}

void SmoothClusterBase::updateMaterialConstants()
{
	materialConstants.clear();

    // Space for packing information, will be filled by getWorldTransforms4x3
    for (unsigned int i = 0; i < 4; ++i)
    {
        materialConstants.push_back(0);
    }

	const Voxel2::MaterialTable::Atlas& atlas = materialTable->getAtlas();

	int tileFullSize = atlas.tileSize + atlas.borderSize * 2;

	for (unsigned int i = 0; i < 18; ++i)
	{
		const Vector3& u = Voxel2::Mesher::getTextureBasisU()[i];

		materialConstants.push_back(u.x);
		materialConstants.push_back(u.y);
		materialConstants.push_back(u.z);
		materialConstants.push_back(0);
	}

	for (unsigned int i = 0; i < 18; ++i)
	{
		const Vector3& v = Voxel2::Mesher::getTextureBasisV()[i];

		materialConstants.push_back(v.x);
		materialConstants.push_back(v.y);
		materialConstants.push_back(v.z);
		materialConstants.push_back(0);
	}

    for (unsigned int i = 0; i < materialTable->getLayerCount(); ++i)
    {
        const Voxel2::MaterialTable::Layer& desc = materialTable->getLayer(i);

		materialConstants.push_back(desc.tiling / Voxel::kCELL_SIZE);
		materialConstants.push_back(desc.detiling);
		materialConstants.push_back(((i % atlas.tileCount) * tileFullSize + atlas.borderSize) / float(atlas.width));
        materialConstants.push_back(((i / atlas.tileCount) * tileFullSize + atlas.borderSize) / float(atlas.height));
    }

	// Align to size of 3x4 matrix
	materialConstants.resize((materialConstants.size() + 11) / 12 * 12);
}

const Vector3int32 SmoothClusterChunked::kChunkSizeLog2(5, 4, 5);
const Vector3int32 SmoothClusterChunked::kChunkSize = Vector3int32::one() << kChunkSizeLog2;
const int SmoothClusterChunked::kChunkBorder = 2;

SmoothClusterChunked::SmoothClusterChunked(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part)
	: SmoothClusterBase(visualEngine, part)
{
}

SmoothClusterChunked::~SmoothClusterChunked()
{
}

void SmoothClusterChunked::updateEntity(bool assetsUpdated)
{
	if (!grid)
	{
		delete this;
		return;
	}

    if (!grid->isAllocated())
        return;

    std::vector<Voxel2::Region> regions = grid->getNonEmptyRegions();

    for (size_t i = 0; i < regions.size(); ++i)
        onTerrainRegionChanged(regions[i]);
}

void SmoothClusterChunked::onTerrainRegionChanged(const Voxel2::Region& region)
{
	Voxel2::Region chunks = region.expand(kChunkBorder);

	Vector3int32 min = chunks.begin() >> kChunkSizeLog2;
	Vector3int32 max = (chunks.end() + kChunkSize - Vector3int32(1, 1, 1)) >> kChunkSizeLog2;

    for (int y = min.y; y <= max.y; ++y)
		for (int z = min.z; z <= max.z; ++z)
			for (int x = min.x; x <= max.x; ++x)
				markDirty(SpatialRegion::Id(x, y, z));
}

void SmoothClusterChunked::markDirty(const SpatialRegion::Id& pos)
{
    ChunkData& chunk = chunks.insert(pos);
    
    SceneUpdater* sceneUpdater = visualEngine->getSceneUpdater();
    
    if (!chunk.dirty)
	{
        chunk.dirty = true;
        sceneUpdater->queueChunkInvalidateMegaCluster(this, pos, false);
    }
}

void SmoothClusterChunked::updateChunk(const SpatialRegion::Id& pos, bool isWaterChunk)
{
	RBXPROFILER_SCOPE("Render", "updateChunk");

    ChunkData& chunk = chunks.insert(pos);
    
    // Delete existing entities
    if (chunk.solidEntity)
    {
        chunk.node->removeEntity(chunk.solidEntity);
        delete chunk.solidEntity;
    }

    if (chunk.waterEntity)
    {
        chunk.node->removeEntity(chunk.waterEntity);
        delete chunk.waterEntity;
    }

    // Update chunk geometry
	unsigned int quads = 0;
	std::pair<RenderEntity*, RenderEntity*> p = createChunkGeometry(pos, &quads);
        
	chunk.solidEntity = p.first;
	chunk.waterEntity = p.second;
	chunk.dirty = false;
        
    if (chunk.solidEntity)
        chunk.node->addEntity(chunk.solidEntity);

	if (chunk.waterEntity)
        chunk.node->addEntity(chunk.waterEntity);
    
    // Update quad stats or destroy node if necessary
    if (!chunk.solidEntity && !chunk.waterEntity)
    {
        chunk = ChunkData();
    }
    else
    {
		chunk.node->setBlockCount(quads / ChunkData::kApproximateQuadsPerPart);
    }
}

RenderNode* SmoothClusterChunked::updateChunkNode(const SpatialRegion::Id& pos)
{
    ChunkData& chunk = chunks.insert(pos);
    
    // Update chunk node
    if (!chunk.node)
    {
        // Create chunk node
        chunk.node.reset(new RenderNode(visualEngine, RenderNode::CullMode_SpatialHash));
        
        Vector3int32 minLocation = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(pos);
        Vector3int32 maxLocation = SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(pos);

        chunk.node->setCoordinateFrame(CoordinateFrame(minLocation.toVector3()));
        chunk.node->updateWorldBounds(Extents(minLocation.toVector3(), maxLocation.toVector3()));
    }

    return chunk.node.get();
}

std::pair<RenderEntity*, RenderEntity*> SmoothClusterChunked::createChunkGeometry(const SpatialRegion::Id& pos, unsigned int* outQuads)
{
	Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(pos);
	Voxel2::Region region = Voxel2::Region(Vector3int32(extents.getMinPos()), Vector3int32(extents.getMaxPos()) + Vector3int32::one()).expand(kChunkBorder);

	Voxel2::Box box = grid->read(region);

	if (box.isEmpty())
	{
        *outQuads = 0;
		return std::pair<RenderEntity*, RenderEntity*>(NULL, NULL);
	}

	using namespace Voxel2::Mesher;

	Options options = { materialTable, /* generateWater= */ true };
	BasicMesh geometry = generateGeometry(box, region.begin(), 0, options);
    
    if (geometry.indices.empty())
        return std::pair<RenderEntity*, RenderEntity*>(NULL, NULL);
    
	auto packInfo = getPackInfo(region);

	auto graphicsGeometry = generateGraphicsGeometryPacked(geometry, packInfo.first, options);

	*outQuads = (graphicsGeometry.solidIndices.size() + graphicsGeometry.waterIndices.size()) / 6;
	
	if (*outQuads == 0)
		return std::pair<RenderEntity*, RenderEntity*>(NULL, NULL);

	return uploadGeometry(updateChunkNode(pos), packInfo.second,
		sizeof(GraphicsVertexPacked), &graphicsGeometry.vertices[0], graphicsGeometry.vertices.size(),
		graphicsGeometry.solidIndices, graphicsGeometry.waterIndices);
}

const int SmoothClusterLOD::kChunkSizeLog2 = 4;
const int SmoothClusterLOD::kChunkBorder = 2;
const int SmoothClusterLOD::kChunkLevels = 5; // 16^3, 32^3, 64^3, 128^3

Voxel2::Region SmoothClusterLOD::Chunk::getRegion() const
{
	return Voxel2::Region::fromChunk(id.first, kChunkSizeLog2 + id.second);
}

SmoothClusterLOD::SmoothClusterLOD(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part)
	: SmoothClusterBase(visualEngine, part)
{
    std::vector<Voxel2::Region> regions = grid->getNonEmptyRegions();

    for (size_t i = 0; i < regions.size(); ++i)
        onTerrainRegionChanged(regions[i]);

	visualEngine->getSceneUpdater()->queueFullInvalidateMegaCluster(this);
}

SmoothClusterLOD::~SmoothClusterLOD()
{
}

void SmoothClusterLOD::updateEntity(bool assetsUpdated)
{
	if (!grid)
	{
		delete this;
		return;
	}

    if (!grid->isAllocated())
        return;

	// update split/merge flags
	Vector3 poi = visualEngine->getSceneManager()->getPointOfInterest();

	if (FFlag::DebugSmoothTerrainRenderFixedLOD)
		poi = Vector3();

	for (auto& p: chunks)
	{
		Chunk& chunk = p.second;

		auto bounds = getChunkBounds(chunk.id);

		float d = (poi - bounds.first).length();
		float r = bounds.second;

		// we want d/r to be ~constant
		float ratio = d / r;

		if (ratio < 4.0f && chunk.id.second > 0)
		{
			chunk.flags |= Flag_NeedsSplit;
		}
		else if (ratio > 8.0f && chunk.id.second + 1 < kChunkLevels)
		{
			chunk.flags |= Flag_NeedsMerge;
		}
	}

	// update chunks
	// TODO: better heuristics for distant chunks, better budgeting, better complexity
	unsigned int updatedChunks = 0;

	while (updatedChunks < 8)
	{
		Chunk* chunk = findDirtyChunk();

		if (!chunk)
			break;

		if (chunk->flags & Flag_NeedsSplit)
		{
			ChunkId id = chunk->id;

			removeChunk(id);

			for (int x = 0; x < 2; ++x)
				for (int y = 0; y < 2; ++y)
					for (int z = 0; z < 2; ++z)
					{
						Chunk& child = getChunk(getChildChunk(id, x, y, z));

						updateChunk(child);

						updatedChunks++;
					}
		}
		else if (chunk->flags & Flag_NeedsMerge)
		{
			// coalesce merge flags
			for (int x = 0; x < 2; ++x)
				for (int y = 0; y < 2; ++y)
					for (int z = 0; z < 2; ++z)
						if (Chunk* sibling = findChunk(getChildChunk(getParentChunk(chunk->id), x, y, z)))
							if (!(sibling->flags & Flag_NeedsMerge))
								chunk->flags &= ~Flag_NeedsMerge;

			if (chunk->flags & Flag_NeedsMerge)
			{
				ChunkId id = chunk->id;

				Chunk& parent = getChunk(getParentChunk(id));

				updateChunk(parent);

				updatedChunks++;

				for (int x = 0; x < 2; ++x)
					for (int y = 0; y < 2; ++y)
						for (int z = 0; z < 2; ++z)
							removeChunk(getChildChunk(parent.id, x, y, z));
			}
		}
		else
		{
			updateChunk(*chunk);

			updatedChunks++;

			chunk->flags = 0;
		}
	}

	// Requeue for updates
	visualEngine->getSceneUpdater()->queueFullInvalidateMegaCluster(this);
}

void SmoothClusterLOD::onTerrainRegionChanged(const Voxel2::Region& region)
{
	std::vector<Vector3int32> chunks = region.expand(kChunkBorder).getChunkIds(kChunkSizeLog2);

	for (auto& cid: chunks)
		markDirty(ChunkId(cid, 0));
}

void SmoothClusterLOD::markDirty(const ChunkId& id)
{
	// Find existing chunk to mark
	ChunkId cid = id;

	while (cid.second < kChunkLevels)
	{
		if (Chunk* chunk = findChunk(cid))
		{
			chunk->flags |= Flag_NeedsUpdate;
			return;
		}

		if (cid.second + 1 < kChunkLevels)
			cid = getParentChunk(cid);
		else
			break;
	}

	// Create new chunk 
	Chunk& chunk = getChunk(id);

	chunk.flags |= Flag_NeedsUpdate;
}

SmoothClusterLOD::ChunkId SmoothClusterLOD::getParentChunk(const ChunkId& id)
{
	RBXASSERT(id.second + 1 < kChunkLevels);

	return ChunkId(id.first >> 1, id.second + 1);
}

SmoothClusterLOD::ChunkId SmoothClusterLOD::getChildChunk(const ChunkId& id, int x, int y, int z)
{
	RBXASSERT(id.second > 0);

	return ChunkId((id.first << 1) + Vector3int32(x, y, z), id.second - 1);
}

std::pair<Vector3, float> SmoothClusterLOD::getChunkBounds(const ChunkId& id)
{
	int sizeLog2 = kChunkSizeLog2 + id.second;
	float size = (Voxel::kCELL_SIZE << sizeLog2);

	return std::make_pair((id.first.toVector3() + Vector3(0.5f)) * size, 0.5f * size);
}

SmoothClusterLOD::Chunk& SmoothClusterLOD::getChunk(const ChunkId& id)
{
	auto it = chunks.find(id);

	if (it != chunks.end())
		return it->second;

	Chunk& chunk = chunks[id];
	chunk.id = id;

	return chunk;
}

SmoothClusterLOD::Chunk* SmoothClusterLOD::findChunk(const ChunkId& id)
{
	auto it = chunks.find(id);

	if (it != chunks.end())
		return &it->second;

	return NULL;
}

void SmoothClusterLOD::removeChunk(const ChunkId& id)
{
	chunks.erase(id);
}

SmoothClusterLOD::Chunk* SmoothClusterLOD::findDirtyChunk()
{
	for (auto& p: chunks)
		if (p.second.flags)
			return &p.second;

	return NULL;
}

void SmoothClusterLOD::updateChunk(Chunk& chunk)
{
	// Delete existing entities
	if (chunk.solidEntity)
	{
		chunk.node->removeEntity(chunk.solidEntity);
		delete chunk.solidEntity;
	}

	if (chunk.waterEntity)
	{
		chunk.node->removeEntity(chunk.waterEntity);
		delete chunk.waterEntity;
	}

	// Update chunk geometry
	unsigned int quads = 0;
	std::pair<RenderEntity*, RenderEntity*> p = createChunkGeometry(chunk, &quads);
		
	chunk.solidEntity = p.first;
	chunk.waterEntity = p.second;
		
	if (chunk.solidEntity)
		chunk.node->addEntity(chunk.solidEntity);

	if (chunk.waterEntity)
		chunk.node->addEntity(chunk.waterEntity);
	
	// Update quad stats or destroy node if necessary
	if (!chunk.solidEntity && !chunk.waterEntity)
	{
		chunk.node.reset();
	}
	else
	{
		chunk.node->setBlockCount(quads / Chunk::kApproximateQuadsPerPart);
	}
}

RenderNode* SmoothClusterLOD::updateChunkNode(Chunk& chunk)
{
    // Update chunk node
    if (!chunk.node)
    {
        // Create chunk node
        chunk.node.reset(new RenderNode(visualEngine, RenderNode::CullMode_SpatialHash));
        
		Voxel2::Region region = chunk.getRegion();

		chunk.node->setCoordinateFrame(CoordinateFrame(region.begin().toVector3() * Voxel::kCELL_SIZE));
		chunk.node->updateWorldBounds(Extents(region.begin().toVector3() * Voxel::kCELL_SIZE, region.end().toVector3() * Voxel::kCELL_SIZE));
    }

    return chunk.node.get();
}

std::pair<RenderEntity*, RenderEntity*> SmoothClusterLOD::createChunkGeometry(Chunk& chunk, unsigned int* outQuads)
{
	int lod = chunk.id.second;

	Voxel2::Region region = chunk.getRegion().expand(kChunkBorder << lod);

	Voxel2::Box box = grid->read(region, lod);

	if (box.isEmpty())
	{
        *outQuads = 0;
		return std::pair<RenderEntity*, RenderEntity*>(NULL, NULL);
	}

	using namespace Voxel2::Mesher;

	Options options = { materialTable, /* generateWater= */ true };
	BasicMesh geometry = generateGeometry(box, region.begin(), lod, options);

    if (geometry.indices.empty())
		return std::pair<RenderEntity*, RenderEntity*>(NULL, NULL);
    
    auto packInfo = getPackInfo(region);

	auto graphicsGeometry = generateGraphicsGeometryPacked(geometry, packInfo.first, options);

	*outQuads = (graphicsGeometry.solidIndices.size() + graphicsGeometry.waterIndices.size()) / 6;
	
	if (*outQuads == 0)
		return std::pair<RenderEntity*, RenderEntity*>(NULL, NULL);

	return uploadGeometry(updateChunkNode(chunk), packInfo.second,
		sizeof(GraphicsVertexPacked), &graphicsGeometry.vertices[0], graphicsGeometry.vertices.size(),
		graphicsGeometry.solidIndices, graphicsGeometry.waterIndices);
}

}
}
