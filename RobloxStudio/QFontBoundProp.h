/**
 * QFontPropDescriptor.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QFont>

// Roblox Headers
#include "reflection/Property.h"

namespace RBX
{
    
template<>
std::string StringConverter<QFont>::convertToString(const QFont& value)
{
    return value.toString().toStdString();
}
    
template<>
bool StringConverter<QFont>::convertToValue(const std::string& text,QFont& value)
{
    return value.fromString(text.c_str());
}
    
namespace Reflection
{

template<>
const Type& Type::getSingleton<QFont>()
{
    static TType<QFont> type("QFont");
    return type;
}
    
template<>
int TypedPropertyDescriptor<QFont>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(QDir) + getValue(instance).toString().size();
}

template<>
bool TypedPropertyDescriptor<QFont>::hasStringValue() const
{
	return true;
}

template<>
std::string TypedPropertyDescriptor<QFont>::getStringValue(const DescribedBase* instance) const
{
    return StringConverter<QFont>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<QFont>::setStringValue(DescribedBase* instance,const std::string& text) const 
{
	QFont value;
    if ( StringConverter<QFont>::convertToValue(text,value) )
	{
		setValue(instance,value);
		return true;
	}
	else
		return false;
}

template<>
void TypedPropertyDescriptor<QFont>::readValue(
    DescribedBase*      instance,
    const XmlElement*   element,
    IReferenceBinder&   binder ) const
{
    if (!element->isXsiNil() )
	{
        std::string text;
        if ( element->getValue(text) )
		{
            QFont value;
            value.fromString(text.c_str());
            setValue(instance,value);
		}
    }
}

template<>
void TypedPropertyDescriptor<QFont>::writeValue(const DescribedBase* instance,XmlElement* element) const
{
    QFont font = getValue(instance);
    element->setValue(font.toString().toStdString());
}

template<>
QFont& Variant::convert<QFont>(void)
{
	return genericConvert<QFont>();
}

}
}