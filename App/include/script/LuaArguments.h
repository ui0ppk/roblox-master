#pragma once

#include "reflection/Function.h"
#include "script/LuaAtomicClasses.h"
#include "script/LuaEnum.h"
#include "script/ThreadRef.h"
#include "script/LuaInstanceBridge.h"
#include "rbx/make_shared.h"
#include "rbx/DenseHash.h"
#include "util/ProtectedString.h"
#include "util/PhysicalProperties.h"

namespace RBX {

	// Utility function that expands a variant to a strongly-typed value
	template<typename R, typename F>
	R withVariantValue(const Reflection::Variant& value, F f)
	{
		if (value.isType<void>())
			return f();

		if (value.isType<bool>())
			return f(value.cast<bool>());

		if (value.isType<int>())
			return f(value.cast<int>());

		if (value.isType<long>())
			return f(value.cast<long>());

		if (value.isType<float>())
			return f(value.cast<float>());

		if (value.isType<double>())
			return f(value.cast<double>());

		if (value.isType<std::string>())
			return f(value.cast<std::string>());

		if (value.isType<RBX::ProtectedString>())
			return f(value.cast<RBX::ProtectedString>());

		if (value.isType< shared_ptr<Instance> >())
			return f(value.cast<shared_ptr<Instance> >());

		if (const Reflection::EnumDescriptor* desc = Reflection::EnumDescriptor::lookupDescriptor(value.type()))
		{
			const Reflection::EnumDescriptor::Item* item = desc->lookup(value);
			if (item == NULL)
				throw RBX::runtime_error("Invalid value for enum %s", desc->name.c_str());
			return f(*item);
		}

		if (value.isType<Lua::WeakFunctionRef>())
			return f(value.cast<Lua::WeakFunctionRef>());

		if (value.isType<shared_ptr<const Reflection::ValueArray> >())
			return f(value.cast<shared_ptr<const Reflection::ValueArray> >());

		if (value.isType<shared_ptr<const Reflection::ValueMap> >())
			return f(value.cast<shared_ptr<const Reflection::ValueMap> >());

		if (value.isType<shared_ptr<const Reflection::ValueTable> >())
			return f(value.cast<shared_ptr<const Reflection::ValueTable> >());

		if (value.isType<shared_ptr<const Instances> >())
			return f(value.cast<shared_ptr<const Instances> >());

		if (value.isType<shared_ptr<const Reflection::Tuple> >())
			return f(value.cast<shared_ptr<const Reflection::Tuple> >());

		if (value.isType< shared_ptr<Lua::GenericFunction> >())
			return f(value.cast< shared_ptr<Lua::GenericFunction> >());

		if (value.isType< shared_ptr<Lua::GenericAsyncFunction> >())
			return f(value.cast< shared_ptr<Lua::GenericAsyncFunction> >());

		if (value.isType<G3D::Vector3int16>())
			return f(value.cast<G3D::Vector3int16>());
		if (value.isType<G3D::Vector2int16>())
			return f(value.cast<G3D::Vector2int16>());
		if (value.isType<G3D::Vector3>())
			return f(value.cast<G3D::Vector3>());
		if (value.isType<RBX::Vector2>())
			return f(value.cast<G3D::Vector2>());
        if (value.isType<G3D::Rect2D>())
			return f(value.cast<G3D::Rect2D>());
		if (value.isType<PhysicalProperties>())
			return f(value.cast<PhysicalProperties>());
		if (value.isType<RBX::RbxRay>())
			return f(value.cast<RBX::RbxRay>());
		if (value.isType<G3D::CoordinateFrame>())
			return f(value.cast<G3D::CoordinateFrame>());
		if (value.isType<G3D::Color3>())
			return f(value.cast<G3D::Color3>());
		if (value.isType<BrickColor>())
			return f(value.cast<BrickColor>());
		if (value.isType<RBX::Region3>())
			return f(value.cast<RBX::Region3>());
		if( value.isType<RBX::Region3int16>())
			return f(value.cast<RBX::Region3int16>());
		if (value.isType<UDim>())
			return f(value.cast<UDim>());
		if (value.isType<UDim2>())
			return f(value.cast<UDim2>());
		if (value.isType<Faces>())
			return f(value.cast<Faces>());
		if (value.isType<Axes>())
			return f(value.cast<Axes>());
		if (value.isType<CellID>())
			return f(value.cast<CellID>());
		if (value.isType<ContentId>())
			return f(value.cast<ContentId>());

		if (value.isType<const Reflection::PropertyDescriptor*>())
			return f(*value.cast<const Reflection::PropertyDescriptor*>());

		if (value.isType<rbx::signals::connection>())
			return f(value.cast<rbx::signals::connection>());

        if (value.isType<NumberSequence>())
            return f(value.cast<NumberSequence>());
        if (value.isType<ColorSequence>())
            return f(value.cast<ColorSequence>());
        if (value.isType<NumberRange>())
            return f(value.cast<NumberRange>());
        if (value.isType<NumberSequenceKeypoint>())
            return f(value.cast<NumberSequenceKeypoint>());
        if (value.isType<ColorSequenceKeypoint>())
            return f(value.cast<ColorSequenceKeypoint>());

		RBXASSERT(0);
		return f();
	}





