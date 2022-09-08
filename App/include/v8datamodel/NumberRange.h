#pragma once

#undef min
#undef max

namespace RBX
{
    class NumberRange
    {
    public:
        float min;
        float max;

        NumberRange(float a = 0) : min(a), max(a) {}
        NumberRange(float a, float b);
        bool operator==(const NumberRange& r) const { return min == r.min && max == r.max; }
        bool operator!=(const NumberRange& r) const { return !operator==(r); }
    };

}
