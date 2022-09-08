#include "../sg.h"

lpD_POINT near_to_hobj(hOBJ hobj, OBJTYPE type, lpD_POINT vp, lpD_POINT p)

{
GEO_OBJ   geo;

  if(!true_geo_info(hobj, &type, &geo))
		return NULL;
  return near_to_geo_obj(type, &geo, vp, p);
}

lpD_POINT near_to_geo_obj(OBJTYPE type, void * geo,
                          lpD_POINT vp, lpD_POINT p)

{
BOOL flag_arc;

  set_flag_el(&type, &flag_arc);
	switch(type){
		case OPOINT:
      dpoint_copy(p, (lpD_POINT)geo);
	    return p;
		case OLINE:
      return near_3d_pl((lpGEO_LINE)geo, vp, p);
		case OARC:
      return near_3d_pa((lpGEO_ARC)geo, flag_arc, vp, p);
		default:
			return NULL;
	}
}
