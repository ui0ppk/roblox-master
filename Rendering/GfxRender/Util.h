#pragma once

#include "Util/G3DCore.h"

namespace RBX
{
namespace Graphics
{

inline float saturate(float v)
{
    return std::min(std::max(v, 0.0f), 1.0f);
}

inline int fastCeilPositiveApprox(float v)
{
    return v + 0.999f;
}

// Note:
// 1. Due to truncating conversion to integer this only works for positive values
// 2. Obviously, it's only accurate to 0.001f - i.e. 0.0005f will give you 0, not 1
// 3. Addition is accurate for the full range of numbers representable in int16
inline Vector3int16 Vector3CeilPositiveApprox(const Vector3& v)
{
    return Vector3int16(v.x + 0.999f, v.y + 0.999f, v.z + 0.999f); 
}

inline int fastFloorInt(float value)
{
    return value < 0 ? static_cast<int>(value - 0.999f) : static_cast<int>(value);
}

inline Vector3int16 fastFloorInt(const Vector3& v)
{
    return Vector3int16(fastFloorInt(v.x), fastFloorInt(v.y), fastFloorInt(v.z));
}

inline float frac(float value) 
{
    return value - floor(value);
}

/** Convert a float32 to a float16 (NV_half_float)
  Courtesy of OpenEXR
*/
inline unsigned short floatToHalf(float f)
{
    union { float f; unsigned int i; } v;
    v.f = f;
    unsigned int i = v.i;

    register int s =  (i >> 16) & 0x00008000;
    register int e = ((i >> 23) & 0x000000ff) - (127 - 15);
    register int m =   i        & 0x007fffff;

    if (e <= 0)
    {
        if (e < -10)
        {
            return 0;
        }
        m = (m | 0x00800000) >> (1 - e);

        return static_cast<unsigned short>(s | (m >> 13));
    }
    else if (e == 0xff - (127 - 15))
    {
        if (m == 0) // Inf
        {
            return static_cast<unsigned short>(s | 0x7c00);
        } 
        else    // NAN
        {
            m >>= 13;
            return static_cast<unsigned short>(s | 0x7c00 | m | (m == 0));
        }
    }
    else
    {
        if (e > 30) // Overflow
        {
            return static_cast<unsigned short>(s | 0x7c00);
        }

        return static_cast<unsigned short>(s | (e << 10) | (m >> 13));
    }
}

inline unsigned int packColor(const Color4& color, bool colorOrderBGR)
{
	unsigned int r = static_cast<int>(G3D::clamp(color.r, 0.f, 1.f) * 255 + 0.5f);
	unsigned int g = static_cast<int>(G3D::clamp(color.g, 0.f, 1.f) * 255 + 0.5f);
	unsigned int b = static_cast<int>(G3D::clamp(color.b, 0.f, 1.f) * 255 + 0.5f);
	unsigned int a = static_cast<int>(G3D::clamp(color.a, 0.f, 1.f) * 255 + 0.5f);

	if (colorOrderBGR)
        return (a << 24) | (r << 16) | (g << 8) | b;
	else
        return (a << 24) | (b << 16) | (g << 8) | r;
}

}
}
