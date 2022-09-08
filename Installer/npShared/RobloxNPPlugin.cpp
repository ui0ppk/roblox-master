/*
 *  RobloxNPPlugin.cpp
 *  NPRobloxMac
 *
 *  Created by Aaron Wilkowski on 1/4/12.
 *  Copyright 2012 Roblox. All rights reserved.
 *
 */

#include <string>
#include "RobloxNPPlugin.h"
#include <map>





RobloxNPPlugin *RobloxNPPlugin::_instance = 0;

RobloxNPPlugin *RobloxNPPlugin::Instance(){
	if (_instance == 0) {
		_instance = new RobloxNPPlugin;
	}
	return _instance;
}

RobloxNPPlugin objList;
RobloxNPPlugin *sObjList = objList.Instance();

NPError aCreate(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved){
	return objList.create(pluginType, instance, mode, argc, argn, argv, saved);
}
NPError aDestroy(NPP instance, NPSavedData **save){
	return objList.destroy(instance, save);
}
NPError aGetValue(NPP instance, NPPVariable variable, void *value){
	return objList.getValue(instance, variable, value);
}
NPError aHandleEvent(NPP instance, void *ev){
	return objList.handleEvent(instance, ev);
}
NPError aSetWindow(NPP instance, NPWindow* pNPWindow){
	return objList.setWindow(instance, pNPWindow);
}

extern "C"
{
	//exposed functions
	
	NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *nppfuncs) 
	{
		objList.writeLogEntry("NP_GetEntryPoints --> Begin");
		
		if (nppfuncs == NULL)
		{
			objList.writeLogEntry("-- ERROR: nppfuncs is NULL");
			return NPERR_GENERIC_ERROR;
		}
		
		nppfuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
		nppfuncs->newp          = aCreate;
		nppfuncs->destroy       = aDestroy;
		nppfuncs->getvalue      = aGetValue;
		nppfuncs->event         = aHandleEvent;
		nppfuncs->setwindow     = aSetWindow;
		
		objList.writeLogEntry("NP_GetEntryPoints <-- End");
		
		return NPERR_NO_ERROR;
	}
	
#ifndef HIBYTE
#define HIBYTE(x) ((((uint32)(x)) & 0xff00) >> 8)
#endif
	
	NPError OSCALL NP_Initialize(NPNetscapeFuncs *npnf)
	{
		objList.writeLogEntry("NP_Initialize --> Begin");
		
		if(npnf == NULL)
		{
			objList.writeLogEntry("-- Error: npnf is NULL.");
			return NPERR_INVALID_FUNCTABLE_ERROR;
		}
		
		if (HIBYTE(npnf->version) > NP_VERSION_MAJOR)
		{
			objList.writeLogEntry("-- Error: version issue.");
			return NPERR_INCOMPATIBLE_VERSION_ERROR;
		}
		
		//s_netscape_funcs = npnf;
		sObjList->npnfs = npnf; 
		
		objList.writeLogEntry("NP_Initialize <-- End");
		
		return NPERR_NO_ERROR;
	}
	
	NPError OSCALL NP_Shutdown() 
	{
		objList.writeLogEntry("NP_Shutdown --> Begin");
		objList.writeLogEntry("NP_Shutdown <-- End\n");
		return NPERR_NO_ERROR;
	}
	
	const char *NP_GetMIMEDescription(void) 
	{
		return (const char *)MIME_DESCRIPTION;
	}
	
	
	/* needs to be present for WebKit based browsers */
	NPError OSCALL NP_GetValue(void *npp, NPPVariable variable, void *value) 
	{
		objList.writeLogEntry("NP_GetValue --> Begin");
		NPError ret_val = aGetValue((NPP)npp, variable, value);
		objList.writeLogEntry("NP_GetValue <-- End");
		return ret_val;
	}
}
//end C

static bool invokeRobloxAdapterHelloWorld(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterHelloWorld 0x%8.8x", (void*)obj);
	IRobloxLauncher * robloxLauncher = sObjList->find(obj);
	
	result->type = NPVariantType_Int32;
	
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		result->value.intValue = robloxLauncher->HelloWorld();
	}
	
	objList.writeLogEntry("NPRobloxProxy: RobloxAdapterHelloWorld result = %d", result->value.intValue);
	
	return true;
}