	namespace Lua {

	class LuaArguments : public Reflection::FunctionDescriptor::Arguments
	{
		typedef DenseHashMap<const void*, bool> TablesCollection;
		static bool getRec(lua_State *L, int luaIndex, Reflection::Variant& value, bool treatNilAsMissing, TablesCollection* visitedTables = NULL);

		const int offset;
		lua_State * const L;
	public:
		LuaArguments(lua_State *L, int offset):L(L),offset(offset)	{}
		
		virtual size_t size() const {
			return lua_gettop(L) - 1;
		}

		// Gets all arguments from the stack and puts them into the ValueArray
		static shared_ptr<Reflection::Tuple> getValues(lua_State* L)
		{
			int argCount = lua_gettop(L);

			shared_ptr<Reflection::Tuple> args(rbx::make_shared<Reflection::Tuple>(argCount));

			for (int i = 0; i<argCount; ++i)
			{
				Reflection::Variant& v = args->values.at(i);
				bool success = RBX::Lua::LuaArguments::get(L, i+1, v, false);
				RBXASSERT(success);
			}

			return args;
		}

		static int pushTuple(const Reflection::Tuple& arguments, lua_State* L)
		{
			return pushValues(arguments.values, L);
		}

		// pushes all the values from the ValueArray onto the stack
		static int pushValues(const Reflection::ValueArray& arguments, lua_State* L)
		{
			int argCount = 0;
			Reflection::ValueArray::const_iterator end = arguments.end(); 
			for (Reflection::ValueArray::const_iterator iter = arguments.begin(); iter != end; ++iter)
			{
				argCount += push(*iter, L);
			}
			return argCount;
		}

		//////////////////////////////////////
		// Implemenent virtual functions
		//
		// Place the value for the requested parameter in "value".
		// 
		// index:       1-based index into the argument list
		// value:       the value to set. If index >= size(), then value is unchanged
		/*implement*/ bool getVariant(int index, Reflection::Variant& value) const {
			const int luaIndex = index + offset;
			RBXASSERT(luaIndex>0);
			return get(L, luaIndex, value, true);
		}
		/*implement*/ bool getLong(int index, long& value) const
		{
			// All numbers in Lua are double, so just call the double version of get
			double v;
			if (getDouble(index, v))
			{
				value = G3D::iRound(v);
				return true;
			}
			return false;
		}
		/*implement*/ bool getDouble(int index, double& value) const;
		/*implement*/ bool getObject(int index, shared_ptr<Reflection::DescribedBase>& value) const;
		/*implement*/ bool getBool(int index, bool& value) const;
		/*implement*/ bool getString(int index, std::string& value) const;
		/*implement*/ bool getVector3(int index, Vector3& value) const;
		/*implement*/ bool getRegion3(int index, Region3& value) const;
		/*implement*/ bool getVector3int16(int index, Vector3int16& value) const;
		/*implement*/ bool getRegion3int16(int index, Region3int16& value) const;
		/*implement*/ bool getRect(int index, Rect2D& value) const;
		/*implement*/ bool getPhysicalProperties(int index, PhysicalProperties& value) const;
		/*implement*/ bool getEnum(int index, const Reflection::EnumDescriptor& desc, int& value) const;
		//
		//////////////////////////////////////

		// Gets a value from the Lua stack. Returns false if no value is found
		static bool get(lua_State *L, int luaIndex, Reflection::Variant& value, bool treatNilAsMissing);
		template<class _InIt>
		static int pushArray(_InIt _First, _InIt _Last, lua_State * const L) {
			lua_createtable(L, _Last - _First, 0);
			unsigned int i = 0;
			while (_First!=_Last)
			{
				int count = push(*_First, L);
				RBXASSERT(count == 1);	// If not 1, then what do we do?
				lua_rawseti(L, -2, ++i);
				++_First;
			}
			return 1;
		}

		static int push(const Reflection::Variant& value, lua_State * const L);

		static int pushReturnValue(const Reflection::Variant& value, lua_State * const L);
		static shared_ptr<Reflection::Tuple> convertToReturnValues(const Reflection::Variant& value);
	};

}}
