#pragma once

#include "SimpleJSON.h"

class ClientInstallerSettings : public SimpleJSON
{
public:
	START_DATA_MAP(ClientInstallerSettings);
		DECLARE_DATA_BOOL(NeedInstallBgTask);
		DECLARE_DATA_BOOL(NeedRunBgTask);
		DECLARE_DATA_BOOL(IsPreDeployOn);
		DECLARE_DATA_STRING(PreVersion);
		DECLARE_DATA_BOOL(ForceSilentMode);
		DECLARE_DATA_INT(CountersLoad);
		DECLARE_DATA_INT(BetaPlayerLoad);
		DECLARE_DATA_BOOL(PerModeLoggingEnabled);
		DECLARE_DATA_BOOL(CountersFireImmediately);
		DECLARE_DATA_STRING(ExeVersion);
		DECLARE_DATA_STRING(GoogleAnalyticsAccountPropertyID);
		DECLARE_DATA_BOOL(ValidateInstalledExeVersion);
		DECLARE_DATA_BOOL(UseNewCdn);
		DECLARE_DATA_BOOL(UseNewVersionFetch);
		DECLARE_DATA_BOOL(CheckIsStudioOutofDate);
		DECLARE_DATA_INT(HttpboostPostTimeout);
		DECLARE_DATA_BOOL(ShowInstallSuccessPrompt);
		DECLARE_DATA_BOOL(LogChromeProtocolFix);
		DECLARE_DATA_STRING(InfluxUrl);
		DECLARE_DATA_STRING(InfluxDatabase);
		DECLARE_DATA_STRING(InfluxUser);
		DECLARE_DATA_STRING(InfluxPassword);
		DECLARE_DATA_INT(InfluxHundredthsPercentage);
		DECLARE_DATA_INT(InfluxInstallHundredthsPercentage);
		DECLARE_DATA_BOOL(UseDataDomain);
		DECLARE_DATA_BOOL(CreateEdgeRegistry);
		DECLARE_DATA_BOOL(DeleteEdgeRegistry);
		DECLARE_DATA_BOOL(ReplaceCdnTxt);
	END_DATA_MAP();
};
