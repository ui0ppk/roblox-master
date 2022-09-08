#include "TextureD3D9.h"

#include "FramebufferD3D9.h"
#include "DeviceD3D9.h"

#include <d3d9.h>

LOGGROUP(Graphics)

FASTFLAG(GraphicsTextureCommitChanges)

namespace RBX
{
namespace Graphics
{

struct TextureUsageD3D9
{
    D3DPOOL pool;
    DWORD usage;
};

static const TextureUsageD3D9 gTextureUsageD3D9[Texture::Usage_Count] =
{
	{ D3DPOOL_MANAGED, 0 },
	{ D3DPOOL_DEFAULT, D3DUSAGE_DYNAMIC },
	{ D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET }
};

static const D3DFORMAT gTextureFormatD3D9[Texture::Format_Count] =
{
	D3DFMT_L8,
	D3DFMT_A8L8,
	D3DFMT_A1R5G5B5,
	D3DFMT_A8R8G8B8,
	D3DFMT_G16R16,
	D3DFMT_A16B16G16R16F,
    D3DFMT_DXT1,
    D3DFMT_DXT3,
    D3DFMT_DXT5,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_UNKNOWN,
	D3DFMT_D16,
	D3DFMT_D24S8,
};

static IDirect3DBaseTexture9* createTexture(IDirect3DDevice9* device9, Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, D3DPOOL pool9, DWORD usage9)
{
    IDirect3DBaseTexture9* result = NULL;

	D3DFORMAT format9 = gTextureFormatD3D9[format];

	HRESULT hr = D3DERR_INVALIDDEVICE;

	switch (type)
	{
	case Texture::Type_2D:
		hr = device9->CreateTexture(width, height, mipLevels, usage9, format9, pool9, reinterpret_cast<IDirect3DTexture9**>(&result), NULL);
        break;

	case Texture::Type_Cube:
		hr = device9->CreateCubeTexture(width, mipLevels, usage9, format9, pool9, reinterpret_cast<IDirect3DCubeTexture9**>(&result), NULL);
        break;

	case Texture::Type_3D:
		hr = device9->CreateVolumeTexture(width, height, depth, mipLevels, usage9, format9, pool9, reinterpret_cast<IDirect3DVolumeTexture9**>(&result), NULL);
        break;
	
	default:
		RBXASSERT(false);
	}

    if (FAILED(hr))
		throw RBX::runtime_error("Error creating texture: %x", hr);

    return result;
}

TextureD3D9::TextureD3D9(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage)
	: Texture(device, type, format, width, height, depth, mipLevels, usage)
    , object(NULL)
    , mirror(NULL)
	, mirrorDirty(false)
{
	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	object = createTexture(device9, type, format, width, height, depth, mipLevels, gTextureUsageD3D9[usage].pool, gTextureUsageD3D9[usage].usage);

	if (usage == Usage_Dynamic)
		mirror = createTexture(device9, type, format, width, height, depth, mipLevels, D3DPOOL_SYSTEMMEM, 0);
}

TextureD3D9::~TextureD3D9()
{
	static_cast<DeviceD3D9*>(device)->getImmediateContextD3D9()->invalidateCachedTexture(this);

    if (object)
        object->Release();

    if (mirror)
        mirror->Release();
}

static RECT* fillRectOptional(RECT& rectData, const TextureRegion* region)
{
    if (!region)
        return NULL;

    rectData.left = region->x;
    rectData.right = region->x + region->width;
    rectData.top = region->y;
    rectData.bottom = region->y + region->height;

    return &rectData;
}

static D3DBOX* fillBoxOptional(D3DBOX& boxData, const TextureRegion* region)
{
    if (!region)
        return NULL;

    boxData.Left = region->x;
    boxData.Right = region->x + region->width;
    boxData.Top = region->y;
    boxData.Bottom = region->y + region->height;
    boxData.Front = region->z;
    boxData.Back = region->z + region->depth;

    return &boxData;
}

static D3DLOCKED_BOX boxFromRect(const D3DLOCKED_RECT& rect)
{
    D3DLOCKED_BOX box;

    box.pBits = rect.pBits;
    box.RowPitch = rect.Pitch;
    box.SlicePitch = 0;

    return box;
}

static D3DLOCKED_BOX lockHelper(IDirect3DBaseTexture9* object, unsigned int index, unsigned int mip, const TextureRegion* region, Texture::Type type, unsigned int flags)
{
	D3DLOCKED_BOX locked = {};
	HRESULT hr = D3DERR_INVALIDDEVICE;

	switch (type)
	{
	case Texture::Type_2D:
        {
            RBXASSERT(index == 0);

            RECT rectData;
			RECT* rect = fillRectOptional(rectData, region);

			D3DLOCKED_RECT lr;
            hr = static_cast<IDirect3DTexture9*>(object)->LockRect(mip, &lr, rect, flags);

			locked = boxFromRect(lr);
        }
        break;

	case Texture::Type_Cube:
        {
            RBXASSERT(index < 6);

            RECT rectData;
			RECT* rect = fillRectOptional(rectData, region);

			D3DLOCKED_RECT lr;
            hr = static_cast<IDirect3DCubeTexture9*>(object)->LockRect(static_cast<D3DCUBEMAP_FACES>(index), mip, &lr, rect, flags);

			locked = boxFromRect(lr);
        }
        break;

	case Texture::Type_3D:
		{
            RBXASSERT(index == 0);

            D3DBOX boxData;
            D3DBOX* box = fillBoxOptional(boxData, region);

            hr = static_cast<IDirect3DVolumeTexture9*>(object)->LockBox(mip, &locked, box, flags);
		}
        break;

	default:
		RBXASSERT(false);
	}

    if (FAILED(hr))
	{
		FASTLOG1(FLog::Graphics, "Failed to lock texture: %x", hr);
		locked.pBits = NULL;
	}

    return locked;
}

static void unlockHelper(IDirect3DBaseTexture9* object, unsigned int index, unsigned int mip, Texture::Type type)
{
	switch (type)
	{
	case Texture::Type_2D:
        RBXASSERT(index == 0);
		static_cast<IDirect3DTexture9*>(object)->UnlockRect(mip);
        break;

	case Texture::Type_Cube:
        RBXASSERT(index < 6);
		static_cast<IDirect3DCubeTexture9*>(object)->UnlockRect(static_cast<D3DCUBEMAP_FACES>(index), mip);
        break;

	case Texture::Type_3D:
        RBXASSERT(index == 0);
		static_cast<IDirect3DVolumeTexture9*>(object)->UnlockBox(mip);
        break;

	default:
		RBXASSERT(false);
	}
}

static void copyHelper(void* targetData, unsigned int targetRowPitch, unsigned int targetSlicePitch,
					   const void* sourceData, unsigned int sourceRowPitch, unsigned int sourceSlicePitch,
					   unsigned int width, unsigned int height, unsigned int depth, Texture::Format format)
{
    unsigned int heightBlocks = Texture::isFormatCompressed(format) ? (height + 3) / 4 : height;
    unsigned int lineSize = Texture::getImageSize(format, width, 1);

	RBXASSERT(lineSize <= sourceRowPitch && lineSize <= targetRowPitch);

	if (targetRowPitch == sourceRowPitch && targetSlicePitch == sourceSlicePitch)
	{
        // Fast path: memory layout is the same
		unsigned int size = (sourceSlicePitch == 0) ? depth * heightBlocks * sourceRowPitch : depth * sourceSlicePitch;

		memcpy(targetData, sourceData, size);
	}
	else
	{
        // Slow path: need to copy line by line
		for (unsigned int z = 0; z < depth; ++z)
			for (unsigned int yb = 0; yb < heightBlocks; ++yb)
			{
				void* target = static_cast<char*>(targetData) + z * targetSlicePitch + yb * targetRowPitch;
				const void* source = static_cast<const char*>(sourceData) + z * sourceSlicePitch + yb * sourceRowPitch;

				memcpy(target, source, lineSize);
			}
	}
}

static IDirect3DSurface9* getSurfaceHelper(IDirect3DBaseTexture9* object, unsigned int index, unsigned int mip, Texture::Type type)
{
    IDirect3DSurface9* result = NULL;

    switch (type)
    {
	case Texture::Type_2D:
        RBXASSERT(index == 0);
        static_cast<IDirect3DTexture9*>(object)->GetSurfaceLevel(mip, &result);
        break;

    case Texture::Type_Cube:
        RBXASSERT(index < 6);
        static_cast<IDirect3DCubeTexture9*>(object)->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(index), mip, &result);
        break;

    default:
        RBXASSERT(false);
    }

