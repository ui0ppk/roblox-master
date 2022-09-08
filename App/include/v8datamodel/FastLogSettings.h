#pragma once

#include "V8DataModel/GlobalSettings.h"

#include "SimpleJSON.h"
#include "util/Statistics.h"

#define CLIENT_APP_SETTINGS_STRING  "ClientAppSettings"
#define CLIENT_SETTINGS_API_KEY     "D6925E56-BFB9-4908-AAA2-A5B1EC4B2D79"

namespace RBX
{
	extern const char* const sFastLogSettings;
	GlobalAdvancedSettings::Item& FastLogSettingsInstance();


	class FastLogJSON : public SimpleJSON
	{
	public:
		virtual void ProcessVariable(const std::string& valueName, const std::string& valueData, FastVarType fastVarType);
		virtual bool DefaultHandler(const std::string& valueName, const std::string& valueData);
	};

	class ClientAppSettings : public RBX::FastLogJSON
	{
	private:
		static ClientAppSettings m_ClientAppSettings;
	public:
		//static ClientAppSettings();
		static void Initialize();
		static ClientAppSettings& singleton();
		
		START_DATA_MAP(ClientAppSettings);
		
		DECLARE_DATA_BOOL(AllowVideoPreRoll);
		DECLARE_DATA_INT(VideoPreRollWaitTimeSeconds);
		DECLARE_DATA_BOOL(CaptureQTStudioCountersEnabled);
		DECLARE_DATA_BOOL(CaptureMFCStudioCountersEnabled);
		DECLARE_DATA_INT(CaptureCountersIntervalInMinutes);
		DECLARE_DATA_INT(CaptureSlowCountersIntervalInSeconds);
		DECLARE_DATA_STRING(StartPageUrl);
		DECLARE_DATA_STRING(PublishedProjectsPageUrl);
		DECLARE_DATA_INT(PublishedProjectsPageWidth);
		DECLARE_DATA_INT(PublishedProjectsPageHeight);
		DECLARE_DATA_BOOL(WebDocAddressBarEnabled);
		DECLARE_DATA_INT(AxisAdornmentGrabSize);
		DECLARE_DATA_STRING(PrizeAwarderURL);
		DECLARE_DATA_STRING(PrizeAssetIDs);
		DECLARE_DATA_INT(MinNumberScriptExecutionsToGetPrize);
		DECLARE_DATA_INT(MinPartsForOptDragging);
		DECLARE_DATA_STRING(GoogleAnalyticsAccountPropertyID);
		DECLARE_DATA_STRING(GoogleAnalyticsAccountPropertyIDPlayer);
		DECLARE_DATA_INT(GoogleAnalyticsThreadPoolMaxScheduleSize);
		DECLARE_DATA_INT(GoogleAnalyticsLoadPlayer);
		DECLARE_DATA_INT(GoogleAnalyticsLoadStudio);
		DECLARE_DATA_BOOL(GoogleAnalyticsInitFix);
        DECLARE_DATA_INT(HttpUseCurlPercentageMacClient);
        DECLARE_DATA_INT(HttpUseCurlPercentageMacStudio);
        DECLARE_DATA_INT(HttpUseCurlPercentageWinClient);
        DECLARE_DATA_INT(HttpUseCurlPercentageWinStudio);
		END_DATA_MAP();
	};
}

