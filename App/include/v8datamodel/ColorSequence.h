#pragma once

#include <vector>
#undef min
#undef max

namespace RBX {

class ColorSequenceKeypoint
{
public:
    float   time;
    Color3  value;
    float   envelope;

    ColorSequenceKeypoint(): time(0), value(0,0,0), envelope(0) {}
    ColorSequenceKeypoint(float t, Color3 v, float e): time(t), value(v), envelope(e) {} 
    bool operator==(const ColorSequenceKeypoint& r) const;
};

class ColorSequence
{
public:
    typedef ColorSequenceKeypoint Key;

    enum { kMaxSize = 20 }; // max number of keypoints permitted

    explicit ColorSequence(Color3 constant = Color3(1,1,1));
    ColorSequence(Color3 a, Color3 b);
    ColorSequence(const std::vector<Key>& keys, bool exceptions = false);
    ColorSequence(const ColorSequence& cs);

    const std::vector<Key>& getPoints() const { return m_data; }
    Key start() const { return m_data.front(); }
    Key end() const { return m_data.back(); }
    void resample(G3D::Vector3* min, G3D::Vector3* max, int numPoints) const;

    bool operator==(const ColorSequence& r) const;

    static bool validate(const std::vector<Key>& keys, bool exceptions);

private:
    std::vector<Key> m_data;
};


}