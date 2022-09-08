/*
 *  RobloxNPPlugin.h
 *  NPRobloxMac
 *
 *  Created by Aaron Wilkowski on 12/20/11.
 *  Copyright 2011 Roblox. All rights reserved.
 *
 */

#include <stdio.h>
#include <string>
#include <vector>

#include "RobloxLauncher.h"

//These files are different on mac and windows
#include "npapi.h"
#include "npfunctions.h"
#include "npruntime.h"

//TODO: remove and put into target settings
#define MAC_TARGET 


#ifdef MAC_TARGET
#define UTF8_LENGTH UTF8Length
#define UTF8_CHARACTERS UTF8Characters
#else
#define UTF8_LENGTH utf8length
#define UTF8_CHARACTERS utf8characters
#endif


//static NPNetscapeFuncs  *s_netscape_funcs            = NULL;


#define MIME_DESCRIPTION   "application/x-vnd-roblox-launcher:.launcher:info@roblox.com"

bool hasMethod(NPObject* obj, NPIdentifier methodName);
bool invoke(NPObject* obj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result);
bool hasProperty(NPObject *obj, NPIdentifier propertyName);
bool getProperty(NPObject *obj, NPIdentifier propertyName, NPVariant *result);
bool invokeDefault(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);

static NPClass s_npc_ref_obj = { NP_CLASS_STRUCT_VERSION,
	NULL,
	NULL,
	NULL,
	hasMethod,
	invoke,
	NULL,
	hasProperty,
	getProperty,
	NULL,
	NULL,
	NULL,
	NULL,
};

class RobloxNPPlugin
{
private:
	static RobloxNPPlugin * _instance;
	IRobloxLauncher *launcher;
	
public:
	static RobloxNPPlugin * Instance();
	//RobloxNPPlugin(IRobloxLauncher *launcher);
	//~RobloxNPPlugin(void);
	NPNetscapeFuncs  *npnfs;

	
	virtual NPError create(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved);
	virtual NPError destroy(NPP instance, NPSavedData **save);
	virtual NPError getValue(NPP instance, NPPVariable variable, void *value);
	virtual NPError handleEvent(NPP instance, void *ev);
	virtual NPError setWindow(NPP instance, NPWindow* pNPWindow);
	
	virtual void writeLogEntry(const char *format, ...);
	
	virtual IRobloxLauncher *find(NPObject *obj);

};










