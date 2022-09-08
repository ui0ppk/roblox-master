// RCCService.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"


#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "conio.h"

#include "gSOAP/generated/RCCServiceSoap.nsmap"
#include "gSOAP/generated/soapRCCServiceSoapService.h"
#include "logmanager.h"
#include "rbx/boost.hpp"
#include "Util/WinHeap.h"
#include "Util/StandardOut.h"
#include "rbx/TaskScheduler.h"
#include "Util/Statistics.h"
#include "util/Http.h"
#include "v8datamodel/datamodel.h"
#include "OperationalSecurity.h"

#pragma optimize( "", off ) 

static long requestCount = 0;
extern long diagCount;
extern long batchJobCount;
extern long openJobCount;
extern long closeJobCount;
extern long helloWorldCount;
extern long getVersionCount;
extern long renewLeaseCount;
extern long executeCount;
extern long getExpirationCount;
extern long getStatusCount;
extern long getAllJobsCount;
extern long closeExpiredJobsCount;
extern long closeAllJobsCount;

#ifdef RBX_TEST_BUILD
extern std::string RCCServiceSettingsKeyOverwrite;
#endif

void process_request(RCCServiceSoapService *service) 
{ 
	::InterlockedIncrement(&requestCount);
	try
	{
		service->serve(); 
		delete service;
	}
	catch (...)
	{
		RBXCRASH();
	}
	::InterlockedDecrement(&requestCount);
} 

static void StringCrash(const char* s)
{
	char str[256];
	strncpy_s(str, 256, s, 256);
	RBXCRASH();
}

template <class Soap>
class ExceptionAwareSoap : public Soap
{
public:
	virtual	int dispatch()
	{
		try
		{
			//printf("%s\n", action); 
			return Soap::dispatch();
		}
		catch (std::exception& e)
		{
			return soap_receiver_fault(this, e.what(), NULL); // return fault to sender 
		}
		catch (std::string& s)
		{
			StringCrash(s.c_str());
			return soap_receiver_fault(this, s.c_str(), NULL); // return fault to sender 
		}
		catch (...)
		{
			RBXCRASH();
			return soap_receiver_fault(this, "Unexpected C++ exception type", NULL); // return fault to sender 
		}
	}
	virtual	RCCServiceSoapService *copy()
	{
		ExceptionAwareSoap<Soap> *dup = new ExceptionAwareSoap<Soap>();
		soap_copy_context(dup, this);
		return dup;	
	}
};


ExceptionAwareSoap<RCCServiceSoapService> service; 

#define SVCNAME TEXT("RCCService")


// http://msdn2.microsoft.com/en-us/library/ms687416(VS.85).aspx
//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(WORD type, LPCTSTR szFunction) 
{ 
	HANDLE hEventSource = ::RegisterEventSource(NULL, SVCNAME);

    if( NULL != hEventSource )
    {
	LPCTSTR* lpStrings = &szFunction;

	ReportEvent(hEventSource,		// event log source handle
						 type,				// event type to log
						 0,					// event category
						 0x20000001L,		// event identifier (GENERIC_MESSAGE) which comes from Message.mc
						 0,				// user security identifier (optional)
						 1,					// number of strings to merge with message
						 0,					// size of binary data, in bytes
						 lpStrings,			// array of strings to merge with message
						 NULL);				// address of binary data

        DeregisterEventSource(hEventSource);
    }
}


void start_CWebService(LPCTSTR contentpath, bool crashUploaderOnly);

static void startupRCC(int port, LPCTSTR contentpath, bool crashUploaderOnly)
{
	printf("Service starting...\n"); 

	start_CWebService(contentpath, crashUploaderOnly);

  //service.send_timeout = 60; // 60 seconds 
  //service.recv_timeout = 60; // 60 seconds 
  service.accept_timeout = 1; // server stops after 1 second
  //soap.max_keep_alive = 100; // max keep-alive sequence 
  SOAP_SOCKET m = service.bind(NULL, port, 100); 
  if (!soap_valid_socket(m)) 
	  throw std::runtime_error(*soap_faultstring(&service)); 

	char buffer[64];
	sprintf_s(buffer, 64, "Service Started on port %d", port); 
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_SENSITIVE, buffer);
	SvcReportEvent(EVENTLOG_INFORMATION_TYPE, buffer);
}

