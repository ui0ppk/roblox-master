#pragma once

#include "SimpleJSON.h"

class WindowsStudioInstallerSettings : public SimpleJSON
{
public:
	START_DATA_MAP(WindowsStudioInstallerSettings);
	DECLARE_DATA_INT(CountersLoad);
	DECLARE_DATA_BOOL(PerModeLoggingEnabled);
	DECLARE_DATA_BOOL(CountersFireImmediately);
	DECLARE_DATA_INT(HttpboostPostTimeout);
	DECLARE_DATA_BOOL(ShowInstallSuccessPrompt);
	DECLARE_DATA_BOOL(LogChromeProtocolFix);
	DECLARE_DATA_STRING(InfluxUrl);
	DECLARE_DATA_STRING(InfluxDatabase);
	DECLARE_DATA_STRING(InfluxUser);
	DECLARE_DATA_STRING(InfluxPassword);
	DECLARE_DATA_INT(InfluxHundredthsPercentage);
	DECLARE_DATA_BOOL(UseNewVersionFetch);
	DECLARE_DATA_BOOL(UseDataDomain);
	DECLARE_DATA_BOOL(CreateEdgeRegistry);
	DECLARE_DATA_BOOL(DeleteEdgeRegistry);
	END_DATA_MAP();
};