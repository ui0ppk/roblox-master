#include "stdafx.h"
#include "SpatialHashedScene.h"

#include "v8World/SpatialHashMultiRes.h"
#include "v8World/Contact.h"

#include "GfxBase/FrameRateManager.h"

#include "CullableSceneNode.h"
#include "Util.h"
#include "RenderCamera.h"

#include "rbx/DenseHash.h"

#include "rbx/Profiler.h"

namespace RBX
{
namespace Graphics
{

#define GFXSPATIALHASHLEVELS 4
#define GFXSPATIALHASH_MAX_CELLS_PER_PRIMITIVE 8

class GfxSpatialHash : public RBX::SpatialHash<CullableSceneNode, RBX::Contact, RBX::ContactManager, GFXSPATIALHASHLEVELS>
{
public:
    GfxSpatialHash()
    : SpatialHash<CullableSceneNode, Contact, ContactManager, GFXSPATIALHASHLEVELS>(NULL, NULL, GFXSPATIALHASH_MAX_CELLS_PER_PRIMITIVE)
	{
	}
};

// 3 for 3D.
// this cooresponds to 8x8x8 spatial hash blocks at the root level.
const float SpatialHashedScene::maxAdmissibleVolume = pow((float)(8 <<GFXSPATIALHASHLEVELS), 3);
// this corresponds to 22 spatial hash blocks at the root level.
const float SpatialHashedScene::maxAdmissibleLength = (float)(22 <<GFXSPATIALHASHLEVELS);

int SpatialHashedScene::globalFrameNumber = 0;

SpatialHashedScene::SpatialHashedScene()
{
    spatialHash.reset(new GfxSpatialHash());
}

SpatialHashedScene::~SpatialHashedScene()
{
}

bool SpatialHashedScene::isAdmissible(CullableSceneNode* child)
{
    const Extents& extents = child->getFastFuzzyExtents();

    return !extents.isNull() && extents.volume() < maxAdmissibleVolume && extents.longestSide() < maxAdmissibleLength;
}

void SpatialHashedScene::internalUpdateChild(CullableSceneNode* child)
{
    if (child->IsInSpatialHash())
    {
        if (isAdmissible(child))
        {
            spatialHash->onPrimitiveExtentsChanged(child);
        }
        else
        {
            spatialHash->onPrimitiveRemoved(child);

            unhashedNodes.insert(child);
        }
    }
	else if (isAdmissible(child))
    {
		unhashedNodes.erase(child);
        
        spatialHash->onPrimitiveAdded(child);
    }
	else
	{
		unhashedNodes.insert(child);
	}
}

void SpatialHashedScene::internalRemoveChild(CullableSceneNode* child)
{
    if (child->IsInSpatialHash())
    {
        spatialHash->onPrimitiveRemoved(child);
    }
    else
    {
		unhashedNodes.erase(child);
    }
}

struct FrustumVisitor: public GfxSpatialHash::SpaceFilter
{
    std::vector<CullableSceneNode*>& nodes;
    const RenderCamera& camera;
    Vector3 pointOfInterest;
    FrameRateManager* frm;
    int frameNumber;

	FrustumVisitor(std::vector<CullableSceneNode*>& nodes, const RenderCamera& camera, const Vector3& pointOfInterest, FrameRateManager* frm, int frameNumber)
        : nodes(nodes)
        , camera(camera)
		, pointOfInterest(pointOfInterest)
		, frm(frm)
        , frameNumber(frameNumber)
    {
    }

    IntersectResult Intersects(const Extents& extents)
    {
        return camera.intersects(extents);
    }

    float Distance(const Extents& extents)
    {
		return extents.computeClosestSqDistanceToPoint(pointOfInterest);
    }

