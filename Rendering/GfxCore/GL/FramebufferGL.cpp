#include "FramebufferGL.h"

#include "TextureGL.h"
#include "DeviceGL.h"

#include "HeadersGL.h"

namespace RBX
{
namespace Graphics
{

RenderbufferGL::RenderbufferGL(Device* device, const shared_ptr<TextureGL>& container, unsigned int target)
	: Renderbuffer(device, container->getFormat(), container->getWidth(), container->getHeight(), 1)
    , owner(container)
    , target(target)
    , bufferId(0)
{
}

RenderbufferGL::RenderbufferGL(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
	: Renderbuffer(device, format, width, height, samples)
    , target(-1)
    , bufferId(0)
{
	RBXASSERT(samples);
	if (samples > device->getCaps().maxSamples)
		throw RBX::runtime_error("Unsupported renderbuffer: too many samples (%d)", samples);

	glGenRenderbuffers(1, &bufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, bufferId);

    if (samples > 1)
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, TextureGL::getInternalFormat(format), width, height);
	else
        glRenderbufferStorage(GL_RENDERBUFFER, TextureGL::getInternalFormat(format), width, height);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

RenderbufferGL::~RenderbufferGL()
{
	if (bufferId)
		glDeleteRenderbuffers(1, &bufferId);
}

FramebufferGL::FramebufferGL(Device* device, unsigned int id)
	: Framebuffer(device, 0, 0, 1)
    , id(id)
{
}

FramebufferGL::FramebufferGL(Device* device, const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth)
	: Framebuffer(device, 0, 0, 0)
    , id(0)
    , color(color)
    , depth(depth)
{
	RBXASSERT(!color.empty());

	if (color.size() > device->getCaps().maxDrawBuffers)
		throw RBX::runtime_error("Unsupported framebuffer configuration: too many buffers (%d)", (int)color.size());

	glGenFramebuffers(1, &id);
	glBindFramebuffer(GL_FRAMEBUFFER, id);

    for (size_t i = 0; i < color.size(); ++i)
	{
		RenderbufferGL* buffer = static_cast<RenderbufferGL*>(color[i].get());
        RBXASSERT(buffer);
		RBXASSERT(!Texture::isFormatDepth(buffer->getFormat()));

		if (buffer->getTextureId())
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, buffer->getTarget(), buffer->getTextureId(), 0);
		else
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, buffer->getBufferId());

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
		RenderbufferGL* buffer = static_cast<RenderbufferGL*>(depth.get());
		RBXASSERT(buffer->getTextureId() == 0);
		RBXASSERT(Texture::isFormatDepth(buffer->getFormat()));

        RBXASSERT(width == buffer->getWidth());
        RBXASSERT(height == buffer->getHeight());
        RBXASSERT(samples == buffer->getSamples());

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer->getBufferId());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer->getBufferId());
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		glDeleteFramebuffers(1, &id);

		throw RBX::runtime_error("Unsupported framebuffer configuration: error %x", status);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FramebufferGL::~FramebufferGL()
{
    if (id)
		glDeleteFramebuffers(1, &id);
}

void FramebufferGL::download(void* data, unsigned int size)
{
    RBXASSERT(size == width * height * 4);

    GLint oldfb = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldfb);

	glBindFramebuffer(GL_FRAMEBUFFER, id);

#ifndef GLES
    glReadBuffer(id == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
#endif
    
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindFramebuffer(GL_FRAMEBUFFER, oldfb);

    std::vector<char> tempRow(width * 4);

    for (unsigned int y = 0; y < height / 2; ++y)
    {
        char* dataRow = static_cast<char*>(data) + y * width * 4;
        char* dataOppositeRow = static_cast<char*>(data) + (height - 1 - y) * width * 4;

        memcpy(&tempRow[0], dataRow, width * 4);
        memcpy(dataRow, dataOppositeRow, width * 4);
        memcpy(dataOppositeRow, &tempRow[0], width * 4);
    }
}

void FramebufferGL::updateDimensions(unsigned int width, unsigned int height)
{
	RBXASSERT(color.empty() && !depth);

    this->width = width;
	this->height = height;
}

}
}
