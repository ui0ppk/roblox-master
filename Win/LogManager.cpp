/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#undef min
#undef max


#include "LogManager.h"
#include "RbxFormat.h"
#include "rbx/Debug.h"
#include "rbx/boost.hpp"
#include "util/StandardOut.h"
#include "util/FileSystem.h"
#include "util/Guid.h"
#include "util/Http.h"
#include "util/Statistics.h"

#include "G3D/debugAssert.h"
#include <direct.h>

#include "atltime.h"
#include "atlfile.h"

#include "versioninfo.h"
#include "rbx/TaskScheduler.h"
#include "VistaTools.h"
#include "DumpErrorUploader.h"
#include "rbx/Log.h"
#include "FastLog.h"

LOGGROUP(CrashReporterInit)

bool LogManager::logsEnabled = true;

MainLogManager* LogManager::mainLogManager = NULL;

RBX::mutex MainLogManager::fastLogChannelsLock;

#pragma comment(lib, "shell32.lib")

static const ATL::CPath& DoGetPath()
{
    // DO NOT REMOVE the CString conversion, this is to preserve the old behavior of losing unicodeness.
	static ATL::CPath path = CString( RBX::FileSystem::getUserDirectory(true, RBX::DirAppData, "logs").native().c_str() );
	return path;
}

void InitPath()
{
	DoGetPath();
}

std::string GetAppVersion()
{
	CVersionInfo vi;
	FASTLOG1(FLog::CrashReporterInit, "Getting app version, module handle: %p", _AtlBaseModule.m_hInst);
	vi.Load(_AtlBaseModule.m_hInst);
	return vi.GetFileVersionAsString();
}

const ATL::CPath& LogManager::GetLogPath() const
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&InitPath, flag);
	return DoGetPath();
}

void MainLogManager::fastLogMessage(FLog::Channel id, const char* message)
{
	RBX::mutex::scoped_lock lock(fastLogChannelsLock);

	if(mainLogManager)
	{
		if(id >= mainLogManager->fastLogChannels.size())
			mainLogManager->fastLogChannels.resize(id+1, NULL);

		if(mainLogManager->fastLogChannels[id] == NULL)
		{

			mainLogManager->fastLogChannels[id] = new RBX::Log(mainLogManager->getFastLogFileName(id).c_str(), "Log Channel");
		}

		mainLogManager->fastLogChannels[id]->writeEntry(RBX::Log::Information, message);
	}
}

std::string MainLogManager::getSessionId()
{
	std::string id = guid;
	// trim part of the guid for readability
	id.erase(8);
	id.erase(0,3);
	return id;
}

std::string MainLogManager::getCrashEventName()
{
	FASTLOG(FLog::CrashReporterInit, "Getting crash event name");
	ATL::CPath path = GetLogPath();

	std::string fileName = "log_";
	fileName += getSessionId();
	fileName += " ";

	fileName += GetAppVersion();
	fileName += crashEventExtention;

	path.Append(fileName.c_str());

	return (LPCTSTR)path;
}

std::string MainLogManager::getLogFileName()
{
	ATL::CPath path = GetLogPath();

	std::string fileName = "log_";
	fileName += getSessionId();
	fileName += ".txt";

	path.Append(fileName.c_str());

	return (LPCTSTR) path;
}

std::string MainLogManager::getFastLogFileName(FLog::Channel channelId)
{
	ATL::CPath path = GetLogPath();

	CString filename;
	filename.Format("log_%s_%d.txt", getSessionId().c_str(), channelId);

	path.Append(filename);

	return (LPCTSTR) path;
}


std::string MainLogManager::MakeLogFileName(const char* postfix)
{
	ATL::CPath path = GetLogPath();

	std::string fileName = "log_";
	fileName += getSessionId();
	fileName += postfix;
	fileName += ".txt";

	path.Append(fileName.c_str());

	return (LPCTSTR) path;
}

