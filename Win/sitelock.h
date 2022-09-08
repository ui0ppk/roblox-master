///////////////////////////////////////////////////////////////////////////////
// SiteLock 1.14
// ATL sample code for restricting activation of ActiveX controls.
// Copyright Microsoft Corporation. All rights reserved.
// Last updated: July 19 2007

// Includes
#include <atlctl.h>
#include <comdef.h>
#include <shlguid.h>
#include <wininet.h>

// Pragmas
#pragma once

// Version information
#define SITELOCK_VERSION 0x00010014 // 1.14

#ifdef SITELOCK_SUPPORT_DOWNLOAD
#pragma message("sitelock.h : This version of SiteLock does not support downloading.")
#endif

// Overview:
// To enable scripting, developers must declare their ActiveX controls as "safe for scripting".
// This is done by implementing interface IObjectSafety, which comes with very important safety 
// assumptions. Once marked as "safe for scripting", ActiveX controls may be activated by untrusted 
// web sites. Therefore "safe for scripting" controls must guarantee all their methods are safe 
// regardless of the activation context. Practically however, it may not be possible for an ActiveX 
// control to guarantee safety in all activation contexts. The site lock framework allows developers 
// to specify which zones and domains can instantiate ActiveX controls. For example, this may allow 
// a developer to implement methods that can only be called if the activation context is the 
// intranet zone.

// Usage:
// 1/ Include current header file sitelock.h (after including ATL header files).
// 
// 2/ Derive from "public IObjectSafetySiteLockImpl <CYourClass, INTERFACESAFE_FOR...>,".
//    This replaces the default IObjectSafetyImpl interface implementation.  
//
// 3/ Add the following to your control's COM map:
//    COM_INTERFACE_ENTRY(IObjectSafety)
//    COM_INTERFACE_ENTRY(IObjectSafetySiteLock)
//
// 4/ Add one of the following to specify allowed activation contexts:
//    a) A public (or friend) member variable, for example:
//    const CYourClass::SiteList CYourClass::rgslTrustedSites[6] =
//       {{ SiteList::Deny,  L"http",  L"users.microsoft.com" },
//        { SiteList::Allow, L"http",  L"microsoft com"       },
//        { SiteList::Allow, L"http",  SITELOCK_INTRANET_ZONE },
//        { SiteList::Deny,  L"https", L"users.microsoft.com" },
//        { SiteList::Allow, L"https", L"microsoft.com"       },
//        { SiteList::Allow, L"https", SITELOCK_INTRANET_ZONE }};
// 
//	  b) A set of site lock macros, for example:
//    #define SITELOCK_USE_MAP (prior to including sitelock.h)
//    BEGIN_SITELOCK_MAP()
//        SITELOCK_DENYHTTP		( L"users.microsoft.com" )
//        SITELOCK_ALLOWHTTP	( L"microsoft com"       )
//        SITELOCK_ALLOWHTTP	( SITELOCK_INTRANET_ZONE )
//        SITELOCK_DENYHTTPS	( L"users.microsoft.com" )
//        SITELOCK_ALLOWHTTPS	( L"microsoft.com"       )
//        SITELOCK_ALLOWHTTPS	( SITELOCK_INTRANET_ZONE )
//    END_SITELOCK_MAP()
// 
//	  The examples above block "*.users.microsoft.com" sites (http and https).
//	  The examples above allow "*.microsoft.com" sites (http and https).
//	  The examples above allow intranet sites (http and https).
//
// 5/ Choose an expiry lifespan:
//    You can specify the lifespan of your control one of two ways.
//    By declaring an enumeration (slightly more efficient):
//       enum { dwControlLifespan = (lifespan in days) };
//    By declaring a member variable:
//       static const DWORD dwControlLifespan = (lifespan in days);
//    When in doubt, choose a shorter duration rather than a longer one. Expiration can be
//    disabled by adding #define SITELOCK_NO_EXPIRY before including sitelock.h.
//    
// 6/ Implement IObjectWithSite or IOleObject:
//    IObjectWithSite is a lightweight interface able to indicate the activation URL to site lock.
//    IOleObject is a heavier interface providing additional OLE capabilities.
//    If you need IOleObject, add #define SITELOCK_USE_IOLEOBJECT before including sitelock.h.
//    Otherwise, simply implement IObjectWithSite:
//			- Derive from "IObjectWithSiteImpl<CYourClass>".
//			- Add COM_INTERFACE_ENTRY(IObjectWithSite) to your control's COM map.
//    You should never implement both IObjectWithSite and IOleObject.
// 
// 7/ Link with urlmon.lib.

