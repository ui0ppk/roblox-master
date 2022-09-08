#include "../sg.h"


typedef struct {
  char *Name;
	SG_PTR_TO_DIGIT Idle;
}COMMONMETHOD;
typedef COMMONMETHOD * pCOMMONMETHOD;

typedef struct {
  char       *Name;
	int        GeoSize;
  int        OldType;
  EL_OBJTYPE ElGeoMask;
}OBJTEMPL;
typedef OBJTEMPL * pOBJTEMPL;


typedef struct {
  int   iObjType;
	int   iMethod;
  SG_PTR_TO_DIGIT MethodData;
}OBJMETHOD;
typedef OBJMETHOD * pOBJMETHOD;


typedef struct {
  INDEX_KW  iKey;
	UINT      PromptID;
}SNAPTEMPL;
typedef SNAPTEMPL * pSNAPTEMPL;


int    GeoObjMethodsCount  = 0;
int    GeoObjTypesCount    = 0;

SG_PTR_TO_DIGIT*       pMethods      = NULL;
char**       ppGeoObjNames = NULL;
int*         pOldTypes     = NULL;
pEL_OBJTYPE  pElTypes      = NULL;

OBJTYPE  ALL_OBJECTS;
OBJTYPE  NULL_OBJECTS;

static lpVDIM pGeoObjVD     = NULL;
static lpVDIM pMethodsVD    = NULL;
static lpVDIM pObjMethodsVD = NULL;
static EL_OBJTYPE CurrElGeoMask = 0x00000001;

static BOOL ObjectsRegStatus = FALSE;

BOOL GeoObjectsInit(void)
{
BOOL          ret = FALSE;
int           i, j;
pCOMMONMETHOD pMt;
pOBJMETHOD    pObjMt;
pOBJTEMPL     pObjTempl;
DWORD         FFlags;

  pGeoObjVD = (lpVDIM)SGMalloc(3*sizeof(VDIM));
  if(!pGeoObjVD)
    return FALSE;
  pMethodsVD = pGeoObjVD + 1;
  pObjMethodsVD = pMethodsVD + 1;
  init_vdim(pGeoObjVD,     sizeof(OBJTEMPL));
  init_vdim(pMethodsVD,    sizeof(COMMONMETHOD));
  init_vdim(pObjMethodsVD, sizeof(OBJMETHOD));

  ObjectsRegStatus = TRUE;
  
  RegCoreObjects();
  if(pExtRegObjects)
    pExtRegObjects();

  
  RegCommonMethods();
  if(pExtRegCommonMethods)
    pExtRegCommonMethods();

  if(!ObjectsRegStatus)
    goto err;

  ObjectsRegStatus = FALSE;
  GeoObjMethodsCount = pMethodsVD->num_elem;
  GeoObjTypesCount = pGeoObjVD->num_elem;

  if(!GeoObjMethodsCount || !GeoObjTypesCount)
    return FALSE;

  
  geo_size = (WORD*)SGMalloc(GeoObjTypesCount*sizeof(WORD));
  if(!geo_size)
    goto err;
  ppGeoObjNames = (char**)SGMalloc(GeoObjTypesCount*sizeof(char*));
  if(!ppGeoObjNames)
    goto err;
  pOldTypes = (int*)SGMalloc(GeoObjTypesCount*sizeof(int));
  if(!pOldTypes)
    goto err;
  pElTypes  = (pEL_OBJTYPE)SGMalloc(GeoObjTypesCount*sizeof(EL_OBJTYPE));
  if(!pElTypes)
    goto err;

  pMethods = (SG_PTR_TO_DIGIT*)SGMalloc(GeoObjTypesCount*GeoObjMethodsCount*sizeof(SG_PTR_TO_DIGIT));
  if(!pMethods)
    goto err;


  for(i = 0; i < GeoObjMethodsCount; i++){
    pMt =(pCOMMONMETHOD)get_elem(pMethodsVD, i);
    for(j = 0; j < GeoObjTypesCount; j++)
      pMethods[GeoObjTypesCount*i + j] = pMt->Idle;
  }

  ALL_OBJECTS  = GeoObjTypesCount + 1;
  NULL_OBJECTS = GeoObjTypesCount + 2;

 
  for(i = 0; i < GeoObjTypesCount; i++){
    pObjTempl =(pOBJTEMPL)get_elem(pGeoObjVD, i);
    geo_size[i] = (WORD)pObjTempl->GeoSize;
    ppGeoObjNames[i] = pObjTempl->Name;
    pOldTypes[i] = pObjTempl->OldType;
    pElTypes[i]  = pObjTempl->ElGeoMask;
  }
  
  InitCoreObjMetods();
  ObjectsRegStatus = TRUE;
  
  if(pExtInitObjMetods)
    pExtInitObjMetods();
  if(!ObjectsRegStatus)
    goto err;
  ObjectsRegStatus = FALSE;
  
  for(i = 0; i < pObjMethodsVD->num_elem; i++){
    pObjMt =(pOBJMETHOD)get_elem(pObjMethodsVD, i);
    pMethods[GeoObjTypesCount*pObjMt->iMethod + pObjMt->iObjType] = pObjMt->MethodData;
  }

 
  add_type(enable_filter,  NULL_OBJECTS);
  add_type(struct_filter,  NULL_OBJECTS);
  add_type(complex_filter, NULL_OBJECTS);
  add_type(frame_filter,   NULL_OBJECTS);
  for(i = 0; i < GeoObjTypesCount; i++){
    add_type(enable_filter, i);
    FFlags = GetObjMethod(i, OMT_COMMON_FILTERS);
    if(FFlags & FF_IS_STRUCT)
      add_type(struct_filter, i);
    if(FFlags & FF_HAS_MATRIX)
      add_type(complex_filter, i);
    if(FFlags & FF_FRAME_ITEM)
      add_type(frame_filter, i);
  }
  ret = TRUE;
  goto met;
err:
  GeoObjectsFreeMem();
met:
  free_vdim(pGeoObjVD);
  free_vdim(pMethodsVD);
  free_vdim(pObjMethodsVD);
  SGFree(pGeoObjVD);
  return ret;
}

