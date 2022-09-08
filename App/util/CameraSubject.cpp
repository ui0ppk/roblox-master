/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/CameraSubject.h"
#include "rbx/Debug.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/Camera.h"
#include "V8World/ContactManager.h"
#include "V8World/World.h"

namespace RBX {

ContactManager* CameraSubject::getContactManager()
{
	if (Instance* instance = dynamic_cast<Instance*>(this)) {
		if (World* world = Workspace::getWorldIfInWorkspace(instance)) {
			return world->getContactManager();
		}
	}
	return NULL;
}

} // namespace