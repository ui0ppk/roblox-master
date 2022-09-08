#include "stdafx.h"
#include "SimpleJSON.h"
#include "ClientInstallerSettings.h"

DATA_MAP_IMPL_START(ClientInstallerSettings)
	IMPL_DATA(NeedInstallBgTask, false);
	IMPL_DATA(NeedRunBgTask, false);
	IMPL_DATA(IsPreDeployOn, false);
	IMPL_DATA(PreVersion, "");
	IMPL_DATA(ForceSilentMode, false);
	IMPL_DATA(CountersLoad, 0);
	IMPL_DATA(BetaPlayerLoad, 100);
	IMPL_DATA(PerModeLoggingEnabled, false);
	IMPL_DATA(CountersFireImmediately, false);
	IMPL_DATA(ExeVersion, "");
	IMPL_DATA(GoogleAnalyticsAccountPropertyID, "UA-43420590-15");
	IMPL_DATA(ValidateInstalledExeVersion, false);
	IMPL_DATA(UseNewCdn, false); 
	IMPL_DATA(UseNewVersionFetch, false);
	IMPL_DATA(CheckIsStudioOutofDate, false);
	IMPL_DATA(HttpboostPostTimeout, 0);
	IMPL_DATA(ShowInstallSuccessPrompt, false);
	IMPL_DATA(LogChromeProtocolFix, false);
	IMPL_DATA(InfluxUrl, "");
	IMPL_DATA(InfluxDatabase, "Default");
	IMPL_DATA(InfluxUser, "rob");
	IMPL_DATA(InfluxPassword, "playfaster");
	IMPL_DATA(InfluxHundredthsPercentage, 0);
	IMPL_DATA(InfluxInstallHundredthsPercentage, 0);
	IMPL_DATA(UseDataDomain, true);
	IMPL_DATA(CreateEdgeRegistry, true);
	IMPL_DATA(DeleteEdgeRegistry, false);
	IMPL_DATA(ReplaceCdnTxt, false);
DATA_MAP_IMPL_END()
