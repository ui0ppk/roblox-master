#pragma once

#include "GfxCore/Texture.h"
#include "GfxCore/States.h"
#include <boost/enable_shared_from_this.hpp>
#include <map>

struct IDirect3DBaseTexture9;

namespace RBX
{
namespace Graphics
{

class Renderbuffer;

class TextureD3D9: public Texture, public boost::enable_shared_from_this<TextureD3D9>
{
public:
	TextureD3D9(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage);
    ~TextureD3D9();

    virtual void upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size);

    virtual bool download(unsigned int index, unsigned int mip, void* data, unsigned int size);

    virtual bool supportsLocking() const;
    virtual LockResult lock(unsigned int index, unsigned int mip, const TextureRegion& region);
    virtual void unlock(unsigned int index, unsigned int mip);

    virtual shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index, unsigned int mip);

	virtual void commitChanges();
    virtual void generateMipmaps();

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

    void flushChanges();

	IDirect3DBaseTexture9* getObject() const { return object; }

    static unsigned int getInternalFormat(Format format);

private:
	IDirect3DBaseTexture9* object;
	IDirect3DBaseTexture9* mirror;
    bool mirrorDirty;

    typedef std::map<std::pair<unsigned int, unsigned int>, weak_ptr<Renderbuffer> > RenderbufferMap;
	RenderbufferMap renderbuffers;

    void markAsDirty();
    void copyData(unsigned int index, unsigned int mip, const TextureRegion& region, void* data, unsigned int size, bool download, unsigned int lockFlags);
};

}
}