std::string ThreadLogManager::getLogFileName()
{
	std::string fileName = mainLogManager->getLogFileName();
	CString id;
	id.Format("_%s_%d", name.c_str(), threadID);
	fileName.insert(fileName.size()-4, id);
	return fileName;
}

RBX::Log* LogManager::getLog()
{
	if (!logsEnabled)
		return NULL;
	if (log==NULL)
	{
		log = new RBX::Log(getLogFileName().c_str(), name.c_str());
		// TODO: delete an old log that isn't in use
	}
	return log;
}


RBX::Log* MainLogManager::provideLog()
{
	if (GetCurrentThreadId()==threadID)
		return this->getLog();
	
	return ThreadLogManager::getCurrent()->getLog();
}


#include < process.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define MAX_CONSOLE_LINES 250;

HANDLE  g_hConsoleOut;                   // Handle to debug console



RobloxCrashReporter::RobloxCrashReporter(const char* outputPath, const char* appName, const char* crashExtention)
{
	controls.minidumpType=MiniDumpWithDataSegs;
	
	if(IsVistaPlus())
		controls.minidumpType |= MiniDumpWithIndirectlyReferencedMemory;
	
	strcpy(controls.pathToMinidump, outputPath);

	strcpy(controls.appName, appName);

	strncpy(controls.appVersion, GetAppVersion().c_str(), 128);
	strncpy(controls.crashExtention, crashExtention, sizeof(controls.crashExtention));
}

bool RobloxCrashReporter::silent;

LONG RobloxCrashReporter::ProcessException(struct _EXCEPTION_POINTERS *info, bool noMsg)
{
	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "StartProcessException...");

	LONG result = __super::ProcessException(info, noMsg);
	static bool showedMessage = silent;
	if (!showedMessage && !noMsg)
	{
		showedMessage = true;
		::MessageBox( NULL, "An unexpected error occurred and ROBLOX needs to quit.  We're sorry!", "ROBLOX Crash", MB_OK );
	}

	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "DoneProcessException");

	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Uploading .crashevent...");
	DumpErrorUploader::UploadCrashEventFile(info);
	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, "Done uploading .crashevent...");

	return result;	
}

void RobloxCrashReporter::logEvent(const char* msg)
{
	LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, msg);
}

void MainLogManager::WriteCrashDump()
{
	std::string appName = "log_";
	appName += getSessionId();
	crashReporter.reset(new RobloxCrashReporter((LPCTSTR) GetLogPath(), appName.c_str(), crashExtention));
	crashReporter->Start();
	CString eventMessage;
	eventMessage.Format("CrashReporter Start");
	RBX::Log::current()->writeEntry(RBX::Log::Information, eventMessage);
};

bool MainLogManager::CreateFakeCrashDump()  
{
	if(!crashReporter)
	{
		// start the service if not started.
		WriteCrashDump();
	}

	// First, write FastLog
	char dumpFilepath[_MAX_PATH];
	if(FAILED(crashReporter->GenerateDmpFileName(dumpFilepath, _MAX_PATH, true)))
	{
		return false;
	}

	FLog::WriteFastLogDump(dumpFilepath, 2000);

	if(FAILED(crashReporter->GenerateDmpFileName(dumpFilepath, _MAX_PATH)))
	{
		return false;
	}

	HANDLE hFile = CreateFile(dumpFilepath,GENERIC_WRITE, FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD cb;
	WriteFile(hFile, "Fake", 5, &cb, NULL);

	CloseHandle(hFile);
	return true;
}

void MainLogManager::EnableImmediateCrashUpload(bool enabled)
{
	if(crashReporter)
	{
		crashReporter->EnableImmediateUpload(enabled);
	}
}

void MainLogManager::DisableHangReporting()
{
	if(crashReporter)
	{
		crashReporter->DisableHangReporting();
	}
}

void MainLogManager::NotifyFGThreadAlive()
{
	if(crashReporter)
	{
#if 0
		//  for debugging only:
		static int alivecount = 0;
		if(alivecount++ % 60 == 0)
		{
			CString eventMessage;
			eventMessage.Format("FGAlive %d", alivecount);
			LogManager::ReportEvent(EVENTLOG_INFORMATION_TYPE, eventMessage);
		}
#endif

		crashReporter->NotifyAlive();
	}
}


static void purecallHandler(void)
{
	CString eventMessage;
	eventMessage.Format("Pure Call Error");
	LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, eventMessage);
#ifdef _DEBUG
	_CrtDbgBreak();
#endif
	// Cause a crash
	RBXCRASH();
}

