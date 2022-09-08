#pragma once

#include "format_string.h"

namespace GoogleAnalyticsHelper
{
	#define GA_CATEGORY_NAME "PC"

	void init(const std::string &accountPropertyID);

	bool trackEvent(simple_logger<wchar_t> &logger, const char *category, const char *action = "custom", const char *label = "none", int value = 0);
	bool trackUserTiming(simple_logger<wchar_t> &logger, const char *category, const char *variable, int milliseconds, const char *label = "none");
}