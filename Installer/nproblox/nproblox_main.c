#include <Windows.h>
#include <stdio.h>
#include <string.h>

#if defined(XULRUNNER_SDK)
#include <npapi.h>
#include "npupp.h"
#include <npruntime.h>
#elif defined(WEBKIT_DARWIN_SDK)
#include <Webkit/npapi.h>
#include <WebKit/npfunctions.h>
#include <WebKit/npruntime.h>
#define OSCALL
#elif defined(WEBKIT_WINMOBILE_SDK) /* WebKit SDK on Windows */
#ifndef PLATFORM
#define PLATFORM(x) defined(x)
#endif
#include <npfunctions.h>
#ifndef OSCALL
#define OSCALL WINAPI
#endif
#endif

#include <vector>
#include "RobloxProxy_i.h"
#include "rolbox_adapter.h"
#include "nppinstancelist.h"
#include <map>
#include <string>

typedef char* String;

// Constant declaration
const String kRobloxHost = "roblox.com";
const String kRobloxTestSiteHost = "robloxlabs.com";

//const String kRobloxHost = "LocalHost";

const String kMIMEDescription  = "application/x-vnd-roblox-launcher:.launcher:info@roblox.com";
const String kPluginDescriptionString = "<a href=\"http://www.roblox.com/\">RobloxLauncherPlugin</a> plugin.";
const String kStartComOk = "CoInitialize Ok";
const String kStartComFail = "CoInitialize fail";
const String kStopComOk = "CoUninitialize Ok";
const String kStopComFail = "CoUninitialize fail";


NPInstanceList objList;

//static NPObject *so              = NULL;
static NPNetscapeFuncs *npnfuncs = NULL;

