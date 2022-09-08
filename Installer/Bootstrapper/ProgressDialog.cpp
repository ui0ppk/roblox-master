#include "StdAfx.h"
#include "ProgressDialog.h"
#include "commonresourceconstants.h"
#include "SharedHelpers.h"
#include <commctrl.h>
#pragma comment (lib, "comctl32.lib")
#include "format_string.h"

#include <Psapi.h>

// can't use these on windows xp :(
//#include <dwmapi.h>
//#pragma comment (lib, "dwmapi.lib")

#define IDT_TIMER 1
const static int WM_MARQUEE = WM_USER + 101;		// See Q196026 
const static int WM_PROGRESS = WM_USER + 102;		// See Q196026 

static HICON icon;
WNDPROC  OldButtonProc;

HWND CProgressDialog::CreateTextHelper(HWND hWndParent, HFONT font, int x, int y, int width, int height, bool hidden)
{
	DWORD style = WS_CHILD | SS_CENTER;
	if (!hidden)
		style |= WS_VISIBLE;

	HWND hWndText = CreateWindowEx(0, L"Static", L"", style, 
		x, y, width, height, 
		hWndParent, NULL, GetModuleHandle(NULL), NULL);

	SendMessage(hWndText, WM_SETFONT, (LPARAM)font, TRUE);

	return hWndText;
}

LRESULT CALLBACK CancelButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	static bool tracking = false;

	switch(uMsg)
	{
	case WM_MOUSEMOVE: 
		{             
			if (!tracking)
			{
				HWND hWndCancelBnt = GetDlgItem(hWnd, IDCANCEL);

				TRACKMOUSEEVENT tme;
				tme.cbSize= sizeof(tme);
				tme.dwFlags= TME_HOVER | TME_LEAVE;
				tme.hwndTrack= hWnd;	//handle of window you want the mouse over message for.
				tme.dwHoverTime= 50;

				tracking = _TrackMouseEvent(&tme);
			}
		} 
		break; 
	case WM_MOUSEHOVER:
		{
			CProgressDialog* dialog = (CProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (!dialog->isSuccessPromptShown())
				SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getCancelBitmap(true));
		}
		break;
	case WM_MOUSELEAVE: 
		{ 
			CProgressDialog* dialog = (CProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (!dialog->isSuccessPromptShown())
				SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getCancelBitmap(false));

			tracking = false;
		} 
		break;
	default: 
		return CallWindowProc(OldButtonProc, hWnd, uMsg, wParam, lParam); 
	}

	return true;
}

LRESULT CALLBACK CProgressDialog::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	static CProgressDialog* dialog = NULL;

	switch (uMsg)
	{
	case WM_CREATE:
		{
			CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			dialog = static_cast<CProgressDialog*>(pCreate->lpCreateParams);
			//OnCreate(hwnd);

			::SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) icon);
			::SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM) icon);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			if (dialog && !dialog->closeCallback.empty())
				dialog->closeCallback();
		}
		break;
	case WM_NCCALCSIZE:
		return 0;
	case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(25, 25, 25));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)hbrBackground;
		}
		break;
	case WM_DESTROY:
		// This message could be sent if dialog was closed from taskbar and bypassed CloseDialog(),
		// so call the closeCallback to indicate it's a user cancel
		if (dialog && !dialog->dialogClosing && !dialog->closeCallback.empty())
			dialog->closeCallback();

		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, hbrBackground);

			HPEN hPen = CreatePen(PS_SOLID, 1, RGB(184, 184, 184));
			SelectObject(hdc, hPen);
			
			RECT rect;
			GetWindowRect(hwnd, &rect);
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;

			Rectangle(hdc, 0, 0, width, height);
			DeleteObject(hPen);

			GetStockObject(DC_PEN);

			EndPaint(hwnd, &ps);
		}
		return 0;
	case WM_ACTIVATE:
		InvalidateRect(hwnd, 0, NULL);
		break;
	case WM_PROGRESS:
		{
			HWND prog = ::GetDlgItem(hwnd, IDC_PROGRESS);
			DWORD style = GetWindowLongPtr(prog, GWL_STYLE);
			if ((style & PBS_MARQUEE) == 0)
				::SendMessage(prog, PBM_SETPOS, wParam, 0);
		}
		break;
	case WM_MARQUEE:
		{
			HWND prog = ::GetDlgItem(hwnd, IDC_PROGRESS);
			DWORD style = GetWindowLongPtr(prog, GWL_STYLE);
			if (wParam)
			{
				if ((style & PBS_MARQUEE) == 0)
				{
					::SetWindowLongPtr(prog, GWL_STYLE, style | PBS_MARQUEE);
					::SendMessage(prog, PBM_SETMARQUEE, TRUE, 0);
				}
			}
			else
			{
				if ((style & PBS_MARQUEE) != 0)
				{
					::SendMessage(prog, PBM_SETMARQUEE, FALSE, 0);
					::SetWindowLongPtr(prog, GWL_STYLE, style & ~PBS_MARQUEE);
				}
			}
		}
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CProgressDialog::CProgressDialog(HINSTANCE hInstance) : hInstance(hInstance), dialogClosing(false)
{
	icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOTSTRAPPER));

	bmpCancel = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_CANCEL));
	bmpCancelOn = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_CANCEL_ON));

	bmpLogo = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO));
}