// Detailed usage:
//	 --- Entries ---:
//	 Site lock entries are defined by the following elements:
//	 iAllowType is:
//		SiteList::Allow:		allowed location
//      SiteList::Deny:			blocked location
//   szScheme is:
//		L"http":				non-SSL location
//		L"https":				SSL-enabled location
//		Other:					in rare cases, the scheme may be outlook:, ms-help:, etc.
//   szDomain is:
//		Doman:					a string defining a domain
//		Zone:					a constant specifying a zone
//
//	  --- Ordering ---:
//    Entries are matched in the order they appear in.
//    The first entry that matches will be accepted.
//	  Deny entries should therefore be placed before allow entries.
//
//	  --- Protocols ---:
//    To support multiple protocols (http and https), define separate entries.
//
//	  --- Domain names ---:
//    This sample code performs a case-sensitive comparison after domain normalization.
//	  Whether domain normalization converts strings to lower case depends on the scheme provider.
//
//    If a domain does not contain any special indicator, only domains with the right suffix will 
//    match. For example:
//		An entry of "microsoft.com" will match "microsoft.com".
//      An entry of "microsoft.com" will match "office.microsoft.com"
//      An entry of "microsoft.com" will not match "mymicrosoft.com"
//		An entry of "microsoft.com" will not match "www.microsoft.com.hacker.com"
//
//    If a domain begins with "*.", only child domains will match.
//	  For example:
//		An entry of "*.microsoft.com" will match "foo.microsoft.com".
//		An entry of "*.microsoft.com" will not match "microsoft.com".
//
//    If a domain begins with "=", only the specified domain will match.
//	  For example:
//		An entry of "=microsoft.com" will match "microsoft.com".
//		An entry of "=microsoft.com" will not match "foo.microsoft.com".
//
//    If a domain is set to "*", all domains will match.
//    This is useful to only restrict to specific schemes (ex: http vs. https).
//
//    If a domain name is NULL, then the scheme provider should return an error when asking for the 
//    domain. This is appropriate for protocols (outlook: or ms-help:) that do not use server names.
//
//    If a domain name is SITELOCK_INTRANET_ZONE, then any server in the Intranet zone will match. 
//    Due to a zone limitation, sites in the user's Trusted Sites list will also match. However, 
//    since Trusted Sites typically permit downloading and running of unsigned, unsafe controls, 
//    security is limited for those sites anyway.
//
//    If a domain name is SITELOCK_MYCOMPUTER_ZONE, then any page residing on the user's local 
//    machine will match.
//
//    If a domain name is SITELOCK_TRUSTED_ZONE, then any page residing in the user's Trusted 
//    Sites list will match.


// Language checks
#ifndef __cplusplus
#error ATL Requires C++
#endif

// Windows constants
#if (WINVER < 0x0600)
#define IDN_USE_STD3_ASCII_RULES  0x02  // Enforce STD3 ASCII restrictions
#endif

// Function prototypes
typedef int (WINAPI * PFN_IdnToAscii)(DWORD, LPCWSTR, int, LPWSTR, int); 

