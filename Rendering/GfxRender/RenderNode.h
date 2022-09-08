#pragma once

#include "CullableSceneNode.h"
#include "RenderQueue.h"

#include "GfxCore/Device.h"

namespace RBX
{
namespace Graphics
{
    class Material;
	class RenderNode;
	
	class RenderEntity: public Renderable
	{
	public:
		RenderEntity(RenderNode* node, const GeometryBatch& geometry, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, unsigned char lodMask = 0xff);
		virtual ~RenderEntity();
		
		RenderNode* getNode() const { return node; }
        
        unsigned char getLodMask() const { return lodMask; }
		
		// Rendering support
		virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, unsigned int lodIndex, RenderQueue::Pass pass);

		// Renderable overrides
        unsigned int getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const override;

        // Extension points
        virtual float getViewDepth(const RenderCamera& camera) const;
	
        static float computeViewDepth(const RenderCamera& camera, const Vector3& position, float offset = 0);

	protected:
		RenderNode* node;
		
		RenderQueue::Id renderQueueId;
        unsigned char lodMask;

        GeometryBatch geometry;
        shared_ptr<Material> material;
	};
	
	class RenderNode: public CullableSceneNode
	{
	public:
		RenderNode(VisualEngine* visualEngine, CullMode cullMode, unsigned int flags = 0);
		virtual ~RenderNode();

        // Coordinate frame
		const CoordinateFrame& getCoordinateFrame() const { return cframe; }
		void setCoordinateFrame(const CoordinateFrame& cframe) { this->cframe = cframe; }

		// Entity list manipulation
        const std::vector<RenderEntity*>& getEntities() const { return entities; }
        
        void addEntity(RenderEntity* entity);
        void removeEntity(RenderEntity* entity);
        
		// SceneNode overrides
		virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass);

    protected:
        std::vector<RenderEntity*> entities;
		CoordinateFrame cframe;

        void debugRenderBoundingBox();
	};

}
}
