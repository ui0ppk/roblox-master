// Launcher.h : Declaration of the CLauncher

#pragma once
#include "resource.h"       // main symbols

#include "RobloxProxy_i.h"
#include "_ILauncherEvents_CP.h"
#include "ProcessInformation.h"
#include "format_string.h"

#include "atlutil.h"
#include "atlsync.h"
#include "VistaTools.h"

#include "SharedLauncher.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

// CLauncher

class ATL_NO_VTABLE CLauncher :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CLauncher, &CLSID_Launcher>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CLauncher>,
	public CProxy_ILauncherEvents<CLauncher>,
	public IObjectWithSiteImpl<CLauncher>,
	public IProvideClassInfo2Impl<&CLSID_Launcher, NULL, &LIBID_RobloxProxyLib>,
	public IObjectSafetySiteLockImpl<CLauncher, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
	public IDispatchImpl<ILauncher, &IID_ILauncher, &LIBID_RobloxProxyLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
	CComBSTR authenticationTicket;
	CEvent syncGameStartEvent;
	CEvent unhideEvent;
	CEvent launcherStartedEvent;
	TCHAR guidString[MAX_PATH];
	bool silenMode;
	simple_logger<wchar_t> logger;
	TCHAR unhideEventName[MAX_PATH];
	bool startInHiddenMode;
	SharedLauncher::LaunchMode launchMode;
	bool stopLogsCleanup;
	ATL::CEvent clenupFinishedEvent;

	std::wstring getLowLogName()
	{
		VistaAPIs apis;
		std::wstring basePath;
		if (apis.isVistaOrBetter())
		{
			LPWSTR lppwstrPath = NULL;
			if (apis.SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, KF_FLAG_CREATE, NULL, &lppwstrPath) != E_NOTIMPL)
			{
				basePath = lppwstrPath;
				::CreateDirectory(basePath.c_str(), NULL);

				basePath += _T("\\RbxLogs\\");
				::CreateDirectory(basePath.c_str(), NULL);
				CoTaskMemFree((LPVOID)lppwstrPath);
			}
		}
		else
		{
			TCHAR szPath[_MAX_DIR]; 
			if (GetTempPath(_MAX_DIR, szPath))
			{
				basePath = szPath;
			}
		}

		GUID g = {0};
		::CoCreateGuid(&g);
		return format_string(_T("%sRBX-%08X.%s"), basePath.c_str(), g.Data1, _T("log"));
	}

	HRESULT ResetState()
	{
		silenMode = false;
		startInHiddenMode = false;
		launchMode = SharedLauncher::Play;

		GUID g1 = {0};
		HRESULT hr = CoCreateGuid(&g1);
		swprintf(guidString, MAX_PATH, _T("RBX-%d-%d-%d-%d%d%d%d%d%d%d%d"), g1.Data1, g1.Data2, g1.Data3, g1.Data4[0], g1.Data4[1], g1.Data4[2], g1.Data4[3], g1.Data4[4], g1.Data4[5], g1.Data4[6], g1.Data4[7]);
		syncGameStartEvent.Close();
		if (!syncGameStartEvent.Create(NULL, FALSE, FALSE, guidString))
		{
			hr = E_FAIL;
		}

		if (SUCCEEDED(hr))
		{
			syncGameStartEvent.Reset();
		}

		GUID g2 = {0};
		hr = CoCreateGuid(&g2);
		swprintf(unhideEventName, MAX_PATH, _T("RBX-%d-%d-%d-%d%d%d%d%d%d%d%d"), g2.Data1, g2.Data2, g2.Data3, g2.Data4[0], g2.Data4[1], g2.Data4[2], g2.Data4[3], g2.Data4[4], g2.Data4[5], g2.Data4[6], g2.Data4[7]);
		logger.write_logentry("CLauncher::ResetState unhideEventName=%S", unhideEventName);
		if (unhideEvent.Create(NULL, FALSE, FALSE, unhideEventName)) 
		{
			logger.write_logentry("CLauncher::ResetState unhide event created");
		}
		else
		{
			DWORD error = GetLastError();
			logger.write_logentry("CLauncher::ResetState event creation failed error=0x%X", error);
		}

		return S_OK;
	}

	void logsCleanUpHelper();
	static unsigned int __stdcall clenupThreadFunc(void *pThis);
public:
	static const SiteList rgslTrustedSites[];

	CLauncher() :
	silenMode(false),
		logger(getLowLogName().c_str()),
		startInHiddenMode(false),
		launchMode(SharedLauncher::Play),
		stopLogsCleanup(false),
		clenupFinishedEvent(FALSE, FALSE, NULL, NULL)
	{
	}

	~CLauncher()
	{
		logger.write_logentry("CLauncher::~CLauncher");
		m_isUpToDateProcessInfo.CloseProcess();
	}

	DECLARE_REGISTRY_RESOURCEID(IDR_LAUNCHER)


	BEGIN_COM_MAP(CLauncher)
		COM_INTERFACE_ENTRY(ILauncher)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY(IConnectionPointContainer)
		COM_INTERFACE_ENTRY(IObjectWithSite)
		COM_INTERFACE_ENTRY(IProvideClassInfo)
		COM_INTERFACE_ENTRY(IProvideClassInfo2)
		COM_INTERFACE_ENTRY(IObjectSafety)
		COM_INTERFACE_ENTRY(IObjectSafetySiteLock)
	END_COM_MAP()

	BEGIN_CONNECTION_POINT_MAP(CLauncher)
		CONNECTION_POINT_ENTRY(__uuidof(_ILauncherEvents))
	END_CONNECTION_POINT_MAP()
	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		logger.write_logentry("CLauncher::FinalConstruct");
		//_beginthreadex(NULL, 1024*1024, clenupThreadFunc, (void*)this, 0, NULL);
		return ResetState();
	}

	void FinalRelease()
	{
		//stopLogsCleanup = true;
		//WaitForSingleObject(clenupFinishedEvent, INFINITE);
	}

private:
	CRegKey GetKey(CString& operation);
	CProcessInformation m_isUpToDateProcessInfo;

public:

	STDMETHOD(StartGame)(BSTR authenticationUrl, BSTR script);
	STDMETHOD(HelloWorld)(void);
	STDMETHOD(Hello)(BSTR* result);
	STDMETHOD(get_InstallHost)(BSTR* pVal);
	STDMETHOD(Update)(void);
	STDMETHOD(put_AuthenticationTicket)(BSTR newVal);
	STDMETHOD(get_IsUpToDate)(VARIANT_BOOL* pVal);
	STDMETHOD(PreStartGame)(void);
	STDMETHOD(get_IsGameStarted(VARIANT_BOOL* pVal));
	STDMETHOD(SetSilentModeEnabled(VARIANT_BOOL silentModeEnbled));
	STDMETHOD(GetKeyValue(BSTR key, BSTR *pVal));
	STDMETHOD(SetKeyValue(BSTR key,	BSTR val));
	STDMETHOD(DeleteKeyValue(BSTR key));
	STDMETHOD(SetStartInHiddenMode(VARIANT_BOOL hiddenModeEnabled));
	STDMETHOD(UnhideApp());
	STDMETHOD(BringAppToFront());
	STDMETHOD(ResetLaunchState());
	STDMETHOD(SetEditMode());
	STDMETHOD(Get_Version)(BSTR* pVal);
};

OBJECT_ENTRY_AUTO(__uuidof(Launcher), CLauncher)
