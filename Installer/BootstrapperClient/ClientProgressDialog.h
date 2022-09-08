#pragma once

#include "ProgressDialog.h"

class Bootstrapper;

class CClientProgressDialog : public CProgressDialog
{
	Bootstrapper* bs;

	HWND hInstallSuccess;
	HWND hWndBntOk;
	
	HBITMAP bmpOk;
	HBITMAP bmpOkOn;

public:
	CClientProgressDialog(HINSTANCE hInstance, Bootstrapper* bootstrapper);
	virtual ~CClientProgressDialog(void);

	void InitDialog();
	void ShowSuccessPrompt();

	HBITMAP getOkBitmap(bool hover);

};
