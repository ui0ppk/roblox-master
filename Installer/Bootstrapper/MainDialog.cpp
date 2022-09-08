#include "StdAfx.h"
#include "MainDialog.h"

#include "LegacyDialog.h"
#include "ProgressDialog.h"
#include "TaskDialog.h"
#include "VistaTools.h"

CMainDialog* CMainDialog::Create(HINSTANCE instance)
{
	CMainDialog *result = NULL;
/*
	We wont hide cancel button.
	Vista uses task dialog, I was not able to find
	the way to hide all buttons on TaskDialog.

	if (IsVista())
		result = new CTaskDialog(instance);
	else
*/
	result = new CProgressDialog(instance);

	result->SetCancelVisible(false);
	return result;
}

