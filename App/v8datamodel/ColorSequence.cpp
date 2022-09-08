#include "stdafx.h"
#include "v8datamodel/ColorSequence.h"
#include "g3d/g3dmath.h"
#include "G3d/g3dmath.h"
using G3D::clamp;
using G3D::lerp;

namespace RBX{ 

static inline Color3 clamp( Color3 v )
{
    v.r = G3D::clamp(v.r,0,1);
    v.g = G3D::clamp(v.g,0,1);
    v.b = G3D::clamp(v.b,0,1);
    return v;
}

ColorSequence::ColorSequence( Color3 constant ) 
: m_data(2)
{
    m_data[0] = Key(0,constant,0);
    m_data[1] = Key(1,constant,0);
}

ColorSequence::ColorSequence( Color3 a, Color3 b )
: m_data(2)
{
    m_data[0] = Key(0,a,0);
    m_data[1] = Key(1,b,0);
}

ColorSequence::ColorSequence(const std::vector<Key>& keys, bool exceptions)
{
    if (!validate(keys, exceptions))
    {
        ColorSequence().m_data.swap(m_data);
        return;
    }
    m_data = keys;
    m_data.front().time = 0;
    m_data.back().time  = 1;
}

ColorSequence::ColorSequence(const ColorSequence& r)
{
    RBXASSERT(validate(r.m_data, false));

    m_data = r.m_data;
    for (unsigned j=0, e=m_data.size(); j<e; ++j )
    {
        m_data[j].time = G3D::clamp(m_data[j].time, 0, 1);
        m_data[j].value = clamp(m_data[j].value);
        m_data[j].envelope = 0;
    }
}

void ColorSequence::resample( G3D::Vector3* min, G3D::Vector3* max, int numPoints) const
{
    // invariant violation: this should not happen under any circumstances:
    RBXASSERT( m_data.size() >= 2 );
    RBXASSERT( m_data.front().time == 0.0f ); // yup, straight equal 
    RBXASSERT( m_data.back().time == 1.0f ); // ditto

    int src = 0;
    float t = 0, dt = 1.0f/(numPoints-1.0f) - 1e-5f;
    const std::vector<Key>& data = m_data;

    min[0] = max[0] = Vector3(data[0].value);

    for( int j=1; j<numPoints; ++j )
    {
        t += dt;
        RBXASSERT( data[src].time <= data[src+1].time ); // invariant violation
        while( m_data[src+1].time < t )
			src++; // find the next key

        float s = (t - data[src].time) / (data[src+1].time - data[src].time );
        max[j] = min[j] = Vector3(lerp( data[src].value, data[src+1].value, s ));
    }
}

bool ColorSequenceKeypoint::operator==(const ColorSequenceKeypoint& r) const
{
    return 0==memcmp(this, &r, sizeof(r));
}

bool ColorSequence::operator ==(const ColorSequence& r) const
{
    return m_data == r.m_data;
}

bool ColorSequence::validate(const std::vector<Key>& keys, bool exc)
{
    if (keys.size()<2)
    {
        if(exc) throw RBX::runtime_error("ColorSequence: requires at least 2 keypoints"); else return false;
    }
    
    if (keys.size()>kMaxSize)
    {
        if (exc) throw RBX::runtime_error("NumberSequence: max number of keypoints exceeded."); else return false;
    }
    
    for (unsigned j=0; j<keys.size(); ++j)
    {
        if (j < keys.size()-1 && keys[j].time >= keys[j+1].time)
        {
            if(exc) throw RBX::runtime_error("ColorSequence: all keypoints must be ordered by time"); else return false;
        }
        
        if (keys[j].value.min()<0 || keys[j].value.max()>1)
        {
            if(exc) throw RBX::runtime_error("ColorSequence: color value out of range"); else return false;
        }
    }

    if (fabsf(keys.front().time) > 1e-4f)
    {
        if(exc) throw RBX::runtime_error("ColorSequence must start at time=0.0"); else return false;
    }
    
    if (fabsf(keys.back().time - 1.0f) > 1e-4f)
    {
        if(exc) throw RBX::runtime_error("ColorSequence must end at time=1.0"); else return false;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////
// serialization support

static std::string tostr( const ColorSequence& cs )
{
    const std::vector<ColorSequence::Key>& keys = cs.getPoints();
    std::ostringstream oss;
    for( int j=0, e=keys.size(); j<e; ++j )
    {
        oss<<keys[j].time<<" "<<keys[j].value.r<<" "<<keys[j].value.g<<" "<<keys[j].value.b<<" "<<keys[j].envelope<<" ";
    }
    return oss.str();
}

static ColorSequence fromstr(const std::string& str)
{
    std::vector<ColorSequence::Key> keys;
    std::istringstream iss(str);
    while (true)
    {
        ColorSequence::Key k;
        iss>>k.time>>k.value.r>>k.value.g>>k.value.b>>k.envelope;
        if (iss.eof())
            break;
        keys.push_back(k);
    }
    return keys;
}

namespace Reflection
{

template<> int TypedPropertyDescriptor<ColorSequence>::getDataSize(const DescribedBase* instance ) const
{
    return sizeof(ColorSequence::Key) * getValue(instance).getPoints().size();
}


template<> void TypedPropertyDescriptor<ColorSequence>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    std::string nodeval;
    element->getValue(nodeval);
    setValue(instance,fromstr(nodeval));
}

template<> void TypedPropertyDescriptor<ColorSequence>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    std::string nodeval = tostr( getValue(instance) );
    element->setValue(nodeval);
}

template<> bool TypedPropertyDescriptor<ColorSequence>::hasStringValue() const { return true; }

template<> std::string TypedPropertyDescriptor<ColorSequence>::getStringValue(const DescribedBase* instance) const
{
    ColorSequence s = getValue(instance);
    std::string text = tostr(s);
    return text;
}

template<> bool TypedPropertyDescriptor<ColorSequence>::setStringValue(DescribedBase* instance, const std::string& text) const
{
    ColorSequence s = fromstr(text);
    setValue(instance, s);
    return true;
}

template<> ColorSequence& Variant::convert<ColorSequence>()
{
    return genericConvert<ColorSequence>();
}

template<> Type const& Type::getSingleton<ColorSequence>()
{
    static TType<ColorSequence> type("ColorSequence");
    return type;
}

//////////////////////////////////////////////////////////////////////////


template<> ColorSequenceKeypoint& Variant::convert<ColorSequenceKeypoint>()
{
    return genericConvert<ColorSequenceKeypoint>();
}
template<> Type const& Type::getSingleton<ColorSequenceKeypoint>()
{
    static TType<ColorSequenceKeypoint> type("ColorSequenceKeypoint");
    return type;
}

} // ns Reflection


template<> bool StringConverter<ColorSequence>::convertToValue(const std::string& text, ColorSequence& value)
{
    if (text.empty()) 
        return false;
    value = fromstr(text);
    return true;
}


template<> std::string StringConverter<ColorSequence>::convertToString(const ColorSequence& value)
{
    return tostr(value);
}


template<> bool StringConverter<ColorSequenceKeypoint>::convertToValue(const std::string& text, ColorSequenceKeypoint& value)
{
    if (text.empty()) 
        return false;

    std::istringstream iss(text);
    iss>>value.time>>value.value.r>>value.value.g>>value.value.b>>value.envelope;
    return true;
}

template<> std::string StringConverter<ColorSequenceKeypoint>::convertToString(const ColorSequenceKeypoint& value)
{
    std::ostringstream oss;
    oss<<value.time<<" "<<value.value.r<<" "<<value.value.g<<" "<<value.value.b<<" "<<value.envelope<<" ";
    return oss.str();
}



} // ns RBX