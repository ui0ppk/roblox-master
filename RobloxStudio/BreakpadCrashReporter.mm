//
//  Crash.mm
//  RobloxStudio
//
//  Created by agrawal on 11/16/11.
//  Copyright 2011 Personal. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h> 

#include <QString>

#include "BreakpadCrashReporter.h"
#include "breakpad.h"
#include "RobloxSettings.h"
#include "RobloxServicesTools.h"

#include "util/Guid.h"
#include "util/FileSystem.h"
#include "v8datamodel/Stats.h"
#include "rbx/RbxDbgInfo.h"


BreakpadRef CrashReporter::breakpad = NULL;
char* CrashReporter::context = NULL;

DYNAMIC_FASTINTVARIABLE(MacInfluxHundredthsPercentage, 1000)

void CrashReporter::setupCrashReporter()
{
	if(breakpad != NULL) 
		return; 
	NSAutoreleasePool *pool  = [[NSAutoreleasePool alloc] init]; 
    NSMutableDictionary *plist = [[NSMutableDictionary alloc] init];
	[plist addEntriesFromDictionary:[[NSBundle mainBundle] infoDictionary]];
    NSString* breakpadUrl = [NSString stringWithUTF8String:GetBreakpadUrl(RobloxSettings::getBaseURL().toStdString(), true).c_str()];
	[plist setValue:breakpadUrl forKey:@BREAKPAD_URL];
    
    // Out-of-process breakpad is broken on 10.10 and newer
    [plist setValue:[NSNumber numberWithBool:TRUE] forKey:@BREAKPAD_IN_PROCESS];
    [plist setValue:NSTemporaryDirectory() forKey:@BREAKPAD_DUMP_DIRECTORY];

	breakpad = BreakpadCreate(plist);
	
	initBreakpadCallback(breakpad);
	
	addBreakpadKeyValue("RobloxProduct", "RobloxStudio");
	
	NSString* version = [plist objectForKey:@"CFBundleShortVersionString"];
	BreakpadAddUploadParameter(breakpad, @"Version", version);
	
	NSString* session = [NSString stringWithFormat:@"%s", RBX::Guid().readableString(6).substr(0,5).c_str()];
	BreakpadAddUploadParameter(breakpad, @"Session", session);
	
	addBreakpadKeyValue("Platform", "Mac");
	
	addDebugInfoToBreakpad();
	
	[pool release];
}

void CrashReporter::addBreakpadLog(const char* file)
{
	if(breakpad == NULL) 
		return; 
	
	NSString* s = [NSString stringWithFormat:@"%s", file];
	BreakpadAddLogFile(breakpad, s);
}

void CrashReporter::addBreakpadKeyValue(const char* key, const char* value)
{
	if(breakpad == NULL) 
		return; 
	
	NSString* sKey = [NSString stringWithFormat:@"%s", key];
	NSString* sVal = [NSString stringWithFormat:@"%s", value];
	
	BreakpadAddUploadParameter(breakpad, sKey, sVal);
}

void CrashReporter::shutDownCrashReporter()
{
	if(breakpad == NULL)
		return; 
	
	// This will generate the symbol dump every time we quit
	// If you need this then just enable the following line
	// BreakpadGenerateAndSendReport(breakpad);
	
	BreakpadRelease(breakpad); 
	breakpad = NULL; 
}

void CrashReporter::addDebugInfoToBreakpad()
{
	char result[64];
	sprintf(result, "%lu", RBX::RbxDbgInfo::s_instance.cPhysicalTotal);
	addBreakpadKeyValue("cPhysicalTotal", result);	
	
	sprintf(result, "%lu", RBX::RbxDbgInfo::s_instance.cbPageSize);
	addBreakpadKeyValue("cbPageSize", result);
	
	sprintf(result, "%lu", RBX::RbxDbgInfo::s_instance.TotalVideoMemory);
	addBreakpadKeyValue("TotalVideoMemory", result);
	
	sprintf(result, "%lu", RBX::RbxDbgInfo::s_instance.NumCores);
	addBreakpadKeyValue("NumCores", result);
	
	addBreakpadKeyValue("GfxCardName", RBX::RbxDbgInfo::s_instance.GfxCardName);
	addBreakpadKeyValue("GfxCardDriverVersion", RBX::RbxDbgInfo::s_instance.GfxCardDriverVersion);
	addBreakpadKeyValue("GfxCardVendorName", RBX::RbxDbgInfo::s_instance.GfxCardVendorName);
	addBreakpadKeyValue("AudioDeviceName", RBX::RbxDbgInfo::s_instance.AudioDeviceName);

}

extern "C" void writeFastLogDumpHelper(const char *fileName, int numEntries);

//After the crash happens this will be called from Breakpad where we will ask the FastLog to dump teh log data to a file provided as context
bool BreakpadCallback(int exception_type, int exception_code, mach_port_t crashing_thread, void *context)
{
    RBX::Analytics::InfluxDb::Points points;
    points.addPoint("SessionReport" , "MacROBLOXStudioCrash");
    points.report("Mac-RobloxStudio-SessionReport", DFInt::MacInfluxHundredthsPercentage);
    
    // FIXME: https://jira.roblox.com/browse/CLI-10561
    // These calls are not safe to be called in crash handling context:
    // 1. They allocate
    // 2. There's a code path that leads to interacting with dead NSRunLoop, thus freezing crash generation
    RBX::Analytics::EphemeralCounter::reportCounter("Mac-ROBLOXStudio-Crash", 1, true);
    RBX::Analytics::EphemeralCounter::reportCounter("ROBLOXStudio-Crash", 1, true);
    
	if (context)
		writeFastLogDumpHelper((const char *)context, 200);

	return true;
}

void CrashReporter::initBreakpadCallback(BreakpadRef breakpad)
{
	//get the log file
    boost::filesystem::path logFile = RBX::FileSystem::getLogsDirectory();
    
    std::string guid;
    RBX::Guid::generateRBXGUID(guid);
    logFile /= "log_" + guid.substr(3,6) + ".txt";
	
	
	//we will set the log file as context so can get it back (DO NOT delete)
	context = new char[strlen(logFile.c_str())+1];
	strcpy(context, logFile.c_str());
	
	// Give Fast Log a chance to put the FastLog into a file so that breakpad can pick it up
	BreakpadSetFilterCallback(breakpad, BreakpadCallback, (void *)context); 
	
	//add log file with Breakpad
	BreakpadAddLogFile(breakpad, [NSString stringWithFormat:@"%s", logFile.c_str()]);
}

void CrashReporter::CreateLogDump()
{
	// Dump the FastLog into Logs
	writeFastLogDumpHelper(context, 200);
}