static bool invokeRobloxAdapterHello(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	char ssMess[2048];
	uint32_t size = (uint32_t)strlen(ssMess) + 1;
	objList.writeLogEntry("invokeRobloxAdapterHello 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->type = NPVariantType_Int32;
		result->value.intValue = -1;
	} else {
		result->type = NPVariantType_String;
		int hr = robloxLauncher->Hello(ssMess);
		objList.writeLogEntry("IRobloxLauncher->Hello() result %d", hr);
		size = (uint32_t)strlen(ssMess) + 1;
		result->value.stringValue.UTF8_LENGTH = size - 1;
		
		result->value.stringValue.UTF8_CHARACTERS = (const NPUTF8 *)sObjList->npnfs->memalloc(size);
		strcpy( (char *)result->value.stringValue.UTF8_CHARACTERS, ssMess );
	}
	
	objList.writeLogEntry("RobloxAdapterHello result=%d size=%d ssMess=%s", result->value.intValue, size, ssMess);
	
	return true;
}

static bool invokeRobloxAdapterStartGame(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterStartGame 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		std::vector<char> zauthFF(args[0].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zauthFF[0], args[0].value.stringValue.UTF8_CHARACTERS, args[0].value.stringValue.UTF8_LENGTH);
		zauthFF[args[0].value.stringValue.UTF8_LENGTH] = 0;
		
		std::vector<char> zscriptFF(args[1].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zscriptFF[0], args[1].value.stringValue.UTF8_CHARACTERS, args[1].value.stringValue.UTF8_LENGTH);
		zscriptFF[args[1].value.stringValue.UTF8_LENGTH] = 0;
		
		objList.writeLogEntry("RobloxAdapterStartGame Auth = %s, Script = %s", &zauthFF[0], &zscriptFF[0]);
		result->value.intValue = robloxLauncher->StartGame(&zauthFF[0], &zscriptFF[0]);
	}
	
	objList.writeLogEntry("RobloxAdapterStartGame result=%d", result->value.intValue);
	return true;
}

static bool invokeRobloxAdapterPreStartGame(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterPreStartGame 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		result->value.intValue = robloxLauncher->PreStartGame();
	}
	
	objList.writeLogEntry("RobloxAdapterPreStartGame result=%d", result->value.intValue);
	return true;
}


static bool invokeRobloxAdapterGet_InstallHost(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	char ssMess[2048];
	uint32_t size=(uint32_t)strlen(ssMess) + 1;
	
	objList.writeLogEntry("invokeRobloxAdapterGet_InstallHost 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->type = NPVariantType_Int32;
		result->value.intValue = -1;
	} else {
		result->type = NPVariantType_String;
		int hr = robloxLauncher->GetInstallHost(ssMess);
		
		size=(uint32_t)strlen(ssMess) + 1;
		// buggy way of doing things, once web site updated use commented line bellow
		result->value.stringValue.UTF8_LENGTH = size;
		// result->value.stringValue.UTF8_LENGTH = size - 1;
		
		result->value.stringValue.UTF8_CHARACTERS = (const NPUTF8 *) sObjList->npnfs->memalloc(size);
		strcpy((char *)result->value.stringValue.UTF8_CHARACTERS, ssMess);
	}
	
	objList.writeLogEntry("RobloxAdapterGet_InstallHost result=%d size=%d ssMess=%s", result->value.intValue, size, ssMess);
	
	return true;
}


static bool invokeRobloxAdapterUpdate(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterUpdate 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
	result->type = NPVariantType_Int32;
	if (robloxLauncher == NULL) {
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.intValue = -1;
	} else {
		result->value.intValue = robloxLauncher->Update();
	}
	
	return true;
}


static bool invokeRobloxAdapterPut_AuthenticationTicket(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterPut_AuthenticationTicket 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		std::vector<char> zticket(args[0].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zticket[0], args[0].value.stringValue.UTF8_CHARACTERS, args[0].value.stringValue.UTF8_LENGTH);
		zticket[args[0].value.stringValue.UTF8_LENGTH] = 0;
		
		objList.writeLogEntry("RobloxAdapterPut_AuthenticationTicket Ticket = %s", &zticket[0]);
		result->value.intValue = robloxLauncher->Put_AuthenticationTicket(&zticket[0]);
	}
	
	objList.writeLogEntry("RobloxAdapterPut_AuthenticationTicket result=%d", result->value.intValue);
	
	return true;
}