DWORD CALLBACK process_request_func(LPVOID param) 
{
	process_request((RCCServiceSoapService*) param);
	return 0;
}

static void stepRCC()
{
	if (requestCount>100)
		throw std::runtime_error(RBX::format("%d outstanding requests, execute=%d, openJob=%d, batchJob=%d, diag=%d, getVersion=%d, renewLease=%d, getAllJobs=%d, getStatusCount=%d, closeExpiredJobs=%d, closeJob=%d, helloWorld=%d, getExpiration=%d, closeAllJobs=%d", 
								 requestCount, executeCount, openJobCount, batchJobCount, diagCount, getVersionCount, renewLeaseCount, getAllJobsCount, getStatusCount, closeExpiredJobsCount, closeJobCount, helloWorldCount, getExpirationCount, closeAllJobsCount));

	SOAP_SOCKET s = service.accept(); 
	if (!soap_valid_socket(s)) 
	{ 
		if (service.errnum)
			throw std::runtime_error(*soap_faultstring(&service)); 
		return;
	} 

	RCCServiceSoapService* copy = service.copy(); // make a safe copy 
	if (!copy) 
		throw std::runtime_error(*soap_faultstring(&service)); 

	if (!QueueUserWorkItem(&process_request_func, copy, WT_EXECUTELONGFUNCTION))
		RBXCRASH();
}

void stop_CWebService();

static void shutdownRCC()
{
	printf("Service shutting down...\n"); 
	stop_CWebService();
}



SERVICE_STATUS_HANDLE handle = 0;

SERVICE_STATUS status = { SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_STOPPED,
                          SERVICE_ACCEPT_STOP };

HANDLE  stopped; // manual, not signaled

void UpdateState(DWORD state)
{
    status.dwCurrentState = state;

    ::SetServiceStatus(handle, &status);
}

static int parsePort(int argc, _TCHAR* argv[])
{
    int port = 64989;
	for (int i = 1; i<argc; ++i)
	{
		if (argv[i][0] == _T('/') || argv[i][0] == _T('-'))
			continue;
		port = atoi(argv[i]); 
	}
	return port;
}

static bool parseBreakRequest(int argc, _TCHAR* argv[])
{
	for (int i = 1; i<argc; ++i)
	{
		if (argv[i][0] == _T('/'))
			argv[i][0] = _T('-');

		if (_tcsicmp(argv[i], _T("-Break")) == 0)
		{
			return true;
		}
	}
	return false;
}

static LPCTSTR parseContent(int argc, _TCHAR* argv[])
{
	const _TCHAR szTag[] = _T("-Content:");
	int cchTagLen = (sizeof(szTag)/sizeof(_TCHAR));
    static const _TCHAR szDefaultValue[] = _T("Content\\");		
	for (int i = 1; i<argc; ++i)
	{
		if (argv[i][0] == _T('/'))
			argv[i][0] = _T('-');

		if (_tcsncicmp(argv[i], szTag, cchTagLen-1) == 0)
		{
			// return the part after the tag.
			return argv[i] + cchTagLen-1;
		}
	}
	return szDefaultValue;
}

static LPCTSTR parse(const _TCHAR szTag[], int tagLen, int argc, _TCHAR* argv[])
{
	static const _TCHAR szDefaultValue[] = _T("");		
	for (int i = 1; i<argc; ++i)
	{
		if (argv[i][0] == _T('/'))
			argv[i][0] = _T('-');

		if (_tcsncicmp(argv[i], szTag, tagLen-1) == 0)
		{
			// return the part after the tag.
			return argv[i] + tagLen-1;
		}
	}
	return szDefaultValue;
}

static int parsePlaceId(int argc, _TCHAR* argv[])
{
	const _TCHAR szTag[] = _T("-PlaceId:");
	LPCTSTR placeId = parse(szTag, sizeof(szTag), argc, argv);
	if (placeId == "")
		return -1;
	return atoi(placeId);
}

