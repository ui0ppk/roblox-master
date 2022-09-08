/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ContentProvider.h"
#include "Util/AnimationId.h"


#include "V8DataModel/Workspace.h"
#include "V8DataModel/Camera.h"

#include "util/standardout.h"

namespace RBX {

// TODO: Refactor: It is a little ugly to have to implement these 6 functions for each "ContentID" derived class
//  Potentially we can template this a little. Maybe define a templated ContentIDPropDescriptor or something.
template<>
std::string StringConverter<AnimationId>::convertToString(const AnimationId& value)
{
	return value.toString();
}

template<>
bool StringConverter<AnimationId>::convertToValue(const std::string& text, AnimationId& value)
{
	value = text;
	return true;
}

namespace Reflection {

template<>
const Type& Type::getSingleton<RBX::AnimationId>()
{
	return Type::singleton<ContentId>();
}


template<>
void TypedPropertyDescriptor<RBX::AnimationId>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		ContentId value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}

template<>
void TypedPropertyDescriptor<RBX::AnimationId>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(ContentId(getValue(instance)));
}



template<>
RBX::AnimationId& Variant::convert<AnimationId>(void)
{
	if (_type->isType<std::string>())
	{
		value = RBX::AnimationId(cast<std::string>());
		_type = &Type::singleton<AnimationId>();
	}
	return genericConvert<RBX::AnimationId>();
}

template<>
int TypedPropertyDescriptor<RBX::AnimationId>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::AnimationId) + getValue(instance).toString().size();
}

template<>
bool TypedPropertyDescriptor<RBX::AnimationId>::hasStringValue() const {
	return true;
}

template<>
std::string TypedPropertyDescriptor<RBX::AnimationId>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::AnimationId>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<RBX::AnimationId>::setStringValue(DescribedBase* instance, const std::string& text) const {
	RBX::AnimationId value;
	if (StringConverter<RBX::AnimationId>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

} // namespace Reflection
} // namespace RBX 