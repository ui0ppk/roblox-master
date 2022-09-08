#pragma once

#include "GfxCore/Texture.h"
#include "ImageInfo.h"

#include <vector>
#include <boost/scoped_array.hpp>

namespace RBX
{
namespace Graphics
{

class Image
{
public:
    enum LoadFlags
	{
        Load_DecodeDXT = 1 << 0,
        Load_RoundToPOT = 1 << 1,
        Load_OutputBGR = 1 << 2,
		Load_ForceFullMipChain = 1 << 3
	};

    struct LoadResult
    {
        shared_ptr<Image> image;
        ImageInfo info;

        LoadResult(const shared_ptr<Image>& image, const ImageInfo& info);
    };

	static LoadResult load(std::istream& stream, unsigned int maxTextureSize, unsigned int flags);
	static void decodeDXT(unsigned char* target, const unsigned char* source, unsigned int width, unsigned int height, unsigned int depth, Texture::Format format);

    Image(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels);
    ~Image();

    void saveDDS(std::ostream& stream);

    unsigned char* getMipData(unsigned int index, unsigned int level);
    const unsigned char* getMipData(unsigned int index, unsigned int level) const;

	Texture::Type getType() const { return type; }
	Texture::Format getFormat() const { return format; }

	unsigned int getWidth() const { return width; }
	unsigned int getHeight() const { return height; }
	unsigned int getDepth() const { return depth; }

	unsigned int getMipLevels() const { return mipLevels; }

private:
    Texture::Type type;
    Texture::Format format;

    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int mipLevels;

    std::vector<unsigned int> mipOffsets;

    boost::scoped_array<unsigned char> data;
    size_t dataSize;
};

}
}