static bool invokeRobloxAdapterGet_IsGameStarted(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
	result->type = NPVariantType_Bool;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.boolValue = 0;
	} else {
		bool started;
		int hr = robloxLauncher->IsGameStarted(&started);
		objList.writeLogEntry("RobloxAdapterGet_IsGameStarted HRESULT = %d", hr);
		result->value.boolValue = started;
	}
	
	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted result=%d", result->value.boolValue);
	
	return true;
}

static bool invokeRobloxAdapterGet_SetSilentModeEnabled(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted 0x%8.8x", (void*)obj);
	
	char *cbufTicket = NULL;
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		result->value.intValue = robloxLauncher->SetSilentModeEnabled(NPVARIANT_TO_BOOLEAN(args[0]));
	}
	
	objList.writeLogEntry("invokeRobloxAdapterGet_IsGameStarted result=%d", result->value.intValue);
	
	return true;
}

static bool invokeRobloxAdapterGet_SetStartInHiddenMode(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_SetStartInHiddenMode 0x%8.8x", (void*)obj);
	
	char *cbufTicket = NULL;
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		result->value.intValue = robloxLauncher->SetStartInHiddenMode(NPVARIANT_TO_BOOLEAN(args[0]));
	}
	
	objList.writeLogEntry("invokeRobloxAdapterGet_SetStartInHiddenMode result=%d", result->value.intValue);
	
	return true;
}


static bool invokeRobloxAdapterGet_UnhideApp(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_UnhideApp 0x%8.8x", (void*)obj);
	
	char *cbufTicket = NULL;
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		result->value.intValue = robloxLauncher->UnhideApp();
	}
	
	objList.writeLogEntry("invokeRobloxAdapterGet_UnhideApp result=%d", result->value.intValue);
	
	return true;
}

static bool invokeRobloxAdapterBringAppToFront(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterBringAppToFront 0x%8.8x", (void*)obj);
	
	char *cbufTicket = NULL;
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		result->value.intValue = robloxLauncher->BringAppToFront();
	}
	
	objList.writeLogEntry("invokeRobloxAdapterBringAppToFront result=%d", result->value.intValue);
	
	return true;
}

static bool invokeRobloxAdapterResetLaunchState(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterResetLaunchState 0x%8.8x", (void*)obj);
	
	char *cbufTicket = NULL;
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
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
		
		result->value.intValue = robloxLauncher->ResetLaunchState();
	}
	
	objList.writeLogEntry("invokeRobloxAdapterBringAppToFront result=%d", result->value.intValue);
	
	return true;
}

static bool invokeRobloxAdapterGet_IsUpToDate(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGet_IsUpToDate 0x%8.8x", (void*)obj);
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
	
	result->type = NPVariantType_Bool;
	if (robloxLauncher == NULL){
		objList.writeLogEntry("ERROR: Launcher not found");
		result->value.boolValue = 0;
	} else {
		bool isUpToDate;
		int hr = robloxLauncher->IsUpToDate(&isUpToDate);
		objList.writeLogEntry("RobloxAdapterGet_IsUpToDate HRESULT = %d", hr);
		result->value.boolValue = isUpToDate;
	}
	
	objList.writeLogEntry("invokeRobloxAdapterGet_IsUpToDate result=%d", result->value.boolValue);
	
	return true;
}

static bool invokeRobloxAdapterGetKeyValue(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterGetKeyValue");
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
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
		char *key = (char *)args[0].value.stringValue.UTF8_CHARACTERS;
		
		std::vector<char> zkey(args[0].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zkey[0], args[0].value.stringValue.UTF8_CHARACTERS, args[0].value.stringValue.UTF8_LENGTH);
		zkey[args[0].value.stringValue.UTF8_LENGTH] = 0;
		
		objList.writeLogEntry("invokeRobloxAdapterGetKeyValue key = %s len = %d", &zkey[0], args[0].value.stringValue.UTF8_LENGTH);
		char strValue[2048] = {0};
		
		int hr = robloxLauncher->GetKeyValue(&zkey[0], strValue);
		if (hr > 0) // !Failed
		{
			int size = (uint32_t)strlen(strValue) + 1;
			result->value.stringValue.UTF8_LENGTH = size - 1;
			objList.writeLogEntry("invokeRobloxAdapterGetKeyValue value = %s len = %d", strValue, result->value.stringValue.UTF8_LENGTH);
			
			result->value.stringValue.UTF8_CHARACTERS = (const NPUTF8 *) sObjList->npnfs->memalloc(size);
			strcpy((char *)result->value.stringValue.UTF8_CHARACTERS, strValue);
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
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
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
		
		std::vector<char> zkey(args[0].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zkey[0], args[0].value.stringValue.UTF8_CHARACTERS, args[0].value.stringValue.UTF8_LENGTH);
		zkey[args[0].value.stringValue.UTF8_LENGTH] = 0;
		
		std::vector<char> zvalue(args[1].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zvalue[0], args[1].value.stringValue.UTF8_CHARACTERS, args[1].value.stringValue.UTF8_LENGTH);
		zvalue[args[1].value.stringValue.UTF8_LENGTH] = 0;
		
		result->value.intValue = robloxLauncher->SetKeyValue(&zkey[0], &zvalue[0]);
		if (result->value.intValue != 0) {
			objList.writeLogEntry("RobloxAdapterSetKeyValue returned %d", result->value.intValue);
		}
	}
	
	return true;
}


