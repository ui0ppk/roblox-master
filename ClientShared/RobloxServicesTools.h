#pragma once
#include <string>
std::string trim_trailing_slashes(const std::string &path);
std::string GetCountersUrl(const std::string &baseUrl, const std::string &key);
std::string GetCountersMultiIncrementUrl(const std::string &baseUrl, const std::string &key);
std::string GetSettingsUrl(const std::string &baseUrl, const std::string &group, const std::string &key);
std::string GetClientVersionUploadUrl(const std::string &baseUrl, const std::string &key);
std::string GetPlayerGameDataUrl(const std::string &baseurl, int userId, const std::string &key = "");
std::string GetWebChatFilterURL(const std::string& baseUrl, const std::string& key = "");
std::string GetGridUrl(const std::string &, bool changeToDataDomain = true);
std::string GetDmpUrl(const std::string &, bool changeToDataDomain = true);
std::string GetBreakpadUrl(const std::string &, bool changeToDataDomain = true);
std::string ReplaceTopSubdomain(const std::string& url, const char* newTopSubDoman);

std::string BuildGenericPersistenceUrl(const std::string& baseUrl, const std::string &servicePath);
std::string BuildGenericGameUrl(const std::string &baseUrl, const std::string &servicePath);

// these should only be used on RCC.
std::string GetSecurityKeyUrl(const std::string &baseUrl, const std::string &key);
std::string GetSecurityKeyUrl2(const std::string &baseUrl, const std::string &key);
std::string GetMD5HashUrl(const std::string &baseUrl, const std::string &key);
std::string GetMemHashUrl(const std::string &baseUrl, const std::string &key);

