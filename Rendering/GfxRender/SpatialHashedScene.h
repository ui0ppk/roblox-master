#pragma once

#include "v8World/SpatialHashMultiRes.h"

#include <boost/unordered_set.hpp>

namespace RBX
{
    class FrameRateManager;
}

namespace RBX
{
namespace Graphics
{

class VisualEngine;
class CullableSceneNode;
class RenderCamera;
class RenderQueue;

class GfxSpatialHash;

class SpatialHashedScene
{
public:
    SpatialHashedScene();
    ~SpatialHashedScene();

    bool isAdmissible(CullableSceneNode* child);

    void internalUpdateChild(CullableSceneNode* child);
    void internalRemoveChild(CullableSceneNode* child);

    GfxSpatialHash* getSpatialHash() const { return spatialHash.get(); }

	void queryFrustumOrdered(std::vector<CullableSceneNode*>& nodes, const RenderCamera& camera, const Vector3& pointOfInterest, FrameRateManager* frm);
	void querySphere(std::vector<CullableSceneNode*>& nodes, const Vector3& center, float radius, unsigned int flags);
	void queryExtents(std::vector<CullableSceneNode*>& nodes, const Extents& extents, unsigned int flags);

    std::vector<CullableSceneNode*> getAllNodes() const;

private:
	typedef boost::unordered_set<CullableSceneNode*> UnhashedNodes;

    static const float maxAdmissibleVolume;
    static const float maxAdmissibleLength;
    static int globalFrameNumber;

	scoped_ptr<GfxSpatialHash> spatialHash;

	UnhashedNodes unhashedNodes;

    static int getNextFrame();
};

}
}
