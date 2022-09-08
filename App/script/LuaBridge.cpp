#include "stdafx.h"
#include "Lua/LuaBridge.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Color3.h"
#include "G3D/Vector3.h"
#include "G3D/Vector2.h"
#include "RbxG3D/RbxRay.h"
#include "util/BrickColor.h"
#include "util/Region3.h"
#include "util/Region3int16.h"
#include "util/Faces.h"
#include "util/Axes.h"
#include "util/CellID.h"
#include "util/UDim.h"
#include "util/PhysicalProperties.h"
#include "script/ThreadRef.h"
#include "script/ScriptEvent.h"
#include "script/LuaSignalBridge.h"
#include "script/LuaEnum.h"
#include "script/LuaLibrary.h"
#include "script/LuaInstanceBridge.h"
#include "v8datamodel/NumberSequence.h"
#include "v8datamodel/ColorSequence.h"
#include "v8datamodel/NumberRange.h"

namespace RBX
{
	namespace Lua
	{
		template<class Class, bool __eq>
		int Bridge<Class, __eq>::on_tostring(const Class& object, lua_State *L)
		{
			std::string s = RBX::StringConverter<Class>::convertToString(object);
			lua_pushstring(L, s);
			return 1;
		}

		
		/// This is to force the above implementation to be generated for following class Types

		/// If you are doing a specialization for the on_tostring() then you do *not* need to add the following line. 
		/// Basically the specialization is needed if your class does not have the RBX::StringConverter<YourClass>::convertToString() implementation
		/// To see an e.g for specialization, just look up LuaEnum.h LuaEnum.cpp, search for on_tostring()
		
		/// We need the forward declaration here because we want the above default on_tostring() implementation to be generated for the following class types
		/// If we do *not* add the following following declaration then the Microsoft C++ compiler complains that it cannot find the implementation of on_string for this classes
		/// On GCC I have *not* seen a need for this forward declaration.
		/// If we keep the above on_string implementation in header then the forward declaration is *not* needed.
		/// But then on GCC we get duplicate implementation for the specialized classes, one with default & one with specialization.
		
		/// For more details refer to http://www.parashift.com/c++-faq-lite/templates.html#faq-35.13
		/// Also this case has been documented in the Porting Error Cases Document found at Roblox Documents
		/// https://docs0.google.com/a/roblox.com/document/d/100HFz6lRHJKP88UOshPrrnRoHavNZ5upT6ePIkgcYFc/edit?hl=en#

		template int Bridge <G3D::Color3>::on_tostring(const  G3D::Color3&, lua_State *);
		template int Bridge <RBX::RbxRay>::on_tostring(const  RBX::RbxRay&, lua_State *);
		template int Bridge <G3D::Vector3int16>::on_tostring(const  G3D::Vector3int16&, lua_State *);	
		template int Bridge <G3D::Vector2int16>::on_tostring(const  G3D::Vector2int16&, lua_State *);	
		template int Bridge <G3D::Vector3>::on_tostring(const  G3D::Vector3&, lua_State *);		
		template int Bridge <RBX::Vector2>::on_tostring(const  RBX::Vector2&, lua_State *);
        template int Bridge <RBX::Rect2D>::on_tostring(const  RBX::Rect2D&, lua_State *);
		template int Bridge <PhysicalProperties>::on_tostring(const RBX::PhysicalProperties&, lua_State *);
		template int Bridge <RBX::BrickColor>::on_tostring(const  RBX::BrickColor&, lua_State *);
		template int Bridge <G3D::CoordinateFrame>::on_tostring(const  G3D::CoordinateFrame&, lua_State *);
		template int Bridge <RBX::Faces>::on_tostring(const  RBX::Faces&, lua_State *);
		template int Bridge <RBX::Axes>::on_tostring(const  RBX::Axes&, lua_State *);
		template int Bridge <RBX::CellID>::on_tostring(const  RBX::CellID&, lua_State *);
		template int Bridge <RBX::UDim>::on_tostring(const RBX::UDim&, lua_State *);
		template int Bridge <RBX::UDim2>::on_tostring(const RBX::UDim2&, lua_State *);

