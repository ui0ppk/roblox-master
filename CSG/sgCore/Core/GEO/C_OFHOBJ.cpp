#include "../sg.h"

#pragma argsused
lpD_POINT center_of_hobj(hOBJ hobj, OBJTYPE type,
                         lpD_POINT vp, lpD_POINT p)
    
{
GEO_OBJ   geo;

  if(!true_geo_info(hobj, &type, &geo))
		return NULL;
  return center_of_geo_obj(type, &geo, vp, p);
}

#pragma argsused
lpD_POINT center_of_geo_obj(OBJTYPE type, void * geo,
                           lpD_POINT vp, lpD_POINT p)
     
{
  return dpoint_copy(p, &(((lpGEO_ARC)geo)->vc));
}
