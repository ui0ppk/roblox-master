#include "stdafx.h"
#include "v8datamodel/NumberRange.h"



namespace RBX
{

    NumberRange::NumberRange(float a, float b)
    : min(a), max(b)
    {
        if (b<a) { min = b; max = a; }
    }

    //////////////////////////////////////////////////////////////////////////

static std::string tostr( const NumberRange& nr )
{
    std::ostringstream oss;
    oss<<nr.min<<" "<<nr.max<<" ";
    return oss.str();
}

static NumberRange fromstr( const std::string& str )
{
    NumberRange nr;
    std::istringstream iss(str);
    iss>>nr.min>>nr.max;
    return nr;
}

namespace Reflection
{
    
template<> int TypedPropertyDescriptor<NumberRange>::getDataSize( const DescribedBase* instance ) const
{
    return sizeof(NumberRange);
}


template<> void TypedPropertyDescriptor<NumberRange>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    std::string nodeval;
    element->getValue(nodeval);
    setValue(instance,fromstr(nodeval));
}

template<> void TypedPropertyDescriptor<NumberRange>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    std::string nodeval = tostr( getValue(instance) );
    element->setValue(nodeval);
}

template<> bool TypedPropertyDescriptor<NumberRange>::hasStringValue() const { return true; }

template<> std::string TypedPropertyDescriptor<NumberRange>::getStringValue(const DescribedBase* instance) const
{
    NumberRange range = getValue(instance);
    std::string text = tostr(range);
    return text;
}

template<> bool TypedPropertyDescriptor<NumberRange>::setStringValue(DescribedBase* instance, const std::string& text) const
{
    NumberRange range = fromstr(text);
    setValue(instance, range);
    return true;
}

template<> NumberRange& Variant::convert<NumberRange>()
{
    return genericConvert<NumberRange>();
}

template<> Reflection::Type const& Reflection::Type::getSingleton<NumberRange>()
{
    static TType<NumberRange> type("NumberRange");
    return type;
}
    
} // end namespace Reflection
    
    
template<> bool StringConverter<NumberRange>::convertToValue(const std::string& text, NumberRange& value)
{
    if (text.empty()) 
        return false;
    value = fromstr(text);
    return true;
}

template<> std::string StringConverter<NumberRange>::convertToString(const NumberRange& value)
{
    return tostr(value);
}

}
