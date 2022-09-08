#pragma once

#include "GfxCore/Texture.h"
#include "GfxCore/States.h"
#include <boost/enable_shared_from_this.hpp>

#include <map>
#include <vector>

namespace RBX
{
namespace Graphics
{

class TextureGL: public Texture, public boost::enable_shared_from_this<TextureGL>
{
public:
	TextureGL(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage);
	TextureGL(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage, unsigned int id);
    ~TextureGL();

    virtual void upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size);

    virtual bool download(unsigned int index, unsigned int mip, void* data, unsigned int size);

    virtual bool supportsLocking() const;
    virtual LockResult lock(unsigned int index, unsigned int mip, const TextureRegion& region);
    virtual void unlock(unsigned int index, unsigned int mip);

    virtual shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index, unsigned int mip);

    virtual void commitChanges();
    virtual void generateMipmaps();

    void bind(unsigned int stage, const SamplerState& state, TextureGL** cache);

	unsigned int getId() const { return id; }
    unsigned int getTarget() const;

    static unsigned int getInternalFormat(Format format);

private:
    struct Change
    {
        unsigned int index;
        unsigned int mip;
        TextureRegion region;

        unsigned int scratchOffset;
    };

    unsigned int id;
    
    SamplerState cachedState;

	bool external;

    unsigned int scratchId;
    unsigned int scratchOffset;
    unsigned int scratchSize;
    std::vector<Change> pendingChanges;

    typedef std::map<std::pair<unsigned,unsigned>, weak_ptr<Renderbuffer> > RenderBufferMap;
    RenderBufferMap renderBuffers;
};

}
}
