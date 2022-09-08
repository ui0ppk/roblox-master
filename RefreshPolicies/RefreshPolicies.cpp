// RefreshPolicies.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "RefreshPolicies.h"

#include   <msi.h>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

// http://blogs.msdn.com/ie/archive/2007/06/13/new-api-smoothes-extension-development-in-protected-mode.aspx
REFRESHPOLICIES_API UINT __stdcall RefreshPolicies(MSIHANDLE hModule)
{
    UINT hr = ERROR_SUCCESS;
    HMODULE hDll = LoadLibrary("ieframe.dll");
    if (NULL != hDll)
    {
        typedef HRESULT (*PFNIEREFRESHELEVATIONPOLICY)();
        PFNIEREFRESHELEVATIONPOLICY pfnIERefreshElePol = (PFNIEREFRESHELEVATIONPOLICY) GetProcAddress(hDll, "IERefreshElevationPolicy");
        if (pfnIERefreshElePol)
        {
            hr = pfnIERefreshElePol();
        } else {
             DWORD error = GetLastError(); 
             hr = HRESULT_FROM_WIN32(error);
         }
        FreeLibrary(hDll);
    } else {
       DWORD error = GetLastError(); 
       hr = HRESULT_FROM_WIN32(error);
    }
    return hr;
}

