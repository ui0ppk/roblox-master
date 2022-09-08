#pragma once

#include "boost/function.hpp"
#include "reflection/Type.h"

struct lua_State;

namespace RBX
{
	class BaseScript;

	namespace Scripts
	{
		typedef boost::function<void(shared_ptr<const Reflection::Tuple> results)> SuccessHandler;
		typedef boost::function<void(const char* message, const char* callStack, shared_ptr<BaseScript> source, int line)> ErrorHandler;
		struct Continuations
		{
			SuccessHandler successHandler;
			ErrorHandler errorHandler;
			bool empty() const
			{
				return successHandler.empty() && errorHandler.empty();
			}
		};
	}

	namespace Lua
	{
		class Continuations
		{
		public:
			Continuations(const Scripts::Continuations& eh);
			Continuations() {}
			boost::function<void(lua_State*)> success;	    // called when the thread exits via ScriptContext::resume
			boost::function<void(lua_State*)> error;		// called when the thread errors via ScriptContext::resume
		private:
			static void onSuccessHandler(lua_State* thread, Scripts::SuccessHandler handler);
			static void onErrorHandler(lua_State* thread, Scripts::ErrorHandler handler);
		};
	}
}