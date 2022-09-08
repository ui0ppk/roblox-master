#pragma once
#include "v8tree/instance.h"
#include "v8tree/service.h"

namespace G3D
{
	class BinaryOutput;
};

namespace RBX
{
	class ContentProvider;
    class ViewBase;
}

extern const char* const sThumbnailGenerator;
class ThumbnailGenerator 
	: public RBX::DescribedCreatable<ThumbnailGenerator, RBX::Instance, sThumbnailGenerator>
	, public RBX::Service
{
public:
	int graphicsMode;
	static volatile long totalCount;
	
	ThumbnailGenerator(void);
	~ThumbnailGenerator(void);
	shared_ptr<const RBX::Reflection::Tuple> click(std::string fileType, int cx, int cy, bool hideSky,bool crop);
	shared_ptr<const RBX::Reflection::Tuple> clickTexture(std::string textureId, std::string fileType, int cx, int cy);

	void renderThumb(RBX::ViewBase* view, void* windowHandle, std::string fileType, int cx, int cy, bool hideSky, bool crop, std::string* strOutput);
    void exportScene(RBX::ViewBase* view, std::string* outStr);

private:
	void configureCaches();
};
