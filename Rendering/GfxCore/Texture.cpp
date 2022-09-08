#include "GfxCore/Texture.h"

#include "rbx/Profiler.h"

FASTFLAGVARIABLE(GraphicsTextureCommitChanges, false)

namespace RBX
{
namespace Graphics
{

struct FormatDescription
{
    unsigned char bpp;
    bool compressed;
    bool depth;
};

static const FormatDescription gTextureFormats[Texture::Format_Count] =
{
	{ 8, false, false },
	{ 16, false, false },
	{ 16, false, false },
	{ 32, false, false },
	{ 32, false, false },
	{ 64, false, false },
	{ 4, true, false },
	{ 8, true, false },
	{ 8, true, false },
	{ 2, true, false },
	{ 2, true, false },
	{ 4, true, false },
	{ 4, true, false },
    { 4, true, false },
	{ 16, false, true },
	{ 32, false, true },
};

TextureRegion::TextureRegion()
    : x(0)
    , y(0)
    , z(0)
    , width(0)
    , height(0)
    , depth(0)
{
}

TextureRegion::TextureRegion(unsigned int x, unsigned int y, unsigned int z, unsigned int width, unsigned int height, unsigned int depth)
	: x(x)
    , y(y)
    , z(z)
    , width(width)
    , height(height)
    , depth(depth)
{
}

TextureRegion::TextureRegion(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
	: x(x)
    , y(y)
    , z(0)
    , width(width)
    , height(height)
    , depth(1)
{
}

Texture::Texture(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage)
	: Resource(device)
    , type(type)
    , format(format)
    , width(width)
    , height(height)
    , depth(depth)
    , mipLevels(mipLevels)
    , usage(usage)
{
	RBXASSERT(width > 0 && height > 0 && depth > 0);
	RBXASSERT(mipLevels > 0 && mipLevels <= getMaxMipCount(width, height, depth));
	RBXASSERT(type == Type_3D || depth == 1);
    
    if (usage != Usage_Renderbuffer)
    {
        RBXPROFILER_COUNTER_ADD("memory/gpu/texture", getTextureSize(type, format, width, height, depth, mipLevels));
    }
}

Texture::~Texture()
{
    if (usage != Usage_Renderbuffer)
    {
        RBXPROFILER_COUNTER_SUB("memory/gpu/texture", getTextureSize(type, format, width, height, depth, mipLevels));
    }
}

bool Texture::isFormatCompressed(Format format)
{
	return gTextureFormats[format].compressed;
}

bool Texture::isFormatDepth(Format format)
{
	return gTextureFormats[format].depth;
}

unsigned int Texture::getFormatBits(Format format)
{
	return gTextureFormats[format].bpp;
}

unsigned int Texture::getImageSize(Format format, unsigned int width, unsigned int height)
{
	const FormatDescription& desc = gTextureFormats[format];

	switch (format)
    {
    case Format_BC1:
    case Format_BC2:
    case Format_BC3:
    case Format_ETC1:
		return ((width + 3) / 4) * ((height + 3) / 4) * (desc.bpp * 16 / 8);
    
    case Format_PVRTC_RGB2:
    case Format_PVRTC_RGBA2:
        return (std::max(width, 16u) * std::max(height, 8u) * 2 + 7) / 8;
    
    case Format_PVRTC_RGB4:
    case Format_PVRTC_RGBA4:
        return (std::max(width, 8u) * std::max(height, 8u) * 4 + 7) / 8;
            
    default:
        RBXASSERT(!desc.compressed);
        return width * height * (desc.bpp / 8);
    }
}

unsigned int Texture::getTextureSize(Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels)
{
    unsigned int result = 0;

    for (unsigned int mip = 0; mip < mipLevels; ++mip)
        result += Texture::getImageSize(format, Texture::getMipSide(width, mip), Texture::getMipSide(height, mip)) * Texture::getMipSide(depth, mip);
    
    return result * (type == Texture::Type_Cube ? 6 : 1);
}

unsigned int Texture::getMipSide(unsigned int value, unsigned int mip)
{
    unsigned int result = value >> mip;

    return result ? result : 1;
}

unsigned int Texture::getMaxMipCount(unsigned int width, unsigned int height, unsigned int depth)
{
    unsigned int result = 0;

    while (width || height || depth)
	{
        result++;

        width /= 2;
        height /= 2;
        depth /= 2;
	}

    return result;
}

}
}
