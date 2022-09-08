#include "stdafx.h"

#include "Script/LuaLibrary.h"

#include "Script/Script.h"
#include "Script/ThreadRef.h"
#include "Script/ScriptContext.h"
#include "Script/LuaSettings.h"
#include "Util/StandardOut.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/ContentProvider.h"

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

namespace RBX { 
	
template<>
std::string StringConverter<Lua::Library>::convertToString(const Lua::Library& value)
{
	return value.getLibraryName();
}

namespace Lua {

	
static void registerLibraryTable(lua_State *L)
{
	lua_pushlightuserdata(L, (void*)&registerLibraryTable);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
}

template<>
const char* Bridge<Library>::className("RbxLibrary");

static int getApi(lua_State *L)
{
	std::list<std::string> api;
	{
		RBXASSERT_BALLANCED_LUA_STACK(L);
		const Library& self = Bridge<Library>::getObject(L, 1);
		//First get the "table" storing the real library's "Table"
		lua_pushlightuserdata(L, (void*)&registerLibraryTable); /* Registry mapping for table. Key is arbitrary. */
		lua_rawget(L, LUA_REGISTRYINDEX);                       // Stack:   t
		RBXASSERT(!lua_isnil( L, -1 ));							// did you forget to call registerClassLibrary

		//Now look up the library associated with our object
		lua_pushstring(L, self.getLibraryName());				// Stack: libraryName, t,
		lua_gettable(L, -2);									// Stack: t[libraryName], t
		RBXASSERT(!lua_isnil( L, -1 ));							// A library was pushed without being created
		RBXASSERT(lua_istable( L, -1 ));						// A library was pushed without being created

		int tableIndex = lua_gettop(L);
		/* table is in the stack at index 'tableIndex' */
		lua_pushnil(L);  /* first key */						// Stack: nil, t[libraryName], t
		while (lua_next(L, tableIndex) != 0) {
			/* uses 'key' (at index -2) and 'value' (at index -1) */	// Stack: value, key, t[libraryName], t
			if(lua_isstring(L, -2)){
				std::string key = safe_lua_tostring(L, -2);
				if(!key.empty()){
					api.push_back(key);
				}
			}
			/* removes 'value'; keeps 'key' for next iteration */
			lua_pop(L, 1);												// Stack: key, t[libraryName], t
		}
		// Stack: t[libraryName], t
		lua_pop(L, 2);
	}

	lua_createtable(L, api.size(), 0);										// Stack: t
	int i = 1;
	for(std::list<std::string>::iterator iter = api.begin(); iter != api.end(); ++iter, ++i)
	{
		lua_pushnumber(L, i);											// Stack: key, t
		lua_pushstring(L, *iter);										// Stack: value, key, t
		lua_settable(L, -3);											// Stack: t
	}
	return 1;
}

template<>
int Bridge<Library>::on_index(const Library& object, const char* name, lua_State *L)
{
	if(strcmp(name,"GetApi") == 0){
		lua_pushcfunction(L, getApi);
		return 1;
	}
	if(strcmp(name,"Name") == 0){
		lua_pushstring(L, object.getLibraryName());
		return 1;
	}

	//First get the "table" storing the real library's "Table"
	lua_pushlightuserdata(L, (void*)&registerLibraryTable); /* Registry mapping for table. Key is arbitrary. */
	lua_rawget(L, LUA_REGISTRYINDEX);                       // Stack:   t
	RBXASSERT(!lua_isnil( L, -1 ));							// did you forget to call registerClassLibrary

	//Now look up the library associated with our object
	lua_pushstring(L, object.getLibraryName());				// Stack: libraryName, t,
	lua_gettable(L, -2);									// Stack: t[libraryName], t
	RBXASSERT(!lua_isnil( L, -1 ));							// A library was pushed without being created
	RBXASSERT(lua_istable( L, -1 ));						// A library was pushed without being created

	//Now find the value they are trying to index
	lua_pushstring(L, name);								// Stack: name, t[libraryName], t
	lua_gettable(L, -2);									// Stack: t[libraryName][name], t[libraryName], t
	
	lua_remove(L, -2);										// Stack: t[libraryName][name], t
	lua_remove(L, -2);										// Stack: t[libraryName][name]

	return 1;
}

template<>
void Bridge<Library>::on_newindex(Library& object, const char* name, lua_State *L)
{
	// Failure
	throw RBX::runtime_error("%s cannot be assigned to", name);
}

void LibraryBridge::saveLibraryResult(lua_State *L, int results, std::string libraryName)
{
	std::string exception = "";
	{
		RBXASSERT_BALLANCED_LUA_STACK(L);

		if(results != 1) {
			exception = "Libraries should return exactly 1 result, and shouldn't wait";
		}
		else if(!lua_istable(L, -1)) {
			exception = "Libraries should return exactly 1 table";
		}
		else{
			//Stack: LibraryTableResult
			lua_pushstring(L, "Help");										//Stack: "Help", LibraryTableResult
			lua_rawget(L, -2);												//Stack: LibraryTableResult.Help, LibraryTableResult
			if(!lua_isfunction(L, -1)){											
				exception = "Libraries must contain a \"Help\" function";
			}
			lua_pop(L, 1);													//Stack: LibraryTableResult
		}
	
		//Grab the table where we store the library results
		lua_pushlightuserdata(L, (void*)&registerLibraryTable);				/* Registry mapping for table. Key is arbitrary. */
		lua_rawget(L, LUA_REGISTRYINDEX);									// Stack:   t
		RBXASSERT(!lua_isnil( L, -1 ));										// did you forget to call registerClassLibrary

		if(!exception.empty()) {
			lua_pushstring(L, libraryName);									//Stack: libName, t
			lua_pushstring(L, exception);									//Stack: exception, libName, t
			lua_rawset(L, -3);												//Stack: t
		}
		else {
			//No exception means exactly one argument, and its a proper table	//Stack: t, LibraryTableResult
			lua_pushstring(L, libraryName.c_str());								//Stack: libName, t, LibraryTableResult
			lua_pushvalue(L, -3);												//Stack: LibraryTableResult, libName, t, LibraryTableResult
			lua_rawset(L, -3);													//Stack: t, LibraryTableResult
		}
		lua_pop(L, 1);															//Stack: results?
	}
	{
		RBXASSERT_BALLANCED_LUA_STACK(L);
		
		if(exception.empty()){
			push(L, Library(libraryName));
			lua_pop(L, 1);
		}
		else{
			lua_pushlightuserdata(L, (void*)&push ); /* Registry mapping for table. Key is arbitrary. */
			lua_rawget(L, LUA_REGISTRYINDEX);								// Stack:   t
			RBXASSERT(!lua_isnil( L, -1 ));									// Did you forget to call registerClassLibrary??

			lua_pushstring(L, libraryName);									//Stack: libName, t
			lua_pushstring(L, exception);									//Stack: msg, libName, t
			lua_rawset(L, -3);												//Stack: t
			
			lua_pop(L, 1);
		}
	}
}

int LibraryBridge::find(lua_State *L, const std::string& libraryName)
{
#ifdef _DEBUG
	int i = lua_gettop(L);
#endif
	// Matt Campbell at Serotek Corporation had a great idea. Create only
	// a single instance of the userdata and re-use it. This way the userdata
	// can be used in table keys
	// Also see http://lua-users.org/lists/lua-l/2004-07/msg00391.html
	lua_pushlightuserdata(L, (void*)&push ); /* Registry mapping for table. Key is arbitrary. */

	lua_rawget(L, LUA_REGISTRYINDEX);                             // Stack:   t
	RBXASSERT(!lua_isnil( L, -1 ));  // Did you forget to call registerClassLibrary??

	// Now the top of the stack is our lookup table
	// See if we already have a UserData for this instance
	lua_pushstring(L, libraryName);							      // Stack:   name, t 
	lua_rawget (L, -2);                                           // Stack:   t[name], t, 

	// TODO: only allow explicit, one-time declaration
	if(lua_isnil( L, -1 )) {                                      // Stack:   nil or I, t
		lua_pop (L, 2);                                         // Stack:   
#ifdef _DEBUG
		RBXASSERT(lua_gettop(L) == i);
#endif
		return 0;
	}
	else if(lua_isstring(L, -1)) {
		lua_pushnil(L);											  // Stack:   nil, errormsg, t
		lua_insert(L, lua_gettop(L) - 1);						  // Stack:   errormsg, nil, t
		lua_remove (L, -3);                                       // Stack:   errormsg, nil
#ifdef _DEBUG
		RBXASSERT(lua_gettop(L) == i + 2);
#endif
		return 2;
	}
	else {
		lua_remove (L, -2);                                       // Stack:   Lib
#ifdef _DEBUG
		RBXASSERT(lua_gettop(L) == i + 1);
#endif
		return 1;
	}
}

void LibraryBridge::push(lua_State *L, const Library& item)
{
#ifdef _DEBUG
	int i = lua_gettop(L);
#endif
	// Matt Campbell at Serotek Corporation had a great idea. Create only
	// a single instance of the userdata and re-use it. This way the userdata
	// can be used in table keys
	// Also see http://lua-users.org/lists/lua-l/2004-07/msg00391.html

	lua_pushlightuserdata(L, (void*)&push ); /* Registry mapping for table. Key is arbitrary. */
	lua_rawget(L, LUA_REGISTRYINDEX);                             // Stack:   t
	RBXASSERT(!lua_isnil( L, -1 ));  // Did you forget to call registerClassLibrary??

	// Now the top of the stack is our lookup table
	// See if we already have a UserData for this instance
	lua_pushstring(L, item.getLibraryName());                     // Stack:   t, name
	lua_rawget (L, -2);                                           // Stack:   t, t[i]

	// TODO: only allow explicit, one-time declaration
	if( lua_isnil( L, -1 ) ) {                                    // Stack:   t, nil or I
		lua_pop (L, 1);                                           // Stack:   t
		pushNewObject(L, item);							          // Stack:   t, I
		/* store the value for later use */
		lua_pushstring (L, item.getLibraryName());               // Stack:   t, I, name
		lua_pushvalue (L, -2);                                    // Stack:   t, I, name, I
		lua_rawset (L, -4);                                       // Stack:   t, I
	}
	lua_remove (L, -2);                                           // Stack:   I

#ifdef _DEBUG
	RBXASSERT(lua_gettop(L) == i + 1);
#endif
}

void LibraryBridge::registerClassLibrary (lua_State *L) 
{
	// Declare the UserData re-use table
	lua_pushlightuserdata(L, (void*)&push);
	lua_newtable(L);	// Not a weak table, unlike SharedPtrBridge
	lua_rawset(L, LUA_REGISTRYINDEX);

	registerLibraryTable(L);
}

}
}
