/*
 *  RbxCrash.cpp
 *  MacClient
 *
 *  Created by elebel on 7/20/10.
 *  Copyright 2010 Roblox. All rights reserved.
 *
 */

#include "rbx/Debug.h"

void RBXCRASH()
{
	RBX::Debugable::doCrash();
}

void RBXCRASH(const char* message)
{
	RBX::Debugable::doCrash(message);
}
