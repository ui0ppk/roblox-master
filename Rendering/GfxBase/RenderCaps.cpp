#include "GfxBase/RenderCaps.h"
#include "FastLog.h"

namespace RBX {

	RenderCaps::RenderCaps(std::string gfxCardName, size_t vidMemSize )
		: gfxCardName(gfxCardName)
		, vidMemSize(vidMemSize)
		, texturePowerOf2Only(false)
		, supportsGBuffer(false)
		, skinningBoneCount(0)
	{
	}
	
}