// Macros
#ifndef cElements
template<typename T> static char cElementsVerify(void const *, T) throw() { return 0; }
template<typename T> static void cElementsVerify(T *const, T *const *) throw() {};
#define cElements(arr) (sizeof(cElementsVerify(arr,&(arr))) * (sizeof(arr)/sizeof(*(arr))))
#endif

// Restrictions
#define SITELOCK_INTRANET_ZONE		((const OLECHAR *)-1)
#define SITELOCK_MYCOMPUTER_ZONE	((const OLECHAR *)-2)
#define SITELOCK_TRUSTED_ZONE		((const OLECHAR *)-3)

#ifndef SITELOCK_NO_EXPIRY
// Helper functions for expiry
#if defined(_WIN64) && defined(_M_IA64)
#pragma section(".base", long, read, write)
extern "C" __declspec(allocate(".base")) extern IMAGE_DOS_HEADER __ImageBase;
#else
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif
#define ImageNtHeaders(pBase) ((PIMAGE_NT_HEADERS)((PCHAR)(pBase) + ((PIMAGE_DOS_HEADER)(pBase))->e_lfanew))

#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))
inline void _UNIXTimeToFILETIME(time_t t, LPFILETIME ft)
{
	// The time_t is a 32-bit value for the number of seconds since January 1, 1970.
	// A FILETIME is a 64-bit for the number of 100-nanosecond periods since January 1, 1601.  
	// Convert by multiplying the time_t value by 1e+7 to get to the same base granularity,
	// then add the numeric equivalent of January 1, 1970 as FILETIME. 

	ULONGLONG qw = ((ULONGLONG)t * 10000000ui64) + 116444736000000000ui64;
	ft->dwHighDateTime = HIDWORD(qw);
	ft->dwLowDateTime = LODWORD(qw);
}
inline time_t _FILETIMEToUNIXTime(LPFILETIME ft)
{
	ULONGLONG qw = (((ULONGLONG)ft->dwHighDateTime)<<32) + ft->dwLowDateTime;
	return (time_t)((qw - 116444736000000000ui64) / 10000000ui64);
}
#endif // SITELOCK_NO_EXPIRY

// Interface declaring "safe for scripting" methods with additional site lock capabilities
class __declspec(uuid("7FEB54AE-E3F9-40FC-AB5A-28A545C0F193")) ATL_NO_VTABLE IObjectSafetySiteLock : public IObjectSafety
{
public:
	// Site lock entry definition
	struct SiteList {
		enum	SiteListCategory { 
			Allow,								// permit
			Deny,								// disallow
			Download							// OBSOLETE, do not use
		}						iAllowType;
		const	OLECHAR *		szScheme;		// scheme (http or https)
		const	OLECHAR *		szDomain;		// domain
	};

	// Capability definition
	enum		Capability {
		CanDownload    = 0x00000001,	// OBSOLETE. Here for backwards compatibility only.
		UsesIOleObject = 0x00000002,	// Use IOleObject instead of IObjectWithSite.
		HasExpiry	   = 0x00000004,	// Control will expire when lifespan elapsed.
	};				

	// Returns capabilities (this can be used by testing tools to query for custom capabilities or version information)
	STDMETHOD	(GetCapabilities)	(DWORD * pdwCapability)							= 0;

	// Returns site lock entries controlling activation
	STDMETHOD	(GetApprovedSites)	(const SiteList ** pSiteList, DWORD * cSites)	= 0;

	// Returns lifespan as number of days and date (version 1.05 or higher)
	STDMETHOD	(GetExpiryDate)		(DWORD * pdwLifespan, FILETIME * pExpiryDate)		= 0;
};


#ifdef SITELOCK_USE_MAP
// Site lock actual map entry macro
#define SITELOCK_ALLOWHTTPS(domain)		{IObjectSafetySiteLock::SiteList::Allow,	L"https",	domain},
#define SITELOCK_DENYHTTPS(domain)		{IObjectSafetySiteLock::SiteList::Deny,		L"https",	domain},
#define SITELOCK_ALLOWHTTP(domain)		{IObjectSafetySiteLock::SiteList::Allow,	L"http",	domain},
#define SITELOCK_DENYHTTP(domain)		{IObjectSafetySiteLock::SiteList::Deny,		L"http",	domain},