void GeoObjectsFreeMem(void)
{
  if(geo_size)
    SGFree(geo_size);
  geo_size = NULL;
  if(ppGeoObjNames)
    SGFree(ppGeoObjNames);
  ppGeoObjNames = NULL;
  if(pOldTypes)
    SGFree(pOldTypes);
  pOldTypes = NULL;
  if(pElTypes)
    SGFree(pElTypes);
  pElTypes = NULL;
  if(pMethods)
    SGFree(pMethods);
  pMethods = NULL;
}

SG_PTR_TO_DIGIT* GetMethodArray(METHOD iMethod)
{
  if(iMethod < 0 || iMethod >= GeoObjMethodsCount)
    return 0;
  return &pMethods[GeoObjTypesCount*iMethod];
}

SG_PTR_TO_DIGIT GetObjMethod(OBJTYPE Type, METHOD iMethod)
{
  if(Type < 0 || Type >= GeoObjTypesCount ||
     iMethod < 0 || iMethod >= GeoObjMethodsCount)
  return 0;
  return pMethods[GeoObjTypesCount*iMethod + Type];
}

void SetTmpObjMethod(OBJTYPE Type, METHOD iMethod,  SG_PTR_TO_DIGIT MethodData)
{
  if(Type < 0 || Type >= GeoObjTypesCount ||
     iMethod < 0 || iMethod >= GeoObjMethodsCount)
  return;
  pMethods[GeoObjTypesCount*iMethod + Type] = MethodData;
}


METHOD  RegCommonMethod(SG_PTR_TO_DIGIT IdleMethod)