//=================================================================================
//=================log functions ==================================================
//=================================================================================
void NPInstanceList::writeLogEntryRaw(char *entry)
{
	EnterCriticalSection(&logLock);
	SYSTEMTIME sysTime = {};
	GetSystemTime(&sysTime);
	char timeBuf[128];
	sprintf(timeBuf, "%02d-%02d-%d %02d:%02d:%02d ", sysTime.wMonth, sysTime.wDay, sysTime.wYear, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	log << timeBuf << entry << "\n";
	log.flush();
	LeaveCriticalSection(&logLock);
}

void NPInstanceList::writeLogEntry(char *format, ...)
{
	if (dontLog) {
		return;
	}
	

	const int stackBufSize = 512;
	char buf[stackBufSize];

	va_list args;
	va_start(args, format);

	int size = _vscprintf(format, args) + 1;
	if (size < stackBufSize)
	{
		vsprintf(buf, format, args);
		writeLogEntryRaw(buf);
	}
	else
	{
		//hopefully we won't go this pass very often
		std::vector<char> bbuf(size);
		vsprintf(&bbuf[0], format, args);
		writeLogEntryRaw(&bbuf[0]);
	}

	va_end(args);
}

bool isRobloxHost(NPP instance,char* hostString) {
	bool result = false;
	NPError err = -1;
    NPObject* sWindowObj;
	NPVariant rval;
	NPVariant vdoc;
	NPObject* doc;

    err = npnfuncs->getvalue(instance, NPNVWindowNPObject, &sWindowObj);

	err = npnfuncs->getproperty(instance, sWindowObj, npnfuncs->getstringidentifier("location"), &vdoc);
	doc = NPVARIANT_TO_OBJECT(vdoc);
	err = npnfuncs->getproperty(instance, doc,npnfuncs->getstringidentifier("host"), &rval);

	
	if (NPVARIANT_IS_STRING(rval)) {
		std::vector<char> zhost(rval.value.stringValue.utf8length + 1);
		memcpy(&zhost[0], rval.value.stringValue.utf8characters, rval.value.stringValue.utf8length);
		zhost[rval.value.stringValue.utf8length] = 0;

		char * siteCopy = _strdup(hostString);

		_strupr(&zhost[0]);
		_strupr(siteCopy);

		// search roblox site
		if (strstr(&zhost[0], siteCopy) != NULL){
			result = true;
			objList.writeLogEntry("location.host contains substring=%s", hostString);
		} else {
			objList.writeLogEntry("location.host DOES NOT contain substring=%s", hostString);
		}

		// release memory
		free(siteCopy);
	} else {// for Debug only
		//sprintf(buff, "rval IS NOT STRING\n");
		//logmsg(buff);
	}

	npnfuncs->releasevariantvalue(&rval);
	npnfuncs->releasevariantvalue(&vdoc);
	//npnfuncs->releaseobject(doc); //duplicate release of vdoc, might have caused problems
	npnfuncs->releaseobject(sWindowObj);

	return result;
}

//=================================================================================
//=================Roblox API implementation=======================================
//=================================================================================


/**
* Invoke HelloWorld method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterHelloWorld(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterHelloWorld 0x%8.8x", (void*)obj);
	ILauncher * robloxLauncher = objList.find(obj);
	
	result->type = NPVariantType_Int32;

	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		result->value.intValue = RobloxAdapterHelloWorld(robloxLauncher);
	}

	objList.writeLogEntry("NPRobloxProxy: RobloxAdapterHelloWorld result = %d", result->value.intValue);

	return true;
}


/**
* Invoke Hello method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterHello(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	char ssMess[2048];
	uint32_t size = (uint32_t)strlen(ssMess) + 1;
	objList.writeLogEntry("invokeRobloxAdapterHello 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->type = NPVariantType_Int32;
		result->value.intValue = -1;
	} else {
		result->type = NPVariantType_String;
		int hr = RobloxAdapterHello(robloxLauncher, ssMess);
		objList.writeLogEntry("IRobloxLauncher->Hello() result %d", hr);
		size = (uint32_t)strlen(ssMess) + 1;
		result->value.stringValue.utf8length = size - 1;

		result->value.stringValue.utf8characters = (const NPUTF8 *)npnfuncs->memalloc(size);
		strcpy( (char *)result->value.stringValue.utf8characters, ssMess );
	}

	objList.writeLogEntry("RobloxAdapterHello result=%d size=%d ssMess=%s", result->value.intValue, size, ssMess);

	return true;
}

/**
* Invoke StartGame method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterStartGame(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterStartGame 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 2) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterStartGame argCount!=2");
			return true;
		}
		if (args[0].type != NPVariantType_String || args[1].type != NPVariantType_String ){
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterStartGame Bad arguments type : MUST BE String");
			return true;
		}

		std::vector<char> zauthFF(args[0].value.stringValue.utf8length + 1);
		memcpy(&zauthFF[0], args[0].value.stringValue.utf8characters, args[0].value.stringValue.utf8length);
		zauthFF[args[0].value.stringValue.utf8length] = 0;

		std::vector<char> zscriptFF(args[1].value.stringValue.utf8length + 1);
		memcpy(&zscriptFF[0], args[1].value.stringValue.utf8characters, args[1].value.stringValue.utf8length);
		zscriptFF[args[1].value.stringValue.utf8length] = 0;

		objList.writeLogEntry("RobloxAdapterStartGame Auth = %s, Script = %s", &zauthFF[0], &zscriptFF[0]);
		result->value.intValue = RobloxAdapterStartGame(robloxLauncher, &zauthFF[0], &zscriptFF[0]);
	}

	objList.writeLogEntry("RobloxAdapterStartGame result=%d", result->value.intValue);
	return true;
}


/**
* Invoke PreStartGame method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterPreStartGame(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterPreStartGame 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 0) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterPreStartGame argCount!=0");
			return true;
		}

		result->value.intValue = RobloxAdapterPreStartGame(robloxLauncher);
	}

	objList.writeLogEntry("RobloxAdapterPreStartGame result=%d", result->value.intValue);
	return true;
}


/**
* Invoke GetInstallHost method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_InstallHost(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	char ssMess[2048];
	uint32_t size=(uint32_t)strlen(ssMess) + 1;

	objList.writeLogEntry("invokeRobloxAdapterGet_InstallHost 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->type = NPVariantType_Int32;
		result->value.intValue = -1;
	} else {
		result->type = NPVariantType_String;
		int hr = RobloxAdapterGet_InstallHost(robloxLauncher, ssMess);

		size=(uint32_t)strlen(ssMess) + 1;
		// buggy way of doing things, once web site updated use commented line bellow
		result->value.stringValue.utf8length = size;
		// result->value.stringValue.utf8length = size - 1;

		result->value.stringValue.utf8characters = (const NPUTF8 *) npnfuncs->memalloc(size);
		strcpy((char *)result->value.stringValue.utf8characters, ssMess);
	}

	objList.writeLogEntry("RobloxAdapterGet_InstallHost result=%d size=%d ssMess=%s", result->value.intValue, size, ssMess);

	return true;
}

/**
* Invoke GetVersion method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_Version(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	char ssMess[2048];
	uint32_t size=(uint32_t)strlen(ssMess) + 1;

	objList.writeLogEntry("invokeRobloxAdapterGet_Version 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->type = NPVariantType_Int32;
		result->value.intValue = -1;
	} else {
		result->type = NPVariantType_String;
		int hr = RobloxAdapterGet_Version(robloxLauncher, ssMess);

		size=(uint32_t)strlen(ssMess) + 1;
		// buggy way of doing things, once web site updated use commented line bellow
		result->value.stringValue.utf8length = size;
		// result->value.stringValue.utf8length = size - 1;

		result->value.stringValue.utf8characters = (const NPUTF8 *) npnfuncs->memalloc(size);
		strcpy((char *)result->value.stringValue.utf8characters, ssMess);
	}

	objList.writeLogEntry("RobloxAdapterGet_Version result=%d size=%d ssMess=%s", result->value.intValue, size, ssMess);

	return true;
}

/**
* Invoke Update method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterUpdate(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterUpdate 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		result->value.intValue = RobloxAdapterUpdate(robloxLauncher);
	}

	return true;
}


/**
* Invoke PutAuthentificationTicket method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterPut_AuthenticationTicket(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterPut_AuthenticationTicket 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 1) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterPut_AuthenticationTicket argCount!=1");
			return true;
		}

		if (args[0].type != NPVariantType_String) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterPut_AuthenticationTicket Bad arguments type : MUST BE String");
			return true;
		}
		std::vector<char> zticket(args[0].value.stringValue.utf8length + 1);
		memcpy(&zticket[0], args[0].value.stringValue.utf8characters, args[0].value.stringValue.utf8length);
		zticket[args[0].value.stringValue.utf8length] = 0;

		objList.writeLogEntry("RobloxAdapterPut_AuthenticationTicket Ticket = %s", &zticket[0]);
		result->value.intValue = RobloxAdapterPut_AuthenticationTicket(robloxLauncher, &zticket[0]);
	}

	objList.writeLogEntry("RobloxAdapterPut_AuthenticationTicket result=%d", result->value.intValue);
	
	return true;
}

/**
* Invoke IsGameStarted method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_IsGameStarted(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Bool;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.boolValue = 0;
	} else {
		bool started;
		int hr = RobloxAdapterGet_IsGameStarted(robloxLauncher, &started);
		objList.writeLogEntry("RobloxAdapterGet_IsGameStarted HRESULT = %d", hr);
		result->value.boolValue = started;
	}

	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted result=%d", result->value.boolValue);

	return true;
}

/**
* Invoke SetSilentModeEnabled method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_SetSilentModeEnabled(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted 0x%8.8x", (void*)obj);

	char *cbufTicket = NULL;

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 1) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterGet_SetSilentModeEnabled argCount!=1");
			return true;
		}

		if (args[0].type != NPVariantType_Bool) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterGet_SetSilentModeEnabled Bad arguments type : MUST be BOOL");
			return true;
		}

		result->value.intValue = RobloxAdapterGet_SetSilentModeEnabled(robloxLauncher, NPVARIANT_TO_BOOLEAN(args[0]));
	}

	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted result=%d", result->value.intValue);

	return true;
}

/**
* Invoke UnhideApp method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_UnhideApp(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_UnhideApp 0x%8.8x", (void*)obj);

	char *cbufTicket = NULL;

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 0) {
			result->value.intValue = -1;

			objList.writeLogEntry("invokeRobloxAdapterGet_UnhideApp argCount!=0");
			return true;
		}

		result->value.intValue = RobloxAdapterGet_UnhideApp(robloxLauncher);
	}

	objList.writeLogEntry("invokeRobloxAdapterGet_UnhideApp result=%d", result->value.intValue);

	return true;
}

/**
* Invoke BringAppToFront method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterBringAppToFront(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterBringAppToFront 0x%8.8x", (void*)obj);

	char *cbufTicket = NULL;

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 0) {
			result->value.intValue = -1;

			objList.writeLogEntry("invokeRobloxAdapterBringAppToFront argCount!=0");
			return true;
		}

		result->value.intValue = RobloxAdapterBringAppToFront(robloxLauncher);
	}

	objList.writeLogEntry("invokeRobloxAdapterBringAppToFront result=%d", result->value.intValue);

	return true;
}

/**
* Invoke ResetLaunchState method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterResetLaunchState(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterResetLaunchState 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 0) {
			result->value.intValue = -1;

			objList.writeLogEntry("invokeRobloxAdapterBringAppToFront argCount!=0");
			return true;
		}

		result->value.intValue = RobloxAdapterResetLaunchState(robloxLauncher);
	}

	objList.writeLogEntry("invokeRobloxAdapterBringAppToFront result=%d", result->value.intValue);

	return true;
}

/**
* Invoke SetEditMode method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterSetEditMode(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterSetEditMode 0x%8.8x", (void*)obj);

	char *cbufTicket = NULL;

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 0) {
			result->value.intValue = -1;

			objList.writeLogEntry("invokeRobloxAdapterSetEditMode argCount!=0");
			return true;
		}

		result->value.intValue = RobloxAdapterSetEditMode(robloxLauncher);
	}

	objList.writeLogEntry("invokeRobloxAdapterSetEditMode result=%d", result->value.intValue);

	return true;
}


/**
* Invoke SetStartInHiddenMode method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_SetStartInHiddenMode(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_SetStartInHiddenMode 0x%8.8x", (void*)obj);

	char *cbufTicket = NULL;

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		if (argCount != 1) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterGet_SetStartInHiddenMode argCount!=1");
			return true;
		}

		if (args[0].type != NPVariantType_Bool) {
			result->value.intValue = -1;

			objList.writeLogEntry("RobloxAdapterGet_SetStartInHiddenMode Bad arguments type : MUST be BOOL");
			return true;
		}

		result->value.intValue = RobloxAdapterGet_SetStartInHiddenMode(robloxLauncher, NPVARIANT_TO_BOOLEAN(args[0]));
	}

	objList.writeLogEntry("invokeRobloxAdapterGet_SetStartInHiddenMode result=%d", result->value.intValue);

	return true;
}

/**
* Invoke get_IsUpToDate method from Roblox ActiveX
*
*/
static bool invokeRobloxAdapterGet_IsUpToDate(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_IsUpToDate 0x%8.8x", (void*)obj);

	ILauncher *robloxLauncher = objList.find(obj);

	result->type = NPVariantType_Bool;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.boolValue = 0;
	} else {
		bool isUpToDate;
		int hr = RobloxAdapterGet_IsUpToDate(robloxLauncher, &isUpToDate);
		objList.writeLogEntry("RobloxAdapterGet_IsUpToDate HRESULT = %d", hr);
		result->value.boolValue = isUpToDate;
	}

	objList.writeLogEntry("invokeRobloxAdapterGet_IsUpToDate result=%d", result->value.boolValue);

	return true;
}

