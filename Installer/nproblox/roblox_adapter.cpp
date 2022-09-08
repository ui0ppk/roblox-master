#include <stdio.h>
#include <comdef.h>

#include "RobloxProxy_i.h"


const IID IID_ILauncher =
{0x699F0898,0xB7BC,0x4DE5,{0xAF,0xEE,0x0E,0xC3,0x8A,0xD4,0x22,0x40}};

#ifdef _WIN64
	const CLSID CLSID_Launcher =
	{0xDEE03C2B,0x0C0C,0x41A9,{0x98,0x77,0xFD,0x4B,0x4D,0x7B,0x6E,0xA3}};
#else
	const CLSID CLSID_Launcher =
	{0x76D50904,0x6780,0x4c8b,{0x89,0x86,0x1A,0x7E,0xE0,0xB1,0x71,0x6D}};
#endif

//=================================================================================
//=================COM Synchronization routing functions========================================
//=================================================================================

bool isComInitialized = false;

void StartCom(void){
	HRESULT	hr;
	if( !isComInitialized ){
		hr = CoInitialize(0);
		if( SUCCEEDED(hr) ){
			isComInitialized = true;
		}
	}
}

void StopCom(void){
	if( isComInitialized ){
		CoUninitialize();
		isComInitialized = false;
	}
}

bool isComStarted(void){
	return isComInitialized;
}

//=================================================================================
//=================String routing functions========================================
//=================================================================================

/**
* Conversion from ASCII to WideChar
*
*/

int Str2Wc( char* pBuf, WCHAR* pwcBuf, int bufLen){
	if( pwcBuf == NULL || pBuf == NULL )
		return -1;

	// If zero then assume the user has allocated enough
	if( bufLen == 0 ){
		bufLen = (int)( strlen(pBuf) + 1 );
	}
	if( MultiByteToWideChar( CP_ACP, 0, pBuf, -1, pwcBuf, bufLen) == 0 ){
		return -1;
	}
	pwcBuf[strlen(pBuf)] = 0;
	return 0;
}
/**
* Conversion from WideChar to ASCII
*
*/
int Wc2Str(WCHAR* pwcBuf, char* pBuf, int bufLen){
	if( pwcBuf == NULL || pBuf == NULL )
		return -1;

// If zero then assume the user has allocated enough
	if( bufLen == 0 ){
		bufLen = (int)( wcslen( pwcBuf) + 1 );
	}
	if( WideCharToMultiByte( CP_ACP, 0, pwcBuf, -1, pBuf, bufLen, NULL, NULL) == 0 ){
		return -1;
	}
	pBuf[wcslen(pwcBuf)] = 0;
	return 0;
}

//=================================================================================
//=================Roblox ActiveX functions call===================================
//=================================================================================

/**
* Create Roblox ActiveX instance
*
*/

ILauncher * GetIRobloxLauncherInstance(void){
	// Declare and HRESULT and a pointer to the RobloxProxy interface
	HRESULT	hr;
	ILauncher * IRobloxLauncher;
	if( isComStarted() ){
		hr = CoCreateInstance( CLSID_Launcher, NULL, CLSCTX_INPROC_SERVER,
						IID_ILauncher, (void**) &IRobloxLauncher);

		if( !SUCCEEDED(hr) ){
			IRobloxLauncher=NULL;
		}
	}
	return IRobloxLauncher;

}

/**
* Invoke HelloWorld method from Roblox ActiveX
*
*/
int RobloxAdapterHelloWorld(ILauncher *iIRobloxLauncher){
	HRESULT	hr = iIRobloxLauncher->HelloWorld();
	return hr;
}

/**
* Invoke Hello method from Roblox ActiveX
*
*/
int RobloxAdapterHello(ILauncher *iIRobloxLauncher,char *retMessage){
	HRESULT			hr;
	BSTR hhh;

	hr = -1;
	hr = iIRobloxLauncher->Hello( &hhh );
	if( SUCCEEDED(hr) ){
		Wc2Str( hhh, retMessage, 0 );
	}
	else {
		strcpy( retMessage, "IRobloxLauncher->Hello() Error occured !" );
	}

	return hr;
}
/**
* Invoke StartGame method from Roblox ActiveX
*
*/
int RobloxAdapterStartGame(ILauncher *iIRobloxLauncher,char *cbufAuth, char *cbufScript ){
	HRESULT			hr;
	_bstr_t authenticationUrl(cbufAuth);
	_bstr_t script(cbufScript);

	hr = -1;
	hr = iIRobloxLauncher->StartGame( authenticationUrl.GetBSTR(), 
									script.GetBSTR() );

	return hr;
}

/**
* Invoke PreStartGame method from Roblox ActiveX
*
*/
int RobloxAdapterPreStartGame(ILauncher *iIRobloxLauncher){
	HRESULT hr = -1;

	hr = iIRobloxLauncher->PreStartGame();

	return hr;
}

/**
* Invoke GetInstallHost method from Roblox ActiveX
*
*/
int RobloxAdapterGet_InstallHost(ILauncher *iIRobloxLauncher,char *cbufHost){
	HRESULT			hr;
	BSTR host;
	hr = -1;
	hr = iIRobloxLauncher->get_InstallHost( &host );

	if( SUCCEEDED(hr) ){
		Wc2Str( host, cbufHost, 0 );
	}
	else {
		strcpy( cbufHost, "IRobloxLauncher->get_InstallHost() Error occured !" );
	}


	return hr;
}

