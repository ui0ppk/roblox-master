#pragma once
#include "ProgressDialog.h"

class Bootstrapper;

class CStudioProgressDialog : public CProgressDialog
{
	HBITMAP bmpLaunchStudio;
	HBITMAP bmpLaunchStudioHover;
	HBITMAP bmpDevLink;
	HBITMAP bmpDevLinkHover;

	HWND hWndDeveloperPageText;
	HWND hInstallSuccess;
	HWND hWndBtnDevLink;
	HWND hWndBtnLaunchStudio;

	Bootstrapper* bs;

public:
	CStudioProgressDialog(HINSTANCE hInstance, Bootstrapper* bootstrapper);
	virtual ~CStudioProgressDialog(void);

	HBITMAP getLaunchStudioBitmap(bool hover);
	HBITMAP getDevLinkBitmap(bool hover);
	HWND getHWndDevLink() { return hWndBtnDevLink; }
	HWND getHWndBtnLaunchStudio() { return hWndBtnLaunchStudio; }

	void LaunchStudio();
	void OpenDevelopPage();

	// CProgressDialog
	/*implement*/ void ShowSuccessPrompt();
	/*implement*/ void InitDialog();
};
