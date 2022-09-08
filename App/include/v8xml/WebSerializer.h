#pragma once

#include "Reflection/Type.h"

class XmlElement;

namespace RBX
{
	class WebSerializer {
	public:
		static XmlElement* writeTable(const RBX::Reflection::ValueMap& result);
		static XmlElement* writeList(const RBX::Reflection::ValueArray& result);
		static XmlElement* writeEntry(const std::string& key, const RBX::Reflection::Variant& value);
		static XmlElement* writeValue(const RBX::Reflection::Variant& value);
	};
}
