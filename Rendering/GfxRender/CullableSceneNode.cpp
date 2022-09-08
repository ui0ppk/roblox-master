#include "stdafx.h"
#include "CullableSceneNode.h"

#include "SceneManager.h"
#include "Util.h"
#include "SpatialHashedScene.h"

#include "VisualEngine.h"
#include "GfxBase/FrameRateManager.h"

namespace RBX
{
namespace Graphics
{

CullableSceneNode::CullableSceneNode(VisualEngine* visualEngine, CullMode cullMode, unsigned int flags)
    : visualEngine(visualEngine)
    , cullMode(cullMode)
    , flags(flags)
    , blockCount(1)
    , sqDistanceToFocus(0)
{
}

CullableSceneNode::~CullableSceneNode()
{
	visualEngine->getSceneManager()->getSpatialHashedScene()->internalRemoveChild(this);
}

bool CullableSceneNode::updateIsCulledByFRM()
{
	if (worldBounds.isNull())
        return true;

    const Vector3& focusPosition = visualEngine->getSceneManager()->getPointOfInterest();

    sqDistanceToFocus = G3D::ClosestSqDistanceToAABB(focusPosition, worldBounds.center(), worldBounds.size() * 0.5f);

    // count blocks and update farplane regardless of cullability.
    if (sqDistanceToFocus > 1e-3f)
        visualEngine->getSceneManager()->processSqPartDistance(sqDistanceToFocus);

    RBX::FrameRateManager* frm = visualEngine->getFrameRateManager();

	frm->AddBlockQuota(blockCount, sqDistanceToFocus, IsInSpatialHash());

	// We don't do distance-cull on huge objects
	if (cullMode == CullMode_SpatialHash && !IsInSpatialHash())
		return false;
	else
		return sqDistanceToFocus > frm->GetRenderCullSqDistance();
}

void CullableSceneNode::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{
}

void CullableSceneNode::updateWorldBounds(const Extents& aabb)
{
    worldBounds = aabb;

    if (aabb.isNull())
		visualEngine->getSceneManager()->getSpatialHashedScene()->internalRemoveChild(this);
	else
        visualEngine->getSceneManager()->getSpatialHashedScene()->internalUpdateChild(this);
}

}
}