#ifdef RBX_TEST_BUILD
static LPCTSTR parseMD5(int argc, _TCHAR* argv[])
{
	const _TCHAR szTag[] = _T("-Md5:");
	return parse(szTag, sizeof(szTag), argc, argv);
}

static LPCTSTR parseSettingsKey(int argc, _TCHAR* argv[])
{
	const _TCHAR szTag[] = _T("-SettingsKey:");
	return parse(szTag, sizeof(szTag), argc, argv);
}
#endif

void WINAPI Handler(DWORD control)
{
    //ASSERT(SERVICE_CONTROL_STOP == control);

    UpdateState(SERVICE_STOP_PENDING);
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "SERVICE_STOP_PENDING");
	SvcReportEvent(EVENTLOG_INFORMATION_TYPE, "SERVICE_STOP_PENDING");

    // Perform shutdown steps here.
	shutdownRCC();

    ::WaitForSingleObject(stopped, 
                          INFINITE);

    UpdateState(SERVICE_STOPPED);
	RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "SERVICE_STOPPED");
	SvcReportEvent(EVENTLOG_INFORMATION_TYPE, "SERVICE_STOPPED");
}

VOID WINAPI ServiceMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
	stopped = CreateEvent( 
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,               // initial state is not signaled
        TEXT("Stopped")  // object name
        ); 

    handle = ::RegisterServiceCtrlHandler(SVCNAME, Handler);
    //ASSERT(0 != handle);

    UpdateState(SERVICE_START_PENDING);

    // Perform any startup steps here.
	try
	{
		int port = parsePort(dwArgc, lpszArgv);
		LPCTSTR contentDir = parseContent(dwArgc, lpszArgv);
		startupRCC(port, contentDir, false);
		UpdateState(SERVICE_RUNNING);
	}
	catch (std::exception& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
		SvcReportEvent(EVENTLOG_ERROR_TYPE, e.what());
	}

	bool breakRequest = parseBreakRequest(dwArgc, lpszArgv);

    while (SERVICE_RUNNING == status.dwCurrentState)
    {
		if (breakRequest)
		{
			::DebugBreak();
			breakRequest = false;
		}

        // Perform main service function here.
		try
		{
			stepRCC();
		}
		catch (std::exception& e)
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
			SvcReportEvent(EVENTLOG_ERROR_TYPE, e.what());
		}
    }

	::SetEvent(stopped);
}


class CServiceHandle
{
	SC_HANDLE handle;
public:
	CServiceHandle(SC_HANDLE handle):handle(handle) {}
	~CServiceHandle() { CloseServiceHandle(handle); }
	operator SC_HANDLE() const { return handle; }
};

bool SvcUninstall(const char* name)
{
    // Get a handle to the SCM database. 
 
    CServiceHandle schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

	CServiceHandle schService = ::OpenService(schSCManager, name, DELETE);
    if (schService == NULL) 
    {
        printf("OpenService failed (%d)\n", GetLastError()); 
        return false;
    }
	bool result = DeleteService(schService) ? true : false;

	return result;
}

void SvcStart(const char* name)
{
    // Get a handle to the SCM database. 
 
    CServiceHandle schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

	CServiceHandle schService = ::OpenService(schSCManager, name, SERVICE_START);
    if (schService == NULL) 
    {
        printf("OpenService failed (%d)\n", GetLastError()); 
        return;
    }
	if (!StartService(schService, NULL, NULL))
        printf("StartService failed (%d)\n", GetLastError()); 
	else
        printf("Service Starting\n"); 
}

void SvcStop(const char* name)
{
    // Get a handle to the SCM database. 
 
    CServiceHandle schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

	CServiceHandle schService = ::OpenService(schSCManager, name, SERVICE_STOP);
    if (schService == NULL) 
    {
        printf("OpenService failed (%d)\n", GetLastError()); 
        return;
    }

	SERVICE_STATUS serviceStatus;
	if (!ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus))
    {
        printf("ControlService failed (%d)\n", GetLastError()); 
    }
}

