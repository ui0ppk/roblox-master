/**
 * RobloxGoogleAnalytics.h
 * Copyright (c) 2013 ROBLOX Corp. All rights reserved.
 */

#pragma once

#include <string>
#include <sstream>

#include <rbx/signal.h>

namespace RBX {
// Singleton for tracking events across Studio using Google Analytics
// to process the data. Events are sent using the Measurement Protocol:
// https://developers.google.com/analytics/devguides/collection/protocol/v1
//
// All events are posted to the server asynchronously and any calls to
// track data should return immediately.

#define GA_CATEGORY_GAME "Game"
#define GA_CATEGORY_ACTION "Action"
#define GA_CATEGORY_ERROR "Error"
#define GA_CATEGORY_STUDIO "Studio"
#define GA_CATEGORY_COUNTERS "Counters"
#define GA_CATEGORY_RIBBONBAR "RibbonBar"
#define GA_CATEGORY_SECURITY "Security"
#define GA_CATEGORY_STUDIO_SETTINGS "StudioSettings"

// timing variables
#define GA_CLIENT_START "ClientStartTime"

namespace RobloxGoogleAnalytics
{
	static const std::string kGoogleAnalyticsBaseURL = "http://www.google-analytics.com/collect";

    // Allow for easy initialization based on a lottery number.
    // Calls setCanUseAnalytics and init.
    void lotteryInit(const std::string &accountPropertyID, size_t maxThreadScheduleSize, int lotteryThreshold, const char * productName = NULL, int robloxAnalyticsLottery = -1, const std::string &sessionKey = "sessionID=");

	// Must be called before using the singleton.
	void init(const std::string &accountPropertyID, size_t maxThreadScheduleSize, const char * productName = NULL);

	bool isInitialized();

	bool getCanUseAnalytics();
	void setCanUseAnalytics();

	void setUserID(int userID);
	void setPlaceID(int placeID);

    // Signal sent on each call to track an analytic.
    rbx::signal<void()>& analyticTrackedSignal();

	void setExperimentVariation(const std::string& name, int value);

    void trackEvent(
        const char *category,
        const char *action = "custom",
        const char *label = "none",
        int value = 0,
        bool sync = false);

	void trackEventWithoutThrottling(
        const char *category,
        const char *action = "custom",
        const char *label = "none",
        int value = 0,
        bool sync = false);

    void trackUserTiming(
        const char *category,
        const char *variable,
        int milliseconds,
        const char *label = "none",
        bool sync = false);

    void sendEventRoblox(const char* category,
        const char* action = "custom",
        const char* label = "none",
        int value = 0,
        bool sync = false);

	const std::string& getSessionId();
}
}
