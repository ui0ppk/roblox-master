/**
 * MachineConfig.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "MachineConfig.h"

// Roblox Headers
#include "MachineConfiguration.h"

// Roblox Studio Headers
#include "RobloxSettings.h"

MachineConfig* MachineConfig::machineConfig = NULL;
////////////////////////////////////////////////////////////////////////////////////

void MachineConfig::PostMachineConfiguration(bool authSuccess)
{
	if(authSuccess)
	{
		disconnect(&AuthenticationHelper::Instance(), SIGNAL(authenticationDone(bool)), machineConfig, SLOT(PostMachineConfiguration(bool)));

        RobloxSettings settings;
        int lastGfxMode = settings.contains("lastGFXMode") ? settings.value("lastGFXMode").toInt() : 0;

		RBX::postMachineConfiguration(RobloxSettings::getBaseURL().toLocal8Bit().constData(), lastGfxMode);
	}
}
/////////////////////////////////////////////////////////////////////////////////