// Site lock begin map entry macro
#define BEGIN_SITELOCK_MAP() \
	static const IObjectSafetySiteLock::SiteList * GetSiteLockMapAndCount(DWORD * dwCount )	\
{																							\
	static IObjectSafetySiteLock::SiteList rgslTrustedSites[] = {							\

// Site lock end map entry macro
#define END_SITELOCK_MAP()																	\
{(IObjectSafetySiteLock::SiteList::SiteListCategory)0,0,0}};								\
	*dwCount = cElements(rgslTrustedSites) - 1;												\
	return rgslTrustedSites;																\
}																							\
	static const IObjectSafetySiteLock::SiteList * GetSiteLockMap()							\
{																							\
	DWORD dwCount = 0;																		\
	return GetSiteLockMapAndCount(&dwCount);												\
}																							\
	static DWORD GetSiteLockMapCount()														\
{																							\
	DWORD dwCount = 0;																		\
	GetSiteLockMapAndCount(&dwCount);														\
	return dwCount;																			\
}																							\

#endif // SITELOCK_USE_MAP

///////////////////////////////////////////////////////////////////////////////
// CSiteLock - Site lock templated class
template <typename T>
class ATL_NO_VTABLE CSiteLock
{
public:
#ifdef SITELOCK_NO_EXPIRY
	bool ControlExpired(DWORD = 0)		{ return false; }
#else
	bool ControlExpired(DWORD dwExpiresDays = T::dwControlLifespan)
	{
		SYSTEMTIME st = {0};
		FILETIME ft = {0};
		
		GetSystemTime(&st);
		if (!SystemTimeToFileTime(&st, &ft))
			return true;

		time_t ttTime = _FILETIMEToUNIXTime(&ft);
		time_t ttExpire = ImageNtHeaders(&__ImageBase)->FileHeader.TimeDateStamp;
		ttExpire += dwExpiresDays*86400;
		
		return (ttTime > ttExpire);
	}
#endif

#ifdef SITELOCK_USE_MAP		
	// Checks if the activation URL is in an allowed domain / zone
	bool InApprovedDomain(const IObjectSafetySiteLock::SiteList * rgslTrustedSites = T::GetSiteLockMap(), int cTrustedSites = T::GetSiteLockMapCount())
#else
	// Checks if the activation URL is in an allowed domain / zone
	bool InApprovedDomain(const IObjectSafetySiteLock::SiteList * rgslTrustedSites = T::rgslTrustedSites, int cTrustedSites = cElements(T::rgslTrustedSites))
#endif
	{
		// Retrieve the activation URL
		CComBSTR	bstrUrl;
		DWORD		dwZone		= URLZONE_UNTRUSTED;
		if (!GetOurUrl(bstrUrl, dwZone))
			return false;

		// Check if the activation URL is in an allowed domain / zone
		return FApprovedDomain(bstrUrl, dwZone, rgslTrustedSites, cTrustedSites);
	}

