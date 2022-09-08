/*
 *  Roblox.cpp
 *  MacClient
 *
 *  Created by Tony on 1/26/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import "RobloxPlayerAppDelegate.h"
#import "CMCrashReporter.h"

#include "Roblox.h"
#include "FunctionMarshaller.h"
#include "rbx/CEvent.h"
#include "rbx/log.h"
#include "util/guid.h"
#include "Util/FileSystem.h"
#include "RobloxView.h"
#include "v8datamodel/TeleportCallback.h"
#include "FastLog.h"
#include <semaphore.h>

const std::string& GetBaseURL();

void callTerminateApp()
{
	[NSApp terminate:nil];
}

void *Roblox::theInstance = NULL;
boost::scoped_ptr<boost::thread> Roblox::singleRunningInstance;
sem_t *Roblox::uniqSemaphore = NULL;
bool Roblox::needTerminateCall = true;

shared_ptr<RBX::TeleportCallback> Roblox::callback;

// This function will locate the path to our application on OS X,
// unlike windows you cannot rely on the current working directory
// for locating your configuration files and resources.
std::string macBundlePath()
{
	char path[1024];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	assert(mainBundle);
	
	CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	assert(mainBundleURL);
	
	CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
	assert(cfStringRef);
	
	CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
	
	CFRelease(mainBundleURL);
	CFRelease(cfStringRef);
	
	return std::string(path);
}

class LogProvider : public RBX::ILogProvider
{
	boost::thread_specific_ptr<RBX::Log> log;	
    boost::filesystem::path logDir;
	boost::mutex breakPadLogMutex;
	int breakpadCount;
	std::vector<RBX::Log*> fastLogChannels;
    std::string logGuid;
	static RBX::mutex fastLogChannelsLock;
	
	static LogProvider* mainLogManager;
public:
	LogProvider()
	:breakpadCount(0)
	{
        RBX::Guid::generateRBXGUID(logGuid);
        logGuid = logGuid.substr(3, 6);
		logDir = RBX::FileSystem::getLogsDirectory();
        
		std::cout << "Writing logs to " << logDir.string() << " with GUID " + logGuid << '\n';
        
		mainLogManager = this;
		
		FLog::SetExternalLogFunc(LogProvider::FastLogMessage);
	}
	
	~LogProvider()
	{
        // Do not delete Log files on Mac Player exit
        // Look for Log under /tmp/roblox.xyz/Roblox/Logs
        // tmp folder is auto cleaned on restart of Mac
		//RBX::FileSystem::clearCacheDirectory("Logs");
	}
	
	virtual RBX::Log* provideLog()
	{
		RBX::Log* result = log.get();
		if (!result)
		{
			std::string name = RBX::get_thread_name();
            boost::filesystem::path logFile = logDir / ("log_" + logGuid + ".txt");
			
			result = new RBX::Log(logFile.c_str(), name.c_str());
			log.reset(result);
			
			boost::mutex::scoped_lock lock(breakPadLogMutex);
			if (breakpadCount++ < 10)
			{
				Roblox::addLogToBreakpad(logFile.c_str());
			}
			
		}
		return result;
	}
	
	static void FastLogMessage(FLog::Channel id, const char* message) {
		
		RBX::mutex::scoped_lock lock(fastLogChannelsLock);
		
		if(mainLogManager)
		{
			if(id >= mainLogManager->fastLogChannels.size())
				mainLogManager->fastLogChannels.resize(id+1, NULL);
			
			if(mainLogManager->fastLogChannels[id] == NULL)
			{
				char temp[20];
				snprintf(temp, 19, "log_%s_%u.txt", mainLogManager->logGuid.c_str(), id);
				temp[19] = 0;
                boost::filesystem::path logFile = mainLogManager->logDir / temp;
				
				mainLogManager->fastLogChannels[id] = new RBX::Log(logFile.c_str(), "Log Channel");
				Roblox::addLogToBreakpad(logFile.c_str());
			}
			
			mainLogManager->fastLogChannels[id]->writeEntry(RBX::Log::Information, message);
		}
	}
	
	static void WriteFastLogDump(){
		RBX::mutex::scoped_lock lock(fastLogChannelsLock);
		
		if(mainLogManager)
		{
            boost::filesystem::path logFile = mainLogManager->logDir / ("log_" + mainLogManager->logGuid + "_dump.txt");
			
			FLog::WriteFastLogDump(logFile.c_str(), 2048);
			Roblox::addLogToBreakpad(logFile.c_str());
		}
	}
};

void MacWriteFastLogDump()
{
	LogProvider::WriteFastLogDump();
}

LogProvider* LogProvider::mainLogManager = NULL;
RBX::mutex LogProvider::fastLogChannelsLock;

static LogProvider logProvider;

class Teleporter : public RBX::TeleportCallback
{
	void *_instance;
public:
	Teleporter(void *instance) { _instance = instance; }
	
	virtual void doTeleport(const std::string &url, const std::string &ticket, const std::string &script) 
	{
        RobloxView *view = nil;
        
        RobloxPlayerAppDelegate *appDelegate = (RobloxPlayerAppDelegate*)_instance;
        view = [appDelegate robloxView];
        
		view->stopJobs();
		view->marshalTeleport(url, ticket, script);
	}
	
	virtual bool isTeleportEnabled() const { return true; }
};

void Roblox::addLogToBreakpad(const char* log)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSResponder *responder = (NSResponder *) theInstance;
	NSString* s = [NSString stringWithFormat:@"%s", log];
	[responder performSelectorOnMainThread:@selector(addLogFile:)  
								withObject:s waitUntilDone:NO];  
	[pool release];
}

void Roblox::addBreakPadKeyValue(const char* key, int value)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSResponder *responder = (NSResponder *) theInstance;
	NSString* s = [NSString stringWithFormat:@"%s", key];
	NSString* v = [NSString stringWithFormat:@"%d", value];
	
	NSDictionary *info = [NSDictionary dictionaryWithObjectsAndKeys:s, @"key", v, @"value", nil];
	[responder performSelectorOnMainThread:@selector(addBreakPadKeyValueFromDictionary:)  
								withObject:info waitUntilDone:NO];  
	[pool release];

}

static char uniqPlayerId[] = "/RobloxPlayerUniq";

bool Roblox::isOtherRunning() 
{
	pid_t id = getpid();
	///NSLog(@"isOtherRunning - pid = %i trying to open semaphore", id);
	sem_t *sem = sem_open(uniqPlayerId, O_CREAT | O_EXCL, 0666, 0);
	if (sem != SEM_FAILED)
	{
		NSLog(@"isOtherRunning - pid = %i waiting on semaphore", id);
		uniqSemaphore = sem; // I guess we need atomic echange????
		sem_wait(sem);
		uniqSemaphore = NULL;
		
		NSLog(@"isOtherRunning - pid = %i closing semaphore", id);
		sem_close(sem);
		sem_unlink(uniqPlayerId);
		return false;
	}
	//NSLog(@"isOtherRunning - pid = %i semaphore open failed", id);
	return true;
}

void Roblox::terminateOther()
{
	sem_t *sem = sem_open(uniqPlayerId, 0);
	if (sem != SEM_FAILED)
	{
		//NSLog(@"terminateOther - pid = %i semaphore is opened", id);
		sem_post(sem);
		sem_close(sem);
	}
}

void Roblox::terminateWaiter()
{
	pid_t id = getpid();
	NSLog(@"Roblox::terminateWaiter - pid = %i", id);
	while(isOtherRunning())
	{
		terminateOther();
		usleep(1000*100); //sleep 100 msecs
	}
	
	if (needTerminateCall)
	{
		NSLog(@"Roblox::terminateWaiter - pid = %i, terminating app", id);
		[NSApp terminate:nil];
	}
}

bool Roblox::initInstance(void *instance, bool isApp)
{
	singleRunningInstance.reset(new boost::thread(&Roblox::terminateWaiter));
    
	theInstance = instance;
    
	callback = shared_ptr<RBX::TeleportCallback>(new Teleporter(instance));
	RBX::TeleportService::SetCallback(callback.get());
	RBX::TeleportService::SetBaseUrl(::GetBaseURL().c_str());
	
	RBX::Log::setLogProvider(&logProvider);
	
	if(globalInit(isApp))
    {
        [CMCrashReporter check];
        return true;
    }
    
    return false;
}

void Roblox::releaseTerminateWaiter()
{
	if (uniqSemaphore != NULL)
	{
		pid_t id = getpid();
		NSLog(@"Roblox::shutdownInstance - pid = %i, thread is still waiting, signaling semaphore", id);
		
		needTerminateCall = false;
		sem_post(uniqSemaphore);
	}
}

void Roblox::shutdownInstance()
{
	releaseTerminateWaiter();
	
	globalShutdown();
	
	theInstance = NULL;	
}

// FunctionMarshaller help - posts worker thread message to UI thread and deals with it next time through
void Roblox::sendAppEvent(void *pClosure)
{
	NSGraphicsContext *context = nil;
	NSInteger windowNumber = 0;
	NSPoint location;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSEvent *ev = [[NSEvent otherEventWithType:NSApplicationDefined location:location modifierFlags:0 timestamp:0 windowNumber:windowNumber context:context 
									   subtype:0 data1:(NSInteger)pClosure data2:0] retain];
	

	RBX::CEvent *waitEvent = ((RBX::FunctionMarshaller::Closure *) pClosure)->waitEvent;
	BOOL waitFlag = (waitEvent == NULL);
			
	// TBD: Make sure the marshallFunction: selector actually exists!
	NSResponder *responder = (NSResponder *) theInstance;
	[responder performSelectorOnMainThread:@selector(marshallFunction:) withObject:ev waitUntilDone:waitFlag];	
	
	if (waitEvent)
	{
		waitEvent->Wait();
	}
	
	[pool release];
}

void Roblox::postAppEvent(void *pClosure)
{
	NSGraphicsContext *context = nil;
	NSInteger windowNumber = 0;
	NSPoint location;
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSEvent *ev = [[NSEvent otherEventWithType:NSApplicationDefined location:location modifierFlags:0 timestamp:0 windowNumber:windowNumber context:context 
									   subtype:0 data1:(NSInteger)pClosure data2:0] retain];
	
	
	// TBD: Make sure the marshallFunction: selector actually exists!
	NSResponder *responder = (NSResponder *) theInstance;
	[responder performSelectorOnMainThread:@selector(marshallFunction:) withObject:ev waitUntilDone:NO];	
	
	[pool release];
}

void Roblox::processAppEvents()
{
    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) == kCFRunLoopRunHandledSource)
        ;
}

void Roblox::handleLeaveGame(void *appWindow)
{
    RobloxPlayerAppDelegate *delegate = (RobloxPlayerAppDelegate *) appWindow;
    [delegate handleLeaveGame];
}

void Roblox::handleShutdownClient(void *appWindow)
{
    RobloxPlayerAppDelegate *delegate = (RobloxPlayerAppDelegate *) appWindow;
    [delegate handleShutdownClient];
}

void Roblox::handleToggleFullScreen(void *appWindow)
{
    RobloxPlayerAppDelegate *delegate = (RobloxPlayerAppDelegate *) appWindow;
    [delegate handleToggleFullScreen];
}

bool Roblox::inFullScreenMode(void *appWindow)
{
    RobloxPlayerAppDelegate *delegate = (RobloxPlayerAppDelegate *) appWindow;
        
    bool fullscreen = [delegate inFullScreenMode];
    return fullscreen;
}