static bool invokeRobloxAdapterDeleteKeyValue(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	objList.writeLogEntry("invokeRobloxAdapterDeleteKeyValue");
	
	IRobloxLauncher *robloxLauncher = sObjList->find(obj);
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
		
		std::vector<char> zkey(args[0].value.stringValue.UTF8_LENGTH + 1);
		memcpy(&zkey[0], args[0].value.stringValue.UTF8_CHARACTERS, args[0].value.stringValue.UTF8_LENGTH);
		zkey[args[0].value.stringValue.UTF8_LENGTH] = 0;
		
		result->value.intValue = robloxLauncher->DeleteKeyValue(&zkey[0]);// RobloxAdapterDeleteKeyValue(robloxLauncher, &zkey[0]);
		
		//_launcher->DeleteKeyValue(%zkey[0]);
		
		if (result->value.intValue != 0) {
			objList.writeLogEntry("RobloxAdapterDeleteKeyValue returned %d", result->value.intValue);
		}
	}
	return true;
}



typedef bool (*invokePtr)(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);

static std::map<std::string, invokePtr> InitKnownMethods()
{
	std::map<std::string, invokePtr> result;
	
	// javascript interface method declaration
	result["HelloWorld"] = invokeRobloxAdapterHelloWorld;
	result["Hello"] = invokeRobloxAdapterHello;
	result["StartGame"] = invokeRobloxAdapterStartGame;
	result["PreStartGame"] = invokeRobloxAdapterPreStartGame;
	result["Get_InstallHost"] = invokeRobloxAdapterGet_InstallHost;
	result["Update"] = invokeRobloxAdapterUpdate;
	result["Put_AuthenticationTicket"] = invokeRobloxAdapterPut_AuthenticationTicket;
	result["Get_GameStarted"] = invokeRobloxAdapterGet_IsGameStarted;
	result["SetSilentModeEnabled"] = invokeRobloxAdapterGet_SetSilentModeEnabled;
	result["SetStartInHiddenMode"] = invokeRobloxAdapterGet_SetStartInHiddenMode;
	result["UnhideApp"] = invokeRobloxAdapterGet_UnhideApp;
	result["BringAppToFront"] = invokeRobloxAdapterBringAppToFront;
	result["ResetLaunchState"] = invokeRobloxAdapterResetLaunchState;
	result["Get_IsUpToDate"] = invokeRobloxAdapterGet_IsUpToDate;
	result["GetKeyValue"] = invokeRobloxAdapterGetKeyValue;
	result["SetKeyValue"] = invokeRobloxAdapterSetKeyValue;
	result["DeleteKeyValue"] = invokeRobloxAdapterDeleteKeyValue;
	
	
	
	return result;
}

/**
 * Invoke GetInstallHost method from Roblox ActiveX
 *
 */
bool invoke(NPObject* obj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result) {
	//gPlugin->invoke(obj, );
	objList.writeLogEntry("invoke");
	static std::map<std::string, invokePtr> methods = InitKnownMethods();
	
	char *name = sObjList->npnfs->utf8fromidentifier(methodName);
	if (name != NULL && methods[name] != NULL)
	{
		return methods[name](obj, args, argCount, result);
	} 
	else
	{
		// aim exception handling
		sObjList->npnfs->setexception(obj, "exception during invocation");
		return false;
	}
}

bool hasMethod(NPObject* obj, NPIdentifier methodName) 
{
	return true;
}

bool hasProperty(NPObject *obj, NPIdentifier propertyName) 
{
	return false;
}

bool getProperty(NPObject *obj, NPIdentifier propertyName, NPVariant *result) 
{
	return false;
}
