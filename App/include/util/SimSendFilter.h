/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/Region2.h"
#include "Util/SystemAddress.h"
#include <string>

namespace RBX {

	class SimSendFilter {
	public:
		typedef enum {EditVisit, Client, Server, dPhysClient, dPhysServer} Mode;

		Mode					mode;
		RBX::SystemAddress	networkAddress;
		Region2					region;
		
		SimSendFilter() : mode(Client)
		{}
	};

} // namespace