    return result;
}

void TextureD3D9::upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size)
{
    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);

    RBXASSERT(mip < mipLevels);
	RBXASSERT(region.x + region.width <= mipWidth);
	RBXASSERT(region.y + region.height <= mipHeight);
	RBXASSERT(region.z + region.depth <= mipDepth);
    
    RBXASSERT(size == getImageSize(format, region.width, region.height) * region.depth);

	bool partialUpload = (region.width != mipWidth || region.height != mipHeight || region.depth != mipDepth);

	IDirect3DBaseTexture9* lockTarget = mirror ? mirror : object;

	D3DLOCKED_BOX locked = lockHelper(lockTarget, index, mip, partialUpload ? &region : NULL, type, 0);

	unsigned int dataRowPitch = Texture::getImageSize(format, region.width, 1);
	unsigned int dataSlicePitch = (type == Type_3D) ? Texture::getImageSize(format, region.width, region.height) : 0;

	copyHelper(locked.pBits, locked.RowPitch, locked.SlicePitch, data, dataRowPitch, dataSlicePitch, region.width, region.height, region.depth, format);

	unlockHelper(lockTarget, index, mip, type);

	markAsDirty();
}

bool TextureD3D9::download(unsigned int index, unsigned int mip, void* data, unsigned int size)
{
	if (gTextureUsageD3D9[usage].pool != D3DPOOL_MANAGED)
        return false;

    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);

    RBXASSERT(mip < mipLevels);
    RBXASSERT(size == getImageSize(format, mipWidth, mipHeight) * mipDepth);

	IDirect3DBaseTexture9* lockTarget = mirror ? mirror : object;

	D3DLOCKED_BOX locked = lockHelper(lockTarget, index, mip, NULL, type, D3DLOCK_READONLY);

	unsigned int dataRowPitch = Texture::getImageSize(format, mipWidth, 1);
	unsigned int dataSlicePitch = (type == Type_3D) ? Texture::getImageSize(format, mipWidth, mipHeight) : 0;

	copyHelper(data, dataRowPitch, dataSlicePitch, locked.pBits, locked.RowPitch, locked.SlicePitch, mipWidth, mipHeight, mipDepth, format);

	unlockHelper(lockTarget, index, mip, type);

	return true;
}

