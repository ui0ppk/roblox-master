#pragma once

#include "Util/SystemAddress.h"
#include "util/G3DCore.h"
#include "RakNetTypes.h"

#ifdef _WIN32
#if defined(_NOOPT) || defined(_DEBUG) || defined(RBX_TEST_BUILD)
#define NETWORK_PROFILER
#define NETWORK_DEBUG
#endif
#endif

namespace RBX {

namespace SpatialRegion {
	class Id;
}

namespace Network {

	const RBX::SystemAddress RakNetToRbxAddress(const RakNet::SystemAddress& raknetAddress);
	std::string RakNetAddressToString(const RakNet::SystemAddress& raknetAddress, bool writePort = true, char portDelineator='|');

}}	// namespace

