#pragma once

#include <vector>

namespace RBX
{


class NumberSequenceKeypoint
{
public:
    float time;
    float value;
    float envelope;

    NumberSequenceKeypoint(): time(0), value(0), envelope(0) {}
    NumberSequenceKeypoint(float t, float v, float e): time(t), value(v), envelope(e) {}
    bool operator==(const NumberSequenceKeypoint& r) const;
};


class NumberSequence
{
public:
    typedef NumberSequenceKeypoint Key;

    enum { kMaxSize = 20 }; // max number of keypoints permitted

    explicit NumberSequence(float value = 0); // constant
    NumberSequence(const std::vector<Key>& keys, bool exceptions = false); // from an array
    NumberSequence(const NumberSequence& r, float min = -1e22f, float max = 1e22f);

    const std::vector<Key>& getPoints() const { return m_data; }
    Key start() const { return m_data.front(); }
    Key end() const { return m_data.back(); }
    void resample(Vector2* result, int numPoints, float minV = -1e22f, float maxV = 1e22f) const; // resamples, result is {min,max} rather than {value, envelope}
    bool operator==(const NumberSequence& r) const;

    static bool validate(const std::vector<Key>& keys, bool exceptions);

private:
    std::vector<Key>   m_data;
};

}
