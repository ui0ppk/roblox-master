#pragma once

#include "Script/Script.h"

#include <boost/optional.hpp>

namespace RBX
{
	extern const char* const sCoreScript;
	class CoreScript
		: public DescribedNonCreatable<CoreScript, BaseScript, sCoreScript, RBX::Reflection::ClassDescriptor::INTERNAL_LOCAL>
	{
	private:
		typedef DescribedNonCreatable<CoreScript, BaseScript, sCoreScript, RBX::Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
        Code code;

	public:
		CoreScript();

        static boost::optional<ProtectedString> fetchSource(const std::string& name);

		virtual Code requestCode(ScriptInformationProvider* scriptInfoProvider=NULL);

		virtual void extraErrorReporting(lua_State *thread);

	protected:
		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	};
}