bool TextureD3D9::supportsLocking() const
{
    return true;
}

Texture::LockResult TextureD3D9::lock(unsigned int index, unsigned int mip, const TextureRegion& region)
{
    unsigned int mipWidth = getMipSide(width, mip);
    unsigned int mipHeight = getMipSide(height, mip);
    unsigned int mipDepth = getMipSide(depth, mip);

    RBXASSERT(mip < mipLevels);
	RBXASSERT(region.x + region.width <= mipWidth);
	RBXASSERT(region.y + region.height <= mipHeight);
	RBXASSERT(region.z + region.depth <= mipDepth);
    
	bool partialLock = (region.width != mipWidth || region.height != mipHeight || region.depth != mipDepth);

	IDirect3DBaseTexture9* lockTarget = mirror ? mirror : object;

	D3DLOCKED_BOX locked = lockHelper(lockTarget, index, mip, partialLock ? &region : NULL, type, 0);

	Texture::LockResult result = {locked.pBits, locked.RowPitch, locked.SlicePitch};

    return result;
}

void TextureD3D9::unlock(unsigned int index, unsigned int mip)
{
	IDirect3DBaseTexture9* lockTarget = mirror ? mirror : object;

	unlockHelper(lockTarget, index, mip, type);

	markAsDirty();
}

