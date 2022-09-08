#pragma once

#include "GfxCore/Framebuffer.h"
#include "TextureD3D9.h"
#include <vector>

struct IDirect3DSurface9;

namespace RBX
{
namespace Graphics
{

class RenderbufferD3D9: public Renderbuffer
{
public:
    enum OwnerType
    {
        Owner_CubePosX, // DO NOT CHANGE THE VALUE, MUST BE 0
        Owner_CubeNegX, // 1
        Owner_CubePosY, // 2
        Owner_CubeNegY, // 3
        Owner_CubePosZ, // 4
        Owner_CubeNegZ, // 5
        Owner_Tex2,
        Owner_None,
    };

	RenderbufferD3D9(Device* device, shared_ptr<TextureD3D9> owner, OwnerType target);
    RenderbufferD3D9(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples, IDirect3DSurface9* object);
	RenderbufferD3D9(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples);
    ~RenderbufferD3D9();

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

	void updateObject(IDirect3DSurface9* value);

	IDirect3DSurface9* getObject() const { return object; }

private:
    IDirect3DSurface9* object;
    shared_ptr<TextureD3D9> owner;
    OwnerType target;
};

class FramebufferD3D9: public Framebuffer
{
public:
	FramebufferD3D9(Device* device, IDirect3DSurface9* colorSurface, IDirect3DSurface9* depthSurface);
	FramebufferD3D9(Device* device, const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth);
    ~FramebufferD3D9();

    virtual void download(void* data, unsigned int size);

    IDirect3DSurface9* grabCopy();

	const std::vector<shared_ptr<Renderbuffer> >& getColor() const { return color; }
	const shared_ptr<Renderbuffer>& getDepth() const { return depth; }

private:
	std::vector<shared_ptr<Renderbuffer> > color;
    shared_ptr<Renderbuffer> depth;
};

}
}
