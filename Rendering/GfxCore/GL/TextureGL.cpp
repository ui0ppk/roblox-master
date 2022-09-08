#include "TextureGL.h"

#include "DeviceGL.h"
#include "FramebufferGL.h"

#include "HeadersGL.h"

LOGGROUP(Graphics)

FASTFLAG(GraphicsTextureCommitChanges)

namespace RBX
{
namespace Graphics
{

static const GLenum gTextureTargetGL[Texture::Type_Count] =
{
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_CUBE_MAP
};

struct TextureFormatGL
{
    GLenum internalFormat;
    GLenum dataFormat;
    GLenum dataType;
};

static const TextureFormatGL gTextureFormatGL[Texture::Format_Count] =
{
	{ GL_R8, GL_RED, GL_UNSIGNED_BYTE },
	{ GL_RG8, GL_RG, GL_UNSIGNED_BYTE },
	{ GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1 },
	{ GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE },
	{ GL_RG16, GL_RG, GL_UNSIGNED_SHORT },
	{ GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT },
	{ GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, 0 },
	{ GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, 0 },
	{ GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, 0 },
	{ GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, 0, 0 },
	{ GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, 0, 0 },
	{ GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, 0, 0 },
	{ GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, 0, 0 },
    { GL_ETC1_RGB8_OES, 0, 0 },
	{ GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT },
	{ GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 },
};

const TextureFormatGL& getTextureFormatGL(Texture::Format format, bool ext3)
{
    if (!ext3 && format == Texture::Format_L8)
    {
        static const TextureFormatGL result = { GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE };
        return result;
    }

    if (!ext3 && format == Texture::Format_LA8)
    {
        static const TextureFormatGL result = { GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE };
        return result;
    }

    return gTextureFormatGL[format];
}

static const GLenum gSamplerFilterMinGL[SamplerState::Filter_Count][2] =
{
	{ GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST },
	{ GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR },
	{ GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR }
};

static const GLenum gSamplerFilterMagGL[SamplerState::Filter_Count] =
{
    GL_NEAREST,
    GL_LINEAR,
	GL_LINEAR
};

static const GLenum gSamplerAddressGL[SamplerState::Address_Count] =
{
	GL_REPEAT,
	GL_CLAMP_TO_EDGE
};

static bool doesFormatSupportPartialUpload(Texture::Format format)
{
	return format != Texture::Format_ETC1;
}

static unsigned int createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, const DeviceCapsGL& caps)
{
    GLenum target = gTextureTargetGL[type];
	const TextureFormatGL& formatGl = getTextureFormatGL(format, caps.ext3);

	unsigned int id = 0;

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &id);
	glBindTexture(target, id);

	if (caps.extTextureStorage)
	{
        if (type == Texture::Type_3D)
        {
 			glTexStorage3D(target, mipLevels, formatGl.internalFormat, width, height, depth);
        }
        else
        {
			glTexStorage2D(target, mipLevels, formatGl.internalFormat, width, height);
        }
    }
	else
	{
        // Is this correct? There's a subtle difference here between desktop GL and GLES
    #ifdef GLES
        GLenum internalFormat = formatGl.dataFormat;
    #else
        GLenum internalFormat = formatGl.internalFormat;
    #endif
                    
        unsigned int faces = (type == Texture::Type_Cube) ? 6 : 1;
        
        for (unsigned int index = 0; index < faces; ++index)
        {
            GLenum faceTarget = (type == Texture::Type_Cube) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + index : target;

            for (unsigned int mip = 0; mip < mipLevels; ++mip)
            {
                unsigned int mipWidth = Texture::getMipSide(width, mip);
                unsigned int mipHeight = Texture::getMipSide(height, mip);
                unsigned int mipDepth = Texture::getMipSide(depth, mip);
                
                if (type == Texture::Type_3D)
                {
                    glTexImage3D(faceTarget, mip, internalFormat, mipWidth, mipHeight, mipDepth, 0, formatGl.dataFormat, formatGl.dataType, NULL);
                }
                else if (Texture::isFormatCompressed(format))
                {
                    unsigned int size = Texture::getImageSize(format, mipWidth, mipHeight);

                    // glCompressedTexImage does not accept NULL as a valid pointer
                    std::vector<char> data(size);

                    glCompressedTexImage2D(faceTarget, mip, formatGl.internalFormat, mipWidth, mipHeight, 0, size, &data[0]);
                }
                else
                {
                    glTexImage2D(faceTarget, mip, internalFormat, mipWidth, mipHeight, 0, formatGl.dataFormat, formatGl.dataType, NULL);
                }
            }
        }
	}