void EventLongUninstall()
{
	CRegKey key;
	if (key.Open(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application", KEY_WRITE) == ERROR_SUCCESS)
		key.DeleteSubKey("RCCService");
}

void EventLogInstall()
{
	CRegKey key;
	if (key.Create(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\RCCService") != ERROR_SUCCESS)
		throw std::runtime_error("couldn't create RCCService reg key");

    TCHAR szPath[MAX_PATH];

    if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
		throw std::runtime_error("GetModuleFileName failed");

	key.SetStringValue("EventMessageFile", szPath, REG_EXPAND_SZ);
	key.SetDWORDValue("TypesSupported", 0x1f);
}

void SvcInstall()
{
    TCHAR szPath[MAX_PATH];

    if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
		throw std::runtime_error("GetModuleFileName failed");

    // Get a handle to the SCM database. 
 
    CServiceHandle schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_CREATE_SERVICE);  
 
    if (NULL == schSCManager) 
		throw std::runtime_error("OpenSCManager failed");

    // Create the service

    CServiceHandle schService = CreateService( 
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        "Roblox Compute Cloud Service",   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_AUTO_START,        // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
 
    if (schService == NULL) 
    {
		if (GetLastError()==ERROR_SERVICE_EXISTS)
		{
			printf("Service already installed\n"); 
			return;
		}
		throw std::runtime_error("CreateService failed"); 
    }
    else 
		printf("Service installed successfully\n"); 

	 SERVICE_FAILURE_ACTIONS fa;
     fa.dwResetPeriod = 100;
     fa.lpRebootMsg = NULL;
     fa.lpCommand = NULL;
     fa.cActions = 3;
     SC_ACTION sa[3];
     sa[0].Delay = 5000; 
     sa[0].Type = SC_ACTION_RESTART;
     sa[1].Delay = 55000;
     sa[1].Type = SC_ACTION_RESTART;
     sa[2].Delay = 60000;
     sa[2].Type = SC_ACTION_RESTART;
     fa.lpsaActions = sa;

	if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &fa))
		throw std::runtime_error("ChangeServiceConfig2 failed"); 
}

class Console
{
public:
	static bool done;
	Console()
	{
		SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );
	}
	static BOOL CtrlHandler( DWORD fdwCtrlType ) 
	{ 
	  switch( fdwCtrlType ) 
	  { 
		case CTRL_SHUTDOWN_EVENT: 
		case CTRL_BREAK_EVENT: 
		case CTRL_C_EVENT: 
		case CTRL_CLOSE_EVENT: 
		  done = true; 
		  return TRUE;
	 
		default: 
		  return FALSE; 
	  }
	} 
};

bool Console::done = false;

namespace RBX {
	extern bool nameThreads;
}

static boost::mutex keyLockMutex;

void ReadAccessKey()
{
	if (RBX::Http::accessKey.empty())
	{
		boost::mutex::scoped_lock lock(keyLockMutex);
		if (RBX::Http::accessKey.empty()) 
		{
			CRegKey key;
			if (SUCCEEDED(key.Open(HKEY_LOCAL_MACHINE, "Software\\ROBLOX Corporation\\Roblox\\", KEY_READ))) 
			{
				CHAR keyData[MAX_PATH];
				ULONG bufLen = MAX_PATH-1;
				if (SUCCEEDED(key.QueryStringValue("AccessKey", keyData, &bufLen))) 
				{
					keyData[bufLen] = 0;
					RBX::Http::accessKey = std::string(keyData);
					RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "Access key read: %s", RBX::Http::accessKey.c_str());
				}
			}
		}
	}
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "Current Access key: %s", RBX::Http::accessKey.c_str());
}

