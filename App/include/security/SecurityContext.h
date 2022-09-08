
#pragma once
#include "rbxformat.h"
#include "rbx/boost.hpp"
#include "g3d/format.h"

namespace boost
{
	template<typename T> class thread_specific_ptr;
}

namespace RBX
{
	namespace Security
	{
		typedef enum { 
			Anonymous = 0, 
			LocalGUI_,					// Any action initiated by Roblox Studio or the mouse 
			GameScript_,				// Execution of a BaseScript object inside any DataModel
			GameScriptInRobloxPlace_,	// Execution of a BaseScript object inside any DataModel, if the place was authored by Roblox
			RobloxGameScript_,			// Execution of a BaseScript object written by Roblox inside any DataModel
			CmdLine_,					// Any script executed from the Studio command line
#if defined(RBX_STUDIO_BUILD)
			StudioPlugin,				// Any Studio plug-in script
#endif
			COM,						// Scripts executed via the COM API (usually comes from roblox.com)
			WebService,					// Scripts executed via the Web Service API (usually comes from roblox.com)
			Replicator_,				// Receiving data via replication
			COUNT_Identities            // Not a true identity. Used for enumeration
		} Identities;

		typedef enum { 
			None				=0,     // Any identity can access this feature, including in-game scripts
            Plugin              =1,     // Second-lowest access level, just above in-game script
			RobloxPlace			=2,     // A Roblox place that we own. Therefore scripts are more trusted and we allow
										// preliminary features
			LocalUser			=3,     // non-game permission. Usually for IDE
			WritePlayer			=4,		// Permissions for changing player name, userId, etc.
			RobloxScript		=5,     // A script, such as a CoreScript, that we run inside a game
			Roblox				=6,     // Highest level of permission

#ifdef RBX_TEST_BUILD
			TestLocalUser       =None,  //For exposing Lua functions to the ReleaseTest build
#else
			TestLocalUser       =LocalUser,
#endif
		} Permissions;
        
        // different classes of VM that derive from the permission level
        typedef enum {
            VM_Default = 0,             // most scripts go here
#if defined(RBX_STUDIO_BUILD)
			VM_StudioPlugin,            // Sandbox for studio plugin scripts
#endif
            VM_RobloxScriptPlus,            // scripts with the permission level of RobloxScript or higher go here
            COUNT_VM_Classes
        } VMClasses;

		class Impersonator;

		class Context
		{
			friend class Impersonator;
					
		public:
			const Identities identity;
			static Context& current();

			// Throws an exception if the current thread's Context doesn't have the requested Role
			void requirePermission(Permissions permission, const char* operation = 0) const
			{
				if (!isInRole(identity, permission)) {
#ifndef _DEBUG
					// obfuscate error string
					// TODO: Can we obfuscate the code without obfuscating the error? 
					// Daniel: NO
					//if (operation)
					//	throw RBX::runtime_error("s %s", operation);
					//else
					//	throw RBX::runtime_error("s");
					throw std::runtime_error("");
#else
                    if (operation) {
						throw RBX::runtime_error("The current identity (%d) cannot %s (requires %d)", identity, operation, permission);
					} else {
						throw RBX::runtime_error("The current identity (%d) cannot perform the requested operation (requires %d)", identity, permission);
                    }
#endif
                }
			}

			bool hasPermission(Permissions permission)
			{
				return isInRole(identity, permission);
			}

			static bool isInRole(Identities identity, Permissions permission);

			static void tssCleanup(Context*);

		private:
			Context(Identities identity):identity(identity) {}
			static boost::thread_specific_ptr<Context>& ptr();
		};

		// Impersonates an identity for the lifetime of the object
		class Impersonator
		{
            Context current;
			Context* previous;
		public:
			Impersonator(Identities identity);
			~Impersonator();
		};
	}
}

