#include "../sg.h"

int  nMEM_TRACE_LEVEL = 2;             
BOOL bINHOUSE_VERSION = FALSE;         
//---------------------------------------------------------------

BOOL (*pExtStartupInits)(void) = NULL;

void (*pExtFreeAllMem)(void) = NULL;

void (*pExtSyntaxInit)(void) = NULL;

void (*pExtActionsInit)(void) = NULL;

void (*pExtRegObjects)(void) = NULL;

void (*pExtRegCommonMethods)(void) = NULL;

void (*pExtInitObjMetods)(void) = NULL;

void (*pExtConfigPars)(void) = NULL;

void (*pExtSysAttrs)(void) = NULL;


BOOL (*pExtBeforeRunCmdLoop)(void) = NULL;

BOOL (*pExtBeforeNew)(void) = NULL;
BOOL (*pExtAfterNew)(void) = NULL;
BOOL (*pExtReLoadCSG)(hOBJ hobj, hCSG *hcsg) = NULL;