class PrintfLogger
{
	rbx::signals::scoped_connection messageConnection;
	HANDLE handle;  
	rbx::spin_mutex mutex;
public:
	PrintfLogger()
		:handle(GetStdHandle(STD_OUTPUT_HANDLE))
	{
		messageConnection = RBX::StandardOut::singleton()->messageOut.connect(boost::bind(&PrintfLogger::onMessage, this, _1));
	}
protected:
	void onMessage(const RBX::StandardOutMessage& message)
	{
		rbx::spin_mutex::scoped_lock lock(mutex);
		switch (message.type)
		{
		case RBX::MESSAGE_OUTPUT:
			SetConsoleTextAttribute(handle, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;
		case RBX::MESSAGE_INFO:
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			break;
		case RBX::MESSAGE_WARNING:
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN);
			break;
		case RBX::MESSAGE_ERROR:
			SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		}
		printf("%s\n", message.message.c_str());
		SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
};


int _tmain(int argc, _TCHAR* argv[])
{
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	static boost::scoped_ptr<PrintfLogger> standardOutLog(new PrintfLogger());

	ReadAccessKey();

	RBX::UTIL::setWindowsNoFragHeap();

	try
	{
		bool isServiceCall = true;
		bool isConsole = false;
		bool isCrashUploader = false;

		for (int i = 1; i<argc; ++i)
		{
			if (argv[i][0] == _T('/'))
				argv[i][0] = _T('-');

			if (_tcsicmp(argv[i], _T("-Install")) == 0)
			{
				EventLogInstall();
				SvcInstall();
				isServiceCall = false;
				continue;
			}

			if (_tcsicmp(argv[i], _T("-AQTime")) == 0)
			{
				RBX::nameThreads = false;
				continue;
			}

			if (_tcsicmp(argv[i], _T("-Uninstall")) == 0)
			{
				SvcUninstall(SVCNAME);
				EventLongUninstall();
				isServiceCall = false;
				continue;
			}

			if (_tcsicmp(argv[i], _T("-Start")) == 0)
			{
				SvcStart(SVCNAME);
				isServiceCall = false;
				continue;
			}

			if (_tcsicmp(argv[i], _T("-Stop")) == 0)
			{
				SvcStop(SVCNAME);
				isServiceCall = false;
				continue;
			}

			if (_tcsicmp(argv[i], _T("-Console")) == 0)
			{
				isConsole = true;
				isServiceCall = false;
				continue;
			}
			if (_tcsicmp(argv[i], _T("-CrashReporter")) == 0)
			{
				isConsole = true;
				isServiceCall = false;

				isCrashUploader = true;
				continue;
			}
		}

		if (isConsole)
		{
#ifdef RBX_TEST_BUILD
			RBX::DataModel::hash = parseMD5(argc, argv);
			RCCServiceSettingsKeyOverwrite = parseSettingsKey(argc, argv);
#endif
			startupRCC(parsePort(argc, argv), parseContent(argc, argv), isCrashUploader);

			int placeId = parsePlaceId(argc, argv);
			if (placeId > -1)
			{
				std::stringstream buffer;
				std::ifstream gameServerFile("gameserver.txt", std::ios::in);
				if (gameServerFile.is_open())
				{
					buffer << gameServerFile.rdbuf();
					gameServerFile.close();
				}

				buffer << "start(" << placeId << ", " << 53640 << ", '" << GetBaseURL() << "')";
				std::string script = buffer.str();

				RCCServiceSoapService* copy = service.copy(); // make a safe copy 
				if (!copy) 
					throw std::runtime_error(*soap_faultstring(&service)); 

				ns1__Job job;
				job.id = "Test";
				job.expirationInSeconds = 600;

				ns1__ScriptExecution se;
				se.name = &std::string("Start Server");
				se.script = &script;

				_ns1__OpenJob openJob;
				_ns1__OpenJobResponse response;
				openJob.script = &se;
				openJob.job = &job;

				copy->OpenJob(&openJob, &response);
			}

			if(!isCrashUploader){
				Console console;
				while (!console.done)
				{
					if (_kbhit())
						if (_getch() == 27)
							break;
						else
							RBX::TaskScheduler::singleton().printDiagnostics(true);
					stepRCC();
				}
			}
			shutdownRCC();
		}
		else if (isServiceCall)
		{

			// This is an honest attempt to start the service. We're ready to begin!!!
			SERVICE_TABLE_ENTRY serviceTable[] = 
			{
				{ SVCNAME, ServiceMain },
				{ 0, 0 } 
			};

			if (!::StartServiceCtrlDispatcher(serviceTable))
			{
				printf("StartServiceCtrlDispatcher failed (%d)\n", GetLastError()); 
			}
		}
	}
	catch (std::exception& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e);
		SvcReportEvent(EVENTLOG_ERROR_TYPE, e.what());
	}
    RBX::clearLuaReadOnly();
}

#pragma optimize( "", on )
