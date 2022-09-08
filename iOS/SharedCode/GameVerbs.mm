/*
 *  GameVerbs.mm
 *  iOS
 *
 *  Created on 5/19/14
 *  Copyright 2014 ROBLOX. All rights reserved.
 *
 */

#import "PlaceLauncher.h"

#include "GameVerbs.h"
#include "RobloxView.h"

#include "v8datamodel/GameBasicSettings.h"

LeaveGameVerb::LeaveGameVerb(RobloxView *pRobloxView, VerbContainer* container) : 
	Verb(container, "Exit")
	,robloxView(pRobloxView)
{
}

void LeaveGameVerb::doIt(RBX::IDataState* dataState)
{
	[[PlaceLauncher sharedInstance] leaveGame:YES];
    
    RBX::GlobalBasicSettings::singleton()->saveState();
}
