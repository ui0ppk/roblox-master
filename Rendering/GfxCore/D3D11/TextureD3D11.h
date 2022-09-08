#pragma once

#include "GfxCore/Texture.h"
#include "GfxCore/States.h"
#include <boost/enable_shared_from_this.hpp>
#include <map>

struct ID3D11Resource;
struct ID3D11ShaderResourceView;
enum DXGI_FORMAT;

namespace RBX
{
namespace Graphics
{

class Renderbuffer;

class TextureD3D11: public Texture, public boost::enable_shared_from_this<TextureD3D11>
{
public:
	TextureD3D11(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage);
    ~TextureD3D11();

    virtual void upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size);
    virtual bool download(unsigned int index, unsigned int mip, void* data, unsigned int size);

    virtual bool supportsLocking() const;
    virtual LockResult lock(unsigned int index, unsigned int mip, const TextureRegion& region);
    virtual void unlock(unsigned int index, unsigned int mip);

    virtual shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index, unsigned int mip);

    virtual void commitChanges();
    virtual void generateMipmaps();

	ID3D11ShaderResourceView* getSRV() const { return objectSRV; }
	ID3D11Resource* getObject() const { return object; }

    static unsigned getInternalFormat(Texture::Format format);

private:
    ID3D11ShaderResourceView* objectSRV;
    ID3D11Resource* object;

    typedef std::map<std::pair<unsigned int, unsigned int>, weak_ptr<Renderbuffer> > RenderbufferMap;
    RenderbufferMap renderbuffers;
};

}
}