CProgressDialog::~CProgressDialog(void)
{
	CloseDialog();
}

void CProgressDialog::InitDialog(WNDPROC wndProc)
{
	INITCOMMONCONTROLSEX c;
	c.dwSize = sizeof(INITCOMMONCONTROLSEX);
	c.dwICC = ICC_PROGRESS_CLASS;
	::InitCommonControlsEx(&c);

	const wchar_t CLASS_NAME[]  = _T("MAINDIALOG");
	WNDCLASS wc = {};
	wc.lpfnWndProc   = wndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&wc);

	int sizeX = 500;
	int sizeY = 320;
	int posX = (::GetSystemMetrics(SM_CXSCREEN) - sizeX) / 2;
	int posY = (::GetSystemMetrics(SM_CYSCREEN) - sizeY) / 2;

	hWndDialog = CreateWindowEx(
		0,
		CLASS_NAME,
		L"",
		WS_POPUP | WS_THICKFRAME,
		posX, posY, sizeX, sizeY,
		NULL,
		NULL,
		hInstance,
		(LPVOID)this
		);

	OldButtonProc = (WNDPROC)SetWindowLongPtr(hWndButton, GWL_WNDPROC, (LONG) CancelButtonProc);
	SetWindowLongPtr(hWndButton, GWL_USERDATA, (LONG)this);
}

void CProgressDialog::OnCreate(HWND hWnd)
{
	RECT rect;
	GetWindowRect(hWnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	// can't use these on windows xp
	//MARGINS borderless = {1,1,1,1};
	//HRESULT hr = DwmExtendFrameIntoClientArea(hWnd, &borderless);

	// set up logo
	int logoSize = 90;
	HWND hPic = CreateWindowEx(0, WC_STATIC, L"", WS_CHILD | WS_VISIBLE | SS_BITMAP, 
		width/2-logoSize/2, 70, logoSize, logoSize, 
		hWnd, NULL, GetModuleHandle(NULL), NULL);
	SendMessage(hPic, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmpLogo);

	// set up text
	int textWidth = width - 10;

	HDC hdc = GetDC(NULL);
	long fontHight = -MulDiv(11, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	fontMsg = CreateFont(fontHight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("Tahoma"));
	hWndText = CreateTextHelper(hWnd, fontMsg, 5, 200, textWidth, 20, false);

	// set up progress bar
	hWndProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		24, 242, 452, 20,
		hWnd, (HMENU)IDC_PROGRESS, GetModuleHandle(NULL), NULL);
	SendMessage(hWndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

	// set up button, start hidden
	int bntWidth = 130;
	int bntHeight = 44;
	hWndButton = CreateWindowEx(0, L"Button", L"", WS_CHILD | BS_FLAT | BS_BITMAP,
		width/2-bntWidth/2, height - 55, bntWidth, bntHeight, hWnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);

	HBITMAP bntBmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_CANCEL));
	SendMessage(hWndButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bntBmp);
	DeleteObject(bntBmp);

	ReleaseDC(hWnd, hdc);
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	// try find Player or Studio process
	DWORD process_id_array[1024]; 
	DWORD bytes_returned; 
	EnumProcesses(process_id_array, 1024*sizeof(DWORD), &bytes_returned); 

	const DWORD num_processes = bytes_returned/sizeof(DWORD); 
	for(DWORD i=0; i<num_processes; i++) 
	{ 
		CHandle hProcess(OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, process_id_array[i] )); 
		TCHAR image_name[1024]; 
		if(GetProcessImageFileName(hProcess,image_name,1024 )) 
		{ 
			image_name[1023] = 0;
			std::wstring s = image_name;
			if (s.find(_T(PLAYEREXENAME)) != std::wstring::npos || 
				s.find(_T(STUDIOQTEXENAME)) != std::wstring::npos)
			{
				// close the dialog via WM_COMMAND handler in WindowProc
				SendMessage(hwnd, WM_CLOSE, 0, 0);
				break;
			}
		}
	}
}

