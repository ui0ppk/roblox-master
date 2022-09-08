#pragma once

#include "boostfunction.hpp"

class CMainDialog
{
	bool silentMode;

public:
	static CMainDialog* Create(HINSTANCE instance);
	CMainDialog() : silentMode(false), showSuccessPrompt(false)
	{ 
		showWindowKey = 0; 
	}
	virtual ~CMainDialog() {}
	boost::function<void()> closeCallback;
	virtual int MessageBox(LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) = 0;
	virtual HWND GetHWnd() const = 0;
	
	void SetSilentMode(bool value) { silentMode = value; }
	bool GetSilentMode() { return silentMode; }

	bool isSuccessPromptShown(void) { return showSuccessPrompt; }

	enum ShowWindowKey
	{
		WindowTimeDelayShow			=0x1,
		WindowNoLongerBoringShow		=0x2,
		WindowSomethingInterestingShow=0x4,	//Always show the window when this is sent
	};
	enum ShowCancelKey
	{
		CancelTimeDelayHide		=0x1,
		CancelTimeDelayShow		=0x2,
		CancelHide				=0x4,
		CancelShow				=0x8,
	};
	virtual void InitDialog() = 0;
	virtual void CloseDialog() = 0;
	virtual void SetMessage(const char* message) = 0;
	virtual void SetProgress(int percent) = 0;
	virtual void DisplayError(const char* message, const char* exceptionText) = 0;
	virtual void FinalMessage(const char* message) = 0;
	virtual void SetMarquee(bool state) = 0;	// switch progress bar Marquee mode
	virtual void ShowSuccessPrompt() {};
protected:

	bool showSuccessPrompt;

	virtual void ShowWindow() = 0;
	virtual void SetCancelEnabled(bool state) = 0;
	virtual void SetCancelVisible(bool state) = 0;

private:
	unsigned int showWindowKey;
public:

    void setTitle(const std::wstring& title) { ::SetWindowText(GetHWnd(),title.c_str()); }

	void ShowWindow(ShowWindowKey key)
	{
		if(key == WindowSomethingInterestingShow){
			ShowWindow();
			return;
		}
		
		showWindowKey  |= key;
		if(showWindowKey == (WindowTimeDelayShow|WindowNoLongerBoringShow)){
			ShowWindow();
			return;
		}
	}
	void ShowCancelButton(ShowCancelKey key)
	{

		switch(key)
		{
			case CancelTimeDelayHide:		
				SetCancelVisible(false);		
				break;
			case CancelTimeDelayShow:		
				SetCancelVisible(true);			
				break;
			case CancelHide:				
				SetCancelEnabled(false);		
				break;
			case CancelShow:				
				SetCancelEnabled(true);		
				break;
		}

	}
};
