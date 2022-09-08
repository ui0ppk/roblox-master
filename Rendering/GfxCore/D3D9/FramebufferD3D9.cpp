#include "FramebufferD3D9.h"

#include "TextureD3D9.h"
#include "DeviceD3D9.h"

#include <d3d9.h>

namespace RBX
{
namespace Graphics
{

static IDirect3DSurface9* createRenderbuffer(IDirect3DDevice9* device9, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
{
    IDirect3DSurface9* result = NULL;

	D3DFORMAT format9 = static_cast<D3DFORMAT>(TextureD3D9::getInternalFormat(format));
	D3DMULTISAMPLE_TYPE msaa9 = (samples > 1) ? static_cast<D3DMULTISAMPLE_TYPE>(samples) : D3DMULTISAMPLE_NONE;

	HRESULT hr = D3DERR_INVALIDDEVICE;

	if (Texture::isFormatDepth(format))
		hr = device9->CreateDepthStencilSurface(width, height, format9, msaa9, 0, false, &result, NULL);
	else
		hr = device9->CreateRenderTarget(width, height, format9, msaa9, 0, false, &result, NULL);

    if (FAILED(hr))
		throw RBX::runtime_error("Error creating renderbuffer: %x", hr);

    return result;
}

static IDirect3DSurface9* getSurface(shared_ptr<TextureD3D9> container, RenderbufferD3D9::OwnerType ot, unsigned level)
{
    if (!container) return 0;

    IDirect3DSurface9* ret = 0;
    switch (ot)
    {
    case RenderbufferD3D9::Owner_Tex2: 
            static_cast<IDirect3DTexture9*>(container->getObject())->GetSurfaceLevel(level, &ret);
            break;
    case RenderbufferD3D9::Owner_CubePosX:
    case RenderbufferD3D9::Owner_CubeNegX:
    case RenderbufferD3D9::Owner_CubePosY:
    case RenderbufferD3D9::Owner_CubeNegY:
    case RenderbufferD3D9::Owner_CubePosZ:
    case RenderbufferD3D9::Owner_CubeNegZ:
            static_cast<IDirect3DCubeTexture9*>(container->getObject())->GetCubeMapSurface((D3DCUBEMAP_FACES)ot, level, &ret);
            break;
    default:
        RBXASSERT(!"bad owner type for renderbuffer");
        break;
    }
    return ret;
}

RenderbufferD3D9::RenderbufferD3D9(Device* device, shared_ptr<TextureD3D9> container, OwnerType tgt)
	: Renderbuffer(device, container->getFormat(), container->getWidth(), container->getHeight(), 1)
    , object(0)
    , target(tgt)
    , owner(container)
{
    object = getSurface(owner, target, 0);
}

RenderbufferD3D9::RenderbufferD3D9(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples, IDirect3DSurface9* object)
    : Renderbuffer(device, format, width, height, samples)
    , object(object)
    , target(Owner_None)
{

}

RenderbufferD3D9::RenderbufferD3D9(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
	: Renderbuffer(device, format, width, height, samples)
    , object(NULL)
    , target(Owner_None)
{
	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	object = createRenderbuffer(device9, format, width, height, samples);
}

RenderbufferD3D9::~RenderbufferD3D9()
{
    if (object)
        object->Release();
}

void RenderbufferD3D9::onDeviceLost()
{
    if (object)
    {
        object->Release();
        object = NULL;
    }
}

void RenderbufferD3D9::onDeviceRestored()
{
    if (!object)
	{
        object = getSurface(owner, target, 0);
    }

    if (!object)
    {
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();
        object = createRenderbuffer(device9, format, width, height, samples);
	}
}

void RenderbufferD3D9::updateObject(IDirect3DSurface9* value)
{
    RBXASSERT(!object);
    object = value;
}

FramebufferD3D9::FramebufferD3D9(Device* device, IDirect3DSurface9* colorSurface, IDirect3DSurface9* depthSurface)
	: Framebuffer(device, 0, 0, 1)
{
	RBXASSERT(colorSurface && depthSurface);

	D3DSURFACE_DESC cdesc = {};
	colorSurface->GetDesc(&cdesc);

	D3DSURFACE_DESC ddesc = {};
	depthSurface->GetDesc(&ddesc);

	RBXASSERT(cdesc.Width == ddesc.Width && cdesc.Height == ddesc.Height);

	width = cdesc.Width;
	height = cdesc.Height;

	color.push_back(shared_ptr<RenderbufferD3D9>(new RenderbufferD3D9(device, Texture::Format_RGBA8, width, height, 1, colorSurface)));
	depth.reset(new RenderbufferD3D9(device, Texture::Format_D24S8, width, height, 1, depthSurface));
}

FramebufferD3D9::FramebufferD3D9(Device* device, const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth)
	: Framebuffer(device, 0, 0, 0)
    , color(color)
    , depth(depth)
{
	RBXASSERT(!color.empty());

	if (color.size() > device->getCaps().maxDrawBuffers)
		throw RBX::runtime_error("Unsupported framebuffer configuration: too many buffers (%d)", color.size());

    for (size_t i = 0; i < color.size(); ++i)
	{
		RenderbufferD3D9* buffer = static_cast<RenderbufferD3D9*>(color[i].get());
        RBXASSERT(buffer);
		RBXASSERT(!Texture::isFormatDepth(buffer->getFormat()));

        if (i == 0)
		{
			width = buffer->getWidth();
			height = buffer->getHeight();
			samples = buffer->getSamples();
		}
		else
		{
			RBXASSERT(width == buffer->getWidth());
			RBXASSERT(height == buffer->getHeight());
			RBXASSERT(samples == buffer->getSamples());
		}
	}

    if (depth)
	{
		RenderbufferD3D9* buffer = static_cast<RenderbufferD3D9*>(depth.get());
		RBXASSERT(Texture::isFormatDepth(buffer->getFormat()));

        RBXASSERT(width == buffer->getWidth());
        RBXASSERT(height == buffer->getHeight());
        RBXASSERT(samples == buffer->getSamples());
	}
}

FramebufferD3D9::~FramebufferD3D9()
{
}

void FramebufferD3D9::download(void* data, unsigned int size)
{
	RBXASSERT(size == width * height * 4);

	IDirect3DSurface9* tempSurface = grabCopy();

    D3DSURFACE_DESC surfaceDesc;
    tempSurface->GetDesc(&surfaceDesc);

	RBXASSERT(surfaceDesc.Format == D3DFMT_A8R8G8B8 || surfaceDesc.Format == D3DFMT_X8R8G8B8);

	D3DLOCKED_RECT surfaceRect = {};
	tempSurface->LockRect(&surfaceRect, NULL, 0);

	RBXASSERT(static_cast<unsigned int>(surfaceRect.Pitch) >= width * 4);

    for (unsigned int y = 0; y < height; ++y)
	{
        char* dataRow = static_cast<char*>(data) + y * width * 4;
		memcpy(dataRow, static_cast<char*>(surfaceRect.pBits) + y * surfaceRect.Pitch, width * 4);

        for (unsigned int x = 0; x < width; ++x)
            std::swap(dataRow[x * 4 + 0], dataRow[x * 4 + 2]);
	}

    tempSurface->UnlockRect();
	tempSurface->Release();
}

IDirect3DSurface9* FramebufferD3D9::grabCopy()
{
	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	RBXASSERT(!color.empty());
	IDirect3DSurface9* colorSurface = static_cast<RenderbufferD3D9*>(color[0].get())->getObject();

    D3DSURFACE_DESC desc;
    colorSurface->GetDesc(&desc);

    // Create a temporary system-memory surface
    IDirect3DSurface9* systemSurface;
    HRESULT hr = device9->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &systemSurface, NULL);

    if (FAILED(hr))
        throw RBX::runtime_error("grabCopy failed: can't create off-screen surface: %x", hr);

    // Copy data to the surface
	hr = device9->GetRenderTargetData(colorSurface, systemSurface);

    if (FAILED(hr))
	{
		systemSurface->Release();

        throw RBX::runtime_error("grabCopy failed: can't read render-target data: %x", hr);
	}

    return systemSurface;
}

}
}
