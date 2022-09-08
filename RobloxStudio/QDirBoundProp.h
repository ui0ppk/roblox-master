/**
 * QDirBoundProp.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QDir>

// Roblox Headers
#include "reflection/Property.h"

namespace RBX
{
    
template<>
std::string StringConverter<QDir>::convertToString(const QDir& value)
{
    return value.absolutePath().toStdString();
}
    
template<>
bool StringConverter<QDir>::convertToValue(const std::string& text,QDir& value)
{
    value.setPath(text.c_str());
    return true;
}
    
namespace Reflection
{

template<>
const Type& Type::getSingleton<QDir>()
{
    static TType<QDir> type("QDir");
    return type;
}
   
template<>
int TypedPropertyDescriptor<QDir>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(QDir) + getValue(instance).absolutePath().size();
}

template<>
bool TypedPropertyDescriptor<QDir>::hasStringValue() const
{
	return true;
}

template<>
std::string TypedPropertyDescriptor<QDir>::getStringValue(const DescribedBase* instance) const
{
    return StringConverter<QDir>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<QDir>::setStringValue(DescribedBase* instance,const std::string& text) const 
{
	QDir value;
    if ( StringConverter<QDir>::convertToValue(text,value) )
	{
		setValue(instance,value);
		return true;
	}
	else
		return false;
}

template<>
void TypedPropertyDescriptor<QDir>::readValue(
    DescribedBase*      instance,
    const XmlElement*   element,
    IReferenceBinder&   binder ) const
{
    if (!element->isXsiNil() )
	{
        std::string text;
        if ( element->getValue(text) )
		{
            QDir value;
            value.setPath(text.c_str());
            setValue(instance,value);
		}
    }
}

template<>
void TypedPropertyDescriptor<QDir>::writeValue(const DescribedBase* instance,XmlElement* element) const
{
    QDir font = getValue(instance);
    element->setValue(font.absolutePath().toStdString());
}

template<>
QDir& Variant::convert<QDir>(void)
{
	return genericConvert<QDir>();
}

}
}