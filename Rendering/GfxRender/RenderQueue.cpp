#include "stdafx.h"

#include "RenderQueue.h"

#include "Material.h"

#include "rbx/Debug.h"

namespace RBX
{
namespace Graphics
{

struct RenderOperationCompareByMaterial
{
	bool operator()(const RenderOperation& lhs, const RenderOperation& rhs) const
    {
		const ShaderProgram* lp = lhs.technique->getProgram();
		const ShaderProgram* rp = rhs.technique->getProgram();

		return (lp == rp) ? lhs.technique < rhs.technique : lp < rp;
    }
};

struct RenderOperationCompareByDistance
{
    bool operator()(const RenderOperation& lhs, const RenderOperation& rhs) const
    {
		return lhs.distanceKey > rhs.distanceKey;
    }
};

Renderable::~Renderable()
{
}

void RenderQueueGroup::clear()
{
	operations.clear();
}

void RenderQueueGroup::sort(SortMode mode)
{
    switch (mode)
	{
	case Sort_None:
        break;

	case Sort_Material:
        std::sort(operations.begin(), operations.end(), RenderOperationCompareByMaterial());
        break;

	case Sort_Distance:
        std::sort(operations.begin(), operations.end(), RenderOperationCompareByDistance());
        break;

	default:
        RBXASSERT(false);
	}
}

RenderQueue::RenderQueue()
{
    clear();
}

void RenderQueue::clear()
{
    for (int id = 0; id < Id_Count; ++id)
		groups[id].clear();

	features = 0;
}

}
}
