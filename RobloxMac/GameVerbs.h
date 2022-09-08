/*
 *  GameVerbs.h
 *  MacClient
 *
 *  Created by Tony on 2/2/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "v8tree/Verb.h"
using namespace RBX;

class LeaveGameVerb : public RBX::Verb
{
	class RobloxView *robloxView;
	
public :
	
	
	LeaveGameVerb(class RobloxView *pRobloxView, VerbContainer* container);

	virtual void doIt(RBX::IDataState* dataState);
	
};

class ShutdownClientVerb : public RBX::Verb
{
	class RobloxView *robloxView;
	
public :
	
	ShutdownClientVerb( class RobloxView *pRobloxView, VerbContainer* container );
	
	virtual void doIt(RBX::IDataState* dataState);
};

class ToggleFullscreenVerb : public RBX::Verb
{
	
private:
	class VideoControl* videoControl;
	class RobloxView *robloxView;
	
public:
	
	ToggleFullscreenVerb(class RobloxView *pRobloxView, VerbContainer* container, VideoControl* videoControl);
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const;
};