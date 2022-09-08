#include "stdafx.h"

#include "Util/ProtectedString.h"

#include "Reflection/Property.h"
#include "Util/MD5Hasher.h"
#include "V8DataModel/HackDefines.h"

#include <string>

namespace RBX {

const ProtectedString ProtectedString::emptyString;

ProtectedString::ProtectedString() : hash(new std::string())
{
	// Call setString here so that the no-arg constructor is guaranteed to act
	// the same as setting the string to empty.
	setString("", "");
}

ProtectedString::ProtectedString(const ProtectedString& other)
    : source(other.source)
    , bytecode(other.bytecode)
    , hash(new std::string(*other.hash))
{
	// Don't use setString here, preserve all of the fields from other 
	// precisely as they are. If the source hasn't been tampered with
	// then the hash will match, otherwise we want to preserve the fact that
	// the hash is invalid for source.
}

ProtectedString ProtectedString::fromTrustedSource(const std::string& trustedString)
{
	ProtectedString result;
	result.setString(trustedString, "");
	return result;
}

ProtectedString ProtectedString::fromBytecode(const std::string& stringRef)
{
	ProtectedString result;
	result.setString("", stringRef);
	return result;
}

void ProtectedString::setString(const std::string& newSource, const std::string& newBytecode)
{
	source = newSource;
    bytecode = newBytecode;
	calculateHash(hash.get());
#if !defined(RBX_STUDIO_BUILD)
	if (source != newSource)
	{
		DataModel::sendStats |= HATE_LUA_SCRIPT_HASH_CHANGED;
	}
#endif
}

const std::string& ProtectedString::getOriginalHash() const
{
	return *hash;
}

void ProtectedString::calculateHash(std::string* out) const
{
	if (!source.empty()) {
		boost::scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());
		// adding salt to hash computation
        // by using the __TIME__ macro we essentially randomize the salt between builds
		hasher->addData(__TIME__);
		hasher->addData(source);
		*out = hasher->toString();
	} else {
		*out = std::string();
	}
}

bool ProtectedString::operator==(const ProtectedString& other) const
{
	return source == other.source && bytecode == other.bytecode;
}

bool ProtectedString::operator!=(const ProtectedString& other) const
{
    return !(*this == other);
}

ProtectedString& ProtectedString::operator=(const ProtectedString& other)
{
	setString(other.source, other.bytecode);
	return *this;
}

size_t hash_value(const ProtectedString& value)
{
    size_t seed = 0;
    boost::hash_combine(seed, value.getSource());
    boost::hash_combine(seed, value.getBytecode());
    return seed;
}
    
}

template<>
bool XmlNameValuePair::getValue(RBX::ProtectedString& value) const {
	std::string tmp;
	if (getValue(tmp)) {
		value = RBX::ProtectedString::fromTrustedSource(tmp);
		return true;
	}
	return false;
}

namespace RBX
{
	template<>
	std::string StringConverter<ProtectedString>::convertToString(const ProtectedString& value)
	{
		return value.getSource();
	}

	template<>
	bool StringConverter<ProtectedString>::convertToValue(const std::string& text, ProtectedString& value)
	{
		value = ProtectedString::fromTrustedSource(text);
		return true;
	}

	namespace Reflection
	{		
		template<>
		const Type& Type::getSingleton<ProtectedString>()
		{
			static TType<RBX::ProtectedString> type("ProtectedString");
			return type;
		}


		template<>
		void TypedPropertyDescriptor<ProtectedString>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
		{
			if (!element->isXsiNil())
			{
				ProtectedString value;
				if (element->getValue(value))
					setValue(instance, value);
			}
		}

		template<>
		void TypedPropertyDescriptor<ProtectedString>::writeValue(const DescribedBase* instance, XmlElement* element) const
		{
			element->setValue(StringConverter<ProtectedString>::convertToString(getValue(instance)));
		}



		template<>
		ProtectedString& Variant::convert<ProtectedString>(void)
		{
			if (_type->isType<std::string>())
			{
				value = cast<std::string>();
				_type = &Type::singleton<ProtectedString>();
			}
			return genericConvert<ProtectedString>();
		}

        template<>
        int TypedPropertyDescriptor<ProtectedString>::getDataSize(const DescribedBase* instance) const 
        {
            return sizeof(ProtectedString) + getValue(instance).getSource().length();
        }

		template<>
		bool TypedPropertyDescriptor<ProtectedString>::hasStringValue() const {
			return true;
		}

		template<>
		std::string TypedPropertyDescriptor<ProtectedString>::getStringValue(const DescribedBase* instance) const{
			return StringConverter<ProtectedString>::convertToString(getValue(instance));
		}

		template<>
		bool TypedPropertyDescriptor<ProtectedString>::setStringValue(DescribedBase* instance, const std::string& text) const {
			ProtectedString value;
			if (StringConverter<ProtectedString>::convertToValue(text, value))
			{
				setValue(instance, value);
				return true;
			}
			else
				return false;
		}
	}
}
