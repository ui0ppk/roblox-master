#include "StdAfx.h"
#include "ShutdownDialog.h"
#include "VistaTools.h"
#include "atlsync.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <commctrl.h>
#include "commonresourceconstants.h"

#ifndef PBST_NORMAL
#define PBST_NORMAL             0x0001
#define PBST_ERROR              0x0002
#define PBST_PAUSED             0x0003
#endif

typedef HRESULT (WINAPI *TaskDialogIndirectProc)(const TASKDIALOGCONFIG *pTaskConfig, __out_opt int *pnButton, __out_opt int *pnRadioButton, __out_opt BOOL *pfVerificationFlagChecked);

class CShutdownTaskDialog : public CShutdownDialog
{
	int dialogResult;
	CEvent readyEvent;
	CEvent doneEvent;
	HWND taskWnd;
	char instructions[256];
public:
	CShutdownTaskDialog(HINSTANCE instance, HWND parent, const char* windowTitle)
		:readyEvent(NULL, TRUE, FALSE, NULL)
		,doneEvent(NULL, TRUE, FALSE, NULL)
		,taskWnd(NULL)
		,dialogResult(-1)
	{
		sprintf_s(instructions, 256, "Roblox needs to close \"%s\"", windowTitle);
		boost::thread(boost::bind(&CShutdownTaskDialog::run, this, instance, parent));
	}
	~CShutdownTaskDialog(void)
	{
		CloseDialog();
		::WaitForSingleObject(doneEvent, INFINITE);
	}

	void run(HINSTANCE instance, HWND parent)
	{
		HMODULE h = ::LoadLibraryW( L"comctl32.dll" );
		TaskDialogIndirectProc func = (TaskDialogIndirectProc)::GetProcAddress(h, "TaskDialogIndirect" );

		TASKDIALOGCONFIG config             = {0};
		config. cbSize                       = sizeof(config);
		config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
		config.hInstance                    = instance;
		config.hwndParent = parent;
		config.dwCommonButtons              = TDCBF_CANCEL_BUTTON;
		config.pszMainIcon                    = MAKEINTRESOURCEW(IDI_BOOTSTRAPPER);
		config.pszWindowTitle               = L"Roblox";
		CComBSTR bstr = CString(instructions);
		config.pszMainInstruction           = (BSTR)bstr;

		TASKDIALOG_BUTTON aCustomButtons[] = {
			{ IDOK, L"&Shut down now\n"
					L"You may lose work that you haven't saved" }
		};
		config.pButtons = aCustomButtons;
		config.cButtons = _countof(aCustomButtons);

		config.pfCallback = (PFTASKDIALOGCALLBACK)CShutdownTaskDialog::callback;
		config.lpCallbackData = reinterpret_cast<LONG_PTR>(this);

		int result;
		func(&config, &result, NULL, NULL);

		doneEvent.Set();
		dialogResult = result;
	}

	/*implement*/ void CloseDialog()
	{
		::WaitForSingleObject(readyEvent, INFINITE);
		::SendMessage(taskWnd, TDM_CLICK_BUTTON, IDCANCEL, 0);
	}
	/*implement*/ bool IsDismissed(DWORD& result) { result = dialogResult; return dialogResult!=-1; }


	HRESULT onCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam)
	{
		switch (uNotification)
		{
		case TDN_CREATED:
			taskWnd = hwnd;
			readyEvent.Set();
			return S_OK;
		
		case TDN_DESTROYED:
			taskWnd = NULL;
			return S_OK;
		
		default:
			return S_OK;
		}
	}
	static HRESULT __stdcall callback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
	{
		CShutdownTaskDialog* dialog = reinterpret_cast<CShutdownTaskDialog*>(dwRefData);
		return dialog->onCallback(hwnd, uNotification, wParam, lParam);
	}
};



class CShutdownLegacyDialog : public CShutdownDialog
{
	int result;
public:
	CShutdownLegacyDialog(HINSTANCE hInstance, HWND parent, const TCHAR* windowTitle)
	{
		CString message;
		message.Format(_T("\"%s\" needs to close.\n\nShut down now?  You may lose work that you haven't saved"), windowTitle);
		result = ::MessageBox(parent,  message, _T("Roblox"), MB_OKCANCEL | MB_ICONQUESTION);
	}

	void CloseDialog(void)
	{
		// noopt
	}
	/*implement*/ bool IsDismissed(DWORD& result) { result = this->result; return result!=-1; }
};


CShutdownDialog* CShutdownDialog::Create(HINSTANCE instance, HWND parent, const TCHAR* windowTitle)
{
	//if (IsVista())
	//	return new CShutdownTaskDialog(instance, parent, windowTitle);
	//else
		return new CShutdownLegacyDialog(instance, parent, windowTitle);
}

