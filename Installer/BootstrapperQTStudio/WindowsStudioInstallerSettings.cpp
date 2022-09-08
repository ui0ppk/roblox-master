#include "stdafx.h"
#include "WindowsStudioInstallerSettings.h"

DATA_MAP_IMPL_START(WindowsStudioInstallerSettings)
IMPL_DATA(CountersLoad, 0);
IMPL_DATA(PerModeLoggingEnabled, false);
IMPL_DATA(CountersFireImmediately, false);
IMPL_DATA(HttpboostPostTimeout, 0);
IMPL_DATA(ShowInstallSuccessPrompt, false);
IMPL_DATA(LogChromeProtocolFix, false);
IMPL_DATA(InfluxUrl, "https://marty-onepointtwentyone-1.c.influxdb.com:8087");
IMPL_DATA(InfluxDatabase, "Default");
IMPL_DATA(InfluxUser, "rob");
IMPL_DATA(InfluxPassword, "playfaster");
IMPL_DATA(InfluxHundredthsPercentage, 0);
IMPL_DATA(UseNewVersionFetch, false);
IMPL_DATA(UseDataDomain, true);
IMPL_DATA(CreateEdgeRegistry, true);
IMPL_DATA(DeleteEdgeRegistry, false);
DATA_MAP_IMPL_END()
