#pragma once

#include <string>
#include <set>
#include <stdarg.h>
#include "format_string.h"

class CountersClient
{
	static std::set<std::wstring> _events;
	std::string _url;
	simple_logger<wchar_t>* _logger;
public:
	CountersClient(std::string baseUrl, std::string key, simple_logger<wchar_t>* logger);
	~CountersClient() {}

	void registerEvent(std::wstring eventName, bool fireImmediately = true);
	void reportEvents();
private:
	void reportEvents(std::set<std::wstring> &events);
};