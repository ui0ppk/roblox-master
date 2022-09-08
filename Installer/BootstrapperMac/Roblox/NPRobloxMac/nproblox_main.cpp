/*
 *  nproblox_main.cpp
 *  NPRobloxMac
 *
 *  Created by Dharmendra on 03/06/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "npapi.h"
#include "npfunctions.h"
#include "npruntime.h"

#include "CookiesEngine.h"

#define PLUGIN_NAME        "RobloxLauncherPlugin"
#define PLUGIN_DESCRIPTION "<a href=\"http://www.roblox.com/\">RobloxLauncherPlugin</a> plugin."

#define MIME_DESCRIPTION   "application/x-vnd-roblox-launcher:.launcher:info@roblox.com"

static const int MaxReTries = 10;

//plugin related data
typedef struct plugin_data
{
	NPP       instance;
	NPObject *scriptable_object;
} Plugin_Data;

enum StartMode
{
    STARTMODE_PLAY,
    STARTMODE_APP,
    STARTMODE_EDIT
};

static NPNetscapeFuncs  *s_netscape_funcs            = NULL;
static bool              s_delay_is_upto_date        = true;
static StartMode         startMode                   = STARTMODE_PLAY;
static StartMode         currentStartMode            = STARTMODE_PLAY;

static char             *s_temp_file                 = NULL;

//game authorization ticket
static char             *s_rbx_game_auth_ticket      = NULL;

//roblox identifiers
static const CFStringRef s_rbx_app_bundle_id         = CFSTR("com.Roblox.RobloxPlayer");
static const CFStringRef s_rbx_app_bootstrapper_bundle_id = CFSTR("com.Roblox.Roblox");
static const CFStringRef s_rbx_pref_file_id          = CFSTR("com.Roblox.Roblox");

static const CFStringRef s_rbx_install_hostKey       = CFSTR("RbxInstallHost");
static const CFStringRef s_rbx_plugin_version_key    = CFSTR("RbxPluginVersion");
static const CFStringRef s_rbx_client_app_path_key   = CFSTR("RbxClientAppPath");
static const CFStringRef s_rbx_bootstrapper_path_key = CFSTR("RbxBootstrapperPath");

//transient keys for plugin and application interaction
static const CFStringRef s_is_uptodate_key           = CFSTR("RbxIsUptoDate");
static const CFStringRef s_is_plugin_handling_key    = CFSTR("RbxIsPluginHandling");
static const CFStringRef s_update_cancelled_key      = CFSTR("RbxUpdateCancelled");

// studio specific keys
static const CFStringRef s_rbx_studio_app_bundle_id  = CFSTR("com.Roblox.RobloxStudio");
static const CFStringRef s_rbx_studio_app_path_key   = CFSTR("RbxStudioAppPath");
static const CFStringRef s_rbx_studio_bootstrapper_path_key = CFSTR("RbxStudioBootstrapperPath");
static const CFStringRef s_studio_update_cancelled_key = CFSTR("RbxStudioUpdateCancelled");
static const CFStringRef s_is_studio_uptodate_key    = CFSTR("RbxIsStudioUptoDate");

static const char       *s_rbx_host_str              = "roblox.com";
static const char		*s_rbx_host_test_str		 = "robloxlabs.com";

//function names that will be called from javascript
static const char       *s_hello_world_method        = "HelloWorld";
static const char       *s_hello_method              = "Hello";
static const char       *s_pre_start_game_method     = "PreStartGame";
static const char       *s_start_game_method         = "StartGame";
static const char       *s_get_game_started_method   = "Get_GameStarted";
static const char       *s_get_install_host_method   = "Get_InstallHost";
static const char       *s_get_plugin_version_method = "Get_Version";
static const char       *s_update_method             = "Update";
static const char       *s_get_is_uptodate_method    = "Get_IsUpToDate";
static const char       *s_put_auth_ticket_method    = "Put_AuthenticationTicket";
static const char       *s_set_silent_mode_method    = "SetSilentModeEnabled";
static const char       *s_get_key_value_method      = "GetKeyValue";
static const char       *s_set_key_value_method      = "SetKeyValue";
static const char       *s_delete_key_value_method   = "DeleteKeyValue";
static const char       *s_set_edit_mode_method      = "SetEditMode";

//functions called from javascript
static bool invoke_rbx_hello_world(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_hello(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_pre_start_game(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_start_game(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_get_game_started(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_get_install_host(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_get_plugin_version(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_update(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_get_is_uptodate(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_put_auth_ticket(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_get_key_value(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_set_key_value(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_delete_key_value(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke_rbx_set_edit_mode(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);

//functions called by browser to communicate with plugin
static NPError create(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved);
static NPError destroy(NPP instance, NPSavedData **save);
static NPError getValue(NPP instance, NPPVariable variable, void *value);
static NPError handleEvent(NPP instance, void *ev); //expected by Safari on Darwin
static NPError setWindow(NPP instance, NPWindow* pNPWindow); //expected by Opera

static bool hasMethod(NPObject* obj, NPIdentifier methodName);
static bool hasProperty(NPObject *obj, NPIdentifier propertyName);
static bool getProperty(NPObject *obj, NPIdentifier propertyName, NPVariant *result);
static bool invokeDefault(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result);
static bool invoke(NPObject* obj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result);

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

//utility functions
static char* getTempFile();
static void  logmsg_arg(const char *format, ...);
static void  logmsg(const char *msg);
static void  log_current_time();
static char* make_upper(char* ioString);
static char* to_char_string(CFStringRef str);
static void  copy_string(char *destination, const char *source, int string_length);

static bool is_roblox_host(NPP instance, const char *hostString);
static bool is_embedded_browser_view(NPP instance);
static bool get_application_path_ref(const CFStringRef path_key, FSRef *application_ref);

static bool is_app_running(const CFStringRef app_bundle_id, ProcessSerialNumber *psn);
static bool is_app_updated(const CFStringRef update_key);

// sync mutex and thread
static pthread_mutex_t  start_game_sync_mutex;
static pthread_t        studio_update_thread;

// shared data
enum StartGameState
{
    STARTGAMESTATE_UNINITIALIZED,
    STARTGAMESTATE_IN_PROGRESS,
    STARTGAMESTATE_COMPLETED
};
static StartGameState   start_game_state = STARTGAMESTATE_UNINITIALIZED;
static bool             request_thread_delete  = false;

struct WebData
{
    CFStringRef auth_url;
    CFStringRef ticket;
    CFStringRef script_url;
};

static bool  launch_player(struct WebData* args);
static void* update_and_launch_studio(void* id);
static bool  update_studio();
static bool  wait_for_update(const CFStringRef update_key, const CFStringRef cancelKey);

extern "C"
{
	//exposed functions
	NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *nppfuncs)
	{
		logmsg("NP_GetEntryPoints --> Begin");
		
		if (nppfuncs == NULL)
		{
			logmsg("-- ERROR: nppfuncs is NULL");
			return NPERR_GENERIC_ERROR;
		}
		
		nppfuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
		nppfuncs->newp          = create;
		nppfuncs->destroy       = destroy;
		nppfuncs->getvalue      = getValue;
		nppfuncs->event         = handleEvent;
		nppfuncs->setwindow     = setWindow;
		
		logmsg("NP_GetEntryPoints <-- End");
		
		return NPERR_NO_ERROR;
	}
	
#ifndef HIBYTE
#define HIBYTE(x) ((((uint32)(x)) & 0xff00) >> 8)
#endif
	
	NPError OSCALL NP_Initialize(NPNetscapeFuncs *npnf)
	{
		logmsg("NP_Initialize --> Begin");
		
		if(npnf == NULL)
		{
			logmsg("-- Error: npnf is NULL.");
			return NPERR_INVALID_FUNCTABLE_ERROR;
		}
		
		if (HIBYTE(npnf->version) > NP_VERSION_MAJOR)
		{
			logmsg("-- Error: version issue.");
			return NPERR_INCOMPATIBLE_VERSION_ERROR;
		}
		
		s_netscape_funcs = npnf;
		
		logmsg("NP_Initialize <-- End");
		
		return NPERR_NO_ERROR;
	}
	
	NPError OSCALL NP_Shutdown()
	{
		logmsg("NP_Shutdown --> Begin");
		logmsg("NP_Shutdown <-- End\n");
		return NPERR_NO_ERROR;
	}
	
	const char* NP_GetMIMEDescription(void)
	{
		return (const char *)MIME_DESCRIPTION;
	}
	
	/* needs to be present for WebKit based browsers */
	NPError OSCALL NP_GetValue(void *npp, NPPVariable variable, void *value)
	{
		logmsg("NP_GetValue --> Begin");
		NPError ret_val = getValue((NPP)npp, variable, value);
		logmsg("NP_GetValue <-- End");
		return ret_val;
	}
}

