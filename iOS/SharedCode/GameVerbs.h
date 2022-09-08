/*
 *  GameVerbs.h
 *  iOS
 *
 *  Created on 5/18/14.
 *  Copyright 2014 ROBLOX. All rights reserved.
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
