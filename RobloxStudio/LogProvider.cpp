/*
 *  LogProvider.cpp
 *  RobloxStudio
 *
 *  Created by Ganesh Agrawal on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

// Roblox Headers
#include "util/FileSystem.h"
#include "util/Guid.h"
#include "rbx/boost.hpp"

// Roblox Studio Headers
#include "LogProvider.h"

#ifdef __APPLE__
#include "BreakpadCrashReporter.h"
#endif


RBX::mutex LogProvider::fastLogChannelsLock;
LogProvider* LogProvider::mainLogManager;

LogProvider::LogProvider()
{
	logDir = RBX::FileSystem::getLogsDirectory();
    RBX::Guid::generateRBXGUID(logGuid);
	mainLogManager = this;
	FLog::SetExternalLogFunc(LogProvider::FastLogMessage);
}

void LogProvider::FastLogMessage(FLog::Channel id, const char* message) {

	RBX::mutex::scoped_lock lock(fastLogChannelsLock);

	if(mainLogManager)
	{
		if(id >= mainLogManager->fastLogChannels.size())
			mainLogManager->fastLogChannels.resize(id+1, NULL);

		if(mainLogManager->fastLogChannels[id] == NULL)
		{
			char temp[20];
			snprintf(temp, 19, "log_%s_%u.txt", mainLogManager->logGuid.substr(3,6).c_str(), id);
			temp[19] = 0;
            
            boost::filesystem::path logFile = mainLogManager->logDir / temp;

			mainLogManager->fastLogChannels[id] = new RBX::Log(logFile.c_str(), "Log Channel");
#ifndef _DEBUG
			CrashReporter::addBreakpadLog(logFile.c_str());
#endif
		}

		mainLogManager->fastLogChannels[id]->writeEntry(RBX::Log::Information, message);
	}
}
	
LogProvider::~LogProvider()
{
}

RBX::Log* LogProvider::provideLog()
{
	RBX::Log* result = log.get();
	if (!result)
	{
		std::string name = RBX::get_thread_name();
        boost::filesystem::path logFile = logDir / ("log_" + logGuid.substr(3,6) +".txt");
		
		result = new RBX::Log(logFile.c_str(), name.c_str());
		log.reset(result);
#ifndef _DEBUG
		CrashReporter::addBreakpadLog(logFile.c_str());
#endif
	}
	return result;
}