/**
* Invoke GetVersion method from Roblox ActiveX
*
*/
int RobloxAdapterGet_Version(ILauncher *iIRobloxLauncher,char *cbufHost) {
	HRESULT			hr;
	BSTR host;
	hr = -1;
	hr = iIRobloxLauncher->get_Version( &host );

	if( SUCCEEDED(hr) ){
		Wc2Str( host, cbufHost, 0 );
	}
	else {
		strcpy( cbufHost, "IRobloxLauncher->get_Version() Error occured !" );
	}


	return hr;
}

/**
* Invoke Update method from Roblox ActiveX
*
*/
int RobloxAdapterUpdate(ILauncher *iIRobloxLauncher){
	HRESULT			hr;
	hr = iIRobloxLauncher->Update();

	return hr;
}

/**
* Invoke PutAuthentificationTicket method from Roblox ActiveX
*
*/
int RobloxAdapterPut_AuthenticationTicket(ILauncher *iIRobloxLauncher,char *cbufTicket ){
	HRESULT			hr;
	_bstr_t ticket(cbufTicket);

	hr = -1;
	hr = iIRobloxLauncher->put_AuthenticationTicket(ticket.GetBSTR());

	return hr;
}

/**
* Invoke IsGameStarted method from Roblox ActiveX
*
*/
int RobloxAdapterGet_IsGameStarted(ILauncher *iIRobloxLauncher, bool *isStarted){
	HRESULT			hr;
	VARIANT_BOOL started;

	hr = -1;
	hr = iIRobloxLauncher->get_IsGameStarted(&started);
	(*isStarted) = (started == VARIANT_TRUE);

	return hr;
}

/**
* Invoke SetSilentModeEnabled method from Roblox ActiveX
*
*/
int RobloxAdapterGet_SetSilentModeEnabled(ILauncher *iIRobloxLauncher, bool silendMode){
	HRESULT			hr;
	VARIANT_BOOL silent = silendMode ? VARIANT_TRUE : VARIANT_FALSE;

	hr = -1;
	hr = iIRobloxLauncher->SetSilentModeEnabled(silent);

	return hr;
}

/**
* Invoke UnhideApp method from Roblox ActiveX
*
*/
int RobloxAdapterGet_UnhideApp(ILauncher *iIRobloxLauncher)
{
	HRESULT			hr;

	hr = -1;
	hr = iIRobloxLauncher->UnhideApp();

	return hr;
}

/**
* Invoke BringAppToFront method from Roblox ActiveX
*
*/
int RobloxAdapterBringAppToFront(ILauncher *iIRobloxLauncher)
{
	HRESULT			hr;

	hr = -1;
	hr = iIRobloxLauncher->BringAppToFront();

	return hr;
}

/**
* Invoke ResetLaunchState method from Roblox ActiveX
*
*/
int RobloxAdapterResetLaunchState(ILauncher *iIRobloxLauncher)
{
	HRESULT			hr;

	hr = -1;
	hr = iIRobloxLauncher->ResetLaunchState();

	return hr;
}

/**
* Invoke SetEditMode method from Roblox ActiveX
*
*/
int RobloxAdapterSetEditMode(ILauncher *iIRobloxLauncher)
{
	HRESULT			hr;

	hr = -1;
	hr = iIRobloxLauncher->SetEditMode();

	return hr;
}

/**
* Invoke SetStartInHiddenMode method from Roblox ActiveX
*
*/
int RobloxAdapterGet_SetStartInHiddenMode(ILauncher *iIRobloxLauncher, bool hiddenMode){
	HRESULT			hr;
	VARIANT_BOOL hidden = hiddenMode ? VARIANT_TRUE : VARIANT_FALSE;

	hr = -1;
	hr = iIRobloxLauncher->SetStartInHiddenMode(hidden);

	return hr;
}

/**
* Invoke get_IsUpToDate method from Roblox ActiveX
*
*/
int RobloxAdapterGet_IsUpToDate(ILauncher *iIRobloxLauncher, bool *isUpToDate){
	int hr;
	VARIANT_BOOL upToDate = VARIANT_FALSE;

	hr = -1;
	hr = iIRobloxLauncher->get_IsUpToDate(&upToDate);

	*isUpToDate = (upToDate == VARIANT_TRUE);
	
	return hr;
}

/**
* Invoke GetKeyValue method from Roblox ActiveX
*
*/
int RobloxAdapterGetKeyValue(ILauncher *iIRobloxLauncher, char *key, char *value)
{
	int hr = -1;
	_bstr_t bstrKey(key);
	BSTR bstrValue;
	VARIANT_BOOL exists = VARIANT_FALSE;

	hr = iIRobloxLauncher->GetKeyValue(bstrKey, &bstrValue);
	if (SUCCEEDED(hr))
	{
		Wc2Str(bstrValue, value, 0);
	} else {
		value = NULL;
	}

	return hr;
}

/**
* Invoke SetKeyValue method from Roblox ActiveX
*
*/
int RobloxAdapterSetKeyValue(ILauncher *iIRobloxLauncher, char *key, char *value)
{
	_bstr_t bstrKey(key);
	_bstr_t bstrValue(value);

	return iIRobloxLauncher->SetKeyValue(bstrKey, bstrValue);
}

/**
* Invoke DeleteKeyValue method from Roblox ActiveX
*
*/
int RobloxAdapterDeleteKeyValue(ILauncher *iIRobloxLauncher, char *key)
{
	_bstr_t bstrKey(key);

	return iIRobloxLauncher->DeleteKeyValue(bstrKey);
}