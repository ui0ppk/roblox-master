 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/SpatialHashMultiRes.h"

namespace RBX
{
	class Primitive;
	class Contact;
	class ContactManager;
	class World;
	class Assembly;
	
#define CONTACTMANAGER_MAXLEVELS 4
	class ContactManagerSpatialHash : public SpatialHash<Primitive, Contact, ContactManager, CONTACTMANAGER_MAXLEVELS>
	{
	public:
		ContactManagerSpatialHash(World* world, ContactManager* contactManager);
	};

}


