#pragma once

namespace RBX
{
namespace Graphics
{

struct ImageInfo
{
    unsigned int width;
    unsigned int height;
    bool alpha;

    ImageInfo()
        : width(0)
        , height(0)
        , alpha(false)
    {
    }

    ImageInfo(unsigned int width, unsigned int height, bool alpha)
        : width(width)
        , height(height)
        , alpha(alpha)
    {
    }
};

}
}