//static functions
static NPError create(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
{
	logmsg("Create --> Begin");
	bool result = false;
	
	// check if current page URL contains roblox site
	if( (!is_roblox_host(instance, s_rbx_host_str) && !is_roblox_host(instance, s_rbx_host_test_str))
       || is_embedded_browser_view(instance))
	{
		logmsg("-- WARINING: Not a roblox host or is an embedded browser");
        logmsg("checking with: ");
        logmsg(s_rbx_host_str);
        logmsg("  and  ");
        logmsg(s_rbx_host_test_str);
        
		return NPERR_GENERIC_ERROR;
		//return NPERR_NO_ERROR;
	}
	
	log_current_time();
	
	//create data associated with the instance
	Plugin_Data *plugin_data = (Plugin_Data*)malloc(sizeof(Plugin_Data));
	plugin_data->instance = instance;
	plugin_data->scriptable_object = s_netscape_funcs->createobject(instance, &s_npc_ref_obj);
	
	//add data with the instance
	instance->pdata = plugin_data;
    
    //initialize mutex
    pthread_mutex_init(&start_game_sync_mutex, 0);
    
    // make sure we remove transient key (it will be set only when we update application)
    CFPreferencesSetAppValue(s_is_plugin_handling_key, NULL, s_rbx_pref_file_id);
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	
    // Setting CoreGraphics to NPDrawingModelCoreGraphics and setting CocoaEvents
    // explicitly is required for plugin to start up (in Chrome as of Oct 2012).
    // select the right drawing model if necessary
    NPBool supportsCoreGraphics = false;
    if (s_netscape_funcs->getvalue(instance, NPNVsupportsCoreGraphicsBool,
                                   &supportsCoreGraphics) == NPERR_NO_ERROR && supportsCoreGraphics) {
        
        s_netscape_funcs->setvalue(instance, NPPVpluginDrawingModel,
                                   (void*)NPDrawingModelCoreGraphics);
    }
    
    // select the Cocoa event model
    NPBool supportsCocoaEvents = false;
    if (s_netscape_funcs->getvalue(instance, NPNVsupportsCocoaBool,
                                   &supportsCocoaEvents) == NPERR_NO_ERROR && supportsCocoaEvents) {
        
        s_netscape_funcs->setvalue(instance, NPPVpluginEventModel,
                                   (void*)NPEventModelCocoa);
    }
    
	logmsg("Create <-- End");
	return NPERR_NO_ERROR;
}

static NPError destroy(NPP instance, NPSavedData **save)
{
	logmsg("Destroy --> Begin");
	
	if (s_rbx_game_auth_ticket)
		free(s_rbx_game_auth_ticket);
	s_rbx_game_auth_ticket = NULL;
	
	//release memory for associated data
	Plugin_Data *plugin_data = (Plugin_Data *)instance->pdata;
	if (plugin_data)
	{
		s_netscape_funcs->releaseobject(plugin_data->scriptable_object);
		free(plugin_data);
		instance->pdata = NULL;
	}
    
    //destroy mutex
    pthread_mutex_destroy(&start_game_sync_mutex);
	
	log_current_time();
	
	logmsg("Destroy <-- End");
	return NPERR_NO_ERROR;
}

static NPError getValue(NPP instance, NPPVariable variable, void *value)
{
	logmsg("getValue --> Begin");
	
	Plugin_Data *plugin_data = (Plugin_Data *)instance->pdata;
	if (!plugin_data)
	{
		logmsg("-- ERROR: Plugin data NULL");
		return NPERR_GENERIC_ERROR;
	}
	
	switch(variable)
	{
		case NPPVpluginNameString:
		{
			logmsg("-- NPPVpluginNameString");
			*((char **)value) = (char *)PLUGIN_NAME;
			break;
		}
		case NPPVpluginDescriptionString:
		{
			logmsg("-- NPPVpluginDescriptionString");
			*((char **)value) = (char *)PLUGIN_DESCRIPTION;
			break;
		}
		case NPPVpluginScriptableNPObject:
		{
			logmsg("-- NPPVpluginScriptableNPObject");
			NPObject *so = plugin_data->scriptable_object;
			s_netscape_funcs->retainobject(so);
			*(NPObject **)value = so;
			break;
		}
		default:
		{
			logmsg_arg("-- Error: Not supported for %d", variable);
			return NPERR_GENERIC_ERROR;
		}
			/*case NPPVpluginNeedsXEmbed:
			 logmsg("getvalue - xembed");
			 *((bool *)value) = true;
			 break;
			 */
	}
	
	logmsg("getValue <-- End");
	return NPERR_NO_ERROR;
}

/* expected by Safari on Darwin */
static NPError handleEvent(NPP instance, void *ev)
{
	return NPERR_NO_ERROR;
}

/* expected by Opera */
static NPError setWindow(NPP instance, NPWindow* pNPWindow)
{
	return NPERR_NO_ERROR;
}

static bool hasMethod(NPObject* obj, NPIdentifier methodName)
{
	return true;
}

static bool hasProperty(NPObject *obj, NPIdentifier propertyName)
{
	return false;
}

static bool getProperty(NPObject *obj, NPIdentifier propertyName, NPVariant *result)
{
	return false;
}

static bool invokeDefault(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invokeDefault --> Begin");
	result->type = NPVariantType_Int32;
	result->value.intValue = 0;
	logmsg("invokeDefault <-- End");
	return true;
}

static bool invoke(NPObject* obj, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	char *name = s_netscape_funcs->utf8fromidentifier(methodName);
	logmsg_arg("invoke method = %s", name);
	
	if(name)
	{
		if (!strcmp((const char *)name, s_hello_world_method))
			return invoke_rbx_hello_world(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_hello_method))
			return invoke_rbx_hello(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_pre_start_game_method))
			return invoke_rbx_pre_start_game(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_start_game_method))
			return invoke_rbx_start_game(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_get_game_started_method))
			return invoke_rbx_get_game_started(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_get_install_host_method))
			return invoke_rbx_get_install_host(obj, args, argCount, result);
        
        if (!strcmp((const char *)name, s_get_plugin_version_method))
			return invoke_rbx_get_plugin_version(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_update_method))
			return invoke_rbx_update(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_get_is_uptodate_method))
			return invoke_rbx_get_is_uptodate(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_put_auth_ticket_method))
			return invoke_rbx_put_auth_ticket(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_get_key_value_method))
			return invoke_rbx_get_key_value(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_set_key_value_method))
			return invoke_rbx_set_key_value(obj, args, argCount, result);
		
		if (!strcmp((const char *)name, s_delete_key_value_method))
			return invoke_rbx_delete_key_value(obj, args, argCount, result);
        
        if (!strcmp((const char *)name, s_set_edit_mode_method))
            return invoke_rbx_set_edit_mode(obj, args, argCount, result);
        
		//TODO: provide implementation for the following methods
		if (!strcmp((const char *)name, s_set_silent_mode_method))
			return invokeDefault(obj, args, argCount, result);
	}
	
	logmsg("-- ERROR: Method not found");
	
	s_netscape_funcs->setexception(obj, "exception during invocation");
	
	return false;
}

