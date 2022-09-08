#pragma once

#include "Lua/LuaBridge.h"
#include "reflection/enumConverter.h"
#include "rbxformat.h"

namespace RBX { namespace Lua {

	class AllEnumDescriptors
	{
	};
	typedef const AllEnumDescriptors* AllEnumDescriptorsPtr;

	// Represents a Reflection::EnumDescriptor::Item in Lua
	class Enums : public SingletonBridge<AllEnumDescriptorsPtr>
	{
	public:
		static void declareAllEnums(lua_State *L);
		static bool getValue(lua_State *L, unsigned int index, RBX::Reflection::Variant& value);
	};

	typedef const Reflection::EnumDescriptor* EnumDescriptorPtr;

	// Represents a Reflection::EnumDescriptor::Item in Lua
	class Enum : public SingletonBridge<EnumDescriptorPtr>
	{
	public:
	};

	typedef const Reflection::EnumDescriptor::Item* EnumDescriptorItemPtr;

	// Represents a Reflection::EnumDescriptor::Item in Lua
	class EnumItem : public SingletonBridge<EnumDescriptorItemPtr>
	{
	public:
		static EnumDescriptorItemPtr getItem(lua_State *L, unsigned int index) {
			return getObject(L, index);
		}
		static bool getItem(lua_State *L, unsigned int index, EnumDescriptorItemPtr& value) {
			return getValue(L, index, value);
		}
	};

	// specialization
	template<>
	int Bridge< AllEnumDescriptorsPtr, false >::on_tostring(const AllEnumDescriptorsPtr& object, lua_State *L);

	// specialization
	template<>
	int Bridge< EnumDescriptorPtr, false >::on_tostring(const EnumDescriptorPtr& object, lua_State *L);

	// specialization
	template<>
	int Bridge< EnumDescriptorItemPtr, false >::on_tostring(const EnumDescriptorItemPtr& object, lua_State *L);

} }
