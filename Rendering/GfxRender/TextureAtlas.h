#pragma once
#include "TextureRef.h"
#include "GfxCore/Texture.h"

namespace RBX
{
namespace Graphics
{

class VisualEngine;

class TextureAtlas
{
public: 
    TextureAtlas(VisualEngine* visualEngine, unsigned width, unsigned height);
    ~TextureAtlas();

    bool getRegion(const boost::uint64_t& id, const TextureRegion*& region);
    bool insert(const boost::uint64_t& id, const Vector2int16& size, unsigned dataPitch, unsigned char* data, const TextureRegion*& outRegion);

    const Vector2int16& getLastFailedSize() { return lastFailedSize; }

    const shared_ptr<Texture>& getTexture() const { return texture.getTexture(); }

    void upload();

private:
    void reset();

    struct Vector2uint32
    {
        Vector2uint32() : x(0), y(0) {}
        Vector2uint32(unsigned xi, unsigned yi)
            : x(xi)
            , y(yi){}
        unsigned x;
        unsigned y;
    };

    unsigned char* textureMirror;
    unsigned updateWindowPosition;
    unsigned char* updateWindow;
    
    TextureRef texture;
    Vector2uint32 textureSize;
    typedef boost::unordered_map<boost::uint64_t, TextureRegion> AtlasEntries;
    AtlasEntries atlasEntries;

    Vector2int16 lastFailedSize;

    typedef std::vector<Vector2uint32> SkylineArray;
    SkylineArray skyline;
    
    unsigned moduloParam; // fast modulo by power of two numbers (number & (potNr - 1)) = number % potNr

    unsigned getUpdateWindowIdx(int x, int y);

    struct Interval1D
    {
        Interval1D(unsigned min, unsigned max): min(min), max(max) {}
        unsigned min, max;
    };
    Interval1D uploadRect;
    
    unsigned updateLineY;
};

}
}