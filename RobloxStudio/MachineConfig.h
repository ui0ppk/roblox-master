/**
 * MachineConfig.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Roblox Studio Headers
#include "AuthenticationHelper.h"

class MachineConfig : public QObject
{
Q_OBJECT

public:
	static MachineConfig* setupMachineConfig() 
	{	
		if(!machineConfig)
		{
			machineConfig = new MachineConfig();
			connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationDone(bool)), machineConfig, SLOT(PostMachineConfiguration(bool)));
		}
		return machineConfig;
	}

private Q_SLOTS:
	void PostMachineConfiguration(bool);
	
private:
	MachineConfig() {}
	MachineConfig(MachineConfig const&);
	MachineConfig& operator=(MachineConfig const&);

	static MachineConfig* machineConfig;
};
