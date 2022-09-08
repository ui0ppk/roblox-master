#pragma once
#include <string>
#include "GfxBase/RenderSettings.h"

namespace RBX { 

	class RenderCaps
	{
		size_t vidMemSize;
		std::string gfxCardName;
		bool texturePowerOf2Only;
		bool supportsGBuffer;

		unsigned int skinningBoneCount;
	public:
		RenderCaps(std::string gfxCardName, size_t vidMemSize );

		void setTexturePowerOf2Only(bool b) { texturePowerOf2Only = b; }
        void setSupportsGBuffer(bool b) { supportsGBuffer = b; }
		void setSkinningBoneCount(unsigned int v) { skinningBoneCount = v; }

		size_t getVidMemSize() const { return vidMemSize; }

		bool getTexturePowerOf2Only() const { return texturePowerOf2Only; }
		const std::string& getGfxCardName() const { return gfxCardName; }

		bool getSupportsGBuffer() const { return supportsGBuffer; }
		
		unsigned int getSkinningBoneCount() const { return skinningBoneCount; }
	};

}