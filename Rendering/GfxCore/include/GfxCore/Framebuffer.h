#pragma once

#include "Resource.h"
#include "Texture.h"

namespace RBX
{
namespace Graphics
{

class Renderbuffer: public Resource
{
public:
    ~Renderbuffer();

	Texture::Format getFormat() const { return format; }

	unsigned int getWidth() const { return width; }
	unsigned int getHeight() const { return height; }
    unsigned int getSamples() const { return samples; }

protected:
    Renderbuffer(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples);

	Texture::Format format;
    unsigned int width;
    unsigned int height;
    unsigned int samples;
};

class Framebuffer: public Resource
{
public:
    ~Framebuffer();

    virtual void download(void* data, unsigned int size) = 0;

	unsigned int getWidth() const { return width; }
	unsigned int getHeight() const { return height; }
    unsigned int getSamples() const { return samples; }

protected:
	Framebuffer(Device* device, unsigned int width, unsigned int height, unsigned int samples);

    unsigned int width;
    unsigned int height;
    unsigned int samples;
};

}
}
