#include "stdafx.h"

#include "RobloxServicesTools.h"
#include "format_string.h"
#include <boost/algorithm/string/predicate.hpp>
#include <sstream>


#ifdef RBX_PLATFORM_DURANGO
#define DEFAULT_URL_SCHEMA "https"
#else
#define DEFAULT_URL_SCHEMA "http"
#endif

std::string trim_trailing_slashes(const std::string& path)
{
	size_t i = path.find_last_not_of('/');
	return path.substr(0, i + 1);
}

static std::string BuildGenericApiUrl(const std::string& baseUrl, const std::string& serviceNameIn, const std::string& path, const std::string& key, const char* scheme = "https")
{
	std::string serviceName(serviceNameIn);
	if ("" != serviceName)
		serviceName += ".";

	bool kSkipBuildApiUrl = true;
	if (kSkipBuildApiUrl)
	{
		std::string baseUrlOut = baseUrl.substr((baseUrl.substr(0, 5) == "https") ? 12 : 11);
		baseUrlOut = baseUrlOut.substr(0, baseUrlOut.size() - 1);

		return format_string("%s://%sapi.%s/%s/?apiKey=%s", ((baseUrl.substr(0, 5) == "https") ? "https" : "http"), serviceName.c_str(), baseUrlOut.c_str(), path.c_str(), key.c_str());
	}

	std::string rbxUrl = ".roblox.com";
	size_t pos = baseUrl.find(rbxUrl);
	if (pos == std::string::npos)
	{
		rbxUrl = ".robloxlabs.com";
		pos = baseUrl.find(rbxUrl);
	}

	std::string subUrl = baseUrl.substr(0, pos);

	if (subUrl == "www" || subUrl == "http://www" || subUrl == "https://www" || subUrl == "m" || subUrl == "http://m") //prod
	{
		subUrl = "";
	}
	std::string httpPrefix = "http://";
	pos = subUrl.find(httpPrefix);
	if (pos != -1)
	{
		subUrl = subUrl.substr(httpPrefix.length(), subUrl.length() - httpPrefix.length());
	}

	std::string url;
	if (subUrl.empty())
	{
		// production
		url = format_string("%s://%sapi.roblox.com/%s/?apiKey=%s", scheme, serviceName.c_str(), path.c_str(), key.c_str());
	}
	else
	{
		if (subUrl.find("www") != -1)
		{
			subUrl = subUrl.replace(0, 4, "");
			url = format_string("%s://%sapi.%s%s/%s/?apiKey=%s", scheme, serviceName.c_str(), subUrl.c_str(), rbxUrl.c_str(), path.c_str(), key.c_str());
		}
		else if (subUrl.find("m.") == 0)
		{
			subUrl = subUrl.replace(0, 2, "");
			url = format_string("%s://%sapi.%s%s/%s/?apiKey=%s", scheme, serviceName.c_str(), subUrl.c_str(), rbxUrl.c_str(), path.c_str(), key.c_str());
		}
		else if (subUrl.find(".sitetest3") != -1) // Special case for URLs like alberto.sitetest3, navin.sitetest3, etc..
		{
			url = format_string("%s://%sapi.sitetest3%s/%s/?apiKey=%s", scheme, serviceName.c_str(), rbxUrl.c_str(), path.c_str(), key.c_str());
		}
	}

	return url;
}

std::string GetCountersUrl(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "ephemeralcounters", "v1.1/Counters/Increment", key, DEFAULT_URL_SCHEMA);
}

std::string GetCountersMultiIncrementUrl(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "ephemeralcounters", "v1.0/MultiIncrement", key, DEFAULT_URL_SCHEMA);
}

std::string GetSettingsUrl(const std::string& baseUrl, const std::string& group, const std::string& key)
{
	 return BuildGenericApiUrl(baseUrl, "clientsettings", format_string("Setting/QuietGet/%s", group.c_str()), key, DEFAULT_URL_SCHEMA);
}

std::string GetSecurityKeyUrl(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "versioncompatibility", "GetAllowedSecurityVersions", key);
}

std::string GetSecurityKeyUrl2(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "versioncompatibility", "GetAllowedSecurityKeys", key);
}

// used by bootstrapper
std::string GetClientVersionUploadUrl(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "versioncompatibility", "GetCurrentClientVersionUpload", key);
}

std::string GetPlayerGameDataUrl(const std::string& baseurl, int userId, const std::string& key)
{
	return BuildGenericApiUrl(baseurl, "", format_string("/game/players/%d", userId), key);
}

std::string GetWebChatFilterURL(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "", "/moderation/filtertext", key);
}

std::string GetMD5HashUrl(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "versioncompatibility", "GetAllowedMD5Hashes", key);
}

std::string GetMemHashUrl(const std::string& baseUrl, const std::string& key)
{
	return BuildGenericApiUrl(baseUrl, "versioncompatibility", "GetAllowedMemHashes", key);
}

std::string GetGridUrl(const std::string& anyUrl, bool changeToDataDomain)
{
	std::string url = trim_trailing_slashes(anyUrl);
	if (changeToDataDomain)
		url = ReplaceTopSubdomain(url, "data");

	url = url + "/Error/Grid.ashx";
	return url;
}

std::string GetDmpUrl(const std::string& anyUrl, bool changeToDataDomain)
{
	std::string url = trim_trailing_slashes(anyUrl);
	if (changeToDataDomain)
		url = ReplaceTopSubdomain(url, "data");

	url = url + "/Error/Dmp.ashx";
	return url;
}

std::string GetBreakpadUrl(const std::string& anyUrl, bool changeToDataDomain)
{
	std::string url = trim_trailing_slashes(anyUrl);
	if (changeToDataDomain)
		url = ReplaceTopSubdomain(url, "data");

	url = url + "/Error/Breakpad.ashx";
	return url;
}

std::string ReplaceTopSubdomain(const std::string& anyUrl, const char* newTopSubDoman)
{
	std::string result(anyUrl);
	std::size_t foundPos = result.find("www.");
	if (foundPos != std::string::npos)
	{
		result.replace(foundPos, 3, newTopSubDoman);
	}
	else if ((foundPos = result.find("m.")) != std::string::npos)
	{
		result.replace(foundPos, 1, newTopSubDoman);
	}
	return result;
}

std::string BuildGenericPersistenceUrl(const std::string& baseUrl, const std::string& servicePath)
{
	std::string constructedURLDomain(baseUrl);
	std::string constructedServicePath(servicePath);

	constructedURLDomain = ReplaceTopSubdomain(constructedURLDomain, "gamepersistence");
	if (!boost::algorithm::ends_with(constructedURLDomain, "/"))
	{
		constructedURLDomain.append("/");
	}
	return constructedURLDomain + constructedServicePath + "/";
}

std::string BuildGenericGameUrl(const std::string& baseUrl, const std::string& servicePath)
{
	std::string constructedURLDomain(baseUrl);
	std::string constructedServicePath(servicePath);

	constructedURLDomain = ReplaceTopSubdomain(constructedURLDomain, "assetgame");
	if (!boost::algorithm::ends_with(constructedURLDomain, "/"))
	{
		constructedURLDomain.append("/");
	}
	if (boost::algorithm::starts_with(constructedServicePath, "/"))
	{
		constructedServicePath.erase(constructedServicePath.begin());
	}
	return constructedURLDomain + constructedServicePath;
}