MainLogManager::MainLogManager(LPCTSTR productName, const char* crashExtention, const char* crashEventExtention)
	:LogManager(productName),
	crashExtention(crashExtention),
	crashEventExtention(crashEventExtention),
	gameState(MainLogManager::GameState::UN_INITIALIZED)
{
	RBX::Guid::generateRBXGUID(guid);
	CullLogs("\\", 1024);
	CullLogs("archive\\", 1024);

	RBXASSERT(mainLogManager == NULL);
	mainLogManager = this;

	RBX::Log::setLogProvider(this);

	RBX::setAssertionHook(&MainLogManager::handleDebugAssert);
	RBX::setFailureHook(&MainLogManager::handleFailure);

   _set_purecall_handler(purecallHandler);

   FLog::SetExternalLogFunc(fastLogMessage);
}

MainLogManager* LogManager::getMainLogManager() {
	return mainLogManager;
}


ThreadLogManager::ThreadLogManager()
	:LogManager(RBX::get_thread_name())
{
}

ThreadLogManager::~ThreadLogManager()
{
}

static float getThisYearTimeInMinutes(SYSTEMTIME time)
{
	return (time.wMonth * 43829.0639f) + (time.wDay * 1440) + (time.wHour * 60) + time.wMinute;
}

std::vector<std::string> MainLogManager::getRecentCseFiles()
{
	std::vector<std::string> archiveResult;

	ATL::CPath archivePath = GetLogPath();
	archivePath.AddBackslash();
	archivePath.Append("archive//");

	WIN32_FIND_DATA FindArchiveData;
	HANDLE archiveCseFind = FindFirstFile(archivePath + "*.cse", &FindArchiveData);

	SYSTEMTIME curTime;
	::GetSystemTime(&curTime);
	float curTimeInMinutes = getThisYearTimeInMinutes(curTime);

	if(archiveCseFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindArchiveData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			{
				SYSTEMTIME fileTime;
				// get our filetime synced up to UTC
				::FileTimeToSystemTime(&FindArchiveData.ftLastWriteTime,&fileTime);
				
				// we check to see if we have uploaded this error (or a similar one) within the last hour
				// if we have, we need to check this file against any new cse files, in case we are uploading
				// a similar error within an hour of doing so previously
				float minuteDiff = curTimeInMinutes - getThisYearTimeInMinutes(fileTime);
				if((curTime.wYear == fileTime.wYear && minuteDiff <= 60) || curTime.wYear > fileTime.wYear)
				{
					std::string archiveFilename = FindArchiveData.cFileName;
					archiveFilename = archiveFilename.substr(0,archiveFilename.size() - 9); // get rid of sessionId, file extension (don't need these for comparison)
					archiveResult.push_back((LPCTSTR)(archiveFilename.c_str()));
				}
			}
		}
		while (FindNextFile(archiveCseFind, &FindArchiveData));
	}

	return archiveResult;

}