	// Retrieves the activation URL
	bool GetOurUrl(CComBSTR &bstrURL, DWORD &dwZone)
	{
		// Declarations
		HRESULT								hr				= S_OK;
		CComPtr<IServiceProvider>			spSrvProv;
		CComPtr<IInternetSecurityManager>	spInetSecMgr;
		CComPtr<IWebBrowser2>				spWebBrowser;

		// Get the current pointer as an instance of the template class
		T * pT = static_cast<T*>(this);

		// Retrieve the activation site
		CComPtr<IOleClientSite> spClientSite;
#ifdef SITELOCK_USE_IOLEOBJECT
		hr = pT->GetClientSite((IOleClientSite **)&spClientSite);
		if (FAILED(hr) || spClientSite == NULL)
			return false;
		hr = spClientSite->QueryInterface(IID_IServiceProvider, (void **)&spSrvProv);
#else
		hr = pT->GetSite(IID_IServiceProvider, (void**)&spSrvProv);
#endif
		if (FAILED(hr))
			return false;

		// Query the site for a web browser object
		hr = spSrvProv->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **)&spWebBrowser);
		if (FAILED(hr))
		{
			// Local declarations
			CComPtr<IHTMLDocument2>		spDoc;
			CComPtr<IOleContainer>		spContainer;
		
			// Reinitialize
			spSrvProv = NULL;
			spClientSite = NULL;

			// Get the client site, container, and provider, etc.
			hr = pT->GetSite(IID_IOleClientSite, (void**)&spClientSite);
			if (FAILED(hr))
				return false;
			hr = spClientSite->GetContainer(&spContainer);
			if (FAILED(hr))
				return false;
			hr = spContainer->QueryInterface(IID_IHTMLDocument2, (void **)&spDoc);
			if (FAILED(hr))
				return false;
			if (FAILED(spDoc->get_URL(&bstrURL)))
				return false;
			hr = spClientSite->QueryInterface(IID_IServiceProvider, (void **)&spSrvProv);
			if (FAILED(hr))
				return false;
		}
		else
		{
			// Query the web browser object for the activation URL
			hr = spWebBrowser->get_LocationURL(&bstrURL);
			if (FAILED(hr))
				return false;
		}

		// Query the site for its associated security manager
		hr = spSrvProv->QueryService(SID_SInternetSecurityManager, IID_IInternetSecurityManager, (void **)&spInetSecMgr);
		if (FAILED(hr))
			return false;

		// Query the security manager for the zone the activation URL belongs to
		hr = spInetSecMgr->MapUrlToZone(bstrURL, &dwZone, 0);
		if (FAILED(hr))
			return false;
		return true;
	}