	if (caps.supportsTexturePartialMipChain)
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
	else
		RBXASSERT(mipLevels == 1 || mipLevels == Texture::getMaxMipCount(width, height, depth));

	glBindTexture(target, 0);

	return id;
}

static void uploadTexture(unsigned int id, Texture::Type type, Texture::Format format, unsigned int index, unsigned int mip, const TextureRegion& region, bool entireRegion, const void* data, const DeviceCapsGL& caps)
{
    GLenum target = gTextureTargetGL[type];
	GLenum faceTarget = (type == Texture::Type_Cube) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + index : target;
	const TextureFormatGL& formatGl = getTextureFormatGL(format, caps.ext3);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, id);
    
    bool needsCustomAlignment = (!Texture::isFormatCompressed(format) && Texture::getFormatBits(format) < 32);

    glPixelStorei(GL_UNPACK_ALIGNMENT, needsCustomAlignment ? 1 : 4);
    
    if (type == Texture::Type_3D)
	{
		glTexSubImage3D(faceTarget, mip, region.x, region.y, region.z, region.width, region.height, region.depth, formatGl.dataFormat, formatGl.dataType, data);
    }
	else
	{
		if (Texture::isFormatCompressed(format))
		{
            unsigned int size = Texture::getImageSize(format, region.width, region.height) * region.depth;

			if (entireRegion && !doesFormatSupportPartialUpload(format) && !caps.extTextureStorage)
			{
				glCompressedTexImage2D(faceTarget, mip, formatGl.internalFormat, region.width, region.height, 0, size, data);
			}
			else
			{
				glCompressedTexSubImage2D(faceTarget, mip, region.x, region.y, region.width, region.height, formatGl.internalFormat, size, data);
			}
		}
		else
		{
			glTexSubImage2D(faceTarget, mip, region.x, region.y, region.width, region.height, formatGl.dataFormat, formatGl.dataType, data);
		}
	}

    glBindTexture(target, 0);
}

