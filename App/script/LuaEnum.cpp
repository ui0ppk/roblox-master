#include "stdafx.h"

#include "Script/LuaEnum.h"
#include "Reflection/EnumConverter.h"


using namespace RBX;
using namespace RBX::Lua;

template<>
const char* Bridge< AllEnumDescriptorsPtr, false >::className("Enums");
template<>
const char* Bridge< EnumDescriptorPtr, false >::className("Enum");
template<>
const char* Bridge< EnumDescriptorItemPtr, false >::className("EnumItem");

namespace RBX { namespace Lua {

//Convert Enum.Foo to a EnumDescriptor<
template<>
int Bridge<AllEnumDescriptorsPtr, false>::on_index(const AllEnumDescriptorsPtr& object, const char* name, lua_State *L)
{
	const Reflection::EnumDescriptor* desc = Reflection::EnumDescriptor::lookupDescriptor(RBX::Name::lookup(name));
	if (desc)
	{
		Enum::push(L, desc);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid EnumItem", name);
}

template<>
void Bridge<AllEnumDescriptorsPtr, false>::on_newindex(AllEnumDescriptorsPtr& object, const char* name, lua_State *L)
{
	throw RBX::runtime_error("Enums cannot be modified");
}

static int pushEnumList(lua_State *L)
{
	const EnumDescriptorPtr& object	= Bridge<EnumDescriptorPtr, false>::getObject(L, 1);

#ifdef RBXASSERTENABLED
	int i = lua_gettop(L);
#endif
	lua_createtable(L, object->getEnumCount(), 0);				//Stack: t
	int pos = 1;
	for(std::vector<EnumDescriptorItemPtr>::const_iterator iter = object->begin(); 
		iter != object->end(); ++iter){

		lua_pushnumber(L, pos++);								//Stack: index, t
		Lua::EnumItem::push(L, *iter);							//Stack: value, index, t
		lua_rawset(L, -3);										//Stack: t
	}
	RBXASSERT(lua_gettop(L) == i + 1);
	return 1;
}
template<>
int Bridge<EnumDescriptorPtr, false>::on_index(const EnumDescriptorPtr& object, const char* name, lua_State *L)
{
	if(strcmp(name,"GetEnumItems") == 0){
		lua_pushcfunction(L, pushEnumList);
		return 1;
	}
	const Reflection::EnumDescriptor::Item* item = object->lookup(name);
	if (item!=NULL)
	{
		EnumItem::push(L, item);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid EnumItem", name);
}

template<>
void Bridge<EnumDescriptorPtr, false>::on_newindex(EnumDescriptorPtr& object, const char* name, lua_State *L)
{
	throw RBX::runtime_error("Enum cannot be modified");
}

template<>
int Bridge<EnumDescriptorItemPtr, false>::on_index(const EnumDescriptorItemPtr& object, const char* name, lua_State *L)
{
	if (strcmp(name, "Name")==0)
	{
		lua_pushstring(L, object->name.toString());
		return 1;
	}

	if (strcmp(name, "Value")==0)
	{
		lua_pushnumber(L, object->value);
		return 1;
	}

	// Failure
	throw RBX::runtime_error("%s is not a valid member", name);
}

template<>
void Bridge<EnumDescriptorItemPtr, false>::on_newindex(EnumDescriptorItemPtr& object, const char* name, lua_State *L)
{
	throw RBX::runtime_error("EnumItem cannot be modified");
}

}} //namespace

static AllEnumDescriptors dummy;

void Enums::declareAllEnums(lua_State *L)
{
	Enums::push(L, &dummy);
	lua_setglobal(L, "Enum");
}

bool Enums::getValue(lua_State *L, unsigned int index, RBX::Reflection::Variant& value)
{
	EnumDescriptorItemPtr item;
	if (EnumItem::getItem(L, index, item)){
		return item->convertToValue(value);
	}
	return false;
}

namespace RBX
{
	namespace Lua
	{
		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.		
		template<>
		int Bridge< AllEnumDescriptorsPtr, false >::on_tostring(const AllEnumDescriptorsPtr& object, lua_State *L)
		{
			lua_pushstring(L, "Enums");
			return 1;
		}

		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge< EnumDescriptorPtr, false >::on_tostring(const EnumDescriptorPtr& object, lua_State *L)
		{
			lua_pushstring(L, object->name.c_str());
			return 1;
		}

		// The default implementation for on_tostring is available in LuaBridge.cpp. This is a specialization.
		template<>
		int Bridge< EnumDescriptorItemPtr, false >::on_tostring(const EnumDescriptorItemPtr& object, lua_State *L)
		{
			std::string s = RBX::format("Enum.%s.%s", object->owner.name.c_str(), object->name.c_str());
			lua_pushstring(L,  s.c_str());
			return 1;
		}
	}
}
