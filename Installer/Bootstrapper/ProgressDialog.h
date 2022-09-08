#pragma once
#include "maindialog.h"
#include "../Bootstrapper/resource.h"

class CProgressDialog : public CMainDialog
{
	HINSTANCE hInstance;

	HFONT fontMsg;

	HWND hWndText;
	HWND hWndProgressBar;
	HWND hWndButton;

	HBITMAP bmpCancel;
	HBITMAP bmpCancelOn;

	bool dialogClosing;

protected:

	HWND hWndDialog;

	HBITMAP bmpLogo;

public:

	CProgressDialog(HINSTANCE hInstance);
	virtual ~CProgressDialog(void);

	LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HBITMAP getCancelBitmap(bool hover);

	virtual void InitDialog() {};
	virtual void ShowSuccessPrompt();
	
	int MessageBox(LPCWSTR text, LPCWSTR caption, UINT uType);
	void OnCreate(HWND hwnd);

	/*implement*/ HWND GetHWnd() const { return hWndDialog; }
	/*implement*/ void CloseDialog();
	/*implement*/ void SetMessage(const char* message);
	/*implement*/ void SetProgress(int percent);
	/*implement*/ void DisplayError(const char* message, const char* exceptionText);
	/*implement*/ void FinalMessage(const char* message);
	/*implement*/ void SetMarquee(bool state);	// switch progress bar Marquee mode
	
	

	static HWND CreateTextHelper(HWND hWndParent, HFONT font, int x, int y, int width, int height, bool hidden);

protected:

	virtual void InitDialog(WNDPROC wndProc);

	/*implement*/ void SetCancelEnabled(bool state);
	/*implement*/ void SetCancelVisible(bool state);
	/*implement*/ void ShowWindow();
};
