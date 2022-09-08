//
//  Copyright (c) 2012 ROBLOX. All rights reserved.
//

#ifndef __RobloxMobile__AndroidSettingsService__
#define __RobloxMobile__AndroidSettingsService__

#include "v8datamodel/FastLogSettings.h"
#include "util/Statistics.h"

#include <iostream>



class AndroidSettingsService : public RBX::FastLogJSON
{
    
public:
    START_DATA_MAP(AndroidSettingsService)
//    DECLARE_DATA_INT(iPadMinimumVersion)
    END_DATA_MAP();
};

#endif /* __RobloxMobile__AndroidSettingsService__ */
