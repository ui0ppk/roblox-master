#include "stdafx.h"

#include "v8datamodel/NumberSequence.h"
#include "reflection/Type.h"
#include "reflection/EnumConverter.h"
#include "G3d/g3dmath.h"
using G3D::clamp;
using G3D::lerp;


namespace RBX
{



NumberSequence::NumberSequence(float value)
{
    m_data.resize(2);
    m_data[0] = Key(0, value, 0);
    m_data[1] = Key(1, value, 0);
}

NumberSequence::NumberSequence(const std::vector<Key>& keys, bool exc)
{
    if (!validate(keys,exc))
    {
        NumberSequence(0.0f).m_data.swap(m_data);
        return;
    }
    m_data = keys;
    m_data.front().time = 0.0f;
    m_data.back().time = 1.0f;
}

NumberSequence::NumberSequence(const NumberSequence& r, float min /* = -1e22f */, float max /* = 1e22f */)
: m_data(r.m_data)
{
    RBXASSERT(validate(m_data, false));

    for (unsigned j=0, e=m_data.size(); j<e; ++j )
    {
        m_data[j].value = clamp(m_data[j].value, min, max);
        m_data[j].envelope = clamp(m_data[j].envelope, 0, max - m_data[j].value);
        m_data[j].envelope = clamp(m_data[j].envelope, 0, m_data[j].value);
    }
}

void NumberSequence::resample(Vector2* result, int numPoints, float minV, float maxV) const
{
    // this should not happen under any circumstances:
    RBXASSERT( m_data.size() >= 2 );
    RBXASSERT( m_data.front().time == 0.0f ); // ==
    RBXASSERT( m_data.back().time == 1.0f );  // 

    int src = 0;
    float t = 0, dt = 1.0f/(numPoints-1.0f) - 1e-5f;
    const std::vector<Key>& data = m_data;

    result[0].x = clamp(data[0].value - data[0].envelope, minV, maxV);
    result[0].y = clamp(data[0].value + data[0].envelope, minV, maxV);

    for( int j=1; j<numPoints; ++j )
    {
        t += dt;
        while( data[src+1].time < t ) src++; // find the next key

        float s = (t - data[src].time) / (data[src+1].time - data[src].time );
        float v = lerp( data[src].value, data[src+1].value, s );
        float e = lerp( data[src].envelope, data[src+1].envelope, s );

        result[j].x = clamp(v - e, minV, maxV);
        result[j].y = clamp(v + e, minV, maxV);
    }
}

bool NumberSequenceKeypoint::operator==(const NumberSequenceKeypoint& b) const
{
    return 0==memcmp(this, &b, sizeof(b));
}

bool NumberSequence::operator==(const NumberSequence& r) const
{
    return m_data == r.m_data;
}

bool NumberSequence::validate(const std::vector<Key>& keys, bool exc)
{
    if (keys.size()<2)
    {
        if (exc) throw RBX::runtime_error("NumberSequence: requires at least 2 keypoints"); else return false;
    }
    
    if (keys.size()>kMaxSize)
    {
        if (exc) throw RBX::runtime_error("NumberSequence: max. number of keypoints exceeded."); else return false;
    }
    
    for (unsigned j=0; j<keys.size(); ++j)
    {
        if ( j<keys.size()-1 && keys[j].time > keys[j+1].time)
        {
            if (exc) throw RBX::runtime_error("NumberSequence: all keypoints must be ordered by time"); else return false;
        }
//        if (keys[j].value < 0)
//            if (exc) throw RBX::runtime_error("NumberSequence: value must be non-negative"); else return false;

        if (keys[j].envelope < 0 )
        {
            if (exc) throw RBX::runtime_error("NumberSequence: envelope must be non-negative"); else return false;
        }
        
        if (keys[j].value - keys[j].envelope < 0)
        {
            if (exc) throw RBX::runtime_error("NumberSequence: envelope must not exceed the value"); else return false;
        }
    }

    if (fabsf(keys.front().time) > 1e-4f)
    {
        if (exc) throw RBX::runtime_error("NumberSequence must start at time=0.0"); else return false;
    }
    
    if (fabsf(keys.back().time - 1.0f) > 1e-4f)
    {
        if (exc) throw RBX::runtime_error("NumberSequence must end at time=1.0"); else return false;
    }
    
    return true;
}

//////////////////////////////////////////////////////////////////////////
// "Tread lightly, for this is a hallowed ground!"  - F. Grigory
//  

static std::string tostr( const NumberSequence& ns )
{
    const std::vector<NumberSequence::Key>& keys = ns.getPoints();
    std::ostringstream oss;
    for( int j=0, e=keys.size(); j<e; ++j )
    {
        oss<<keys[j].time<<" "<<keys[j].value<<" "<<keys[j].envelope<<" ";
    }
    return oss.str();
}

static NumberSequence fromstr( const std::string& str )
{
    std::vector<NumberSequence::Key> keys;
    std::istringstream iss(str);
    while (true)
    {
        NumberSequence::Key k;
        iss>>k.time>>k.value>>k.envelope;

        if(iss.eof()) 
            break;

        keys.push_back(k);
    }

    return keys;
}

namespace Reflection{

template<> int TypedPropertyDescriptor<NumberSequence>::getDataSize( const DescribedBase* instance ) const
{
    return sizeof(NumberSequence::Key) * getValue(instance).getPoints().size();
}


template<> void TypedPropertyDescriptor<NumberSequence>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    std::string nodeval;
    element->getValue(nodeval);
    setValue(instance, fromstr(nodeval));
}

template<> void TypedPropertyDescriptor<NumberSequence>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    std::string nodeval = tostr( getValue(instance) );
    element->setValue(nodeval);
}

