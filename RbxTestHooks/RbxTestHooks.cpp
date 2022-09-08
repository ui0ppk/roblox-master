// RbxTestHooks.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "comutil.h"

#include "RbxTestHooks.h"
using namespace RBX;

HRESULT IWorkspaceTestHooks_ExecScript(IWorkspaceTestHooks *p, BSTR script, VARIANT arg1, VARIANT arg2, VARIANT arg3, VARIANT arg4, SAFEARRAY** result)
{
	return p->ExecScriptImpl(script, arg1, arg2, arg3, arg4, result);
}
