
#include "stdafx.h"
#include "script/LuaArguments.h"
#include "util/ProtectedString.h"

DYNAMIC_FASTFLAGVARIABLE(LuaCrashOnIncorrectTables, false)

namespace RBX { namespace Lua {

bool LuaArguments::getString(int index, std::string& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	switch (lua_type(L, index))
	{
	case LUA_TSTRING:
		{
			size_t len;
			const char* s = lua_tolstring(L, index, &len);
			value.assign(s, len);
		}
		return true;
	default:
		return false;
	}
}

bool LuaArguments::getVector3int16(int index, Vector3int16& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	return Bridge<RBX::Vector3int16>::getValue(L, index, value);
}

bool LuaArguments::getRegion3int16(int index, Region3int16& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	return Bridge<RBX::Region3int16>::getValue(L, index, value);
}

bool LuaArguments::getVector3(int index, Vector3& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	return Bridge<RBX::Vector3>::getValue(L, index, value);
}

bool LuaArguments::getRegion3(int index, Region3& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	return Bridge<RBX::Region3>::getValue(L, index, value);
}

bool LuaArguments::getRect(int index, Rect2D& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	return Bridge<RBX::Rect2D>::getValue(L, index, value);
}

bool LuaArguments::getPhysicalProperties(int index, PhysicalProperties& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	return Bridge<PhysicalProperties>::getValue(L, index, value);
}

bool LuaArguments::getObject(int index, shared_ptr<Reflection::DescribedBase>& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	switch (lua_type(L, index))
	{
	case LUA_TNIL:
		value = shared_ptr<Instance>();
		return true;

	case LUA_TUSERDATA:
		{
			if (ObjectBridge::getPtr(L, index, value))
				return true;
			else
				return false;
		}

	default:
		return false;
	}
}

bool LuaArguments::getDouble(int index, double& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	switch (lua_type(L, index))
	{
	case LUA_TNUMBER:
		value = lua_tonumber(L, index);
		return true;
	default:
		return false;
	}
}

bool LuaArguments::getBool(int index, bool& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	switch (lua_type(L, index))
	{
	case LUA_TBOOLEAN:
		value = lua_toboolean(L, index) ? true : false;
		return true;
	default:
		// TODO: Convert NUMBER to bool?
		return false;
	}
}

bool LuaArguments::getEnum(int index, const Reflection::EnumDescriptor& desc, int& value) const
{
	index += offset;

	if (index > lua_gettop(L))
		return false;

	switch (lua_type(L, index))
	{
	case LUA_TUSERDATA:
		EnumDescriptorItemPtr item;
		if (EnumItem::getItem(L, index, item))
		{
			if (item->owner != desc)
				return false;
			value = item->value;
			return true;
		}
		return false;

	case LUA_TNUMBER:
		value = lua_tonumber(L, index);
		return desc.isValue( value );

	default:
		return false;
	}
}

// Gets a value from the Lua stack. Returns false if no value is found
bool LuaArguments::get(lua_State *L, int luaIndex, Reflection::Variant& value, bool treatNilAsMissing)
{
	TablesCollection localTables(NULL);
	return getRec(L, luaIndex, value, treatNilAsMissing, &localTables);
}
bool LuaArguments::getRec(lua_State *L, int luaIndex, Reflection::Variant& value, bool treatNilAsMissing, TablesCollection* visitedTables)
{
	if (luaIndex > lua_gettop(L))
		return false;

	switch (lua_type(L, luaIndex))
	{
	case LUA_TNUMBER:	// ArgHelper has a special case for this
		value = lua_tonumber(L, luaIndex);
		return true;

	case LUA_TBOOLEAN:	// ArgHelper has a special case for this
		value = lua_toboolean(L, luaIndex) ? true : false;
		return true;

	case LUA_TSTRING:	// ArgHelper has a special case for this
		{
			size_t len;
			const char* s = lua_tolstring(L, luaIndex, &len);
			value = std::string(s, len);
		}
		return true;

	case LUA_TTABLE:
		{
			// mark table as visited
			const void* tablePtr = 0;
			if (visitedTables)
			{
				tablePtr = lua_topointer(L, luaIndex);
				if (tablePtr)
				{
					bool& visited = (*visitedTables)[tablePtr];
					if (visited)
						throw std::runtime_error("tables cannot be cyclic");
					visited = true;
				}
			}

			const int n = lua_objlen(L, luaIndex);  /* get size of table */

			if (n > 0)
			{
				// Create a new collection so that we can populate it
				shared_ptr<Reflection::ValueArray> values(rbx::make_shared<Reflection::ValueArray>(n));

				// Now populate the collection with values from the Lua *array* (recursive call to get)
				for (int i=1; i<=n; i++)
				{
					Reflection::Variant& v = (*values)[i-1];
					lua_rawgeti(L, luaIndex, i);  /* push t[i] */
					getRec(L, -1, v, false, visitedTables);
					lua_pop(L, 1);
				}

				// Assign the values object to our ValueArray
				value = shared_ptr<const RBX::Reflection::ValueArray>(values);
			}
			else
			{
				// Create a new collection so that we can populate it
				shared_ptr<Reflection::ValueTable> values;

				// Convert the stack index to a positive value:
				int tableIndex = luaIndex>0 ? luaIndex : lua_gettop(L) + luaIndex + 1;

				// Traverse the *general table*
				// See http://www.luafaq.org/#T7.1
				lua_pushnil(L);  /* first key */
				while (lua_next(L, tableIndex) != 0)
				{
					/* uses 'key' (at index -2) and 'value' (at index -1) */
					std::string key;
					switch (lua_type(L, -2))
					{
					case LUA_TSTRING:
						key = lua_tostring(L, -2);
						break;
					default:
						throw std::runtime_error("keys must be strings");
					}

					Reflection::Variant v;
					getRec(L, -1, v, false, visitedTables);

					if (!values)
						values = rbx::make_shared<Reflection::ValueTable>();

					(*values)[key] = v;

					/* removes 'value'; keeps 'key' for next iteration */
					lua_pop(L, 1);
				}			

				if (values)
					value = shared_ptr<const RBX::Reflection::ValueTable>(values);
				else
					// Assume an empty Lua table is an empty ValueArray (they can auto-convert later)
					value = rbx::make_shared<const Reflection::ValueArray>();

				// reset visited status
				if (visitedTables && tablePtr)
					(*visitedTables)[tablePtr] = false;
			}
		}
		return true;

	case LUA_TNIL:
        // See http://lua-users.org/wiki/TrailingNilParameters
        if (treatNilAsMissing)
            return false;
        value = Reflection::Variant();
		return true;

	case LUA_TFUNCTION:
		value = lua_tofunction(L, luaIndex);
		return true;

	case LUA_TTHREAD:
	case LUA_TLIGHTUSERDATA:
	default:
		// Treat as void (nil)
		value = Reflection::Variant();
		return true;

	case LUA_TUSERDATA:
		{
			// TODO: Support GenericFunctionBridge

			if (Enums::getValue(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
			if (ObjectBridge::getPtr(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
			if (CoordinateFrameBridge::getValue(L, luaIndex, value))
				return true;
			if (Region3Bridge::getValue(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
			if (Region3int16Bridge::getValue(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
			if (Vector3int16Bridge::getValue(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
			if (Vector2int16Bridge::getValue(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
			if (Vector3Bridge::getValue(L, luaIndex, value))	// ArgHelper has a special case for this
				return true;
            if (Rect2DBridge::getValue(L, luaIndex, value))
				return true;
			if (Vector2Bridge::getValue(L, luaIndex, value))
				return true;
			if (RbxRayBridge::getValue(L, luaIndex, value))
				return true;
			if (Color3Bridge::getValue(L, luaIndex, value))
				return true;
			if (BrickColorBridge::getValue(L, luaIndex, value))
				return true;
			if (UDimBridge::getValue(L, luaIndex, value))
				return true;
			if (UDim2Bridge::getValue(L, luaIndex, value))
				return true;
			if (FacesBridge::getValue(L, luaIndex, value))
				return true;
			if (AxesBridge::getValue(L, luaIndex, value))
				return true;
			if (CellIDBridge::getValue(L, luaIndex, value))
				return true;
			if (PhysicalPropertiesBridge::getValue(L, luaIndex, value))
				return true;
			

            if (NumberSequenceBridge::getValue(L, luaIndex, value))
                return true;
            if (ColorSequenceBridge::getValue(L, luaIndex, value))
                return true;
            if (NumberRangeBridge::getValue(L, luaIndex, value))
                return true;
            if (NumberSequenceKeypointBridge::getValue(L, luaIndex, value))
                return true;
            if (ColorSequenceKeypointBridge::getValue(L, luaIndex, value))
                return true;

			// Treat as void (nil)
			value = Reflection::Variant();
		}
		return true;	// TODO: Shouldn't we return false?
	}
}

class ArgumentPusher
{
	lua_State * const L;
public:
	ArgumentPusher(lua_State * const L)
		:L(L)
	{
	}

	int operator()(void)
	{
        lua_pushnil(L);
        return 1;
	}

	int operator()(bool value)
	{
		lua_pushboolean(L, value);
		return 1;
	}

	template<class T>
	int operator()(T value, typename boost::enable_if<boost::is_arithmetic<T> >::type* dummy = 0)
	{
		lua_pushnumber(L, value);
		return 1;
	}

	int operator()(const std::string& value)
	{
		lua_pushstring(L, value);
		return 1;
	}

	int operator()(const RBX::ProtectedString& value)
	{
		lua_pushstring(L, value.getSource());
		return 1;
	}

	int operator()(const shared_ptr<Instance>& value)
	{
		ObjectBridge::push(L, value);
		return 1;
	}

	int operator()(const Reflection::EnumDescriptor::Item& value)
	{
		EnumItem::push(L, &value);
		return 1;
	}

	int operator()(shared_ptr<const Reflection::Tuple> tuple)
	{
		// "unpack" the values of the array to the stack
		if (!tuple)
			return 0;	// null is equivalent to 0 items (optimization)
		int count = 0;
		Reflection::ValueArray::const_iterator iter = tuple->values.begin();
		Reflection::ValueArray::const_iterator end = tuple->values.end();
		while (iter!=end)
		{
			count += LuaArguments::push(*iter, L);
			++iter;
		}
		return count;
	}

	int operator()(const Lua::WeakFunctionRef& value)
	{
		lua_pushfunction(L, value);
		return 1;
	}

	int operator()(shared_ptr<const Reflection::ValueTable> valueMap)
	{
		if (!valueMap)
		{
			// return an empty table (never return nil)
			lua_createtable(L, 0, 0);		
			return 1;
		}

		Reflection::ValueTable::const_iterator _First = valueMap->begin();
		Reflection::ValueTable::const_iterator _Last = valueMap->end();
		lua_createtable(L, 0, valueMap->size());		
		while (_First!=_Last)
		{
			lua_pushstring(L, _First->first);
			int top = lua_gettop(L);
			LuaArguments::push(_First->second, L);
			if (DFFlag::LuaCrashOnIncorrectTables)
			{
				if (lua_gettop(L) != top + 1)
				{
					static std::string k = _First->first;
					static Reflection::Variant v = _First->second;
					RBXCRASH();
				}
			}
			lua_settable(L, -3);
			++_First;
		}
		return 1;
	}

	int operator()(shared_ptr<const Reflection::ValueArray> values)
	{
		if (!values)
		{
			// return an empty table (never return nil)
			lua_createtable(L, 0, 0);		
			return 1;
		}
		return LuaArguments::pushArray(values->begin(), values->end(), L);
	}
	int operator()(shared_ptr<const Reflection::ValueMap> valueMap)
	{
		if (!valueMap)
		{
			// return an empty table (never return nil)
			lua_createtable(L, 0, 0);		
			return 1;
		}

		Reflection::ValueMap::const_iterator _First = valueMap->begin();
		Reflection::ValueMap::const_iterator _Last = valueMap->end();
		lua_createtable(L, 0, valueMap->size());		
		while (_First!=_Last)
		{
			RBXASSERT(!_First->first.empty());
			lua_pushstring(L, _First->first);
			LuaArguments::push(_First->second, L);
			lua_settable(L, -3);
			++_First;
		}
		return 1;
	}

	int operator()(shared_ptr<const Instances> values)
	{
		if (values)
			return LuaArguments::pushArray(values->begin(), values->end(), L);
		else
		{
			// return an empty table (never return nil)
			lua_createtable(L, 0, 0);
			return 1;
		}
	}

	int operator()(shared_ptr<Lua::GenericFunction> value)
	{
		lua_pushfunction(L, value);
		return 1;
	}

	int operator()(shared_ptr<Lua::GenericAsyncFunction> value)
	{
		lua_pushfunction(L, value);
		return 1;
	}

	int operator()(const ContentId& value)
	{
		lua_pushstring(L, value.toString());
		return 1;
	}

	int operator()(const Reflection::PropertyDescriptor& value)
	{
		lua_pushstring(L,value.name.toString());
		return 1;
	}

	template<class T>
	int operator()(const T& value, typename boost::disable_if<boost::is_arithmetic<T> >::type* dummy = 0)
	{
		Bridge<T>::pushNewObject(L, value);
		return 1;
	}

};

int LuaArguments::push(const Reflection::Variant& value, lua_State * const L) 
{
	ArgumentPusher pusher(L);
	return withVariantValue<int>(value, pusher);
}

int LuaArguments::pushReturnValue(const Reflection::Variant& value, lua_State * const L)
{
    return value.isVoid() ? 0 : push(value, L);
}

shared_ptr<Reflection::Tuple> LuaArguments::convertToReturnValues(const Reflection::Variant& value)
{
    shared_ptr<Reflection::Tuple> result;

    if (value.isVoid())
    {
        result.reset(new Reflection::Tuple(0));
    }
    else
    {
        result.reset(new Reflection::Tuple(1));
        result->values[0] = value;
    }

    return result;
}

}}
