#include "../sg.h"

//       
char* GetObjTypeCaption(OBJTYPE type, char* str, int len)
{
/*  LoadString(hInstLD, GetObjMethod(type, OMT_OBJTYPE_CAPTION), str, len - 1);
	str[len - 1] = 0;
	return str;*/return NULL;
}

//     .
//   -  .   -   ..
//  BREP - , , 
char* GetObjDescriptionStr(char* str, int len, hOBJ hobj)
{
lpOBJ     obj;
OBJTYPE   type;
void      (*fstr)(hOBJ hobj, char *str, int len);

	obj = (lpOBJ)hobj;
	type = obj->type;
  fstr = (void (*)(hOBJ hobj, char *str, int len))GetObjMethod(type, OMT_OBJ_DESCRIPT);
  if(!fstr)
    return GetObjTypeCaption(type, str, len);
  fstr(hobj, str, len);
	return str;
}

void IdleObjDecriptStr(hOBJ hobj, char *str, int len)
//    
{
  GetObjTypeCaption(((lpOBJ)hobj)->type, str, len);
}

void ot_brep_str(hOBJ hobj, char *str, int len)
{
	CODE_MSG   	cod;
	lpGEO_BREP 	geo;
	lpOBJ      	obj;
	LNP		  	lnp;
	int		  	num;
	BREPTYPE	 	type;


	obj = (lpOBJ)hobj;
	geo = (lpGEO_BREP)(obj->geo_data);
	num = geo->num;
	type = (BREPTYPE)geo->type;
	if ( !(read_elem(&vd_brep,num,&lnp)) ) return;
	switch ( lnp.kind ) {
		case COMMON:
			switch(type){
				case BODY:
					cod = BRBODY_MSG;
					break;
				case SURF:
					cod = BRSURF_MSG;
					break;
				default:
					cod = BRFRAME_MSG;
			}
			break;
		case BOX:
			cod = BOX_MSG;
			break;
		case CYL:
			cod = CYLINDER_MSG;
			break;
		case SPHERE:
			cod = SPHERE_MSG;
			break;
		case CONE:
			cod = CONE_MSG;
			break;
		case TOR:
			cod = TORUS_MSG;
			break;
		case PRISM:
			cod = PRISM_MSG;
			break;
		case PYRAMID:
			cod = PYRAMID_MSG;
			break;
		case ELIPSOID:
			cod = MSG_SHU_016;
			break;
		case SPHERIC_BAND:
			cod = MSG_SHU_017;
			break;
		default:
			cod = UNKNOWN_OBJ_MSG;
			break;
	}
	strncpy(str, get_message(cod)->str, len - 1);
	str[len - 1] = 0;
}

void ot_insert_str(hOBJ hobj, char *str, int len)
{
int         bnum;
lpIBLOCK     blk;
lpGEO_INSERT geo;
lpOBJ        obj;


	obj = (lpOBJ)hobj;
	geo = (lpGEO_INSERT)(obj->geo_data);
	bnum = geo->num;
  GetObjTypeCaption(obj->type, str, len);

	if((blk = (IBLOCK *)get_elem(&vd_blocks, bnum)) != NULL){
		strncat(str, "\"", len - strlen(str) - 1);
		str[len - 1] = 0;
		strncat(str, blk->name, len - strlen(str) - 1);
		strncat(str, "\"", len - strlen(str) - 1);
	}
}

void ot_path_str(hOBJ hobj, char *str, int len)
{
lpOBJ     obj;
CODE_MSG	code;
void*     par = NULL;

  switch (get_primitive_param_2d(hobj, &par)) {
		case RECTANGLE:
			strncpy(str, get_message(MSG_SHU_038)->str, len - 1);
			str[len - 1] = 0;
      break;
    case POLYGON:
			strncpy(str, get_message(MSG_SHU_039)->str, len - 1);
			str[len - 1] = 0;
      break;
    default:
			obj = (lpOBJ)hobj;
		  GetObjTypeCaption(obj->type, str, len);
      if (obj->status & ST_FLAT) code = MSG_PATH_FLATE;
      else                       code = MSG_PATH_NO_FLATE;
      strncat(str, get_message(code)->str, len - strlen(str) - 1);
      str[len - 1] = 0;
      if (obj->status & ST_CLOSE) code = MSG_PATH_CLOSE;
      else                        code = MSG_PATH_NO_CLOSE;
      strncat(str, get_message(code)->str, len - strlen(str) - 1);
      if (obj->status & ST_SIMPLE) code = MSG_PATH_NO_CROSS;
      else                         code = MSG_PATH_CROSS;
      strncat(str, get_message(code)->str, len - strlen(str) - 1);
	}
  if (par) SGFree(par); par = NULL;
}