		template<class Class, bool __eq>
		void Bridge<Class, __eq>::registerClass (lua_State *L)
		{
			// Register the object events
			luaL_newmetatable(L, className);
			Lua::protect_metatable(L, -1);

			lua_pushstring(L, "__type");
			lua_pushstring(L, className);
			lua_settable(L, -3);

			lua_pushstring(L, "__index");
			lua_pushcfunction(L, on_index);
			lua_settable(L, -3);

			lua_pushstring(L, "__newindex");
			lua_pushcfunction(L, on_newindex);
			lua_settable(L, -3);

			lua_pushstring(L, "__gc");
			lua_pushcfunction(L, on_gc);
			lua_settable(L, -3);

			if (__eq)
			{
				lua_pushstring(L, "__eq");
				lua_pushcfunction(L, on_eq);
				lua_settable(L, -3);
			}

			lua_pushstring(L, "__tostring");
			lua_pushcfunction(L, on_tostring);
			lua_settable(L, -3);

			lua_setreadonly(L, -1, true);
			lua_pop(L, 1);
		}

		/// This is to force the above implementation to be generated for following class Types

		/// If you are doing a specialization for the registerClass() then you do *not* need to add the following line. 
		
		/// We need the forward declaration here because we want the above default registerClass() implementation to be generated for the following class types
		/// If we do *not* add the following following declaration then the Microsoft C++ compiler complains that it cannot find the implementation of registerClass for this classes
		/// On GCC I have *not* seen a need for this forward declaration.
		/// If we keep the above registerClass implementation in header then the forward declaration is *not* needed.
		/// But then on GCC we get duplicate implementation for the specialized classes, one with default & one with specialization.
		
		/// For more details refer to http://www.parashift.com/c++-faq-lite/templates.html#faq-35.13
		/// Also this case has been documented in the Porting Error Cases Document found at Roblox Documents
		/// https://docs0.google.com/a/roblox.com/document/d/100HFz6lRHJKP88UOshPrrnRoHavNZ5upT6ePIkgcYFc/edit?hl=en#

        template void Bridge <RBX::Axes>::registerClass(lua_State *);
        template void Bridge <RBX::CellID>::registerClass(lua_State *);
        template void Bridge <RBX::Faces>::registerClass(lua_State *);
        template void Bridge <RBX::BrickColor>::registerClass(lua_State *);
        template void Bridge <RBX::RbxRay>::registerClass(lua_State *);
        template void Bridge <RBX::Region3>::registerClass(lua_State *);
        template void Bridge <RBX::Region3int16>::registerClass(lua_State *);
        template void Bridge <G3D::Color3>::registerClass(lua_State *);
        template void Bridge <boost::intrusive_ptr<class WeakThreadRef::Node> >::registerClass(lua_State *);
        template void Bridge <shared_ptr<GenericFunction> >::registerClass(lua_State *);
        template void Bridge <shared_ptr<GenericAsyncFunction> >::registerClass(lua_State *);
        template void Bridge <shared_ptr<class RBX::Instance>, false >::registerClass(lua_State *);
        template void Bridge <rbx::signals::connection>::registerClass(lua_State *);
        template void Bridge <Library>::registerClass(lua_State *);
        template void Bridge <EventInstance>::registerClass(lua_State *);
        template void Bridge <AllEnumDescriptorsPtr, false>::registerClass(lua_State *);
        template void Bridge <EnumDescriptorPtr, false>::registerClass(lua_State *);
        template void Bridge <EnumDescriptorItemPtr, false>::registerClass(lua_State *);
        template void Bridge <RBX::NumberSequenceKeypoint>::registerClass(lua_State *);
        template void Bridge <RBX::ColorSequenceKeypoint>::registerClass(lua_State *);
        template void Bridge <RBX::NumberSequence>::registerClass(lua_State *);
        template void Bridge <RBX::ColorSequence>::registerClass(lua_State *);
        template void Bridge <RBX::NumberRange>::registerClass(lua_State *);
	}
}