template<> bool TypedPropertyDescriptor<NumberSequence>::hasStringValue() const { return true; }

template<> std::string TypedPropertyDescriptor<NumberSequence>::getStringValue(const DescribedBase* instance) const
{
    return tostr( getValue(instance) );
}

template<> bool TypedPropertyDescriptor<NumberSequence>::setStringValue(DescribedBase* instance, const std::string& text) const
{
    setValue(instance, fromstr(text));
    return true;
}

template<> NumberSequence& Variant::convert<NumberSequence>()
{
    return genericConvert<NumberSequence>();
}

template<> Type const& Type::getSingleton<NumberSequence>()
{
    static TType<NumberSequence> type("NumberSequence");
    return type;
}

//////////////////////////////////////////////////////////////////////////
// 

template<> NumberSequenceKeypoint& Variant::convert<NumberSequenceKeypoint>()
{
    return genericConvert<NumberSequenceKeypoint>();
}

template<> Type const& Type::getSingleton<NumberSequenceKeypoint>()
{
    static TType<NumberSequenceKeypoint> type("NumberSequenceKeypoint");
    return type;
}


} // ns Reflection


template<> std::string StringConverter<NumberSequence>::convertToString(const NumberSequence& value)
{
    return tostr(value);
}


template<> bool StringConverter<NumberSequence>::convertToValue(const std::string& text, NumberSequence& value)
{
    if (text.empty()) 
        return false;
    value = fromstr(text);
    return true;
}

template<> bool StringConverter<NumberSequenceKeypoint>::convertToValue(const std::string& text, NumberSequenceKeypoint& value)
{
    if (text.empty()) 
        return false;
    
    std::istringstream iss(text);
    iss>>value.time>>value.value>>value.envelope;
    return true;
}

template<> std::string StringConverter<NumberSequenceKeypoint>::convertToString(const NumberSequenceKeypoint& value)
{
    std::ostringstream oss;
    oss<<value.time<<" "<<value.value<<" "<<value.envelope<<" ";
    return oss.str();
}

} // ns RBX