private:
	// Checks if an activation URL is in an allowed domain / zone
	bool FApprovedDomain(const OLECHAR * wzUrl, DWORD dwZone, const IObjectSafetySiteLock::SiteList * rgslTrustedSites, int cTrustedSites)
	{
		// Declarations
		HRESULT		hr												= S_OK;
		OLECHAR		wzDomain[INTERNET_MAX_HOST_NAME_LENGTH + 1]		= {0};
		OLECHAR		wzScheme[INTERNET_MAX_SCHEME_LENGTH + 1]		= {0};

		// Retrieve the normalized domain and scheme
		hr = GetDomainAndScheme(wzUrl, wzScheme, cElements(wzScheme), wzDomain, cElements(wzDomain));
		if (FAILED(hr))
			return false;

		// Try to match the activation URL with each entry in order
		DWORD cbScheme = (::lstrlenW(wzScheme) + 1) * sizeof(OLECHAR);
		for (int i=0; i < cTrustedSites; i++)
		{
			// Try to match by scheme
			DWORD cbSiteScheme = (::lstrlenW(rgslTrustedSites[i].szScheme) + 1) * sizeof(OLECHAR);
			if (cbScheme != cbSiteScheme)
				continue;
			if (0 != ::memcmp(wzScheme, rgslTrustedSites[i].szScheme, cbScheme))
				continue;

			// Try to match by zone
			if (rgslTrustedSites[i].szDomain == SITELOCK_INTRANET_ZONE)
			{
				if ((dwZone == URLZONE_INTRANET) || (dwZone == URLZONE_TRUSTED))
					return rgslTrustedSites[i].iAllowType == IObjectSafetySiteLock::SiteList::Allow;
			}
			else if (rgslTrustedSites[i].szDomain == SITELOCK_MYCOMPUTER_ZONE)
			{
				if (dwZone == URLZONE_LOCAL_MACHINE)
					return rgslTrustedSites[i].iAllowType == IObjectSafetySiteLock::SiteList::Allow;
			}
			else if (rgslTrustedSites[i].szDomain == SITELOCK_TRUSTED_ZONE)
			{
				if (dwZone == URLZONE_TRUSTED)
					return rgslTrustedSites[i].iAllowType == IObjectSafetySiteLock::SiteList::Allow;
			}

			// Try to match by domain name
			else if (MatchDomains(rgslTrustedSites[i].szDomain, wzDomain))
			{
				return rgslTrustedSites[i].iAllowType == IObjectSafetySiteLock::SiteList::Allow;
			}
		}
		return false;
	};

	// Normalizes an international domain name
	HRESULT NormalizeDomain(OLECHAR * wzDomain, int cchDomain)
	{
		// Data validation
		if (!wzDomain)
			return E_POINTER;

		// If the domain is only 7-bit ASCII, normalization is not required
		bool fFoundUnicode = false;
		for (const OLECHAR * wz = wzDomain; *wz != 0; wz++)
		{
			if (0x80 <= *wz)
			{
				fFoundUnicode = true;
				break;
			}
		}
		if (!fFoundUnicode)
			return S_OK;

		// Construct a fully qualified path to the Windows system directory
		static const WCHAR wzNormaliz[] = L"normaliz.dll";
		static const int cchNormaliz = cElements(wzNormaliz);
		WCHAR wzDllPath[MAX_PATH + 1] = {0};
		if (!::GetSystemDirectoryW(wzDllPath, cElements(wzDllPath) - cchNormaliz - 1))
			return E_FAIL;
		int cchDllPath = ::lstrlenW(wzDllPath);
		if (!cchDllPath)
			return E_FAIL;
		if (wzDllPath[cchDllPath-1] != L'\\')
			wzDllPath[cchDllPath++] = L'\\';
		::CopyMemory(wzDllPath + cchDllPath, wzNormaliz, cchNormaliz * sizeof(WCHAR));

		// Load the DLL used for domain normalization
		HMODULE hNormaliz = ::LoadLibraryExW(wzDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!hNormaliz)
			return E_FAIL;

		HRESULT hr = E_FAIL;

		// Locate the entry point used for domain normalization
		PFN_IdnToAscii pfnIdnToAscii = (PFN_IdnToAscii)::GetProcAddress(hNormaliz, "IdnToAscii");
		if (!pfnIdnToAscii)
			goto cleanup;

		// Normalize the domain name
		WCHAR wzEncoded[INTERNET_MAX_HOST_NAME_LENGTH + 1];
		int cchEncode = pfnIdnToAscii(IDN_USE_STD3_ASCII_RULES, wzDomain, ::lstrlenW(wzDomain), wzEncoded, cElements(wzEncoded));
		if (0 == cchEncode)
		{
			hr = HRESULT_FROM_WIN32(::GetLastError());
			goto cleanup;
		}
		
		// Copy results to the input buffer
		if (cchEncode >= cchDomain)
		{
			hr = E_OUTOFMEMORY;
			goto cleanup;
		}

		::CopyMemory(wzDomain, wzEncoded, cchEncode * sizeof(WCHAR));
		hr = S_OK;

cleanup:
		if (hNormaliz)
			::CloseHandle(hNormaliz);

		return hr;
	}

	// Extracts a normalized domain and scheme from an activation URL
	HRESULT GetDomainAndScheme(const OLECHAR * wzUrl, OLECHAR * wzScheme, DWORD cchScheme, OLECHAR * wzDomain, DWORD cchDomain)
	{
		// Data validation
		if (!wzDomain || !wzScheme)
			return E_POINTER;

		// Extract the scheme
		HRESULT hr = ::UrlGetPartW(wzUrl, wzScheme, &cchScheme, URL_PART_SCHEME, 0);
		if (FAILED(hr))
			return E_FAIL;

		// Extract the host name
		DWORD cchDomain2 = cchDomain;
		hr = ::UrlGetPartW(wzUrl, wzDomain, &cchDomain2, URL_PART_HOSTNAME, 0);
		if (FAILED(hr))
			*wzDomain = 0;

		// Exclude any URL specifying a user name or password
		if ((0 == ::_wcsicmp(wzScheme, L"http")) || (0 == ::_wcsicmp(wzScheme, L"https")))
		{
			DWORD cch = 1;
			WCHAR wzTemp[1] = {0};
			::UrlGetPartW(wzUrl, wzTemp, &cch, URL_PART_USERNAME, 0);
			if (1 < cch)
				return E_FAIL;
			::UrlGetPartW(wzUrl, wzTemp, &cch, URL_PART_PASSWORD, 0);
			if (1 < cch)
				return E_FAIL;
		}

		// Normalize the domain name
		return NormalizeDomain(wzDomain, cchDomain);
	}

	// Attempts to match an activation URL with a domain name
	bool MatchDomains(const OLECHAR * wzTrustedDomain, const OLECHAR * wzOurDomain)
	{
		// Data validation
		if (!wzTrustedDomain)
			return (0 == *wzOurDomain); // match only if empty

		// Declarations
		int		cchTrusted			= ::lstrlenW(wzTrustedDomain);
		int		cchOur				= ::lstrlenW(wzOurDomain);
		bool	fForcePrefix		= false;
		bool	fDenyPrefix			= false;

		// Check if all activation URLs should be matched
		if (0 == ::wcscmp(wzTrustedDomain, L"*"))
			return true;

		// Check if the entry is like *. and setup the comparison range
		if ((2 < cchTrusted) && (L'*' == wzTrustedDomain[0]) && (L'.' == wzTrustedDomain[1]))
		{
			fForcePrefix = true;
			wzTrustedDomain += 2;
			cchTrusted -= 2;
		}

		// Check if the entry is like = and setup the comparison range
		else if ((1 < cchTrusted) && (L'=' == wzTrustedDomain[0]))
		{
			fDenyPrefix = true;
			wzTrustedDomain++;
			cchTrusted--;
		};

		// Check if there is a count mismatch
		if (cchTrusted > cchOur)
			return false;

		// Compare URLs on the desired character range
		if (0 != ::memcmp(wzOurDomain + cchOur - cchTrusted, wzTrustedDomain, cchTrusted * sizeof(OLECHAR)))
			return false;

		// Compare URLs without allowing child domains
		if (!fForcePrefix && (cchTrusted == cchOur))
			return true;

		// Compare URLs requiring child domains
		if (!fDenyPrefix && (wzOurDomain[cchOur - cchTrusted - 1] == L'.'))
			return true;

		return false;
	}
};