{
COMMONMETHOD Mt;

  if(!ObjectsRegStatus)
    return -1;

  Mt.Idle = IdleMethod;
  if(!add_elem(pMethodsVD, &Mt))
    goto err;
  return pMethodsVD->num_elem - 1;
err:
  ObjectsRegStatus = FALSE;
  return -1;
}

OBJTYPE RegObjType(char* ObjName, int ObjGeoSize, int OldType, pEL_OBJTYPE pElObjType, BOOL bInGeoObj)

{
int       i;
OBJTEMPL ObjTempl;
pOBJTEMPL pObjTempl;

  if(!ObjectsRegStatus)
    return -1;

  if(pGeoObjVD->num_elem >= MAX_OBJECTS_TYPES)
    goto err;
  for(i = 0; i < pGeoObjVD->num_elem; i++){
    pObjTempl =(pOBJTEMPL)get_elem(pGeoObjVD, i);
    if(!z_stricmp(pObjTempl->Name, ObjName))
      goto err;
  }
  ObjTempl.Name = ObjName;
  ObjTempl.GeoSize = ObjGeoSize;
  ObjTempl.OldType = OldType;
  ObjTempl.ElGeoMask = 0;
  if(bInGeoObj && (sizeof(GEO_OBJ) < ObjGeoSize)){
    if(bINHOUSE_VERSION){
      char Buf[100];
      strcpy(Buf, " MAX_SIMPLE_GEO_SIZE  o_model.h  ");
      //itoa(ObjGeoSize, &Buf[lstrlen(Buf)], 10);
//	    MessageBox(NULL, Buf, "!", MB_OK|MB_ICONERROR|MB_TASKMODAL);
    }
    goto err;
  }
  if(pElObjType){ 
    if(CurrElGeoMask == 0xFFFFFFFF){ 
      if(bINHOUSE_VERSION)

      goto err;
    }
    *pElObjType = ObjTempl.ElGeoMask = CurrElGeoMask;
    if(CurrElGeoMask == 0x80000000)
      CurrElGeoMask = 0xFFFFFFFF;
    else
      CurrElGeoMask = CurrElGeoMask << 1;
  }
  if(!add_elem(pGeoObjVD, &ObjTempl))
    goto err;
  return pGeoObjVD->num_elem - 1;
err:
  ObjectsRegStatus = FALSE;
  return -1;
}

OBJTYPE RegCoreObjType(COREOBJTYPE Type, char* ObjName, int ObjGeoSize, int OldType,
                       pEL_OBJTYPE pElObjType, BOOL bInGeoObj)

{
  if(Type == RegObjType(ObjName, ObjGeoSize, OldType, pElObjType, bInGeoObj))
    return Type;
  ObjectsRegStatus = FALSE;
  return -1;
}

BOOL RegObjMethod(OBJTYPE Type, METHOD iMethod, SG_PTR_TO_DIGIT MethodData)
{
OBJMETHOD OMt;

  if(!ObjectsRegStatus)
    return FALSE;

  if(Type >= pGeoObjVD->num_elem || Type < 0)
    goto err;
  if(iMethod >= pMethodsVD->num_elem || iMethod < 0)
    goto err;
  OMt.iObjType = Type;
  OMt.iMethod = iMethod;
  OMt.MethodData = MethodData;
  if(!add_elem(pObjMethodsVD, &OMt))
    goto err;
  return TRUE;
err:
  ObjectsRegStatus = FALSE;
  return FALSE;
}


OBJTYPE FindObjTypeByName(char* TypeName)

{
OBJTYPE i;

  if(!ppGeoObjNames)
    return -1;
  for(i = 0; i < GeoObjTypesCount; i++){
    if(!z_stricmp(ppGeoObjNames[i], TypeName))
      return i;
  }
  return -1;
}

OBJTYPE FindObjTypeByOldType(int OldType)

{
OBJTYPE i;

  if(!pOldTypes)
    return -1;
  for(i = 0; i < GeoObjTypesCount; i++){
    if(OldType == pOldTypes[i])
      return i;
  }
  return -1;
}