void CProgressDialog::ShowSuccessPrompt()
{
	showSuccessPrompt = true;

	::ShowWindow(hWndButton, SW_HIDE);
	::ShowWindow(hWndText, SW_HIDE);
	::ShowWindow(hWndProgressBar, SW_HIDE);

	// start timer to poll for launch of Player or Studio
	SetTimer(hWndDialog, IDT_TIMER, 300, (TIMERPROC)TimerProc);
}

int CProgressDialog::MessageBox(LPCWSTR text, LPCWSTR caption, UINT uType) 
{
	if ( GetSilentMode() )
		return 0;

	return ::MessageBox(hWndDialog, text, caption, uType);
}

void CProgressDialog::ShowWindow()
{
	if (GetSilentMode())
	{
		return;
	}

	::ShowWindow(hWndDialog, SW_SHOWNORMAL);
}

void CProgressDialog::CloseDialog(void)
{
	dialogClosing = true;

	KillTimer(hWndDialog, IDT_TIMER); 

	DeleteObject(bmpLogo);
	DeleteObject(bmpCancel);
	DeleteObject(bmpCancelOn);
	DeleteObject(fontMsg);

	DestroyWindow(hWndDialog);
}


void CProgressDialog::SetMessage(const char* message)
{
	if (GetSilentMode())
	{
		return;
	}

	SendMessage(hWndText, WM_SETTEXT, NULL, (LPARAM)convert_s2w(std::string(message)).c_str());
}

void CProgressDialog::DisplayError(const char* message, const char* exceptionText)
{
	std::string text = message;
	if (exceptionText)
	{
		text += "\n\nDetails: ";
		text += exceptionText;
	}
	MessageBox(convert_s2w(text).c_str(), _T("Roblox"), MB_OK | MB_ICONERROR);
}

void CProgressDialog::FinalMessage(const char* message)
{
	MessageBox(convert_s2w(message).c_str(), _T("Roblox"), MB_OK);
}

void CProgressDialog::SetCancelEnabled(bool state)
{
	if (GetSilentMode() || isSuccessPromptShown())
	{
		return;
	}

	HWND ctrl = ::GetDlgItem(hWndDialog, IDCANCEL);
	::EnableWindow(ctrl, state ? TRUE : FALSE);
	::ShowWindow(ctrl, state ? SW_SHOWNORMAL : SW_HIDE);
}

void CProgressDialog::SetCancelVisible(bool state)
{
	if (GetSilentMode() || isSuccessPromptShown())
	{
		return;
	}

	HWND ctrl = ::GetDlgItem(hWndDialog, IDCANCEL);
	::ShowWindow(ctrl, state ? TRUE : FALSE);
}

void CProgressDialog::SetMarquee(bool state)
{
	if (GetSilentMode())
	{
		return;
	}

	SendMessage(hWndDialog, WM_MARQUEE, state ? TRUE : FALSE, 0);
}

void CProgressDialog::SetProgress(int percent)
{
	if (GetSilentMode())
	{
		return;
	}

	SendMessage(hWndDialog, WM_PROGRESS, percent, 0);
}

HBITMAP CProgressDialog::getCancelBitmap(bool hover)
{
	return hover ? bmpCancelOn : bmpCancel;
}
