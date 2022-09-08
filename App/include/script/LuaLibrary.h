#pragma once
#include "Lua/LuaBridge.h"

namespace RBX { 
namespace Lua {

	class Library
	{
		std::string libraryName;
	public:
		Library(std::string libraryName)
			:libraryName(libraryName)
		{};

		const std::string& getLibraryName() const { return libraryName; }
		bool operator ==(const Library& other) const
		{
			return this->libraryName == other.libraryName; 
		}		
	};
	// Represents a Reflection::EnumDescriptor::Item in Lua
	class LibraryBridge : public Bridge<Library>
	{
	public:
		static void registerClassLibrary (lua_State *L);
		static int find(lua_State *L, const std::string& libraryName);
		static void push(lua_State *L, const Library& item);
		static void saveLibraryResult(lua_State *L, int results, std::string libraryName);
	};
}
}
