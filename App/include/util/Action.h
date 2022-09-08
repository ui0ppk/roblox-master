/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

//#include "Util/SoundWorld.h"
#include <string>


namespace RBX {

	class Action
	{
	public:
		enum ActionType {	NO_ACTION = 0,
									PAUSE_ACTION,
									LOSE_ACTION,
									DRAW_ACTION,
									WIN_ACTION, 
									NUM_ACTION_TYPES };
	private:
		Action();

};

}	// namespace RBX