static bool invokeRobloxAdapterGetKeyValue(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGetKeyValue");

	ILauncher *robloxLauncher = objList.find(obj);
	result->type = NPVariantType_String;
	if (robloxLauncher == NULL) {
		result->type = NPVariantType_Int32;
		result->value.intValue = -1;
	} else {
		if (argCount != 1) {
			result->type = NPVariantType_Int32;
			result->value.intValue = -1;

			objList.writeLogEntry("invokeRobloxAdapterGetKeyValue argCount != 1");
			return true;
		}
		
		if (args[0].type != NPVariantType_String) {
			result->value.intValue = -1;
			result->type = NPVariantType_Int32;

			objList.writeLogEntry("invokeRobloxAdapterGetKeyValue arg[0] : must be string");
			return true;
		}
		char *key = (char *)args[0].value.stringValue.utf8characters;

		std::vector<char> zkey(args[0].value.stringValue.utf8length + 1);
		memcpy(&zkey[0], args[0].value.stringValue.utf8characters, args[0].value.stringValue.utf8length);
		zkey[args[0].value.stringValue.utf8length] = 0;
		
		objList.writeLogEntry("invokeRobloxAdapterGetKeyValue key = %s len = %d", &zkey[0], args[0].value.stringValue.utf8length);
		char strValue[2048] = {0};

		int hr = RobloxAdapterGetKeyValue(robloxLauncher, &zkey[0], strValue);
		if (!FAILED(hr))
		{
			int size = (uint32_t)strlen(strValue) + 1;
			result->value.stringValue.utf8length = size - 1;
			objList.writeLogEntry("invokeRobloxAdapterGetKeyValue value = %s len = %d", strValue, result->value.stringValue.utf8length);

			result->value.stringValue.utf8characters = (const NPUTF8 *) npnfuncs->memalloc(size);
			strcpy((char *)result->value.stringValue.utf8characters, strValue);
		} else {
			result->value.intValue = -1;
			result->type = NPVariantType_Int32;

			objList.writeLogEntry("invokeRobloxAdapterGetKeyValue failed error = %d", hr);
		}
	}
	return true;
}

