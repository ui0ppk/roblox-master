#include "stdafx.h"
#include "SettingsLoader.h"
#include "RobloxServicesTools.h"
#include "HttpTools.h"


std::string SettingsLoader::GetSettingsString(std::string groupName)
{
	std::string url = GetSettingsUrl(_baseUrl, groupName.c_str(), "76E5A40C-3AE1-4028-9F10-7C62520BD94F");
	return HttpTools::httpGetString(url);
}