TextureGL::TextureGL(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage)
    : Texture(device, type, format, width, height, depth, mipLevels, usage)
	, id(0)
    , cachedState(SamplerState::Filter_Count) // initialize state to invalid value to force a cache miss on the next setup
	, external(false)
    , scratchId(0)
    , scratchOffset(0)
    , scratchSize(0)
{
    const DeviceCapsGL& caps = static_cast<DeviceGL*>(device)->getCapsGL();

	const TextureFormatGL& formatGl = getTextureFormatGL(format, caps.ext3);

    if (!formatGl.internalFormat)
		throw RBX::runtime_error("Unsupported format: %d (platform)", format);

	if ((format == Format_RGBA16F) && !caps.supportsTextureHalfFloat)
		throw RBX::runtime_error("Unsupported format: %d (caps)", format);

	if ((format == Format_BC1 || format == Format_BC2 || format == Format_BC3) && !caps.supportsTextureDXT)
		throw RBX::runtime_error("Unsupported format: %d (caps)", format);

	if ((format == Format_PVRTC_RGB2 || format == Format_PVRTC_RGBA2 || format == Format_PVRTC_RGB4 || format == Format_PVRTC_RGBA4) && !caps.supportsTexturePVR)
		throw RBX::runtime_error("Unsupported format: %d (caps)", format);

    if (format == Format_ETC1 && !caps.supportsTextureETC1)
        throw RBX::runtime_error("Unsupported format: %d (capse)", format);

	if (!caps.supportsTextureNPOT && ((width & (width - 1)) || (height & (height - 1)) || (depth & (depth - 1))) && (usage != Usage_Renderbuffer || mipLevels > 1))
		throw RBX::runtime_error("Unsupported dimensions: %dx%dx%d (NPOT)", width, height, depth);
    
    if (!caps.supportsTexture3D && type == Type_3D)
        throw RBX::runtime_error("Unsupported type: 3D");

    if (FFlag::GraphicsTextureCommitChanges && usage == Usage_Dynamic && isFormatCompressed(format))
        throw RBX::runtime_error("Unsupported format: compressed formats are not supported for dynamic textures");

    // createTexture poisons the binding on the first stage
	static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedTextureStage(0);

    id = createTexture(type, format, width, height, depth, mipLevels, caps);

    if (FFlag::GraphicsTextureCommitChanges && usage == Usage_Dynamic && caps.ext3)
    {
        scratchSize = getTextureSize(type, format, width, height, depth, mipLevels);
        
        glGenBuffers(1, &scratchId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, scratchId);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, scratchSize, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
}

TextureGL::TextureGL(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage, unsigned int id)
    : Texture(device, type, format, width, height, depth, mipLevels, usage)
    , cachedState(SamplerState::Filter_Count) // initialize state to invalid value to force a cache miss on the next setup
	, id(id)
	, external(true)
    , scratchId(0)
    , scratchOffset(0)
    , scratchSize(0)
{
}

TextureGL::~TextureGL()
{
	static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedTexture(this);

    if (scratchId)
        glDeleteBuffers(1, &scratchId);

	if (!external)
		glDeleteTextures(1, &id);
}

void TextureGL::upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size)
{
    const DeviceCapsGL& caps = static_cast<DeviceGL*>(device)->getCapsGL();

	RBXASSERT(index < (type == Type_Cube ? 6u : 1u));

    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);

    RBXASSERT(mip < mipLevels);
	RBXASSERT(region.x + region.width <= mipWidth);
	RBXASSERT(region.y + region.height <= mipHeight);
	RBXASSERT(region.z + region.depth <= mipDepth);
    
    RBXASSERT(size == getImageSize(format, region.width, region.height) * region.depth);

    if (supportsLocking())
    {
        unsigned int bpp = getFormatBits(format) / 8;
        
        LockResult lr = lock(index, mip, region);

        for (unsigned int z = 0; z < region.depth; ++z)
            for (unsigned int y = 0; y < region.height; ++y)
            {
                unsigned int sourceOffset = (region.width * region.height * z + region.width * y) * bpp;
                unsigned int targetOffset = lr.slicePitch * z + lr.rowPitch * y;

                memcpy(static_cast<char*>(lr.data) + targetOffset, static_cast<const char*>(data) + sourceOffset, region.width * bpp);
            }
        
        unlock(index, mip);
    }
    else
    {
        // uploadTexture poisons the binding on the first stage
    	static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedTextureStage(0);
        
        bool entireRegion = (region.x == 0 && region.y == 0 && region.width == mipWidth && region.height == mipHeight);

        uploadTexture(id, type, format, index, mip, region, entireRegion, data, caps);
    }
}

#ifdef GLES
bool TextureGL::download(unsigned int index, unsigned int mip, void* data, unsigned int size)
{
    return false;
}
#else
bool TextureGL::download(unsigned int index, unsigned int mip, void* data, unsigned int size)
{
    const DeviceCapsGL& caps = static_cast<DeviceGL*>(device)->getCapsGL();

    GLenum target = gTextureTargetGL[type];
	const TextureFormatGL& formatGl = getTextureFormatGL(format, caps.ext3);

	RBXASSERT(index < (type == Type_Cube ? 6u : 1u));
	GLenum faceTarget = (type == Type_Cube) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + index : target;

    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);

    RBXASSERT(mip < mipLevels);
	RBXASSERT(size == getImageSize(format, mipWidth, mipHeight) * mipDepth);

	static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedTextureStage(0);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, id);
    
    bool needsCustomAlignment = (!isFormatCompressed(format) && getFormatBits(format) < 32);

    glPixelStorei(GL_PACK_ALIGNMENT, needsCustomAlignment ? 1 : 4);

	if (isFormatCompressed(format))
		glGetCompressedTexImage(faceTarget, mip, data);
	else
        glGetTexImage(faceTarget, mip, formatGl.dataFormat, formatGl.dataType, data);

    glBindTexture(target, 0);

    return true;
}
#endif

bool TextureGL::supportsLocking() const
{
    if (FFlag::GraphicsTextureCommitChanges)
        return scratchId != 0;

    return false;
}