static bool invokeRobloxAdapterSetKeyValue(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterSetKeyValue");

	ILauncher *robloxLauncher = objList.find(obj);
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	if (robloxLauncher != NULL) {
		if (argCount != 2) {
			objList.writeLogEntry("invokeRobloxAdapterSetKeyValue argCount != 2");
			return true;
		}

		if (args[0].type != NPVariantType_String) {
			objList.writeLogEntry("invokeRobloxAdapterSetKeyValue arg[0] : must be string");
			return true;
		}

		if (args[1].type != NPVariantType_String) {
			objList.writeLogEntry("invokeRobloxAdapterSetKeyValue arg[1] : must be string");
			return true;
		}

		std::vector<char> zkey(args[0].value.stringValue.utf8length + 1);
		memcpy(&zkey[0], args[0].value.stringValue.utf8characters, args[0].value.stringValue.utf8length);
		zkey[args[0].value.stringValue.utf8length] = 0;

		std::vector<char> zvalue(args[1].value.stringValue.utf8length + 1);
		memcpy(&zvalue[0], args[1].value.stringValue.utf8characters, args[1].value.stringValue.utf8length);
		zvalue[args[1].value.stringValue.utf8length] = 0;

		result->value.intValue = RobloxAdapterSetKeyValue(robloxLauncher, &zkey[0], &zvalue[0]);
		if (result->value.intValue != 0) {
			objList.writeLogEntry("RobloxAdapterSetKeyValue returned %d", result->value.intValue);
		}
	}

	return true;
}