std::vector<std::string> MainLogManager::gatherScriptCrashLogs()
{
	
	std::vector<std::string> result;

	ATL::CPath path = GetLogPath();
	path.AddBackslash();
	
	WIN32_FIND_DATA FindCseData;
	HANDLE cseFind = FindFirstFile(path + "*.cse", &FindCseData);

	std::vector<std::string> archiveResult = getRecentCseFiles();

	// look for cse files. cse files are simply logs of core script errors
	if (cseFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindCseData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			{
				std::string filename = FindCseData.cFileName;
				bool deleted = false;

				for(std::vector<std::string>::iterator iter = archiveResult.begin(); iter != archiveResult.end() && !deleted; ++iter)
				{
					// we have uploaded this error recently, just delete this log (we will eventually reupload this anyway if it reoccurs)
					if(filename.find((*iter)) != std::string::npos)
					{
						::DeleteFile(path + filename.c_str());
						deleted = true;
					}
				}

				if (!deleted)
				{
					// attach our sessionId number to the filename (this won't be the same sessionId as when this occurred, but helps with uniquing script errors)
					filename = filename.substr(0,filename.size() - 4);
					std::string id = MainLogManager::getMainLogManager()->getSessionId();
					filename.append(MainLogManager::getMainLogManager()->getSessionId()).append(".cse");
					rename(path + FindCseData.cFileName,path + filename.c_str());
					result.push_back((LPCTSTR)(path + filename.c_str()));
				}
			}
		}
		while (FindNextFile(cseFind, &FindCseData));
	}
	return result;
}

// Gather all dmp files (and associated log files)
std::vector<std::string> MainLogManager::gatherCrashLogs()
{
	std::vector<std::string> result;
	ATL::CPath path = GetLogPath();
	path.AddBackslash();

	WIN32_FIND_DATA FindDmpData;
	HANDLE hFind = FindFirstFile(path + "*.dmp", &FindDmpData);

	// look for regular dmp files
	if (hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindDmpData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			{
				// archive our dump file now.
				result.push_back((LPCTSTR)(path + FindDmpData.cFileName));

				// We got a dmp file. archive associated log files
				std::string wildCard = FindDmpData.cFileName;
				wildCard = wildCard.substr(0, 9) + "*.*";
				std::vector<std::string> logs = gatherAssociatedLogs(wildCard);
				std::copy(logs.begin(), logs.end(), std::back_inserter(result));

			}
		}
		while (FindNextFile(hFind, &FindDmpData));
	}

	return result;
}

// Gather all files associated with a sessionId
std::vector<std::string> MainLogManager::gatherAssociatedLogs(const std::string& filenamepattern)
{
	std::vector<std::string> result;
	ATL::CPath path = GetLogPath();
	path.AddBackslash();
				
	// We got a dmp file. Now find associated log files
	WIN32_FIND_DATA FindOtherFileData;
	HANDLE hFindOther = FindFirstFile(path + filenamepattern.c_str(), &FindOtherFileData);
	if (hFindOther!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindOtherFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			{
				if(std::string(FindOtherFileData.cFileName).find(".dmp") == std::string::npos)
					result.push_back((LPCTSTR)(path + FindOtherFileData.cFileName)); 
			}
		}
		while (FindNextFile(hFindOther, &FindOtherFileData));
	}
	return result;
}

bool MainLogManager::hasCrashLogs(std::string extension) const
{
	ATL::CPath path = GetLogPath();
	path.AddBackslash();
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(path + "*" + extension.c_str(), &FindFileData);
	if(hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
				return true;
		}
		while (FindNextFile(hFind, &FindFileData));
	}
	return false;
}

void MainLogManager::CullLogs(const char* folder, int filesRemaining)
{
	ATL::CPath path = GetLogPath();
	path.Append(folder);
	path.AddBackslash();

	std::list<WIN32_FIND_DATA> files;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(path + "*.*", &FindFileData);
	if (hFind!=INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			{
				CString f = FindFileData.cFileName;
				files.push_back(FindFileData);
			}
		}
		while (FindNextFile(hFind, &FindFileData));
	}

	struct olderthan
	{
		bool operator()(const WIN32_FIND_DATA& lhs, const WIN32_FIND_DATA& rhs)
		{
			if(lhs.ftCreationTime.dwHighDateTime == rhs.ftCreationTime.dwHighDateTime)
				return lhs.ftCreationTime.dwLowDateTime < rhs.ftCreationTime.dwLowDateTime;

			return lhs.ftCreationTime.dwHighDateTime < rhs.ftCreationTime.dwHighDateTime;
		}
	};

	files.sort(olderthan());

	std::list<WIN32_FIND_DATA>::iterator iter = files.begin();

	for (size_t count = filesRemaining; count<files.size(); iter++, count++)
	{
		::DeleteFile(path + (*iter).cFileName);
	}
}


