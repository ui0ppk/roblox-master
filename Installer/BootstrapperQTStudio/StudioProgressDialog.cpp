#include "StdAfx.h"
#include "StudioProgressDialog.h"
#include "BootstrapperQTStudio.h"
#include "shellapi.h"

#include <commctrl.h>
#include <WindowsX.h>

WNDPROC  OldButtonProc2;

LRESULT CALLBACK StudioButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	static bool tracking = false;

	switch(uMsg)
	{
	case WM_MOUSEMOVE: 
		{             
			if (!tracking)
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize= sizeof(tme);
				tme.dwFlags= TME_HOVER | TME_LEAVE;
				tme.hwndTrack= hWnd;	//handle of window you want the mouse over message for.
				tme.dwHoverTime= 10;

				tracking = _TrackMouseEvent(&tme);
			}
		} 
		break; 
	case WM_MOUSEHOVER:
		{
			CStudioProgressDialog* dialog = (CStudioProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (dialog->isSuccessPromptShown())
			{
				if (dialog->getHWndDevLink() == hWnd)
					SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getDevLinkBitmap(true));
				else
					SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getLaunchStudioBitmap(true));
			}
			else
				SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getCancelBitmap(true));
		}
		break;
	case WM_MOUSELEAVE: 
		{ 
			CStudioProgressDialog* dialog = (CStudioProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (dialog->isSuccessPromptShown())
			{
				if (dialog->getHWndDevLink() == hWnd)
					SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getDevLinkBitmap(false));
				else
					SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getLaunchStudioBitmap(false));
			}
			else
				SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getCancelBitmap(false));

			tracking = false;
		} 
		break;
	case WM_LBUTTONUP:
		{
			int xPos = GET_X_LPARAM(lParam); 
			int yPos = GET_Y_LPARAM(lParam);

			RECT rect;
			GetWindowRect(hWnd, &rect);
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;

			// make sure we are inside the button
			if (xPos < 0 || xPos > width || yPos < 0 || yPos > height)
				break;

			CStudioProgressDialog* dialog = (CStudioProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (dialog)
			{
				if (dialog->getHWndDevLink() == hWnd)
					dialog->OpenDevelopPage();
				else if (dialog->getHWndBtnLaunchStudio())
					dialog->LaunchStudio();

				dialog->CloseDialog();
			}

			SendMessage(GetParent(hWnd), WM_CLOSE, 0, 0);
		}
		break;
	default: 
		return CallWindowProc(OldButtonProc2, hWnd, uMsg, wParam, lParam); 
	}

	return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	static CStudioProgressDialog* dialog = NULL;

	switch (uMsg)
	{
	case WM_CREATE:
		{
			CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			dialog = static_cast<CStudioProgressDialog*>(pCreate->lpCreateParams);
			dialog->OnCreate(hwnd);
		}
		break;
	}

	if (dialog)
		return dialog->WndProc(hwnd, uMsg, wParam, lParam);


	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CStudioProgressDialog::CStudioProgressDialog(HINSTANCE hInstance, Bootstrapper* bootstrapper) : CProgressDialog(hInstance), bs(bootstrapper)
{
	bmpLogo = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_STUDIO_LOGO));

	bmpLaunchStudio = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_LAUNCH_STUDIO));
	bmpLaunchStudioHover = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_LAUNCH_STUDIO_ON));

	bmpDevLink = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_DEV));
	bmpDevLinkHover = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_DEV_ON));
}

CStudioProgressDialog::~CStudioProgressDialog(void)
{
	DeleteObject(bmpLaunchStudio);
	DeleteObject(bmpLaunchStudioHover);
	DeleteObject(bmpDevLink);
	DeleteObject(bmpDevLinkHover);
}

void CStudioProgressDialog::InitDialog()
{
	CProgressDialog::InitDialog(WindowProc);

	RECT rect;
	GetWindowRect(hWndDialog, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	// install success message
	HBITMAP text = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_STUDIO_TEXT));
	hInstallSuccess = CreateWindowEx(0, WC_STATIC, L"", WS_CHILD | SS_BITMAP, 
		width/2-225, 196, 450, 48, 
		hWndDialog, NULL, GetModuleHandle(NULL), NULL);
	SendMessage(hInstallSuccess, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)text);

	// developer page button
	int bntWidth = 110;
	int bntHeight = 14;
	hWndBtnDevLink = CreateWindowEx(0, L"Button", L"", WS_CHILD | BS_FLAT | BS_BITMAP,
		width/2-bntWidth/2, height - 24, bntWidth, bntHeight, hWndDialog, NULL, GetModuleHandle(NULL), NULL);
	SendMessage(hWndBtnDevLink, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmpDevLink);

	// set up "launch studio" button, start hidden
	bntWidth = 130;
	bntHeight = 44;
	hWndBtnLaunchStudio = CreateWindowEx(0, L"Button", L"", WS_CHILD | BS_FLAT | BS_BITMAP,
		width/2-bntWidth/2, 250, bntWidth, bntHeight, hWndDialog, (HMENU)IDB_BTN_OK, GetModuleHandle(NULL), NULL);
	SendMessage(hWndBtnLaunchStudio, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)getLaunchStudioBitmap(false));


	// hook up proc function for launch studio button
	OldButtonProc2 = (WNDPROC)SetWindowLongPtr(hWndBtnLaunchStudio, GWL_WNDPROC, (LONG)StudioButtonProc);
	SetWindowLongPtr(hWndBtnLaunchStudio, GWL_USERDATA, (LONG)this);

	(WNDPROC)SetWindowLongPtr(hWndBtnDevLink, GWL_WNDPROC, (LONG)StudioButtonProc);
	SetWindowLongPtr(hWndBtnDevLink, GWL_USERDATA, (LONG)this);
	
}

void CStudioProgressDialog::ShowSuccessPrompt()
{
	CProgressDialog::ShowSuccessPrompt();

	::ShowWindow(hInstallSuccess, SW_SHOWNORMAL);
	::ShowWindow(hWndBtnDevLink, SW_SHOWNORMAL);
	::ShowWindow(hWndBtnLaunchStudio, SW_SHOWNORMAL);

	InvalidateRect(hWndDialog, 0, NULL);
}

HBITMAP CStudioProgressDialog::getLaunchStudioBitmap(bool hover)
{
	return hover ? bmpLaunchStudioHover : bmpLaunchStudio;
}

HBITMAP CStudioProgressDialog::getDevLinkBitmap(bool hover)
{
	return hover ? bmpDevLinkHover : bmpDevLink;
}

void CStudioProgressDialog::LaunchStudio()
{
	if (BootstrapperQTStudio* studio_bs = dynamic_cast<BootstrapperQTStudio*>(bs))
		studio_bs->StartRobloxApp(false);
}

void CStudioProgressDialog::OpenDevelopPage()
{
	if (!bs)
		return;

	std::wstring url = format_string(_T("%S%s"), bs->BaseHost().c_str(), _T("/develop"));
	if (url.find(_T("www.")) != 0 && url.find(_T("http:")) != 0) 
	{
		url = format_string(_T("http://%s"), url.c_str());
	}
	LLOG_ENTRY1(bs->logger, "Opening developer page: url=%S", url.c_str());
	ShellExecute(0, _T("open"), url.c_str(), 0, 0, 1);
}