static bool invokeRobloxAdapterDeleteKeyValue(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterDeleteKeyValue");

	ILauncher *robloxLauncher = objList.find(obj);
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	if (robloxLauncher != NULL) {
		if (argCount != 1) {
			objList.writeLogEntry("invokeRobloxAdapterDeleteKeyValue argCount != 1");
			return true;
		}

		if (args[0].type != NPVariantType_String) {
			objList.writeLogEntry("invokeRobloxAdapterDeleteKeyValue arg[0] : must be string");
			return true;
		}

		std::vector<char> zkey(args[0].value.stringValue.utf8length + 1);
		memcpy(&zkey[0], args[0].value.stringValue.utf8characters, args[0].value.stringValue.utf8length);
		zkey[args[0].value.stringValue.utf8length] = 0;

		result->value.intValue = RobloxAdapterDeleteKeyValue(robloxLauncher, &zkey[0]);
		if (result->value.intValue != 0) {
			objList.writeLogEntry("RobloxAdapterDeleteKeyValue returned %d", result->value.intValue);
		}
	}
	return true;
}

//=================================================================================
//=================NS API implementation===========================================
//=================================================================================

typedef bool (*invokePtr)(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);

static std::map<std::string, invokePtr> InitKnownMethods()
{
	std::map<std::string, invokePtr> result;

	// javascript interface method declaration
	result["HelloWorld"] = invokeRobloxAdapterHelloWorld;
	result["Hello"] = invokeRobloxAdapterHello;
	result["StartGame"] = invokeRobloxAdapterStartGame;
	result["Get_InstallHost"] = invokeRobloxAdapterGet_InstallHost;
	result["Get_Version"] = invokeRobloxAdapterGet_Version;
	result["Update"] = invokeRobloxAdapterUpdate;
	result["Put_AuthenticationTicket"] = invokeRobloxAdapterPut_AuthenticationTicket;
	result["Get_GameStarted"] = invokeRobloxAdapterGet_IsGameStarted;
	result["SetSilentModeEnabled"] = invokeRobloxAdapterGet_SetSilentModeEnabled;
	result["SetStartInHiddenMode"] = invokeRobloxAdapterGet_SetStartInHiddenMode;
	result["UnhideApp"] = invokeRobloxAdapterGet_UnhideApp;
	result["DeleteKeyValue"] = invokeRobloxAdapterDeleteKeyValue;
	result["Get_IsUpToDate"] = invokeRobloxAdapterGet_IsUpToDate;
	result["PreStartGame"] = invokeRobloxAdapterPreStartGame;
	result["GetKeyValue"] = invokeRobloxAdapterGetKeyValue;
	result["SetKeyValue"] = invokeRobloxAdapterSetKeyValue;
	result["DeleteKeyValue"] = invokeRobloxAdapterDeleteKeyValue;
	result["BringAppToFront"] = invokeRobloxAdapterBringAppToFront;
	result["ResetLaunchState"] = invokeRobloxAdapterResetLaunchState;
	result["SetEditMode"] = invokeRobloxAdapterSetEditMode;

	return result;
}