static bool invoke_rbx_hello_world(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_hello_world --> Begin");
	
	result->type           = NPVariantType_Int32;
	result->value.intValue = 0;
	
	//display a notification dialog
	CFUserNotificationDisplayNotice(0,
									kCFUserNotificationNoteAlertLevel,
									NULL,
									NULL,
									NULL,
									CFSTR("NP Roblox"),
									CFSTR("Hello World"),
									NULL);
	
	logmsg("invoke_rbx_hello_world <-- End");
    
	return true;
}

static bool invoke_rbx_hello(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_hello --> Begin");
	
	const char *worldStr                     = "World";
	uint32_t size                            = (uint32_t)strlen(worldStr) + 1;
	
	result->type                             = NPVariantType_String;
	result->value.stringValue.UTF8Length     = size;
	
	result->value.stringValue.UTF8Characters = (const NPUTF8 *)s_netscape_funcs->memalloc(size);
	strcpy((char *)result->value.stringValue.UTF8Characters, worldStr); //allocated memory will be released by browser
	
	logmsg("invoke_rbx_hello <-- End");
	
	return true;
}

static bool invoke_rbx_pre_start_game(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	//logmsg("invoke_rbx_pre_start_game --> Begin");
	
    result->type            = NPVariantType_Int32;
	result->value.intValue  = 0;
	
	//logmsg("invoke_rbx_pre_start_game <-- End");
	return true;
}

static bool invoke_rbx_set_edit_mode(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    logmsg("invoke_rbx_set_edit_mode --> Begin");
    
    startMode = STARTMODE_EDIT;
    
    logmsg("invoke_rbx_set_edit_mode --> End");
    return true;
}

static CFStringRef getStartModeArg()
{
    switch (currentStartMode)
    {
        case STARTMODE_APP: return CFSTR("--app");
        case STARTMODE_PLAY: return CFSTR("--play");
        case STARTMODE_EDIT: return CFSTR("-ide");
            
        default: return CFSTR("");
    }
}

