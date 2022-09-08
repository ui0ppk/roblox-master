/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Backpack.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/Workspace.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Script/Script.h"

namespace RBX {

const char* const sBackpack = "Backpack";		// rename class ultimately

Backpack::Backpack() 
{
	setName("Backpack");
}



/*	When scripts run in the backpack

	BaseScript:		1.	In HopperBin if local backpack		(runs local)
				2.	If backend Processing				(runs backend)

	LocalScript:3.	If local backpack					(runs local)
*/

bool Backpack::scriptShouldRun(BaseScript* script)
{
	RBXASSERT(isAncestorOf(script));

	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	if (workspace)
	{
		RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(this);
		bool isLocalBackpack = (this->getParent() == localPlayer);
		bool isLocalScript = (script->fastDynamicCast<LocalScript>() != NULL);
			
		if (isLocalBackpack && isLocalScript)
		{
			script->setLocalPlayer(shared_from(localPlayer));
			return true;
		}

		bool isBackendProcessing = Network::Players::backendProcessing(this);

		if (!isLocalScript && isBackendProcessing)
        {
			return true;
		}
	}
	return false;
}


}	// namespace
