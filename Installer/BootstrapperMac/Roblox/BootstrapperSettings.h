#pragma once

#include "SimpleJSON.h"

class BootstrapperSettings: public SimpleJSON
{
public:
    START_DATA_MAP(BootstrapperSettings);
    DECLARE_DATA_BOOL(ShowInstallSuccessPrompt);
    DECLARE_DATA_STRING(InfluxUrl);
    DECLARE_DATA_STRING(InfluxUser);
    DECLARE_DATA_STRING(InfluxPassword);
    DECLARE_DATA_STRING(InfluxDatabase);
    DECLARE_DATA_INT(InfluxInstallHundredthsPercentage);
    END_DATA_MAP();
};

