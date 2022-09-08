#include "StdAfx.h"
#include "commonresourceconstants.h"
#include "TaskDialog.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <commctrl.h>

#ifndef PBST_NORMAL
#define PBST_NORMAL             0x0001
#define PBST_ERROR              0x0002
#define PBST_PAUSED             0x0003
#endif

const int RBX_TDE_MESSAGE = TDE_MAIN_INSTRUCTION;
//const int RBX_TDE_MESSAGE = TDE_EXPANDED_INFORMATION;

CTaskDialog::CTaskDialog(HINSTANCE instance)
:readyEvent(NULL, TRUE, FALSE, NULL)
,doneEvent(NULL, TRUE, FALSE, NULL)
,instance(instance)
,taskWnd(NULL)
,isMarquee(true)
,cancelEnabled(true)
,isStarted(false)
,progress(0)
{
}

CTaskDialog::~CTaskDialog(void)
{
	CloseDialog();
	::WaitForSingleObject(doneEvent, INFINITE);
}


HRESULT CTaskDialog::callback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	CTaskDialog* dialog = reinterpret_cast<CTaskDialog*>(dwRefData);
	return dialog->onCallback(hwnd, uNotification, wParam, lParam);
}

typedef HRESULT (WINAPI *TaskDialogIndirectProc)(const TASKDIALOGCONFIG *pTaskConfig, __out_opt int *pnButton, __out_opt int *pnRadioButton, __out_opt BOOL *pfVerificationFlagChecked);

void CTaskDialog::run()
{
	HMODULE h = ::LoadLibraryW( L"comctl32.dll" );
	TaskDialogIndirectProc func = (TaskDialogIndirectProc)::GetProcAddress(h, "TaskDialogIndirect" );

	TASKDIALOGCONFIG config             = {0};
	config.cbSize                       = sizeof(config);
	config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR;
	config.hInstance                    = instance;
	config.dwCommonButtons              = TDCBF_CANCEL_BUTTON;
	config.pszMainIcon                    = MAKEINTRESOURCEW(IDI_BOOTSTRAPPER);
	config.pszWindowTitle               = L"Roblox";
	config.pszMainInstruction           = L"Starting Roblox";
	if (RBX_TDE_MESSAGE == TDE_EXPANDED_INFORMATION)
	{
		config.pszExpandedInformation       = L"Please Wait...";		// http://msdn.microsoft.com/en-us/library/bb760536(VS.85).aspx states: If pszExpandedInformation is NULL and you attempt to send a TDM_UPDATE_ELEMENT_TEXT with TDE_EXPANDED_INFORMATION, nothing will happen.
	}

	config.pfCallback = (PFTASKDIALOGCALLBACK)CTaskDialog::callback;
	config.lpCallbackData = reinterpret_cast<LONG_PTR>(this);

	int nButtonPressed = 0;
	func(&config, &nButtonPressed, NULL, NULL);
	switch (nButtonPressed)
	{
		case IDOK:
			break; // the user pressed button 0 (change password).
		case IDCANCEL:
			if (!closeCallback.empty())
				closeCallback();
			break; // user cancelled the dialog
		default:
			break; // should never happen
	}

	doneEvent.Set();
}

void CTaskDialog::SetMessage(const char* value)
{
	boost::unique_lock<boost::mutex> lock(mut);
	if (message==value)
		return;
	message = value;
	if (!isStarted)
		return;

	::WaitForSingleObject(readyEvent, INFINITE);
	//::SendMessage(taskWnd, TDM_SET_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)(LPCWSTR)btsr);
	CComBSTR btsr = CString(value);
	::SendMessage(taskWnd, TDM_SET_ELEMENT_TEXT, RBX_TDE_MESSAGE, (LPARAM)(LPCWSTR)btsr);
}

void CTaskDialog::SetCancelEnabled(bool state)
{
	boost::unique_lock<boost::mutex> lock(mut);
	if (state==cancelEnabled)
		return;
	cancelEnabled = state;
	if (!isStarted)
		return;
	::WaitForSingleObject(readyEvent, INFINITE);
	::SendMessage(taskWnd, TDM_ENABLE_BUTTON, IDCANCEL, cancelEnabled ? 1 : 0);
}

