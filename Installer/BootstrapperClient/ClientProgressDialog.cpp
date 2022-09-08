#include "StdAfx.h"
#include "ClientProgressDialog.h"

#include <commctrl.h>
#include <WindowsX.h>

WNDPROC ClientOldButtonProc;

LRESULT CALLBACK ClientButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
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
				tme.dwHoverTime= 50;

				tracking = _TrackMouseEvent(&tme);
			}
		} 
		break; 
	case WM_MOUSEHOVER:
		{
			CClientProgressDialog* dialog = (CClientProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (dialog->isSuccessPromptShown())
				SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getOkBitmap(true));
		}
		break;
	case WM_MOUSELEAVE: 
		{ 
			CClientProgressDialog* dialog = (CClientProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (dialog->isSuccessPromptShown())
				SendMessage(hWnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)dialog->getOkBitmap(false));

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

			CClientProgressDialog* dialog = (CClientProgressDialog*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			dialog->CloseDialog();

			SendMessage(GetParent(hWnd), WM_CLOSE, 0, 0);
		}
		break;
	default:
		return CallWindowProc(ClientOldButtonProc, hWnd, uMsg, wParam, lParam); 
	}

	return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	static CClientProgressDialog* dialog = NULL;

	switch (uMsg)
	{
	case WM_CREATE:
		{
			CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			dialog = static_cast<CClientProgressDialog*>(pCreate->lpCreateParams);
			dialog->OnCreate(hwnd);
		}
		break;
	}

	if (dialog)
		return dialog->WndProc(hwnd, uMsg, wParam, lParam);


	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CClientProgressDialog::CClientProgressDialog(HINSTANCE hInstance, Bootstrapper* bootstrapper) : CProgressDialog(hInstance), bs(bootstrapper)
{
	bmpOk = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_OK));
	bmpOkOn = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BTN_OK_ON));
}

CClientProgressDialog::~CClientProgressDialog(void)
{
	::DeleteObject(bmpOk);
	::DeleteObject(bmpOkOn);
}

void CClientProgressDialog::InitDialog()
{
	CProgressDialog::InitDialog(WindowProc);

	RECT rect;
	GetWindowRect(hWndDialog, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	// install success message
	HBITMAP text = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CLIENT_TEXT));
	hInstallSuccess = CreateWindowEx(0, WC_STATIC, L"", WS_CHILD | SS_BITMAP, 
		width/2-225, 200, 450, 48, 
		hWndDialog, NULL, GetModuleHandle(NULL), NULL);
	SendMessage(hInstallSuccess, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)text);

	// set up "ok" button, start hidden
	int bntWidth = 130;
	int bntHeight = 44;
	hWndBntOk = CreateWindowEx(0, L"Button", L"", WS_CHILD | BS_FLAT | BS_BITMAP,
		width/2-bntWidth/2, height - 55, bntWidth, bntHeight, hWndDialog, (HMENU)IDB_BTN_OK, GetModuleHandle(NULL), NULL);
	SendMessage(hWndBntOk, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)getOkBitmap(false));

	ClientOldButtonProc = (WNDPROC)SetWindowLongPtr(hWndBntOk, GWL_WNDPROC, (LONG) ClientButtonProc);
	SetWindowLongPtr(hWndBntOk, GWL_USERDATA, (LONG)this);
}

void CClientProgressDialog::ShowSuccessPrompt()
{
	CProgressDialog::ShowSuccessPrompt();

	::ShowWindow(hInstallSuccess, SW_SHOWNORMAL);
	::ShowWindow(hWndBntOk, SW_SHOWNORMAL);
	InvalidateRect(hWndDialog, 0, NULL);
}

HBITMAP CClientProgressDialog::getOkBitmap(bool hover)
{
	return hover ? bmpOkOn : bmpOk;
}