MainLogManager::~MainLogManager()
{
	RBX::mutex::scoped_lock lock(fastLogChannelsLock);

	FLog::SetExternalLogFunc(NULL);

	for(std::size_t i = 0; i < fastLogChannels.size(); i++)
		delete fastLogChannels[i];

	mainLogManager = NULL;
}


LogManager::~LogManager()
{
    if (log != NULL)
	{
		std::string logFile = log->logFile;
        delete log;	// this will close the file so that we can move it
		log = NULL;
	}

}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, LPCSTR lpszDesc,
	DWORD dwHelpID, LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	ATLASSERT(lpszDesc != NULL);
	if (lpszDesc == NULL)
		return E_POINTER;
	USES_CONVERSION_EX;
	CComBSTR desc = CString(lpszDesc);
	if(desc == NULL)
		return E_OUTOFMEMORY;
	
	CComBSTR helpFile = NULL;
	if(lpszHelpFile != NULL)
	{
		helpFile = CString(lpszHelpFile);
		if(helpFile == NULL)
			return E_OUTOFMEMORY;
	}
		
	return AtlSetErrorInfo(clsid, desc.Detach(), dwHelpID, helpFile.Detach(), iid, hRes, NULL);
}


inline HRESULT WINAPI RbxReportError(const CLSID& clsid, UINT nID, const IID& iid = GUID_NULL,
	HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), 0, NULL, iid, hRes, hInst);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, UINT nID, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0, 
	HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), dwHelpID,
		lpszHelpFile, iid, hRes, hInst);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, LPCSTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return RbxReportError(clsid, lpszDesc, 0, NULL, iid, hRes);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlSetErrorInfo(clsid, lpszDesc, 0, NULL, iid, hRes, NULL);
}

inline HRESULT WINAPI RbxReportError(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlSetErrorInfo(clsid, lpszDesc, dwHelpID, lpszHelpFile, iid, hRes, NULL);
}

HRESULT LogManager::ReportCOMError(const CLSID& clsid, LPCOLESTR lpszDesc, HRESULT hRes)
{
	LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, CString(lpszDesc));
	return RbxReportError(clsid, lpszDesc, GUID_NULL, hRes);
}

HRESULT LogManager::ReportCOMError(const CLSID& clsid, LPCSTR lpszDesc, HRESULT hRes)
{
	LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, lpszDesc);
	return RbxReportError(clsid, lpszDesc, GUID_NULL, hRes);
}

HRESULT LogManager::ReportCOMError(const CLSID& clsid, HRESULT hRes)
{
	std::string message = RBX::format("HRESULT 0x%X", hRes);
	LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, message.c_str());
	return RbxReportError(clsid, message.c_str(), GUID_NULL, hRes);
}

#ifdef _MFC_VER
HRESULT LogManager::ReportCOMError(const CLSID& clsid, CException* exception)
{
	CString fullError;
	HRESULT hr = COleException::Process(exception);
	CString sError;
	if (exception->GetErrorMessage(sError.GetBuffer(1024), 1023))
	{
		sError.ReleaseBuffer();
		fullError.Format("%s (0x%X)", sError, hr);
	}
	else
		fullError.Format("Error 0x%X", hr);

	LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, fullError);
	return RbxReportError(clsid, fullError, GUID_NULL, hr);
}
#endif

bool MainLogManager::handleG3DDebugAssert(
    const char* _expression,
    const std::string& message,
    const char* filename,
    int lineNumber,   
    bool useGuiPrompt)
{
	return handleDebugAssert(_expression,filename,lineNumber);
}

bool MainLogManager::handleDebugAssert(
	const char* expression,
	const char* filename,
	int         lineNumber
)
{
	CString eventMessage;
	eventMessage.Format("Assertion failed: %s\n%s(%d)", expression, filename, lineNumber);
	LogManager::ReportEvent(EVENTLOG_WARNING_TYPE, eventMessage);
#ifdef _DEBUG
	RBXCRASH();
	return true;
#else
	return false;
#endif

}


