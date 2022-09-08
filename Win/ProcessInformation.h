
#pragma once

class CProcessInformation
{
public:
	PROCESS_INFORMATION pi;
	CProcessInformation()
	{
		pi.hThread = 0;
		pi.hProcess = 0;
	}
	operator PROCESS_INFORMATION& () { return pi; }
	operator PROCESS_INFORMATION* () { return &pi; }
	~CProcessInformation()
	{
		CloseProcess();
	}
	DWORD WaitForSingleObject(DWORD timeout) { return ::WaitForSingleObject(pi.hProcess, timeout); }
	bool GetExitCode(DWORD& exitCode) const { return ::GetExitCodeProcess(pi.hProcess, &exitCode)==TRUE; }
	void CloseProcess() 
	{
		if(InUse()){
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			pi.hThread = 0;
			pi.hProcess = 0;
		}
	}

	bool InUse()
	{
		return pi.hThread != NULL || pi.hProcess != NULL;
	}
};