Texture::LockResult TextureGL::lock(unsigned int index, unsigned int mip, const TextureRegion& region)
{
    if (!scratchId)
    {
    	Texture::LockResult result = {0, 0, 0};

        return result;
    }

    RBXASSERT(index < (type == Type_Cube ? 6u : 1u));
    
    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);
    
    RBXASSERT(mip < mipLevels);
    RBXASSERT(region.x + region.width <= mipWidth);
    RBXASSERT(region.y + region.height <= mipHeight);
    RBXASSERT(region.z + region.depth <= mipDepth);

    unsigned int size = getImageSize(format, region.width, region.height) * region.depth;
    unsigned int flags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
    
    if (scratchOffset + size > scratchSize)
    {
        commitChanges();
        
        scratchOffset = 0;
        flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, scratchId);
    void* data = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, scratchOffset, size, flags);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    
    Change change = { index, mip, region, scratchOffset };

    scratchOffset += size;
    pendingChanges.push_back(change);

	Texture::LockResult result = {data, getImageSize(format, region.width, 1), getImageSize(format, region.width, region.height) };
    
    return result;
}

void TextureGL::unlock(unsigned int index, unsigned int mip)
{
    if (!scratchId)
        return;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, scratchId);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

shared_ptr<Renderbuffer> TextureGL::getRenderbuffer(unsigned int index, unsigned int mip)
{
	RBXASSERT(usage == Usage_Renderbuffer);

    RBXASSERT(mip == 0);
	RBXASSERT(index < (type == Type_Cube ? 6u : 1u));
    
    weak_ptr<Renderbuffer>& slot = renderBuffers[std::make_pair(index,mip)];
    shared_ptr<Renderbuffer> rb = slot.lock();

    if (!rb)
    {
        GLenum target = (type == Texture::Type_Cube) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + index : gTextureTargetGL[type];
 
        rb.reset(new RenderbufferGL(device, shared_from_this(), target));
        slot = rb;
    }

    return rb;
}

void TextureGL::commitChanges()
{
	if (!FFlag::GraphicsTextureCommitChanges)
		return;
    
    const DeviceCapsGL& caps = static_cast<DeviceGL*>(device)->getCapsGL();

    if (pendingChanges.empty())
        return;

    // uploadTexture poisons the binding on the first stage
    static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedTextureStage(0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, scratchId);
    
    for (auto& change: pendingChanges)
    {
        RBXASSERT(change.scratchOffset < scratchOffset);

        bool entireRegion = false; // this is only important for compressed formats that we don't support

        uploadTexture(id, type, format, change.index, change.mip, change.region, entireRegion, reinterpret_cast<void*>(change.scratchOffset), caps);
    }
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    
    pendingChanges.clear();
}

void TextureGL::generateMipmaps()
{
	RBXASSERT(usage != Usage_Dynamic);

    if (mipLevels <= 1)
        return;

    static_cast<DeviceGL*>(device)->getImmediateContextGL()->invalidateCachedTextureStage(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(gTextureTargetGL[type], id);

    glGenerateMipmap(gTextureTargetGL[type]);

    glBindTexture(gTextureTargetGL[type], 0);
}

void TextureGL::bind(unsigned int stage, const SamplerState& state, TextureGL** cache)
{
    if (*cache != this || cachedState != state)
	{
        glActiveTexture(GL_TEXTURE0 + stage);

		GLenum target = gTextureTargetGL[type];

        if (*cache != this)
		{
            *cache = this;

            glBindTexture(target, id);
		}

        if (cachedState != state)
		{
            cachedState = state;

			bool hasMips = mipLevels > 1;

			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, gSamplerFilterMinGL[state.getFilter()][hasMips]);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, gSamplerFilterMagGL[state.getFilter()]);

			glTexParameteri(target, GL_TEXTURE_WRAP_S, gSamplerAddressGL[state.getAddress()]);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, gSamplerAddressGL[state.getAddress()]);

		#ifndef GLES
            int anisotropy = state.getFilter() == SamplerState::Filter_Anisotropic ? state.getAnisotropy() : 1;

			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(anisotropy));
		#endif
		}
	}
}

unsigned int TextureGL::getTarget() const
{
	return gTextureTargetGL[type];
}

unsigned int TextureGL::getInternalFormat(Format format)
{
	return gTextureFormatGL[format].internalFormat;
}

}
}
