/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/SystemAddress.h"
#include "Util/Color.h"

namespace RBX {
	namespace Network {

	class NetworkOwner 
	{
	public:
		static const RBX::SystemAddress Server() {
			static RBX::SystemAddress s(1, 0);
			return s;
		}

		// created on server, have not be properly assigned via network owner job
		static const RBX::SystemAddress ServerUnassigned() {
			static RBX::SystemAddress s(1, 1);
			return s;
		}

		// default
		static const RBX::SystemAddress Unassigned() {
			static RBX::SystemAddress s;
			RBXASSERT(s == SystemAddress());
			RBXASSERT(s != NetworkOwner::Server());
			RBXASSERT(s != NetworkOwner::ServerUnassigned());
			RBXASSERT(s != NetworkOwner::AssignedOther());
			return s;
		}

		// generic value used on client indicating assigned to other clients or server (i.e. not self)
		static const RBX::SystemAddress AssignedOther() {
			static RBX::SystemAddress s(0, 1);
			return s;
		}

		static bool isClient(const RBX::SystemAddress& address) {
			return (	(address != Server())
					&&	(address != Unassigned())	
					&&  (address != ServerUnassigned()));
		}

		static bool isServer(const RBX::SystemAddress& address) {
			return address == Server() || address == ServerUnassigned();
		}
		
		static Color3 colorFromAddress(const RBX::SystemAddress& systemAddress) {
			if (systemAddress == Server()) {
				return Color3::white();
			}
			else if (systemAddress == Unassigned() || systemAddress == ServerUnassigned()) {
				return Color3::black();
			}
			else if (systemAddress == AssignedOther())
				return Color3::gray();
			else {
				unsigned int address = systemAddress.getAddress();
				unsigned int port = systemAddress.getPort();
				address += port;
				return RBX::Color::colorFromInt(address);
			}
		}

	};

}}	// namespace