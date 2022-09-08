#pragma once

#include "GfxCore/Framebuffer.h"
#include "TextureGL.h"
#include <vector>

namespace RBX
{
namespace Graphics
{

class RenderbufferGL: public Renderbuffer
{
public:
	RenderbufferGL(Device* device, const shared_ptr<TextureGL>& owner, unsigned int target);
	RenderbufferGL(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples);
    ~RenderbufferGL();

	unsigned int getTextureId() const { return owner ? owner->getId() : 0; }
    unsigned int getTarget() const { return target; }
	unsigned int getBufferId() const { return bufferId; }

private:
    unsigned int target; // TEXTURE_2D or TEXTURE_CUBE_MAP_POSITIVE_X, ...
    unsigned int bufferId;
    shared_ptr<TextureGL> owner;
};

class FramebufferGL: public Framebuffer
{
public:
	FramebufferGL(Device* device, unsigned int id);
	FramebufferGL(Device* device, const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth);
    ~FramebufferGL();

    virtual void download(void* data, unsigned int size);

    void updateDimensions(unsigned int width, unsigned int height);

	unsigned int getId() const { return id; }

	unsigned int getDrawBuffers() const { return color.size(); }

private:
    unsigned int id;

    std::vector<shared_ptr<Renderbuffer> > color;
    shared_ptr<Renderbuffer> depth;
};

}
}
