#include "StdAfx.h"
#include "LegacyDialog.h"
#include "commonresourceconstants.h"
#include <commctrl.h>
#pragma comment (lib, "comctl32.lib")
#include "format_string.h"

const static int WM_MARQUEE = WM_USER + 101;		// See Q196026 
const static int WM_PROGRESS = WM_USER + 102;		// See Q196026 

static HICON icon;

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	static CLegacyDialog* dialog = NULL;	// Hax. It would be nicer to use ATL Dialogs instead. Ah well.

	switch (message)
	{
	case WM_INITDIALOG:
		{
			ATLASSERT(dialog==NULL);
			dialog = (CLegacyDialog*) lParam;

			::ShowWindow(hDlg, SW_HIDE);

			::SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM) icon);
			::SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM) icon);

			HWND ctrl = ::GetDlgItem(hDlg, IDC_MESSAGE);
			::SetWindowText(ctrl, _T(""));

			ctrl = GetDlgItem(hDlg, IDC_PROGRESS);
			::SendMessage(ctrl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		}

		return (INT_PTR)TRUE;

	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		if (dialog && !dialog->closeCallback.empty())
			dialog->closeCallback();
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			if (dialog && !dialog->closeCallback.empty())
				dialog->closeCallback();
			return (INT_PTR)TRUE;
		}
		break;

	case WM_PROGRESS:
		{
			HWND prog = ::GetDlgItem(hDlg, IDC_PROGRESS);
			DWORD style = GetWindowLongPtr(prog, GWL_STYLE);
			if ((style & PBS_MARQUEE) == 0)
				::SendMessage(prog, PBM_SETPOS, wParam, 0);
		}
		break;

	case WM_MARQUEE:
		{
			HWND prog = ::GetDlgItem(hDlg, IDC_PROGRESS);
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
	return (INT_PTR)FALSE;
}



CLegacyDialog::CLegacyDialog(HINSTANCE hInstance)
{
	icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOTSTRAPPER));

	INITCOMMONCONTROLSEX c;
	c.dwSize = sizeof(INITCOMMONCONTROLSEX);
	c.dwICC = ICC_PROGRESS_CLASS;
	::InitCommonControlsEx(&c);

	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProc, (LPARAM) this); 
	// TODO: Error check
}

CLegacyDialog::~CLegacyDialog(void)
{
	CloseDialog();
}

int CLegacyDialog::MessageBox(LPCWSTR text, LPCWSTR caption, UINT uType) 
{
	if ( GetSilentMode() )
		return 0;

	return ::MessageBox(hDlg, text, caption, uType);
}

void CLegacyDialog::ShowWindow()
{
	if (GetSilentMode())
	{
		return;
	}

	::ShowWindow(hDlg, SW_SHOWNORMAL);
}

void CLegacyDialog::CloseDialog(void)
{
	::DestroyWindow(hDlg);
}


void CLegacyDialog::SetMessage(const char* message)
{
	if (GetSilentMode())
	{
		return;
	}

	//ShowWindow();
	HWND ctrl = ::GetDlgItem(hDlg, IDC_MESSAGE);
	::SetWindowText(ctrl, convert_s2w(std::string(message)).c_str());
}

void CLegacyDialog::DisplayError(const char* message, const char* exceptionText)
{
	std::string text = message;
	if (exceptionText)
	{
		text += "\n\nDetails: ";
		text += exceptionText;
	}
	MessageBox(convert_s2w(text).c_str(), _T("Roblox"), MB_OK | MB_ICONERROR);
}

void CLegacyDialog::FinalMessage(const char* message)
{
	MessageBox(convert_s2w(message).c_str(), _T("Roblox"), MB_OK);
}

void CLegacyDialog::SetCancelEnabled(bool state)
{
	if (GetSilentMode())
	{
		return;
	}

	HWND ctrl = ::GetDlgItem(hDlg, IDCANCEL);
	::EnableWindow(ctrl, state ? TRUE : FALSE);
}

void CLegacyDialog::SetCancelVisible(bool state)
{
	if (GetSilentMode())
	{
		return;
	}

	HWND ctrl = ::GetDlgItem(hDlg, IDCANCEL);
	//::ShowWindow(ctrl, state ? TRUE : FALSE);
}

void CLegacyDialog::SetMarquee(bool state)
{
	if (GetSilentMode())
	{
		return;
	}

	SendMessage(hDlg, WM_MARQUEE, state ? TRUE : FALSE, 0);
}

void CLegacyDialog::SetProgress(int percent)
{
	if (GetSilentMode())
	{
		return;
	}

	SendMessage(hDlg, WM_PROGRESS, percent, 0);
}