/**
* Invoke GetInstallHost method from Roblox ActiveX
*
*/
static bool invoke(NPObject* obj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invoke");
	static std::map<std::string, invokePtr> methods = InitKnownMethods();

	char *name = npnfuncs->utf8fromidentifier(methodName);
	if (name != NULL && methods[name] != NULL)
	{
		return methods[name](obj, args, argCount, result);
	} 
	else
	{
		// aim exception handling
		npnfuncs->setexception(obj, "exception during invocation");
		return false;
	}
}

static bool hasProperty(NPObject *obj, NPIdentifier propertyName) {
	objList.writeLogEntry("hasProperty");
	return false;
}

static bool getProperty(NPObject *obj, NPIdentifier propertyName, NPVariant *result) {
	objList.writeLogEntry("getProperty");
	return false;
}

static bool hasMethod(NPObject* obj, NPIdentifier methodName) {
	objList.writeLogEntry("hasMethod");
	return true;
}

static bool invokeDefault(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeDefault");
	result->type = NPVariantType_Int32;
	result->value.intValue = 42;
	return true;
}

static NPClass npcRefObject = {
	NP_CLASS_STRUCT_VERSION,
	NULL,
	NULL,
	NULL,
	hasMethod,
	invoke,
	invokeDefault,
	hasProperty,
	getProperty,
	NULL,
	NULL,
};

/* NPP */

static NPError nevv(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char *argn[], char *argv[], NPSavedData *saved) {
	objList.writeLogEntry("newv");

	// check if current page URL contains roblox site
	bool result = isRobloxHost(instance, kRobloxHost);
	if (!result)
		result = isRobloxHost(instance, kRobloxTestSiteHost);

	if( result == false ){
		objList.writeLogEntry("newv isRobloxHost = FALSE");
		return NPERR_NO_ERROR;
	}

	// create link between NPP instance and ILauncher * IRobloxLauncher 

	NP_ROBLOX_INSTANCE * object = (NP_ROBLOX_INSTANCE *)malloc(sizeof(NP_ROBLOX_INSTANCE));
	if (object == NULL) {
		objList.writeLogEntry("newv malloc(sizeof(NP_ROBLOX_INSTANCE) -> NULL");
		return NPERR_OUT_OF_MEMORY_ERROR;
	}

	object->instance = instance;
	// create ILauncher * IRobloxLauncher
	object->IRobloxLauncher = GetIRobloxLauncherInstance();
	if (object->IRobloxLauncher == NULL) {
		objList.writeLogEntry("newv GetIRobloxLauncherInstance() = NULL");
		return NPERR_INVALID_INSTANCE_ERROR;
	}

	// create NPObject *so
	object->so = npnfuncs->createobject(instance, &npcRefObject);
	if (object->so == NULL) {
		objList.writeLogEntry("newv npnfuncs->createobject(instance, &npcRefObject) = NULL");
		return NPERR_INVALID_INSTANCE_ERROR;
	}

	// now add this link to NPInstanceList objList;
	objList.add(object);

	return NPERR_NO_ERROR;
}

static NPError destroy(NPP instance, NPSavedData **save) {
	objList.writeLogEntry("destroy");

	NP_ROBLOX_INSTANCE * object = objList.findAndRemove(instance);
	if (object == NULL ) {
		return NPERR_INVALID_INSTANCE_ERROR;
	}

	// release ILauncher * IRobloxLauncher
	object->IRobloxLauncher->Release();
	// release NPObject *so
	npnfuncs->releaseobject(object->so);
	// now release memory
	free(object);

	return NPERR_NO_ERROR;
}

