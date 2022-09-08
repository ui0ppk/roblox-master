#include "StdAfx.h"
#include "ShutdownRobloxApp.h"
#include "ShutdownDialog.h"
#include <vector>

#include "commonresourceconstants.h"

#include <windows.h>
#include <commctrl.h>
#include <Psapi.h>
#include <string>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#pragma comment (lib,"Psapi.lib")

ShutdownRobloxApp::~ShutdownRobloxApp(void)
{
}

#define TA_FAILED 0
#define TA_SUCCESS_CLEAN 1
#define TA_SUCCESS_KILL 2

struct TerminateAppEnumData
{
	DWORD pid;
	bool foundOne;
	TCHAR windowTitle[256];
};

BOOL CALLBACK TerminateAppEnum( HWND hwnd, DWORD lParam )
{
	DWORD dwID;
	GetWindowThreadProcessId(hwnd, &dwID);
	TerminateAppEnumData* data = (TerminateAppEnumData*) lParam;
	if (dwID == data->pid)
		if (::IsWindowVisible(hwnd))
		{
			SetForegroundWindow(hwnd);
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			data->foundOne = true;
			::GetWindowText(hwnd, data->windowTitle, sizeof(data->windowTitle));
		}
	return TRUE;
}

void ShutdownRobloxApp::terminateApp(DWORD pid)
{
  // If we can't open the process with PROCESS_TERMINATE rights, then we give up immediately.
  CHandle hProc(OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, pid));
  if(hProc != NULL)
  {
	// TerminateAppEnum() posts WM_CLOSE to all windows whose PID matches the process's.
	TerminateAppEnumData data;
	data.pid = pid;
	data.foundOne = false;
	EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM) &data);

	bool processClosed = false;
	if (data.foundOne)
	{
		this->result = true;
		// Wait on the handle. If it signals, great. If it times out, then kill it.
		const int count = timeoutInSeconds * 10;
		int i(count);
		while (!processClosed && i>0)
		{
			DWORD r = WaitForSingleObject(hProc, 100);
			if (r!=WAIT_TIMEOUT)
				processClosed = true;
			--i;
			if (!callback(count - i, count))
				throw installer_error_exception(installer_error_exception::InstallerTimeout);
		}
		if (!processClosed)
		{
			boost::scoped_ptr<CShutdownDialog> dialog(CShutdownDialog::Create(hInstance, parent(), data.windowTitle));
			if (dialog)
				while (!processClosed)
				{
					DWORD dialogResult;
					if (dialog->IsDismissed(dialogResult))
					{
						if (dialogResult==IDOK)
						{
							// User reqested a kill process
							break;
						}
						else
							throw cancel_exception(true);
					}

					DWORD r = WaitForSingleObject(hProc, 1000);
					if (r!=WAIT_TIMEOUT)
						processClosed = true;
				}
		}
	}

	if (!processClosed)
	{
		// Kill the process
		DWORD exitCode;
		if (!::GetExitCodeProcess(hProc, &exitCode) || exitCode==STILL_ACTIVE)
			TerminateProcess(hProc,0);
	}
  }
}

bool ShutdownRobloxApp::run()
{
	DWORD process_id_array[1024]; 
	DWORD bytes_returned; 
	EnumProcesses(process_id_array, 1024*sizeof(DWORD), &bytes_returned); 
	
	const DWORD num_processes = bytes_returned/sizeof(DWORD); 

	std::vector<DWORD> pids;

	for(DWORD i=0; i<num_processes; i++) 
	{ 
		CHandle hProcess(OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, process_id_array[i] )); 
		TCHAR image_name[1024]; 
		if(GetProcessImageFileName(hProcess,image_name,1024 )) 
		{ 
			image_name[1023] = 0;
			std::wstring s = image_name;
			if (s.find(appExeName) != std::wstring::npos)
				pids.push_back(process_id_array[i]);
		}
	} 
	this->result = false;
	std::for_each(pids.begin(), pids.end(), boost::bind(&ShutdownRobloxApp::terminateApp, this, _1));

	return this->result;
}



