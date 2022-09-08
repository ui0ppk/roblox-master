/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <vector>

namespace RBX
{
	class Instance;
	class DataModel;
	namespace Network
	{
		typedef enum { Accept, Reject } FilterResult;

		extern std::string versionB;
		extern std::string securityKey;
		bool isPlayerAuthenticationEnabled();
		void initWithoutSecurity();	    // Used by Studio (so that it can't be used as a hack vector against RccService)
		void initWithPlayerSecurity();	// Used by Player
		void initWithServerSecurity();  // Used by RccService
		void initWithCloudEditSecurity(); // Used to set up cloud edit replication password
		bool isNetworkClient(const Instance* context);
		bool getSystemUrlLocal(DataModel *dataModel);
		void setSecurityVersions(const std::vector<std::string>& versions);

		// Used for debugging and development:
		void setVersion(const char* version);

		bool isTrustedContent(const char* url);
	}

	void spawnDebugCheckThreads(boost::weak_ptr<RBX::DataModel> weakDataModel);
    extern unsigned int initialProgramHash;
}