#pragma once

#include <string>

class SettingsLoader
{
	std::string _baseUrl;
public:
	SettingsLoader(std::string baseUrl) : _baseUrl(baseUrl) {}
	
	std::string GetSettingsString(std::string groupName);
};