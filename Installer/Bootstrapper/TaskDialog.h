#pragma once

#include "MainDialog.h"
#include "atlsync.h"
#include "boost/thread.hpp"

// Vista TaskDialog!
// Because TaskDialog is modal, we put it into another thread :)
class CTaskDialog : public CMainDialog
{
	CEvent readyEvent;
	CEvent doneEvent;
	HINSTANCE instance;
	HWND taskWnd;
	boost::mutex mut;
	bool isMarquee;
	int progress;
	bool cancelEnabled;
	std::string message;
	bool isStarted;
public:
	CTaskDialog(HINSTANCE instance);
	~CTaskDialog(void);

	void run();

	/*implement*/ int MessageBox(LPCWSTR text, LPCWSTR caption, UINT uType) {
		ShowWindow();
		// TODO: Use TaskDialog!!!!
		return ::MessageBox(taskWnd, text, caption, uType);
	}
	/*implement*/ HWND GetHWnd() const { ::WaitForSingleObject(readyEvent, INFINITE); return taskWnd; }
	/*implement*/ void CloseDialog();
	/*implement*/ void SetMessage(const char* message);
	/*implement*/ void SetProgress(int percent);
	/*implement*/ void DisplayError(const char* message, const char* exceptionText);
	/*implement*/ void FinalMessage(const char* message);
	/*implement*/ void SetMarquee(bool state);	// switch progress bar Marquee mode

	HRESULT onCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam);
	static HRESULT __stdcall callback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);

protected:
	/*implement*/ void SetCancelEnabled(bool state);
	/*implement*/ void SetCancelVisible(bool state) {}
	/*implement*/ void ShowWindow();
};
