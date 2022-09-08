#include "stdafx.h"

#include "Util/ScriptInformationProvider.h"
#include "Util/http.h"
#include <sstream>

namespace RBX
{
const char* const sScriptInformationProvider = "ScriptInformationProvider";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<ScriptInformationProvider, void(std::string)> func_setAssetUrl( &ScriptInformationProvider::setAssetUrl, "SetAssetUrl", "url", Security::LocalUser);
static Reflection::BoundFuncDesc<ScriptInformationProvider, void(std::string)> func_setAccessKey( &ScriptInformationProvider::setAccessKey, "SetAccessKey", "access", Security::Roblox);
REFLECTION_END();

ScriptInformationProvider::ScriptInformationProvider()
	:DescribedNonCreatable<ScriptInformationProvider,Instance,sScriptInformationProvider>()
{
}

}//namespace

