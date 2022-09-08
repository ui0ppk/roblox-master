#include "stdafx.h"
#include "RenderNode.h"

#include "Material.h"

#include "GfxBase/FrameRateManager.h"
#include "VisualEngine.h"
#include "Util.h"
#include "VertexStreamer.h"

namespace RBX
{
namespace Graphics
{

RenderEntity::RenderEntity(RenderNode* node, const GeometryBatch& geometry, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, unsigned char lodMask)
    : node(node)
    , renderQueueId(renderQueueId)
    , lodMask(lodMask)
    , geometry(geometry)
    , material(material)
{
}

RenderEntity::~RenderEntity()
{
}
    
void RenderEntity::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, unsigned int lodIndex, RenderQueue::Pass pass)
{
    const shared_ptr<Material>& m = material;

    if (const Technique* technique = m->getBestTechnique(lodIndex, pass))
    {
        float distanceKey = (renderQueueId == RenderQueue::Id_Transparent) ? getViewDepth(camera) : 0.f;

        RenderOperation rop = {this, distanceKey, technique, &geometry};

        queue.getGroup(renderQueueId).push(rop);
    }
}

unsigned int RenderEntity::getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const
{
    if (useCache(cacheKey, node)) return 0;

    RBXASSERT(maxTransforms >= 1);

    const CoordinateFrame& cframe = node->getCoordinateFrame();

    memcpy(&buffer[0], cframe.rotation[0], sizeof(float) * 3);
    buffer[3] = cframe.translation.x;

    memcpy(&buffer[4], cframe.rotation[1], sizeof(float) * 3);
    buffer[7] = cframe.translation.y;

    memcpy(&buffer[8], cframe.rotation[2], sizeof(float) * 3);
    buffer[11] = cframe.translation.z;

    return 1;
}

float RenderEntity::getViewDepth(const RenderCamera& camera) const
{
    return computeViewDepth(camera, node->getCoordinateFrame().translation);
}

float RenderEntity::computeViewDepth(const RenderCamera& camera, const Vector3& position, float offset)
{
    float result = (position - camera.getPosition()).dot(camera.getDirection()) + offset;
    
    return Math::isNan(result) ? 0 : result;
}

RenderNode::RenderNode(VisualEngine* visualEngine, CullMode cullMode, unsigned int flags)
    : CullableSceneNode(visualEngine, cullMode, flags)
{
}

RenderNode::~RenderNode()
{
    // delete all entities
    for (size_t i = 0; i < entities.size(); ++i)
    {
        delete entities[i];
    }
}

void RenderNode::addEntity(RenderEntity* entity)
{
    RBXASSERT(entity);
    entities.push_back(entity);
}

void RenderNode::removeEntity(RenderEntity* entity)
{
    std::vector<RenderEntity*>::iterator it = std::find(entities.begin(), entities.end(), entity);
    RBXASSERT(it != entities.end());
    
    entities.erase(it);
}

void RenderNode::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{
    if (updateIsCulledByFRM())
        return;

    FrameRateManager* frm = getVisualEngine()->getFrameRateManager();

	unsigned int lodIndex = frm->getGBufferSetting() ? 0 : getSqDistanceToFocus() < frm->getShadingSqDistance() ? 1 : 2;

	unsigned int lodMask = 1 << lodIndex;

    // Add all entities
    for (size_t i = 0; i < entities.size(); ++i)
    {
        if (entities[i]->getLodMask() & lodMask)
            entities[i]->updateRenderQueue(queue, camera, lodIndex, pass);
    }

    // Render bounding box
	if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes())
	{
		debugRenderBoundingBox();
	}
}

void RenderNode::debugRenderBoundingBox()
{
    VertexStreamer* vs = getVisualEngine()->getVertexStreamer();
    const Extents& b = getWorldBounds();

    if (!b.isNull())
    {
        static const int edges[12][2] = {{0, 2}, {2, 6}, {6, 4}, {4, 0}, {1, 3}, {3, 7}, {7, 5}, {5, 1}, {0, 1}, {2, 3}, {4, 5}, {6, 7}};

        for (int edge = 0; edge < 12; ++edge)
        {
            Vector3 e0 = b.getCorner(edges[edge][0]);
            Vector3 e1 = b.getCorner(edges[edge][1]);
            
            vs->line3d(e0.x, e0.y, e0.z, e1.x, e1.y, e1.z, Color3::white());
        }
    }
}

}
}
