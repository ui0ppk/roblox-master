#include "stdafx.h"

#include "security/SecurityContext.h"
#include "rbx/Thread.hpp"
#include "rbx/Debug.h"

namespace RBX
{
	namespace Security
	{
		bool Context::isInRole(Identities identity, Permissions p)
		{
			if (p == None)
				return true;

			switch (identity)
			{
			case Anonymous:
			case GameScript_:
				return false;
#if defined(RBX_STUDIO_BUILD)
            case StudioPlugin:
                return p == Plugin;
#endif
			case GameScriptInRobloxPlace_:
				return                p == RobloxPlace;
			case RobloxGameScript_:
				return p == Plugin || p == RobloxPlace || p == LocalUser ||                      p == RobloxScript;
			case LocalGUI_:
			case CmdLine_:
				return p == Plugin || p == RobloxPlace || p == LocalUser;
			case Replicator_:
				return                p == RobloxPlace ||                   p == WritePlayer  || p == RobloxScript;
			case COM:
			case WebService:
				return true;
			default:
				RBXASSERT(false);
				return false;
			}
		}

		Context& Context::current()
		{
			Context* t = ptr().get();
			if (!t)
			{
				static Context anonymous(Anonymous);
				t = &anonymous;
				ptr().reset(t);
			}
			return *t;
		}


		void Context::tssCleanup(Context*)
		{

		}

		boost::thread_specific_ptr<Context>& Context::ptr()
		{
			static boost::thread_specific_ptr<Context> value(tssCleanup);
			return value;
		}

		Impersonator::Impersonator(Identities identity)
			  : current(identity)
		{
			previous = Context::ptr().get();
			Context::ptr().reset(&current);
		}

		Impersonator::~Impersonator()
		{
			Context::ptr().reset(previous);
		}


	}
}