shared_ptr<Renderbuffer> TextureD3D9::getRenderbuffer(unsigned int index, unsigned int mip)
{
    RBXASSERT(mip < mipLevels);

	weak_ptr<Renderbuffer>& slot = renderbuffers[std::make_pair(index, mip)];
    shared_ptr<Renderbuffer> result = slot.lock();

    if (!result)
	{
        RenderbufferD3D9::OwnerType tgt = RenderbufferD3D9::Owner_None;
        switch (getType())
        {
        case Type_2D: tgt = RenderbufferD3D9::Owner_Tex2; break;
        case Type_Cube: tgt = (RenderbufferD3D9::OwnerType)index; break;
        default: RBXASSERT(!"Renderbuffer for this kind of texture is not yet implemented."); return shared_ptr<Renderbuffer>();
        }

		result.reset(new RenderbufferD3D9(device, shared_from_this(), tgt));

        slot = result;
	}

    return result;
}

void TextureD3D9::commitChanges()
{
	if (!FFlag::GraphicsTextureCommitChanges)
		return;

	if (mirror && mirrorDirty)
	{
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

		device9->UpdateTexture(mirror, object);

		mirrorDirty = false;
	}
}

void TextureD3D9::generateMipmaps()
{
    if (mipLevels <= 1)
        return;

    IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

    unsigned int faces = (type == Type_Cube) ? 6 : 1;

    for (unsigned int face = 0; face < faces; ++face)
    {
        for (unsigned int level = 1; level < mipLevels; ++level)
        {
            IDirect3DSurface9* src = getSurfaceHelper(object, face, level - 1, type);
            IDirect3DSurface9* dst = getSurfaceHelper(object, face, level, type);

            device9->StretchRect(src, 0, dst, 0, D3DTEXF_LINEAR);

            src->Release();
            dst->Release();
        }
    }
}

void TextureD3D9::onDeviceLost()
{
	if (object && gTextureUsageD3D9[usage].pool == D3DPOOL_DEFAULT)
	{
		object->Release();
        object = NULL;
	}
}

void TextureD3D9::onDeviceRestored()
{
	if (gTextureUsageD3D9[usage].pool == D3DPOOL_DEFAULT)
	{
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

        object = createTexture(device9, type, format, width, height, depth, mipLevels, gTextureUsageD3D9[usage].pool, gTextureUsageD3D9[usage].usage);
	}
}

void TextureD3D9::markAsDirty()
{
    if (mirror)
	{
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

		// NVidia driver performs significantly better if there is an UpdateTexture after every Lock/Unlock in case of multiple updates per frame
		if (static_cast<DeviceD3D9*>(device)->getCapsD3D9().vendor == DeviceCapsD3D9::Vendor_NVidia)
            device9->UpdateTexture(mirror, object);
		else
			mirrorDirty = true;
	}
}

void TextureD3D9::flushChanges()
{
	if (FFlag::GraphicsTextureCommitChanges)
		return;

	if (mirror && mirrorDirty)
	{
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

		device9->UpdateTexture(mirror, object);

		mirrorDirty = false;
	}
}

unsigned int TextureD3D9::getInternalFormat(Format format)
{
	return gTextureFormatD3D9[format];
}

}
}
