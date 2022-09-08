#pragma once

#include "MainDialog.h"

class CLegacyDialog : public CMainDialog
{
	HWND hDlg;
	bool trackingMouse;
public:
	CLegacyDialog(HINSTANCE hInstance);
	~CLegacyDialog(void);

	bool isTrackingMouse() { return trackingMouse; }
	void setTrackingMouse(bool track) { trackingMouse = track; }
	virtual int MessageBox(LPCWSTR text, LPCWSTR caption, UINT uType);

	/*implement*/ HWND GetHWnd() const { return hDlg; }
	/*implement*/ void CloseDialog();
	/*implement*/ void SetMessage(const char* message);
	/*implement*/ void SetProgress(int percent);
	/*implement*/ void DisplayError(const char* message, const char* exceptionText);
	/*implement*/ void FinalMessage(const char* message);
	/*implement*/ void SetMarquee(bool state);	// switch progress bar Marquee mode
protected:
	/*implement*/ void SetCancelEnabled(bool state);
	/*implement*/ void SetCancelVisible(bool state);
	/*implement*/ void ShowWindow();
};
