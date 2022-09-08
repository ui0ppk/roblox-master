#include "../sg.h"

#pragma argsused
lpD_POINT middle_of_hobj(hOBJ hobj, OBJTYPE type,
                         lpD_POINT vp, lpD_POINT p)

{
GEO_OBJ geo;

  if(!true_geo_info(hobj, &type, &geo))
		return NULL;
  return middle_of_geo_obj(type, &geo, vp, p);
}

#pragma argsused
lpD_POINT middle_of_geo_obj(OBJTYPE type, void * geo,
                            lpD_POINT vp, lpD_POINT p)

{
lpGEO_ARC  a, a2d = &el_geo1;
lpGEO_LINE l;
D_POINT    w;
BOOL       flag_arc;

  set_flag_el(&type, &flag_arc);

	switch(type){
	  case OPOINT:
      dpoint_copy(p, (lpD_POINT)geo);
	    return p;
		case OLINE:
      l = (lpGEO_LINE)geo;
			if(!test_wrd_line(l))
				return NULL;
      return dpoint_parametr(&l->v1, &l->v2, 0.5, p);
		case OARC:
      a = (lpGEO_ARC)geo;
			if(!test_wrd_arc(a, &flag_arc))
		    return NULL;
      trans_arc_to_acs(a, flag_arc, a2d, el_g_e, el_e_g);
      w.x = a->r * cos(a->ab + a->angle/2.);
      w.y = a->r * sin(a->ab + a->angle/2.);
			w.z = 0.;
      o_hcncrd(el_e_g, &w, p);
			return p;
		default:
			return NULL;
	}
}
