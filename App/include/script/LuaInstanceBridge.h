
#pragma once
#include "Lua/LuaBridge.h"
#include "V8Tree/Instance.h"

namespace RBX { namespace Lua {

	// specialization
	template<>
	int Bridge< shared_ptr<Instance>, false >::on_tostring(const shared_ptr<Instance>& object, lua_State *L);

	class ObjectBridge : public SharedPtrBridge<Instance>
	{
		friend class SharedPtrBridge<Instance>;
		static const luaL_reg classLibrary[];
	public:
		static int callMemberFunction(lua_State *L);
		static int callMemberYieldFunction(lua_State *L);

		static void registerInstanceClassLibrary (lua_State *L) {

			// Register the "new" function for Instances
			luaL_register(L, "Instance", classLibrary); 
			lua_setreadonly(L, -1, true);
			lua_pop(L,1);		// Pop table from stack.   http://lua-users.org/lists/lua-l/2003-12/msg00139.html
		}

		static int newInstance(lua_State *L);
		static int lockInstance(lua_State *L);
		static int unlockInstance(lua_State *L);

		static boost::shared_ptr<Instance> getInstance(lua_State *L, unsigned int index)
		{
			return getPtr(L, index);
		}
	};

	template<>
	void Bridge< shared_ptr<Instance>, false >::on_newindex(shared_ptr<Instance>& object, const char* name, lua_State *L);

} }