static bool invoke_rbx_start_game(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_start_game --> Begin");
	
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
    
    // if already a start_game in progress, if in Studio mode, close update thread
    StartGameState current_start_game_state = STARTGAMESTATE_UNINITIALIZED;
    pthread_mutex_lock(&start_game_sync_mutex);
    current_start_game_state = start_game_state;
    pthread_mutex_unlock(&start_game_sync_mutex);
    
    if (current_start_game_state == STARTGAMESTATE_IN_PROGRESS)
    {
        if (currentStartMode == STARTMODE_EDIT)
        {
            // request thread delete
            pthread_mutex_lock(&start_game_sync_mutex);
            request_thread_delete  = true;
            pthread_mutex_unlock(&start_game_sync_mutex);
        }
        
        int counter = 0;
        while (counter < 3)
        {
            counter++;
            sleep(1);
            
            pthread_mutex_lock(&start_game_sync_mutex);
            current_start_game_state = start_game_state;
            pthread_mutex_unlock(&start_game_sync_mutex);
            
            if ((current_start_game_state == STARTGAMESTATE_COMPLETED) || (current_start_game_state == STARTGAMESTATE_UNINITIALIZED))
                break;
        }
        
        // if start game still in progress, then do nothing!
        if (current_start_game_state == STARTGAMESTATE_IN_PROGRESS)
        {
            logmsg("-- Error: another start game in progress");
            return true;
        }
    }
    
    pthread_mutex_lock(&start_game_sync_mutex);
    start_game_state = STARTGAMESTATE_IN_PROGRESS;
    pthread_mutex_unlock(&start_game_sync_mutex);
    
	//check for error conditions
	if ((argCount != 2) || (args[0].type != NPVariantType_String) || (args[1].type != NPVariantType_String) || (s_rbx_game_auth_ticket == NULL))
	{
		logmsg("-- Error: wrong number/type arguments");
        pthread_mutex_lock(&start_game_sync_mutex);
        start_game_state = STARTGAMESTATE_COMPLETED;
        pthread_mutex_unlock(&start_game_sync_mutex);
		return true;
	}
	
	//somehow strcpy or strncpy doesn't work correctly for copying string so using copy_string
	char *auth_url_str = (char *)malloc(sizeof(char) * (args[0].value.stringValue.UTF8Length + 1));
	copy_string(auth_url_str, args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
	
	char *script_url_str = (char *)malloc(sizeof(char) * (args[1].value.stringValue.UTF8Length + 1));
	copy_string(script_url_str, args[1].value.stringValue.UTF8Characters, args[1].value.stringValue.UTF8Length);
	
	CFStringRef ticket_cfstr     = CFStringCreateWithCString(NULL, s_rbx_game_auth_ticket, kCFStringEncodingUTF8);
	CFStringRef auth_url_cfstr   = CFStringCreateWithCString(NULL, auth_url_str, kCFStringEncodingUTF8);
	CFStringRef script_url_cfstr = CFStringCreateWithCString(NULL, script_url_str, kCFStringEncodingUTF8);
	
	if (ticket_cfstr == NULL || auth_url_cfstr == NULL || script_url_cfstr == NULL)
	{
		logmsg("-- Error: one of the argument is NULL");
        pthread_mutex_lock(&start_game_sync_mutex);
        start_game_state = STARTGAMESTATE_COMPLETED;
        pthread_mutex_unlock(&start_game_sync_mutex);
		return true;
	}
	
	logmsg("-- Got launch arguments");
    
    // data will be cleaned up in respective function calls
    struct WebData *data = (struct WebData *)malloc(sizeof(struct WebData));
    data->ticket = CFStringCreateCopy(kCFAllocatorDefault, ticket_cfstr);
    data->auth_url = CFStringCreateCopy(kCFAllocatorDefault, auth_url_cfstr);
    data->script_url = CFStringCreateCopy(kCFAllocatorDefault, script_url_cfstr);
    
    // NOTE: in play we do not get startmode set, so reset now
    currentStartMode = startMode;
    startMode = STARTMODE_PLAY;
    
    logmsg_arg("Attempting to launch - %d mode", currentStartMode);
    
    // now check whether we need to launch Studio or Player?
    bool isStudioMode = (currentStartMode == STARTMODE_EDIT);
    if (isStudioMode)
    {
        // start new thread so we do not block main thread
        pthread_create(&studio_update_thread, NULL, update_and_launch_studio, (void*)data);
        result->value.intValue = 0;
    }
    else
    {
        // launch player application directly, it must have been updated by now
        if (launch_player(data))
            result->value.intValue = 0;
        // update start_game_state
        pthread_mutex_lock(&start_game_sync_mutex);
        start_game_state = STARTGAMESTATE_COMPLETED;
        pthread_mutex_unlock(&start_game_sync_mutex);
    }
    
    //release memory
    CFRelease(script_url_cfstr);
    CFRelease(auth_url_cfstr);
    CFRelease(ticket_cfstr);
    
    free(auth_url_str);
    free(script_url_str);
    
    free(s_rbx_game_auth_ticket);
    s_rbx_game_auth_ticket = NULL;
	
	logmsg("invoke_rbx_start_game <-- End");
	
	return true;
}

static bool invoke_rbx_get_game_started(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	//logmsg("invoke_rbx_get_game_started --> Begin");
	
    result->type            = NPVariantType_Bool;
    result->value.boolValue = false;
    
    bool game_started = false;
    pthread_mutex_lock(&start_game_sync_mutex);
    game_started = (start_game_state == STARTGAMESTATE_COMPLETED) ? true : false;
    pthread_mutex_unlock(&start_game_sync_mutex);
	
    if (game_started)
    {
        result->value.boolValue = true;
        
        pthread_mutex_lock(&start_game_sync_mutex);
        start_game_state = STARTGAMESTATE_UNINITIALIZED;
        pthread_mutex_unlock(&start_game_sync_mutex);
        
        bool isStudioMode = (currentStartMode == STARTMODE_EDIT);
        CFStringRef app_bundle_id = isStudioMode ? s_rbx_studio_app_bundle_id : s_rbx_app_bundle_id;
        ProcessSerialNumber psn = {kNoProcess, kNoProcess};
        
        // bring application to the front
        if (is_app_running(app_bundle_id, &psn))
            SetFrontProcess(&psn);
        
        logmsg("---- Game Started ----");
    }
	
	//logmsg("invoke_rbx_get_game_started <-- End");
	
	return true;
}

static bool invoke_rbx_get_install_host(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_get_install_host --> Begin");
	
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	
	//verify whether the application actually exists, if not then return
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	CFStringRef client_app_path = (CFStringRef)(CFPreferencesCopyAppValue(s_rbx_client_app_path_key, s_rbx_pref_file_id));
	
	if (client_app_path)
	{
		char *client_app_path_str = to_char_string(client_app_path);
		CFRelease(client_app_path);
		
		if (client_app_path_str)
		{
			FSRef application_ref = {0};
			OSStatus success_status = FSPathMakeRef((UInt8 *)client_app_path_str, &application_ref, NULL);
			free(client_app_path_str);
			
			if (success_status != noErr)
			{
				logmsg("-- ERROR: Application not found");
				return true;
			}
		}
	}
	
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	CFStringRef install_host = (CFStringRef)(CFPreferencesCopyAppValue(s_rbx_install_hostKey, s_rbx_pref_file_id));
	if (install_host == NULL)
	{
		logmsg("-- ERROR: Host not found");
		return true;
	}
	
	char *install_host_str  = to_char_string(install_host);
	if (install_host_str == NULL)
	{
		logmsg("-- ERROR: Host not found");
		return true;
	}
	
	uint32_t string_length  = (uint32_t)strlen(install_host_str);
	
	//copy string appropriately (use npapi alloc)
	NPUTF8 *install_host_utf8 = (NPUTF8*)(s_netscape_funcs->memalloc(string_length+1));//memory should be released by browser
	copy_string(install_host_utf8, install_host_str, string_length);
	
	result->type                             = NPVariantType_String;
	result->value.stringValue.UTF8Length     = string_length+1;
	result->value.stringValue.UTF8Characters = (const NPUTF8 *)install_host_utf8;
	
	logmsg_arg("-- install host = %s", install_host_utf8);
	
	//release memory
	free(install_host_str);
	CFRelease(install_host);
	
	logmsg("invoke_rbx_get_install_host <-- End");
	
	return true;
}

static bool invoke_rbx_get_plugin_version(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_get_plugin_version --> Begin");
	
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	
	//verify whether the application actually exists, if not then return
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	CFStringRef client_app_path = (CFStringRef)(CFPreferencesCopyAppValue(s_rbx_client_app_path_key, s_rbx_pref_file_id));
	
	if (client_app_path)
	{
		char *client_app_path_str = to_char_string(client_app_path);
		CFRelease(client_app_path);
		
		if (client_app_path_str)
		{
			FSRef application_ref = {0};
			OSStatus success_status = FSPathMakeRef((UInt8 *)client_app_path_str, &application_ref, NULL);
			free(client_app_path_str);
			
			if (success_status != noErr)
			{
				logmsg("-- ERROR: Application not found");
				return true;
			}
		}
	}
	
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	CFStringRef plugin_version = (CFStringRef)(CFPreferencesCopyAppValue(s_rbx_plugin_version_key, s_rbx_pref_file_id));
	if (plugin_version == NULL)
	{
		logmsg("-- ERROR: Version found");
		return true;
	}
	
	char *plugin_version_str  = to_char_string(plugin_version);
	if (plugin_version_str == NULL)
	{
		logmsg("-- ERROR: Version not found");
		return true;
	}
	
	uint32_t string_length  = (uint32_t)strlen(plugin_version_str);
	
	//copy string appropriately (use npapi alloc)
	NPUTF8 *plugin_version_utf8 = (NPUTF8*)(s_netscape_funcs->memalloc(string_length+1));//memory should be released by browser
	copy_string(plugin_version_utf8, plugin_version_str, string_length);
	
	result->type                             = NPVariantType_String;
	result->value.stringValue.UTF8Length     = string_length+1;
	result->value.stringValue.UTF8Characters = (const NPUTF8 *)plugin_version_utf8;
	
	logmsg_arg("-- plugin version = %s", plugin_version_utf8);
	
	//release memory
	free(plugin_version_str);
	CFRelease(plugin_version);
	
	logmsg("invoke_rbx_get_plugin_version <-- End");
	
	return true;
}

static bool invoke_rbx_update(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_update --> Begin");
	
	result->type           = NPVariantType_Int32;
	result->value.intValue = -1;
	
	FSRef application_ref = {0};
	if (!get_application_path_ref(s_rbx_bootstrapper_path_key, &application_ref))
	{
		logmsg("-- ERROR: Bootstrapper not found");
		return true;
	}
    
    //make sure we modify the isUptoDate flag
	CFPreferencesSetAppValue(s_is_uptodate_key, CFSTR("false"), s_rbx_pref_file_id);
    CFPreferencesSetAppValue(s_is_plugin_handling_key, CFSTR("true"), s_rbx_pref_file_id);
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	
	const int   application_argc = 4;
	CFStringRef      arguments[] = { CFSTR("-check"),
		CFSTR("true"),
		CFSTR("-updateUI"),
		CFSTR("false"),
	};
	
	CFArrayRef application_argv = CFArrayCreate(NULL, (const void **)&arguments, application_argc, &kCFTypeArrayCallBacks);
	
	//prepare for application launch
	LSApplicationParameters application_params = {0, kLSLaunchDefaults};
	application_params.application             = &application_ref;
	application_params.argv                    =  application_argv;
	
	//launch application
	OSStatus success_status = LSOpenApplication(&application_params, NULL);
	
	if (success_status == noErr)
	{
		//let browser know that we are doing an update
		result->value.intValue = 0;
		//delay call for is_upto_data
		s_delay_is_upto_date = true;
		
		logmsg("-- ##### Application launched for update #####");
	}
	
	logmsg("invoke_rbx_update <-- End");
	
	return true;
}

static bool invoke_rbx_get_is_uptodate(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	//logmsg("invoke_rbx_get_is_uptodate --> Begin");
	
	//delay the execution to allow update call to complete
	if (s_delay_is_upto_date)
	{
		logmsg("invoke_rbx_get_is_uptodate");
		usleep(1300000);
		s_delay_is_upto_date = false;
	}
	
    result->type            = NPVariantType_Bool;
	result->value.boolValue = 0;
	
	//make sure plugin handling key is set (sometimes due to random plugin instance destruction, this key can get removed in FF)
	CFStringRef isPluginHandling = (CFStringRef)CFPreferencesCopyAppValue(s_is_plugin_handling_key, s_rbx_pref_file_id);
	if (isPluginHandling == NULL)
		CFPreferencesSetAppValue(s_is_plugin_handling_key, CFSTR("true"), s_rbx_pref_file_id);
	else
		CFRelease(isPluginHandling);
	
	// as soon as the file is deleted means we are done with update!!!
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	CFStringRef is_uptodate_str  = (CFStringRef)CFPreferencesCopyAppValue(s_is_uptodate_key, s_rbx_pref_file_id);
	if (is_uptodate_str)
	{
		bool is_uptodate = (CFStringCompare(is_uptodate_str, CFSTR("true"), kCFCompareCaseInsensitive) == kCFCompareEqualTo);
		CFRelease(is_uptodate_str);
		
		if (is_uptodate)
		{
			result->value.boolValue = 1;
			logmsg("-- ***** Application updated *****");
            // application updated remove transient key
            CFPreferencesSetAppValue(s_is_plugin_handling_key, NULL, s_rbx_pref_file_id);
            CFPreferencesAppSynchronize(s_rbx_pref_file_id);
		}
	}
	
	//logmsg("invoke_rbx_get_is_uptodate <-- End");
	
	return true;
}

static bool invoke_rbx_put_auth_ticket(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_put_auth_ticket --> Begin");
	
	if (s_rbx_game_auth_ticket)
	{
		free(s_rbx_game_auth_ticket);
		s_rbx_game_auth_ticket = NULL;
	}
	
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	
	if (argCount != 1 || args[0].type != NPVariantType_String)
	{
		logmsg("-- Error: wrong number of arguments");
		return true;
	}
	
	result->value.intValue = 0;
	
	s_rbx_game_auth_ticket    = (char *)malloc(sizeof(char) * (args[0].value.stringValue.UTF8Length + 1));
	copy_string(s_rbx_game_auth_ticket, args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
	
	logmsg("invoke_rbx_put_auth_ticket <-- End");
	
	return true;
}

bool launch_player(struct WebData* args)
{
	FSRef application_ref = {0};
	if (!get_application_path_ref(s_rbx_client_app_path_key, &application_ref))
	{
		logmsg("-- Error: Client application not found");
		return true;
	}
    
	logmsg("-- Got client application");
    
    // first set transient key
    CFPreferencesSetAppValue(s_is_plugin_handling_key, CFSTR("true"), s_rbx_pref_file_id);
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    
    const int   application_argc = 8;
	CFStringRef      arguments[] = { getStartModeArg(),
        CFSTR("-ticket"),
		args->ticket,
		CFSTR("-authURL"),
		args->auth_url,
		CFSTR("-scriptURL"),
		args->script_url,
		CFSTR("-plugin")
	};
	
	CFArrayRef application_argv = CFArrayCreate(NULL, (const void **)&arguments, application_argc, &kCFTypeArrayCallBacks);
	
	//prepare for application launch
	LSApplicationParameters application_params;
	memset(&application_params, 0, sizeof(application_params));
	
	application_params.version     =  0;
	application_params.flags       =  kLSLaunchNewInstance;
	application_params.application = &application_ref;
	application_params.argv        =  application_argv;
	
    //launch application
	if (LSOpenApplication(&application_params, NULL) == noErr)
	{
		logmsg("-- Application launched successfully...");
        
        // delay removal of mutex by a small amount
        sleep(3);
    }
	
    // remove transient key
    CFPreferencesSetAppValue(s_is_plugin_handling_key, NULL, s_rbx_pref_file_id);
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    
	//release memory
    CFRelease(application_argv);
    CFRelease(args->script_url);
    CFRelease(args->auth_url);
    CFRelease(args->ticket);
    free(args);
    
    return true;
}

void* update_and_launch_studio(void* data)
{
    logmsg("update_and_launch_studio ---> begin");
    
    // first set transient key
    CFPreferencesSetAppValue(s_is_plugin_handling_key, CFSTR("true"), s_rbx_pref_file_id);
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    
	if (data && update_studio())
	{
        FSRef application_ref = {0};
        if (get_application_path_ref(s_rbx_studio_app_path_key, &application_ref))
        {
            logmsg("-- Got Studio application");
            
            struct WebData *args = (struct WebData *)(data);
            int  application_argc  = 8;
            // in Studio default mode is build
            CFStringRef arguments[] = { CFSTR("-build"),
                                        CFSTR("-ticket"),
                                        args->ticket,
                                        CFSTR("-url"),
                                        args->auth_url,
                                        CFSTR("-script"),
                                        args->script_url,
                                        CFSTR("-plugin") };
                
            CFArrayRef application_argv = CFArrayCreate(NULL, (const void **)arguments, application_argc, &kCFTypeArrayCallBacks);
            
            //prepare for application launch
            LSApplicationParameters application_params;
            memset(&application_params, 0, sizeof(application_params));
            
            application_params.version     =  0;
            application_params.flags       =  kLSLaunchNewInstance;
            application_params.application = &application_ref;
            application_params.argv        =  application_argv;
            
            //launch application
            if (LSOpenApplication(&application_params, NULL) == noErr)
            {
                logmsg("-- Studio launched successfully...");
                
                // delay removal of mutex by a small amount
                sleep(3);
            }

            //release memory
            CFRelease(application_argv);
            CFRelease(args->script_url);
            CFRelease(args->auth_url);
            CFRelease(args->ticket);
            free(args);
        }
    }
    
    logmsg("update_and_launch_studio <--- End");
    
    // update start_game_state
    pthread_mutex_lock(&start_game_sync_mutex);
    start_game_state = STARTGAMESTATE_COMPLETED;
    request_thread_delete  = false;
    pthread_mutex_unlock(&start_game_sync_mutex);
    
    // remove transient key
    CFPreferencesSetAppValue(s_is_plugin_handling_key, NULL, s_rbx_pref_file_id);
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    
    pthread_exit(NULL);
    
    return 0;
}

bool update_studio()
{
    logmsg("update_studio --> Begin");
    
    // if studio is not installed, wait to get Player installed (NOTE: application must be launched by the user after download)
    FSRef studio_application_ref = {0};
    if (!get_application_path_ref(s_rbx_studio_app_path_key, &studio_application_ref))
    {
        logmsg("-- Studio app not found not found, waiting for player installation");
        
        // update player related keys
        CFPreferencesSetAppValue(s_is_uptodate_key, CFSTR("false"), s_rbx_pref_file_id);
        CFPreferencesSetAppValue(s_update_cancelled_key, CFSTR("false"), s_rbx_pref_file_id);
        CFPreferencesSetAppValue(s_is_plugin_handling_key, CFSTR("true"), s_rbx_pref_file_id);
        CFPreferencesAppSynchronize(s_rbx_pref_file_id);
        
        // check if Player's bootstrapper is running,
        ProcessSerialNumber psn = {kNoProcess, kNoProcess};
        if (!is_app_running(s_rbx_app_bootstrapper_bundle_id, &psn))
        {
            logmsg("-- Player bootstrapper not running");
            
            // if it's not running then launch player bootstrapper
            FSRef player_application_ref = {0};
            if (!get_application_path_ref(s_rbx_bootstrapper_path_key, &player_application_ref))
            {
                logmsg("-- Player bootstrapper not found");
                logmsg("update_studio <-- End");
                return false;
            }
            
            const int   application_argc = 6;
            CFStringRef      arguments[] = { CFSTR("-check"),
                CFSTR("true"),
                CFSTR("-updateUI"),
                CFSTR("false"),
                CFSTR("-checkStudio"),
                CFSTR("true")
            };
            
            CFArrayRef application_argv = CFArrayCreate(NULL, (const void **)&arguments, application_argc, &kCFTypeArrayCallBacks);
            
            //prepare for application launch
            LSApplicationParameters application_params = {0, kLSLaunchDefaults};
            application_params.application             = &player_application_ref;
            application_params.argv                    =  application_argv;
            
            // launch player bootstrapper application
            ProcessSerialNumber new_psn = {kNoProcess, kNoProcess};
            OSStatus success_status = LSOpenApplication(&application_params, &new_psn);
            
            if (success_status != noErr)
            {
                logmsg("-- Player bootstrapper cannot be launched");
                logmsg("update_studio <-- End");
                return false;
            }
            
            logmsg("-- Player bootstrapper running");
        }
        
        // waiting to get player updated
        if (!wait_for_update(s_is_uptodate_key, s_update_cancelled_key))
        {
            logmsg("-- Studio not found");
            logmsg("update_studio <-- End");
            return false;
        }
    }

    //make sure we modify the studio related keys
	CFPreferencesSetAppValue(s_is_studio_uptodate_key, CFSTR("false"), s_rbx_pref_file_id);
    CFPreferencesSetAppValue(s_studio_update_cancelled_key, CFSTR("false"), s_rbx_pref_file_id);
    CFPreferencesSetAppValue(s_is_plugin_handling_key, CFSTR("true"), s_rbx_pref_file_id);
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    
    FSRef application_ref = {0};
	if (!get_application_path_ref(s_rbx_studio_bootstrapper_path_key, &application_ref))
	{
		logmsg("-- ERROR: Studio Bootstrapper not found");
        logmsg("update_studio <-- End");
		return 0;
	}
	
	const int   application_argc = 4;
	CFStringRef      arguments[] = { CFSTR("-check"),
		CFSTR("true"),
		CFSTR("-updateUI"),
		CFSTR("false"),
	};
	
	CFArrayRef application_argv = CFArrayCreate(NULL, (const void **)&arguments, application_argc, &kCFTypeArrayCallBacks);
	
	//prepare for application launch
	LSApplicationParameters application_params = {0, kLSLaunchDefaults};
	application_params.application             = &application_ref;
	application_params.argv                    =  application_argv;
	
	// launch bootstrapper application
    ProcessSerialNumber psn = {kNoProcess, kNoProcess};
	OSStatus success_status = LSOpenApplication(&application_params, &psn);
	
	if (success_status == noErr)
	{
        bool update = wait_for_update(s_is_studio_uptodate_key, s_studio_update_cancelled_key);
        logmsg_arg("-- Studio update - %s", update ? "up to date" : "update error");
        logmsg("update_studio <-- End");
        return update;
    }
    
    logmsg("update_studio <-- End");
    return false;
}

bool is_app_updated(const CFStringRef update_key)
{
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    CFStringRef is_uptodate_str  = (CFStringRef)CFPreferencesCopyAppValue(update_key, s_rbx_pref_file_id);
    if (is_uptodate_str)
    {
        bool is_uptodate = (CFStringCompare(is_uptodate_str, CFSTR("true"), kCFCompareCaseInsensitive) == kCFCompareEqualTo);
        CFRelease(is_uptodate_str);
        
        if (is_uptodate)
        {
            logmsg("-- Application updated");
            return true;
        }
    }
    
    return false;
}

bool wait_for_update(const CFStringRef update_key, const CFStringRef cancelKey)
{
    // wait till application is updated
    while (!is_app_updated(update_key))
    {
        sleep(1); // 1 sec
        
        bool delete_requested = false;
        pthread_mutex_lock(&start_game_sync_mutex);
        delete_requested = request_thread_delete;
        pthread_mutex_unlock(&start_game_sync_mutex);
        
        if (delete_requested)
        {
            logmsg("-- update thread delete requested");
            return false;
        }
    }
    
    bool updated = true;
    CFPreferencesAppSynchronize(s_rbx_pref_file_id);
    CFStringRef update_cancelled_str  = (CFStringRef)CFPreferencesCopyAppValue(cancelKey, s_rbx_pref_file_id);
    if (update_cancelled_str)
    {
        updated = (CFStringCompare(update_cancelled_str, CFSTR("false"), kCFCompareCaseInsensitive) == kCFCompareEqualTo);
        CFRelease(update_cancelled_str);
    }
    
    return updated;
}

static bool invoke_rbx_get_key_value(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_get_key_value <-- Begin");
	
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	
	if (argCount != 1 || args[0].type != NPVariantType_String)
	{
		logmsg("-- wrong number/type of arguments");
		return true;
	}
	
	char *key = (char *)malloc(sizeof(char) * (args[0].value.stringValue.UTF8Length + 1));
	copy_string(key, args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
	logmsg_arg("Key to get %s", key);
	
	std::wstring path = CookiesEngine::getCookiesFilePath();
	if (path.empty())
	{
		logmsg("File path not set");
		return true;
	}
	
	CookiesEngine engine(path);
	int counter = MaxReTries;
	
	while (counter)
	{
		bool valid = false;
		int success = 0;
		std::string value = engine.GetValue(key, &success, &valid);
		if (success == 0)
		{
			int     size = 0;
			NPUTF8 *key_value_utf8 = NULL;
			
			if (valid)
			{
				logmsg_arg("-- Key Value = %s", value.c_str());
				
				size = (uint32_t)strlen(value.c_str());
				//copy string appropriately (use npapi alloc)
				key_value_utf8 = (NPUTF8*)(s_netscape_funcs->memalloc(size+1));//memory should be released by browser
				copy_string(key_value_utf8, value.c_str(), size);
			}
			
			result->type = NPVariantType_String;
			result->value.stringValue.UTF8Length = size;
			result->value.stringValue.UTF8Characters = (const NPUTF8 *)key_value_utf8;
			
			break;
		}
		
		usleep(50000);
		counter --;
	}
	
	free(key);
	logmsg("invoke_rbx_get_key_value <-- End");
	
	return true;
}

static bool invoke_rbx_set_key_value(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_set_key_value --> Begin");
    
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	
	if (argCount != 2 || args[0].type != NPVariantType_String || args[0].type != NPVariantType_String)
	{
		logmsg("-- Error: wrong number/type of arguments");
		return true;
	}
    
	char *key = (char *)malloc(sizeof(char) * (args[0].value.stringValue.UTF8Length + 1));
	copy_string(key, args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
	
	char *value = (char *)malloc(sizeof(char) * (args[1].value.stringValue.UTF8Length + 1));
	copy_string(value, args[1].value.stringValue.UTF8Characters, args[1].value.stringValue.UTF8Length);
	
	logmsg_arg("-- Key = %s, Value = %s", key, value);
	
	std::wstring path = CookiesEngine::getCookiesFilePath();
	if (path.empty())
	{
		logmsg("-- Error: File path not set");
		return true;
	}
	
	CookiesEngine engine(path);
	int counter = MaxReTries;
	
	while (counter)
	{
		int success = engine.SetValue(key, value);
		if (success == 0)
		{
			logmsg("-- Value set");
			result->value.intValue = 0;
			break;
		}
		
		usleep(50000);
		counter --;
	}
	
	free(value);
	free(key);
	
	logmsg("invoke_rbx_set_key_value <-- End");
	
	return true;
}

static bool invoke_rbx_delete_key_value(NPObject *obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
	logmsg("invoke_rbx_delete_key_value --> Begin");
	
	result->type = NPVariantType_Int32;
	result->value.intValue = -1;
	
	if (argCount != 1 || args[0].type != NPVariantType_String)
	{
		logmsg("-- Error: wrong number/type of arguments");
		return true;
	}
	
	char *key = (char *)malloc(sizeof(char) * (args[0].value.stringValue.UTF8Length + 1));
	copy_string(key, args[0].value.stringValue.UTF8Characters, args[0].value.stringValue.UTF8Length);
	
	logmsg_arg("-- Key for deletion = %s");
	
	std::wstring path = CookiesEngine::getCookiesFilePath();
	if (path.empty())
	{
		logmsg("-- Error: File path not set");
		return true;
	}
	
	CookiesEngine engine(path);
	int counter = MaxReTries;
	
	while (counter)
	{
		int success = engine.DeleteValue(key);
		if (success == 0)
		{
			logmsg("-- Key deleted");
			result->value.intValue = 0;
			break;
		}
		
		usleep(50000);
		counter--;
	}
	
	free(key);
	
	logmsg("invoke_rbx_delete_key_value <-- End");
	
	return true;
}

static bool is_roblox_host(NPP instance, const char *hostString)
{
	bool result = false;
	
    NPObject* window_obj = NULL;
    s_netscape_funcs->getvalue(instance, NPNVWindowNPObject, &window_obj);
	
	if (window_obj)
	{
		NPVariant location_var;
		s_netscape_funcs->getproperty(instance, window_obj, s_netscape_funcs->getstringidentifier("location"), &location_var);
		
		NPObject* location_obj = NPVARIANT_TO_OBJECT(location_var);
		if (location_obj)
		{
			NPVariant host_val;
			s_netscape_funcs->getproperty(instance, location_obj, s_netscape_funcs->getstringidentifier("host"), &host_val);
			
			if (NPVARIANT_IS_STRING(host_val))
			{
				//make a copy of the strings
				char * host_copy = strdup(host_val.value.stringValue.UTF8Characters);
				char * site_copy = strdup(hostString);
				
				make_upper(host_copy);
				make_upper(site_copy);
                logmsg("host: ");
				logmsg(host_copy);
                
                logmsg("site: ");
                logmsg(site_copy);
                // search roblox site
				if (strstr(host_copy, site_copy) != NULL)
					result = true;
				
                if (result)
                    logmsg("same");
                else
                    logmsg("different");
                
				// release memory
				free(host_copy);
				free(site_copy);
			}
			
			s_netscape_funcs->releasevariantvalue(&host_val);
			//s_netscape_funcs->releaseobject(location_obj); //duplicate release of location var, seemed to cause chrome problems
		}
		
		s_netscape_funcs->releasevariantvalue(&location_var);
		s_netscape_funcs->releaseobject(window_obj);
	}
	
	return result;
}

static bool is_embedded_browser_view(NPP instance)
{
	bool result = false;
	
    NPObject* window_obj = NULL;
	s_netscape_funcs->getvalue(instance, NPNVWindowNPObject, &window_obj);
	
	if (window_obj)
	{
		NPVariant external_var;
		s_netscape_funcs->getproperty(instance, window_obj, s_netscape_funcs->getstringidentifier("external"), &external_var);
		
		NPObject* external_obj = NPVARIANT_TO_OBJECT(external_var);
		if (external_obj)
		{
			NPVariant startgame_var;
			s_netscape_funcs->getproperty(instance, external_obj, s_netscape_funcs->getstringidentifier("StartGame"), &startgame_var);
			
			//if start game object is present then it is player's embedded browser
			if (NPVARIANT_IS_OBJECT(startgame_var))
				result = true;
			
			s_netscape_funcs->releasevariantvalue(&startgame_var);
		}
		
		s_netscape_funcs->releasevariantvalue(&external_var);
		//s_netscape_funcs->releaseobject(external_obj); //duplicate release of external var, seemed to cause chrome problems
		s_netscape_funcs->releaseobject(window_obj);
	}
	
	return result;
}

static bool get_application_path_ref(const CFStringRef path_key, FSRef *application_ref)
{
	CFPreferencesAppSynchronize(s_rbx_pref_file_id);
	CFStringRef app_path = (CFStringRef)(CFPreferencesCopyAppValue(path_key, s_rbx_pref_file_id));
	if (!app_path)
		return false;
	
	char *app_path_str  = to_char_string(app_path);
	CFRelease(app_path);
	if (app_path_str == NULL)
		return false;
	
	OSStatus success_status = FSPathMakeRef((UInt8 *)app_path_str, application_ref, NULL);
	free(app_path_str);
	
	return success_status == noErr;
}

static bool is_app_running(const CFStringRef app_bundle_id, ProcessSerialNumber *psn)
{
	bool success = false;
	while (GetNextProcess(psn) == noErr)
	{
		CFDictionaryRef application_dictionary = ProcessInformationCopyDictionary(psn,  kProcessDictionaryIncludeAllInformationMask);
		if (application_dictionary)
		{
			CFStringRef bundle_id = (CFStringRef)(CFDictionaryGetValue(application_dictionary, kCFBundleIdentifierKey));
			if (bundle_id && (CFStringCompare(bundle_id, app_bundle_id, kCFCompareCaseInsensitive) == kCFCompareEqualTo))
			{
				success = true;
				break;
			}
			
			CFRelease(application_dictionary);
		}
	}
	
	return success;
}

static char* getTempFile()
{
	if (s_temp_file == NULL)
	{
		s_temp_file = (char *)malloc(sizeof(char) * 48);
		snprintf(s_temp_file, 48, "/tmp/nprbx_%s.log", getlogin());
	}
	
	return s_temp_file;
}

static void logmsg_arg(const char *format, ...)
{
	FILE *out = fopen(getTempFile(), "a");
	if(out)
	{
		va_list args;
		va_start(args, format);
		
		char *formattedMesg = NULL;
		vasprintf(&formattedMesg, format, args);
		if (formattedMesg)
		{
			fputs(formattedMesg, out);
			fputs("\n", out);
			free(formattedMesg);
		}
		
		fclose(out);
		va_end(args);
	}
}

static void logmsg(const char *msg)
{
	FILE *out = fopen(getTempFile(), "a");
	if(out)
	{
		fputs(msg, out);
		fputs("\n", out);
		fclose(out);
	}
}

static void log_current_time()
{
	FILE *out = fopen(getTempFile(), "a");
	if(out)
	{
		time_t rawtime;
		time(&rawtime);
		
		fputs("@@@@@@@ current time - ", out);
		fputs(ctime(&rawtime), out);
		fclose(out);
	}
}

static char* make_upper(char* ioString)
{
	int len = strlen(ioString), ii = 0;
	
	for(ii=0; ii<len; ++ii)
		ioString[ii] = toupper((unsigned char)ioString[ii]);
	
	return ioString;
}

static char* to_char_string(CFStringRef cf_str)
{
	CFIndex size     = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cf_str), kCFStringEncodingUTF8);
	char   *char_str = (char *)malloc(sizeof(char) * (size + 1));
	
	if (char_str != NULL && CFStringGetCString(cf_str, char_str, size, kCFStringEncodingUTF8))
		return char_str;
	
	return NULL;
}

//somehow strcpy or strncpy doesn't work correctly for copying string so using copy_string
static void copy_string(char *destination, const char *source, int string_length)
{
	int ii;
	for (ii=0; ii < string_length; ++ii)
		destination[ii] = source[ii];
	destination[string_length] = '\0';
}