static NPError getValue(NPP instance, NPPVariable variable, void *value) {
	NP_ROBLOX_INSTANCE * object;
	switch(variable) {
	default:
		objList.writeLogEntry("getvalue - default");
		return NPERR_GENERIC_ERROR;
	case NPPVpluginNameString:
		objList.writeLogEntry("getvalue - name string");
		*((char **)value) = "RobloxLauncherPlugin";
		break;
	case NPPVpluginDescriptionString:
		objList.writeLogEntry("getvalue - description string");
		*((char **)value) = kPluginDescriptionString;
		break;
		
	case NPPVpluginScriptableNPObject:
		object = objList.find(instance);
		objList.writeLogEntry("getvalue - scriptable object : NPP instance=%p objList.find(instance)=%p", instance, object);
		if( object == NULL ) {
			return NPERR_INVALID_INSTANCE_ERROR;
		}
		npnfuncs->retainobject(object->so);
		*(NPObject **)value = object->so;
		break;
#if defined(XULRUNNER_SDK)
	case NPPVpluginNeedsXEmbed:
		objList.writeLogEntry("getvalue - xembed");
		*((PRBool *)value) = PR_FALSE;
		break;
#endif
	}
	return NPERR_NO_ERROR;
}

/* expected by Safari on Darwin */
static NPError  handleEvent(NPP instance, void *ev) {
	objList.writeLogEntry("handleEvent");
	return NPERR_NO_ERROR;
}

/* expected by Opera */
static NPError setWindow(NPP instance, NPWindow* pNPWindow) {
	objList.writeLogEntry("setWindow");
	return NPERR_NO_ERROR;
}

/* EXPORT */
#ifdef __cplusplus
extern "C" {
#endif

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *nppfuncs) {
	objList.writeLogEntry("NP_GetEntryPoints");
	nppfuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
	nppfuncs->newp          = nevv;
	nppfuncs->destroy       = destroy;
	nppfuncs->getvalue      = getValue;
	nppfuncs->event         = handleEvent;
	nppfuncs->setwindow     = setWindow;

	return NPERR_NO_ERROR;
}

#ifndef HIBYTE
#define HIBYTE(x) ((((uint32)(x)) & 0xff00) >> 8)
#endif

NPError OSCALL NP_Initialize(NPNetscapeFuncs *npnf
#if !defined(_WINDOWS) && !defined(WEBKIT_DARWIN_SDK)
			, NPPluginFuncs *nppfuncs)
#else
			)
#endif
{
	objList.writeLogEntry("NPRobloxProxy: NP_Initialize");
	if (npnf == NULL) {
		return NPERR_INVALID_FUNCTABLE_ERROR;
	}

	if (HIBYTE(npnf->version) > NP_VERSION_MAJOR) {
		return NPERR_INCOMPATIBLE_VERSION_ERROR;
	}

	npnfuncs = npnf;
#if !defined(_WINDOWS) && !defined(WEBKIT_DARWIN_SDK)
	NP_GetEntryPoints(nppfuncs);
#endif

	if (!isComStarted()){
		StartCom();
	}
	if (isComStarted()) {
		objList.writeLogEntry(kStartComOk);
	} else {
		objList.writeLogEntry(kStartComFail);
	}
	return NPERR_NO_ERROR;
}

NPError OSCALL NP_Shutdown() {
	objList.writeLogEntry("NPRobloxProxy: NP_Shutdown");

	if (isComStarted()){
		StopCom();
	}

	if (!isComStarted()) {
		objList.writeLogEntry(kStopComOk);
	} else {
		objList.writeLogEntry(kStopComFail);
	}

	return NPERR_NO_ERROR;
}

char * NP_GetMIMEDescription(void) {
	objList.writeLogEntry("NPRobloxProxy: NP_GetMIMEDescription");
	return kMIMEDescription;
}



/* needs to be present for WebKit based browsers */
NPError OSCALL NP_GetValue(void *npp, NPPVariable variable, void *value) {
	return getValue((NPP)npp, variable, value);
}

#ifdef __cplusplus
}
#endif
