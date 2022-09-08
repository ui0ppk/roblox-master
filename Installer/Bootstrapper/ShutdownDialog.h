#pragma once

class CShutdownDialog
{
public:
	static CShutdownDialog* Create(HINSTANCE instance, HWND parent, const TCHAR* windowTitle);
	virtual ~CShutdownDialog() {}
	virtual bool IsDismissed(DWORD& result) = 0;	// result==IDOK means user requested a kill process
	virtual void CloseDialog() = 0;
};
