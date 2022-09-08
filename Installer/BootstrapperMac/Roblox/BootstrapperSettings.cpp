#include "BootstrapperSettings.h"

DATA_MAP_IMPL_START(BootstrapperSettings)
IMPL_DATA(ShowInstallSuccessPrompt, false);
IMPL_DATA(InfluxUrl, "");
IMPL_DATA(InfluxDatabase, "Default");
IMPL_DATA(InfluxUser, "rob");
IMPL_DATA(InfluxPassword, "playfaster");
IMPL_DATA(InfluxInstallHundredthsPercentage, 0);
DATA_MAP_IMPL_END()