void CTaskDialog::CloseDialog()
{
	boost::unique_lock<boost::mutex> lock(mut);
	if (!isStarted)
	{
		readyEvent.Set();
		doneEvent.Set();
		return;
	}
	::WaitForSingleObject(readyEvent, INFINITE);
	::SendMessage(taskWnd, TDM_CLICK_BUTTON, IDCANCEL, 0);
}

void CTaskDialog::ShowWindow()
{
	{
		boost::unique_lock<boost::mutex> lock(mut);
		if (!isStarted)
		{
			boost::thread(boost::bind(&CTaskDialog::run, this));
			isStarted = true;
		}
	}
	::WaitForSingleObject(readyEvent, INFINITE);
}

void CTaskDialog::DisplayError(const char* message, const char* exceptionText)
{

	TASKDIALOGCONFIG config             = {0};
	config.cbSize                       = sizeof(config);
	config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
	config.hInstance                    = NULL;
	config.dwCommonButtons              = TDCBF_CLOSE_BUTTON;
	config.pszMainIcon                    = TD_ERROR_ICON;
	config.pszWindowTitle               = L"Roblox";
	CComBSTR btsr = CString(message);
	config.pszMainInstruction           = btsr;
	CComBSTR btsr2 = CString(exceptionText);
	config.pszExpandedInformation       = btsr2;

	ShowWindow();
	int nButtonPressed = 0;
	SendMessage(taskWnd, TDM_NAVIGATE_PAGE, 0, (LPARAM) &config);  
	::WaitForSingleObject(doneEvent, INFINITE);
}

void CTaskDialog::FinalMessage(const char* message)
{
	TASKDIALOGCONFIG config             = {0};
	config.cbSize                       = sizeof(config);
	config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
	config.hInstance                    = NULL;
	config.dwCommonButtons              = TDCBF_OK_BUTTON;
	config.pszMainIcon                    = MAKEINTRESOURCEW(IDI_BOOTSTRAPPER);
	config.pszWindowTitle               = L"Roblox";
	CComBSTR btsr = CString(message);
	config.pszMainInstruction           = btsr;

	ShowWindow();
	int nButtonPressed = 0;
	SendMessage(taskWnd, TDM_NAVIGATE_PAGE, 0, (LPARAM) &config);  
	::WaitForSingleObject(doneEvent, INFINITE);
}

void CTaskDialog::SetProgress(int percent)
{
	boost::unique_lock<boost::mutex> lock(mut);
	if (percent==progress)
		return;
	progress = percent;
	if (!isStarted)
		return;

	::WaitForSingleObject(readyEvent, INFINITE);
	if (!isMarquee)
		::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_POS, progress, 0);
}

void CTaskDialog::SetMarquee(bool state)
{
	boost::unique_lock<boost::mutex> lock(mut);
	if (isMarquee==state)
		return;
	isMarquee = state;
	if (!isStarted)
		return;

	::WaitForSingleObject(readyEvent, INFINITE);
	::SendMessage(taskWnd, TDM_SET_MARQUEE_PROGRESS_BAR, isMarquee ? TRUE : FALSE, 0);
	::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_MARQUEE, isMarquee ? TRUE : FALSE, 0);
	if (!isMarquee)
	{
		::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0);
		::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM(0, 100));
	}
	isMarquee = state;
}

HRESULT CTaskDialog::onCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam)
{
	switch (uNotification)
	{
	case TDN_DIALOG_CONSTRUCTED:
		{
		taskWnd = hwnd;

		::SendMessage(taskWnd, TDM_ENABLE_BUTTON, IDCANCEL, cancelEnabled ? 1 : 0);
		::SendMessage(taskWnd, TDM_SET_MARQUEE_PROGRESS_BAR, isMarquee ? TRUE : FALSE, 0);
		::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_MARQUEE, isMarquee ? TRUE : FALSE, 0);
		if (!isMarquee)
		{
			::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_NORMAL, 0);
			::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_RANGE, 0, MAKELPARAM(0, 100));
			::SendMessage(taskWnd, TDM_SET_PROGRESS_BAR_POS, progress, 0);
		}
		CComBSTR btsr = CString(message.c_str());
		::SendMessage(taskWnd, TDM_SET_ELEMENT_TEXT, RBX_TDE_MESSAGE, (LPARAM)(LPCWSTR)btsr);

		readyEvent.Set();
		return S_OK;
		}

	case TDN_DESTROYED:
		taskWnd = NULL;
		return S_OK;
	
	default:
		return S_OK;
	}
}
