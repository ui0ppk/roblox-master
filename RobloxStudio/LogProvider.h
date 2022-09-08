/*
 *  LogProvider.h
 *  RobloxStudio
 *
 *  Created by Ganesh Agrawal on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

// 3rd Party Headers
#include "boost/filesystem.hpp"
#include "boost/thread/tss.hpp"

// Roblox Headers
#include "rbx/Log.h"
#include "rbx/threadsafe.h"

class LogProvider : public RBX::ILogProvider
{
	static RBX::mutex fastLogChannelsLock;
	static LogProvider* mainLogManager;
	std::vector<RBX::Log*> fastLogChannels;
	static void FastLogMessage(FLog::Channel id, const char* message);
private:
	boost::thread_specific_ptr<RBX::Log> log;	
    boost::filesystem::path logDir;
    std::string logGuid;

public:
	LogProvider();
	~LogProvider();
	virtual RBX::Log* provideLog();
};