bool MainLogManager::handleG3DFailure(
    const char* _expression,
    const std::string& message,
    const char* filename,
    int lineNumber,
    bool useGuiPrompt)
{
	return handleFailure(_expression, filename, lineNumber);
}
bool MainLogManager::handleFailure(
	const char* expression,
	const char* filename,
	int         lineNumber
)
{
	CString eventMessage;
	eventMessage.Format("G3D Error: %s\n%s(%d)", expression, filename, lineNumber);
	LogManager::ReportEvent(EVENTLOG_ERROR_TYPE, eventMessage);
#ifdef _DEBUG
	_CrtDbgBreak();
#endif
	// Cause a crash
	RBXCRASH(); 
	return false;
}

HRESULT LogManager::ReportExceptionAsCOMError(const CLSID& clsid, std::exception const& exp)
{
	return ReportCOMError(clsid, exp.what());
}

void LogManager::ReportException(std::exception const& exp)
{
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, exp);
}

void LogManager::ReportLastError(LPCSTR message)
{
	DWORD error = GetLastError();
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "%s, GetLastError=%d", message, error);
}

void LogManager::ReportEvent(WORD type, LPCSTR message)
{
	switch (type)
	{
	case EVENTLOG_SUCCESS:
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "%s", message);
		break;
	case EVENTLOG_ERROR_TYPE:
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "%s", message);
		break;
	case EVENTLOG_INFORMATION_TYPE:
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "%s", message);
		break;
	case EVENTLOG_AUDIT_SUCCESS:
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "%s", message);
		break;
	case EVENTLOG_AUDIT_FAILURE:
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "%s", message);
		break;
	}
#ifdef _DEBUG
	switch (type)
	{
	case EVENTLOG_SUCCESS:
		ATLTRACE("EVENTLOG_SUCCESS %s\n", message);
		break;
	case EVENTLOG_ERROR_TYPE:
		ATLTRACE("EVENTLOG_ERROR_TYPE %s\n", message);
		break;
	case EVENTLOG_INFORMATION_TYPE:
		ATLTRACE("EVENTLOG_INFORMATION_TYPE %s\n", message);
		break;
	case EVENTLOG_AUDIT_SUCCESS:
		ATLTRACE("EVENTLOG_AUDIT_SUCCESS %s\n", message);
		break;
	case EVENTLOG_AUDIT_FAILURE:
		ATLTRACE("EVENTLOG_AUDIT_FAILURE %s\n", message);
		break;
	}
#endif
}

void LogManager::ReportEvent(WORD type, LPCSTR message, LPCSTR fileName, int lineNumber)
{
	CString m;
	m.Format("%s\n%s(%d)", message, fileName, lineNumber);
	LogManager::ReportEvent(type, m);
}

#ifdef _MFC_VER
void LogManager::ReportEvent(WORD type, HRESULT hr, LPCSTR fileName, int lineNumber)
{
	COleException e;
	e.m_sc = hr;
	TCHAR s[1024];
	e.GetErrorMessage(s, 1024);

	CString m;
	m.Format("HRESULT = %d: %s\n%s(%d)", hr, s, fileName, lineNumber);
	LogManager::ReportEvent(type, m);
}

#endif

namespace log_detail
{
    boost::once_flag once_init = BOOST_ONCE_INIT;
	static boost::thread_specific_ptr<ThreadLogManager>* ts;
    void init(void)
    {
		static boost::thread_specific_ptr<ThreadLogManager> value;
		ts = &value;
    }
}

ThreadLogManager* ThreadLogManager::getCurrent()
{
	boost::call_once(log_detail::init, log_detail::once_init);
	ThreadLogManager* logManager = log_detail::ts->get(); 
	if (!logManager)
	{
		logManager = new ThreadLogManager();		
		log_detail::ts->reset(logManager);
	}
	return logManager;		
}