    // return false to break iteration.
    bool onPrimitive(CullableSceneNode* n, IntersectResult intersectResult, float distance)
    {
        // framenum check to eliminate double-visits.
        if (n->lastFrustumVisibleFrameNumber < frameNumber)
        {
            n->lastFrustumVisibleFrameNumber = frameNumber;
            
            if (intersectResult == irFull || camera.isVisible(n->getWorldBounds()))
                nodes.push_back(n);

			if (distance > frm->GetViewCullSqDistance())
                return false;

            return true;
        }

        return true;
    }
};

void SpatialHashedScene::queryFrustumOrdered(std::vector<CullableSceneNode*>& nodes, const RenderCamera& camera, const Vector3& pointOfInterest, FrameRateManager* frm)
{
    RBXPROFILER_SCOPE("Render", "queryFrustumOrdered");

    for (UnhashedNodes::iterator it = unhashedNodes.begin(); it != unhashedNodes.end(); ++it)
    {
        CullableSceneNode* n = *it;
        
        if (camera.isVisible(n->getWorldBounds()))
            nodes.push_back(n);
    }

    FrustumVisitor visitor(nodes, camera, pointOfInterest, frm, getNextFrame());

    spatialHash->visitPrimitivesInSpace(&visitor);
}

static bool sphereIntersects(const Vector3& center, float radius, const Extents& extents)
{
    float d = extents.computeClosestSqDistanceToPoint(center);

    return d <= radius * radius;
}

void SpatialHashedScene::querySphere(std::vector<CullableSceneNode*>& nodes, const Vector3& center, float radius, unsigned int flags)
{
	RBXPROFILER_SCOPE("Render", "querySphere");

	for (CullableSceneNode* node: unhashedNodes)
	{
		if ((node->getFlags() & flags) == flags && sphereIntersects(center, radius, node->getWorldBounds()))
			nodes.push_back(node);
	}

	DenseHashSet<CullableSceneNode*> result(NULL);
	spatialHash->getPrimitivesOverlappingRec(Extents::fromCenterRadius(center, radius), result);

	for (CullableSceneNode* node: result)
	{
		if ((node->getFlags() & flags) == flags && sphereIntersects(center, radius, node->getWorldBounds()))
			nodes.push_back(node);
	}
}

void SpatialHashedScene::queryExtents(std::vector<CullableSceneNode*>& nodes, const Extents& extents, unsigned int flags)
{
	RBXPROFILER_SCOPE("Render", "queryExtents");

    for (CullableSceneNode* node: unhashedNodes)
    {
        if ((node->getFlags() & flags) == flags && extents.overlapsOrTouches(node->getWorldBounds()))
            nodes.push_back(node);
    }
    
    DenseHashSet<CullableSceneNode*> result(NULL);
    spatialHash->getPrimitivesOverlappingRec(extents, result);

    for (CullableSceneNode* node: result)
    {
        if ((node->getFlags() & flags) == flags)
            nodes.push_back(node);
    }
}

std::vector<CullableSceneNode*> SpatialHashedScene::getAllNodes() const
{
    struct NodeVisitor: public GfxSpatialHash::SpaceFilter
    {
		std::vector<CullableSceneNode*>& result;
        int frameNumber;

		NodeVisitor(std::vector<CullableSceneNode*>& result, int frameNumber)
            : result(result) 
            , frameNumber(frameNumber)
        {
        }

        IntersectResult Intersects(const Extents& extents)
        {
			return irFull;
        }

        float Distance(const Extents& extents)
        {
			return 0;
        }

        // return false to break iteration.
        bool onPrimitive(CullableSceneNode* n, IntersectResult intersectResult, float distance)
        {
            // framenum check to eliminate double-visits.
            if (n->lastFrustumVisibleFrameNumber < frameNumber)
            {
                n->lastFrustumVisibleFrameNumber = frameNumber;

				result.push_back(n);
            }

            return true;
        }
	};

	std::vector<CullableSceneNode*> result;

    for (UnhashedNodes::iterator it = unhashedNodes.begin(); it != unhashedNodes.end(); ++it)
		result.push_back(*it);

	NodeVisitor visitor(result, getNextFrame());

    spatialHash->visitPrimitivesInSpace(&visitor);

    return result;
}

int SpatialHashedScene::getNextFrame()
{
	return ++globalFrameNumber;
}

}
}
