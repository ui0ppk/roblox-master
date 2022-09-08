#include "../sg.h"

BOOL trans_geo_info(hOBJ hobj, OBJTYPE *type, BOOL *flag_el, void  *geo)
{
//   
  if(!true_geo_info(hobj, type, geo))
		return FALSE;
  set_flag_el(type, flag_el);
	return TRUE;
}

BOOL true_geo_info(hOBJ hobj, OBJTYPE *type, void  *geo)
{
//    
/*  if(get_true_geo(hobj, &serch_finds) != O_OK){
    el_geo_err = BAD_TRUE_GEO;
    return FALSE;
  }*/
	*type = serch_finds.type;
	memcpy(geo, &serch_finds.geo, geo_size[*type]);
	return TRUE;
}
void set_flag_el(OBJTYPE *type, BOOL *flag_el){
	*flag_el = TRUE;
	if(*type == OCIRCLE){
    *flag_el = FALSE;
		*type = OARC;
	}
}
