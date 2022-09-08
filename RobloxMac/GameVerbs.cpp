/*
 *  GameVerbs.cpp
 *  MacClient
 *
 *  Created by Tony on 2/2/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "GameVerbs.h"
#include "RobloxView.h"

LeaveGameVerb::LeaveGameVerb(RobloxView *pRobloxView, VerbContainer* container) : 
	Verb(container, "Exit")
	,robloxView(pRobloxView)
{
}

void LeaveGameVerb::doIt(RBX::IDataState* dataState)
{
	if (robloxView)
	{
		robloxView->leaveGame();
	}
}

ShutdownClientVerb::ShutdownClientVerb(RobloxView *pRobloxView, VerbContainer* container) :
	Verb( container, "ShutdownClient")
	,robloxView(pRobloxView)
{
}

void ShutdownClientVerb::doIt(RBX::IDataState* dataState)
{
	if (robloxView)
	{
		robloxView->shutdownClient();
	}
}

ToggleFullscreenVerb::ToggleFullscreenVerb(RobloxView *pRobloxView, VerbContainer* container, VideoControl* videoControl) :
	Verb(container, "ToggleFullScreen")
	, videoControl(videoControl)
	,robloxView(pRobloxView)
{}

void ToggleFullscreenVerb::doIt(RBX::IDataState* dataState)
{
	if (robloxView)
	{
		robloxView->toggleFullScreen();
	}
}

bool ToggleFullscreenVerb::isEnabled() const
{
	return true; 
}