///////////////////////////////////////////////////////////////////////////////
// IObjectSafetySiteLockImpl - "Safe for scripting" template
template <typename T, DWORD dwSupportedSafety>
class ATL_NO_VTABLE IObjectSafetySiteLockImpl : public IObjectSafetySiteLock, public CSiteLock<T>
{
public:
	// Constructor
	IObjectSafetySiteLockImpl(): m_dwCurrentSafety(0) {}

	// Returns safety options
	STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD * pdwSupportedOptions, DWORD * pdwEnabledOptions)
	{
		// Data validation
		if (!pdwSupportedOptions || !pdwEnabledOptions)
			return E_POINTER;

		// Declarations
		HRESULT			hr		= S_OK;
		IUnknown *		pUnk	= NULL;

		// Get the current pointer as an instance of the template class
		T * pT = static_cast<T*>(this);

		// Check if the requested COM interface is supported
		hr = pT->GetUnknown()->QueryInterface(riid, (void**)&pUnk);
		if (FAILED(hr))
		{
			*pdwSupportedOptions = 0;
			*pdwEnabledOptions   = 0;
			return hr;
		}

		// Release the interface
		pUnk->Release();

		// Check expiry and if the activation URL is allowed
		if (!ControlExpired() && InApprovedDomain())
		{
			*pdwSupportedOptions = dwSupportedSafety;
			*pdwEnabledOptions   = m_dwCurrentSafety;
		}
		else
		{
			*pdwSupportedOptions = dwSupportedSafety;
			*pdwEnabledOptions   = 0;
		}
		return S_OK;
	}

	// Sets safety options
	STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
	{		
		// Declarations
		HRESULT			hr		= S_OK;
		IUnknown *		pUnk	= NULL;

		// Get the current pointer as an instance of the template class
		T * pT = static_cast<T*>(this);

		// Check if we support the interface and return E_NOINTERFACE if we don't
		// Check if the requested COM interface is supported
		hr = pT->GetUnknown()->QueryInterface(riid, (void**)&pUnk);
		if (FAILED(hr))
			return hr;

		// Release the interface
		pUnk->Release();

		// Reject unsupported requests
		if (dwOptionSetMask & ~dwSupportedSafety)
			return E_FAIL;

		// Calculate safety options
		DWORD dwNewSafety = (m_dwCurrentSafety  & ~dwOptionSetMask) | (dwOptionSetMask & dwEnabledOptions);
		if (m_dwCurrentSafety != dwNewSafety)
		{
			// Check expiry and if the activation URL is allowed
			if (ControlExpired() || !InApprovedDomain())
				return E_FAIL;

			// Set safety options
			m_dwCurrentSafety = dwNewSafety;
		}
		return S_OK;
	}

	// Returns capabilities (this can be used by testing tools to query for custom capabilities or version information)
	STDMETHOD(GetCapabilities)(DWORD * pdwCapability)
	{
		// Data validation
		if (!pdwCapability)
			return E_POINTER;

		// Return the version if 0 is passed in
		if (0 == *pdwCapability)
		{
			*pdwCapability = SITELOCK_VERSION;
			return S_OK;
		}

		// Return the options if 1 is passed in
		if (1 == *pdwCapability)
		{
			*pdwCapability =
#ifdef SITELOCK_USE_IOLEOBJECT
				Capability::UsesIOleObject |
#endif
#ifndef SITELOCK_NO_EXPIRY
				Capability::HasExpiry |
#endif
				0;
			return S_OK;
		}			

		// Return not implemented otherwise
		*pdwCapability = 0;
		return E_NOTIMPL;
	}

	// Returns site lock entries controlling activation
	STDMETHOD(GetApprovedSites)(const SiteList ** pSiteList, DWORD * pcEntries)
	{
		// Data validation
		if (!pSiteList || !pcEntries)
			return E_POINTER;

		// Return specified site lock entries
#ifdef SITELOCK_USE_MAP
		// Use the site lock map
		*pSiteList = T::GetSiteLockMapAndCount(*pcEntries);
#else
		// Use the static member
		*pSiteList = T::rgslTrustedSites;
		*pcEntries = cElements(T::rgslTrustedSites);
#endif
		return S_OK;
	}

	STDMETHOD(GetExpiryDate)(DWORD * pdwLifespan, FILETIME * pExpiryDate)
	{
		if (!pdwLifespan || !pExpiryDate)
			return E_POINTER;
	
#ifdef SITELOCK_NO_EXPIRY
		*pdwLifespan = 0;
		::ZeroMemory((void*)pExpiryDate, sizeof(FILETIME));
		return E_NOTIMPL;
#else
		*pdwLifespan = T::dwControlLifespan;

		// Calculate expiry date from life span
		time_t ttExpire = ImageNtHeaders(&__ImageBase)->FileHeader.TimeDateStamp;
		ttExpire += T::dwControlLifespan*86400;	// seconds per day
		_UNIXTimeToFILETIME(ttExpire, pExpiryDate);

		return S_OK;
#endif
	}

private:
	// Current safety
	DWORD m_dwCurrentSafety;
};
