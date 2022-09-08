#pragma once

#include "Resource.h"

namespace RBX
{
namespace Graphics
{

class Renderbuffer;

struct TextureRegion
{
    unsigned int x;
    unsigned int y;
    unsigned int z;

    unsigned int width;
    unsigned int height;
    unsigned int depth;

    TextureRegion();
    TextureRegion(unsigned int x, unsigned int y, unsigned int z, unsigned int width, unsigned int height, unsigned int depth);
    TextureRegion(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
};

class Texture: public Resource
{
public:
    enum Type
	{
        Type_2D,
        Type_3D,
        Type_Cube,

        Type_Count
	};

    enum Usage
    {
        Usage_Static,
        Usage_Dynamic,
        Usage_Renderbuffer,

        Usage_Count
    };

    enum Format
	{
        Format_L8,
        Format_LA8,
        Format_RGB5A1,
        Format_RGBA8,
        Format_RG16,
        Format_RGBA16F,
        Format_BC1,
        Format_BC2,
        Format_BC3,
        Format_PVRTC_RGB2,
        Format_PVRTC_RGBA2,
        Format_PVRTC_RGB4,
        Format_PVRTC_RGBA4,
        Format_ETC1,
        Format_D16,
        Format_D24S8,

        Format_Count
	};

    struct LockResult
	{
        void* data;
        unsigned int rowPitch;
        unsigned int slicePitch;
	};

    ~Texture();

    virtual void upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size) = 0;

    virtual bool download(unsigned int index, unsigned int mip, void* data, unsigned int size) = 0;

    virtual bool supportsLocking() const = 0;
    virtual LockResult lock(unsigned int index, unsigned int mip, const TextureRegion& region) = 0;
    virtual void unlock(unsigned int index, unsigned int mip) = 0;
    
    virtual shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index, unsigned int mip) = 0;

    virtual void commitChanges() = 0;
    virtual void generateMipmaps() = 0;

	Type getType() const { return type; }
	Format getFormat() const { return format; }

	unsigned int getWidth() const { return width; }
	unsigned int getHeight() const { return height; }
	unsigned int getDepth() const { return depth; }

	unsigned int getMipLevels() const { return mipLevels; }
        
	Usage getUsage() const { return usage; }

    static bool isFormatCompressed(Format format);
    static bool isFormatDepth(Format format);
    static unsigned int getFormatBits(Format format);
    static unsigned int getImageSize(Format format, unsigned int width, unsigned int height);

    static unsigned int getTextureSize(Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels);

    static unsigned int getMipSide(unsigned int value, unsigned int mip);
    static unsigned int getMaxMipCount(unsigned int width, unsigned int height, unsigned int depth);

protected:
	Texture(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage);

    Type type;
    Format format;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int mipLevels;
    Usage usage;
};

}
}
